#include "DataStructures/Arenas/DynamicArray.h"
#include "tokenizer.h"
#include "globals.h"
#include "ast.h"
#include <string.h>
#include <stdio.h>

static ASTNode *command_list();
static int nesting_depth; 

static inline bool check(TokenType type) {
    return parser.current.type == type;
}

static void parse_error(const char *msg, const char *pos) {
    if (parser.error_pos) return;    
    parser.error_pos = pos;
    snprintf(parser.error_msg, sizeof(parser.error_msg), "%s", msg);
}

static inline void error_at_current(const char *msg) {
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

static void init_parser(char *line) {
    nesting_depth = 0;
    parser.error_pos = NULL;
    memset(parser.error_msg, '\0', sizeof(parser.error_msg));
    init_scanner(line);
    advance();
}

static inline Token peek_token() {
    return parser.current;
}

static inline TokenType peek_type(){
    return parser.current.type;
}

static inline bool in_quotes(Token token) {
    return *token.start == '\'' || *token.start == '"';
}

static inline bool is_word(TokenType token) {
    return token == TOKEN_IDENTIFIER;
}

static char *word_text(Token token) {
    if (in_quotes(token))
        return arena_strndup(&command_arena, token.start + 1, token.length - 2);   // drop both quotes
    return arena_strndup(&command_arena, token.start, token.length);
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
        if (!(parse_int_const(peek_token().start, peek_token().length, &n) && n >= 0) && !(peek_token().length == 1 && peek_token().start[0] == '-')) {
            error_at_current("expected a file descriptor number or '-'");
            return false;
        }
    }
    
    red->target = word_text(peek_token());
    advance();
    return true;
}

static bool check_command_limits(size_t *arg_total, size_t *arg_count) {
    *arg_total += peek_token().length;
    (*arg_count)++;

    if (*arg_total > ARG_MAX) {
        error_at_current("ShadowShell: argument list too long..\n");
        return false;
    }

    if (*arg_count > MAX_ARGS) {
        error_at_current("ShadowShell: too many arguments.\n");    
        return false;
    }

    return true;
}

static ASTNode *simple_command() {
    RedirectArray *r = RedirectArray_create(&command_arena, 4);
    ArgsArray *a = ArgsArray_create(&command_arena, 4);

    ASTNode *node = (ASTNode *)arena_alloc(&command_arena,  sizeof(ASTNode));
    node->type = NODE_COMMAND;
    node->cmd.args = a;
    node->cmd.redirect = r;

    size_t arg_total = 0, arg_count = 0;
    while (1) {
        if (is_word(peek_type())) {
            if (!check_command_limits(&arg_total, &arg_count)) return NULL;
            ArgsArray_push(&command_arena, a, word_text(peek_token()));
            advance();
        } else if (is_redirect_op(peek_type()) || check(TOKEN_IO_NUMBER)) {
            Redirect red;
            if (!parse_redirect(&red)) return NULL;
            RedirectArray_push(&command_arena, r, red);            
        } else { 
           break;
        }
    }

    if (a->size == 0 && r->size == 0) {
        error_at_current("expected a command");
        return NULL;
    }

    return node;
}


static ASTNode *command() {
    if (check(TOKEN_LEFT_PAREN)) { // SubShell
        if (nesting_depth >= MAX_NESTING) {
            error_at_current("subshells nesting limit exceeded");
            return NULL;
        }
        nesting_depth++;
        advance();
        ASTNode *node = command_list();
        nesting_depth--;
        if (!node || !check(TOKEN_RIGHT_PAREN)) {
            error_at_current("expected ')'");
            return NULL;
        }

        advance();
        ASTNode *wrapper = (ASTNode *)arena_alloc(&command_arena,  sizeof(ASTNode));
        wrapper->type = NODE_SUBSHELL;
        wrapper->unary.child = node;
        return wrapper;
    }

    return simple_command();
}

static ASTNode *pipeline() {
    ASTNode *cmd = command();
    if (!check(TOKEN_OR) || !cmd) return cmd;

    ASTNode *node = (ASTNode *)arena_alloc(&command_arena,  sizeof(ASTNode));
    node->type = NODE_PIPELINE;
    node->list.array = NodeArray_create(&command_arena, 4);
    NodeArray_push(&command_arena, node->list.array, cmd);
    while (check(TOKEN_OR)) {
        advance();
        cmd = command();

        if (!cmd) {
            error_at_current("pipe without a second operand");
            return cmd;
        }
        NodeArray_push(&command_arena, node->list.array, cmd);
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
    
    ASTNode *node = (ASTNode *)arena_alloc(&command_arena, sizeof(ASTNode));
    node->type = NODE_NEGATION;
    node->unary.child = p;
    return node;
}


static ASTNode *logical(void) {
    ASTNode *node = negation();
    if (!node || (!check(TOKEN_AND_AND) && !check(TOKEN_OR_OR))) return node;    
    
    ASTNode *list = (ASTNode *)arena_alloc(&command_arena, sizeof(ASTNode));
    list->type = NODE_LOGICAL_LIST;
    list->logical.items = LogicalArray_create(&command_arena, 4);

    Operator op = OP_AND_AND;          
    while(1) {
        LogicalArray_push(&command_arena, list->logical.items, (LogicalNode){ .op = op, .node = node });

        if (check(TOKEN_AND_AND))      op = OP_AND_AND;
        else if (check(TOKEN_OR_OR))   op = OP_OR_OR;
        else break;

        advance();
        node = negation();
        
        if (!node) return node;

    }
    
    return list;
}

static ASTNode *command_list() {
    ASTNode *node = logical();
    if (!node || (!check(TOKEN_AND) && !check(TOKEN_SEMICOLON))) return node;

    ASTNode *list = (ASTNode *)arena_alloc(&command_arena, sizeof(ASTNode));
    list->type = NODE_COMMAND_LIST;
    list->list.array = NodeArray_create(&command_arena, 4);

    while(1) {        
        if (check(TOKEN_AND)) {
            ASTNode *bg = (ASTNode *)arena_alloc(&command_arena, sizeof(ASTNode));
            bg->type = NODE_BACKGROUND;
            bg->unary.child = node;
            node = bg;
        }
        
        NodeArray_push(&command_arena, list->list.array, node);
        advance();

        if (check(TOKEN_EOF) || check(TOKEN_RIGHT_PAREN)) break;

        node = logical();
        if (!node) return NULL;

        if (!check(TOKEN_AND) && !check(TOKEN_SEMICOLON)) {        
            NodeArray_push(&command_arena, list->list.array, node);
            break;
        }
    }

    return list;
}


ASTNode *run_parser(char * line) {
    init_parser(line);
    if (check(TOKEN_EOF)) return NULL; 

    ASTNode *n = command_list();
    if (n && !check(TOKEN_EOF)) {              
        parse_error("unexpected token", scanner.start);
        n = NULL;
    }
    if (!n) {
        fprintf(stderr, "\x1b[31msyntax error:\x1b[0m %s\n", parser.error_msg);
        fprintf(stderr, "  %s  %*s\x1b[31m^\n\x1b[0m", line, (int)(parser.error_pos - line), "");
    }
    return n;
}