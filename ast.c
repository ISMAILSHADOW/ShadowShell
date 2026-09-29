#include "ast.h"
#include "DynamicArray.h"
#include "tokenizer.h"
#include "globals.h"
#include <string.h>

static ASTNode *command_list();

static bool check(TokenType type) {
    return parser.current.type == type;
}

static void parse_error(const char *msg, const char *pos) {
    if (parser.error_pos) return;    
    parser.error_pos = pos;
    snprintf(parser.error_msg, sizeof(parser.error_msg), "%s", msg);
}

static void error_at_current(const char *msg) {
    parse_error(msg, parser.current.start);
}

static Token advance() {
    parser.current = scan_token();
    if (parser.current.type == TOKEN_ERROR) {
        char msg[128];
        snprintf(msg, sizeof msg, "%.*s", (int)parser.current.length, parser.current.start);
        parse_error(msg, scanner.start);
    }
    return parser.current;
}

static void init_parser() {
    parser.error_pos = NULL;
    memset(parser.error_msg, '\0', sizeof(parser.error_msg));
    init_scanner();
    advance();
}

static Token peek_token() {
    return parser.current;
}

static TokenType peek_type(){
    return parser.current.type;
}


static bool is_word(TokenType token) {
    return token == TOKEN_IDENTIFIER || token == TOKEN_STRING;
}

static char *word_text(Token token) {
    if (token.type == TOKEN_STRING)
        return strndup(token.start + 1, token.length - 2);   // drop both quotes
    return strndup(token.start, token.length);
}

static bool is_redirect_op(TokenType token) {
    switch (token) {
        case TOKEN_LESS:
        case TOKEN_GREATER:
        case TOKEN_LESS_LESS:
        case TOKEN_GREATER_GREATER:
        case TOKEN_LESS_AND:
        case TOKEN_GREATER_AND:
        case TOKEN_AND_GREATER:
        case TOKEN_AND_GREATER_GREATER:
        case TOKEN_LESS_GREATER:
            return true;
        default:
            return false;
    }
}

static bool parse_redirect(Redirect *red) {
    red->fd = -1;

    if (check(TOKEN_IO_NUMBER)) {
        if (!parse_int_const(peek_token().start, peek_token().length, &red->fd)) {
            error_at_current("file descriptor number is invalid");
            return false;
        }
        advance();
    }
    
    if (!is_redirect_op(peek_type())) {
        error_at_current("expected a redirection operator");
        return false;
    }

    red->op = peek_type();
    advance();

    
    if (!is_word(peek_type())){ // redirection without a file
        error_at_current("expected a filename after redirection");
        return false;
    }  
    
    if (red->op == TOKEN_LESS_AND || red->op == TOKEN_GREATER_AND) {
        int n;
        if (!(parse_int_const(peek_token().start, peek_token().length, &n) && n >= 0)) {
            error_at_current("expected a file descriptor number or '-'");
            return false;
        }
    }
    
    red->target = word_text(peek_token());
    advance();
    return true;
}

static ASTNode *simple_command() {
    RedirectArray *r = RedirectArray_create(4);
    ArgsArray *a = ArgsArray_create(4);

    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = NODE_COMMAND;
    node->cmd.args = a;
    node->cmd.redirect = r;

    while (1) {
        if (is_word(peek_type())) {
            ArgsArray_push(a, word_text(peek_token()));
            advance();
        } else if (is_redirect_op(peek_type()) || check(TOKEN_IO_NUMBER)) {
            Redirect red;
            if (!parse_redirect(&red)) {
                free_ast(node);
                return NULL;
            }
            RedirectArray_push(r, red);            
        } else { 
           break;
        }
    }

    if (a->size == 0 && r->size == 0) {
        error_at_current("expected a command");
        free_ast(node);
        return NULL;
    }


    return node;
}


static ASTNode *command() {
    if (check(TOKEN_LEFT_PAREN)) { // SubShell
        advance();
        ASTNode *node = command_list();
        if (!node || !check(TOKEN_RIGHT_PAREN)) {
            error_at_current("expected ')'");
            free_ast(node);
            return NULL;
        }

        advance();
        ASTNode *wrapper = malloc(sizeof(ASTNode));
        wrapper->type = NODE_SUBSHELL;
        wrapper->unary.child = node;
        return wrapper;
    }

    return simple_command();
}

