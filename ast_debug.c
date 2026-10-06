#ifdef DEBUG
#include <stdio.h>
#include "ast.h"

static const char *redirect_str(TokenType t) {
    switch (t) {
        case TOKEN_LESS:                return "<";
        case TOKEN_GREATER:             return ">";
        case TOKEN_LESS_LESS:           return "<<";
        case TOKEN_GREATER_GREATER:     return ">>";
        case TOKEN_LESS_AND:            return "<&";
        case TOKEN_GREATER_AND:         return ">&";
        case TOKEN_AND_GREATER:         return "&>";
        case TOKEN_AND_GREATER_GREATER: return "&>>";
        default:                        return "?";
    }
}

static size_t child_count(const ASTNode *n) {
    switch (n->type) {
        case NODE_COMMAND_LIST:
        case NODE_PIPELINE:     return n->list.array->size;
        case NODE_LOGICAL_LIST: return n->logical.items->size;
        case NODE_NEGATION:
        case NODE_BACKGROUND:
        case NODE_SUBSHELL:     return 1;
        default:                return 0;  
    }
}

static const ASTNode *child_at(const ASTNode *n, size_t i) {
    switch (n->type) {
        case NODE_COMMAND_LIST:
        case NODE_PIPELINE:     return n->list.array->data[i];
        case NODE_LOGICAL_LIST: return n->logical.items->data[i].node;
        default:                return n->unary.child;
    }
}

static const char *child_tag(const ASTNode *n, size_t i) {
    if (n->type != NODE_LOGICAL_LIST || i == 0) return "";
    return n->logical.items->data[i].op == OP_AND_AND ? "[AND] " : "[OR] ";
}

static void print_label(const ASTNode *n) {
    switch (n->type) {
        case NODE_COMMAND_LIST: printf("COMMAND_LIST\n"); break;
        case NODE_LOGICAL_LIST: printf("LOGICAL\n");      break;
        case NODE_PIPELINE:     printf("PIPELINE\n");     break;
        case NODE_NEGATION:     printf("NOT\n");          break;
        case NODE_BACKGROUND:   printf("BACKGROUND\n");   break;
        case NODE_SUBSHELL:     printf("SUBSHELL\n");     break;
        case NODE_COMMAND:
            printf("COMMAND");
            for (size_t i = 0; i < n->cmd.args->size; i++)
                printf(" \"%s\"", n->cmd.args->data[i]);
            for (size_t i = 0; i < n->cmd.redirect->size; i++) {
                Redirect *r = &n->cmd.redirect->data[i];
                printf("  [");
                if (r->fd != -1) printf("%d", r->fd);
                printf("%s %s]", redirect_str(r->op), r->target);
            }
            printf("\n");
            break;
    }
}

static void print_children(const ASTNode *n, const char *prefix);

static void print_tree(const ASTNode *n, const char *prefix, bool is_last, const char *tag) {
    printf("%s%s%s", prefix, is_last ? "└── " : "├── ", tag);
    print_label(n);

    char child_prefix[512];
    snprintf(child_prefix, sizeof child_prefix, "%s%s", prefix, is_last ? "    " : "│   ");
    print_children(n, child_prefix);
}

static void print_children(const ASTNode *n, const char *prefix) {
    size_t count = child_count(n);
    for (size_t i = 0; i < count; i++) {
        const ASTNode *c = child_at(n, i);
        if (!c) { printf("%s└── (NULL)\n", prefix); continue; }
        print_tree(c, prefix, i == count - 1, child_tag(n, i));
    }
}

void print_ast(const ASTNode *root) {
    if (!root) { printf("(empty)\n"); return; }
    print_label(root);
    print_children(root, "");
}
#endif