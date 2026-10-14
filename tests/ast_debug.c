#ifdef DEBUG
#include <stdio.h>
#include <stdbool.h>
#include "../ast.h"
#include "../DataStructures/Arenas/StringBuilder.h"

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
        case TOKEN_LESS_GREATER:        return "<>";
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

static void add_label(ArenaAllocator *arena, StringBuilder *sb, const ASTNode *n) {
    switch (n->type) {
        case NODE_COMMAND_LIST: sb_append_cstring(arena, sb, "COMMAND_LIST "); break;
        case NODE_LOGICAL_LIST: sb_append_cstring(arena, sb, "LOGICAL ");      break;
        case NODE_PIPELINE:     sb_append_cstring(arena, sb, "PIPELINE ");     break;
        case NODE_NEGATION:     sb_append_cstring(arena, sb, "NOT ");          break;
        case NODE_BACKGROUND:   sb_append_cstring(arena, sb, "BACKGROUND ");   break;
        case NODE_SUBSHELL:     sb_append_cstring(arena, sb, "SUBSHELL ");     break;
        case NODE_COMMAND:
            sb_append_cstring(arena, sb, "COMMAND ");
            bool not_empty = n->cmd.args->size > 0 || n->cmd.redirect->size > 0;
            if (not_empty) {
                sb_append_cstring(arena, sb, "(");
                for (size_t i = 0; i < n->cmd.args->size; i++)
                    sb_appendf(arena, sb, " \"%s\"", n->cmd.args->data[i]);
                for (size_t i = 0; i < n->cmd.redirect->size; i++) {
                    Redirect *r = &n->cmd.redirect->data[i];
                    sb_append_cstring(arena, sb, " [");
                    if (r->fd != -1) sb_appendf(arena, sb, "%d", r->fd);
                    sb_appendf(arena, sb, "%s %s]", redirect_str(r->op), r->target);
                }
                sb_append_cstring(arena, sb, " ) ");
            }
            break;
    }
}

static const char *child_seperator(ASTNode *n, size_t index) {
    switch (n->type) {
        case NODE_PIPELINE: return "| ";
        case NODE_COMMAND_LIST: return "; ";
        case NODE_LOGICAL_LIST: 
        LogicalArray *arr = n->logical.items;
        if (index < arr->size - 1) return arr->data[index + 1].op == OP_AND_AND ? "&& " : "|| ";
        case NODE_NEGATION:
            break;
    }
}

void ast_to_string(ArenaAllocator *arena, StringBuilder *sb, ASTNode *n) {
    add_label(arena, sb, n);
    switch(n->type) {
        case NODE_COMMAND_LIST:
        case NODE_PIPELINE:  
        NodeArray *arr = n->list.array;
        sb_append_cstring(arena, sb, "( "); 
        for (size_t i = 0;i < arr->size;i++) {
            ast_to_string(arena, sb, arr->data[i]);
            if (i < arr->size - 1) sb_append_cstring(arena, sb, child_seperator(n, i));
        }
        sb_append_cstring(arena, sb, ") ");
        break;
        case NODE_LOGICAL_LIST: 
        LogicalArray *list = n->logical.items;
        sb_append_cstring(arena, sb, "( "); 
        for (size_t i = 0;i < list->size;i++) {
            ast_to_string(arena, sb, list->data[i].node);
            if (i < list->size - 1) sb_append_cstring(arena, sb, child_seperator(n, i));
        }
        sb_append_cstring(arena, sb, ") ");
        break;
        case NODE_NEGATION:
        case NODE_BACKGROUND:   
        case NODE_SUBSHELL:
        sb_append_cstring(arena, sb, "( ");
        ast_to_string(arena, sb, n->unary.child);
        sb_append_cstring(arena, sb, ") ");
        break;
    }
}
#endif