static ASTNode *pipeline() {
    ASTNode *cmd = command();
    if (!check(TOKEN_OR) || !cmd) return cmd;

    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = NODE_PIPELINE;
    node->list.array = NodeArray_create(4);
    NodeArray_push(node->list.array, cmd);
    while (check(TOKEN_OR)) {
        advance();
        cmd = command();

        if (!cmd) {
            error_at_current("pipe without a second operand");
            free_ast(node);
            return cmd;
        }
        NodeArray_push(node->list.array, cmd);
    }

    return node;
}

static ASTNode *negation() {
    bool negate = false;
    while (check(TOKEN_BANG)) {
        advance();
        negate = !negate;
    }

    ASTNode *p = pipeline();
    if (!negate || !p) return p;
    
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = NODE_NEGATION;
    node->unary.child = p;
    return node;
}


static ASTNode *logical(void) {
    ASTNode *list = malloc(sizeof(ASTNode));
    list->type = NODE_LOGICAL_LIST;
    list->logical.items = LogicalArray_create(4);

    Operator op = OP_AND_AND;          
    ASTNode *node = negation();

    if (!node) {
        free_ast(list);
        return node;
    }

    while(1) {
        LogicalArray_push(list->logical.items, (LogicalNode){ .op = op, .node = node });

        if (check(TOKEN_AND_AND))      op = OP_AND_AND;
        else if (check(TOKEN_OR_OR))   op = OP_OR_OR;
        else break;

        advance();
        node = negation();
        
        if (!node) {
            free_ast(list);
            return node;
        }
    }

    // Only one item means there was no operator, so return the one node
    if (list->logical.items->size == 1) {
        LogicalArray_destroy(list->logical.items);
        free(list);
        return node;
    }
    return list;
}

static ASTNode *command_list() {
    ASTNode *list = malloc(sizeof(ASTNode));
    list->type = NODE_COMMAND_LIST;
    list->list.array = NodeArray_create(4);


    while(1) {
        ASTNode *node = logical();
        if (!node) {
            free_ast(list);
            return node;
        }

        bool more = false;
        if (check(TOKEN_AND)) {
            ASTNode *bg = malloc(sizeof(ASTNode));
            bg->type = NODE_BACKGROUND;
            bg->unary.child = node;
            node = bg;
            advance();
            more = true;
        } else if (check(TOKEN_SEMICOLON)) {
            advance();
            more = true;
        }

        if (!more && list->list.array->size == 0) {
            free_ast(list);
            return node;
        }

        NodeArray_push(list->list.array, node);
        if (!more || check(TOKEN_EOF) || check(TOKEN_RIGHT_PAREN)) break;
    }

    return list;
}


ASTNode *run_parser() {
    init_parser();
    if (check(TOKEN_EOF)) return NULL; 

    ASTNode *n = command_list();
    if (n && !check(TOKEN_EOF)) {              
        parse_error("unexpected token", scanner.start);
        free_ast(n);
        n = NULL;
    }
    if (!n) {
        fprintf(stderr, "\x1b[31msyntax error:\x1b[0m %s\n", parser.error_msg);
        fprintf(stderr, "  %s  %*s\x1b[31m^\n\x1b[0m", line, (int)(parser.error_pos - line), "");
    }
    return n;
}

void free_ast(ASTNode *node) {
    if (!node) return;

    switch (node->type) {
        case NODE_COMMAND_LIST:
        case NODE_PIPELINE:
            for (int i = 0;i < node->list.array->size;i ++) 
                free_ast(node->list.array->data[i]);
            NodeArray_destroy(node->list.array);
            break;
        case NODE_LOGICAL_LIST:
            for (int i = 0;i < node->logical.items->size;i ++) 
                free_ast(node->logical.items->data[i].node);
            LogicalArray_destroy(node->logical.items);      
            break;
        case NODE_NEGATION:
        case NODE_BACKGROUND:
        case NODE_SUBSHELL:
            free_ast(node->unary.child);    
            break;
        case NODE_COMMAND:
            for (int i = 0;i < node->cmd.args->size;i++)
                free(node->cmd.args->data[i]);
            for (int i = 0;i < node->cmd.redirect->size;i++)
                free(node->cmd.redirect->data[i].target);
            ArgsArray_destroy(node->cmd.args);
            RedirectArray_destroy(node->cmd.redirect);
            break;
        default:
            break;
    }

    free(node);
}