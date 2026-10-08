#ifndef AST_H
#define AST_H
#include "DataStructures/Arenas/DynamicArray.h"
#include "tokenizer.h"

typedef struct ASTNode ASTNode;
typedef struct Redirect Redirect;
typedef struct LogicalNode LogicalNode;

DECLARE_DYNAMIC_ARRAY(char *, ArgsArray)
DECLARE_DYNAMIC_ARRAY(ASTNode *, NodeArray)
DECLARE_DYNAMIC_ARRAY(Redirect, RedirectArray)
DECLARE_DYNAMIC_ARRAY(LogicalNode, LogicalArray)

typedef enum {
    NODE_COMMAND,
    NODE_PIPELINE,
    NODE_COMMAND_LIST,
    NODE_LOGICAL_LIST,
    NODE_SUBSHELL,
    NODE_BACKGROUND,
    NODE_NEGATION,
} NodeType;


typedef enum {
    OP_AND_AND,
    OP_OR_OR,
    OP_SEMICOLON,
    OP_BACKGROUND,
    OP_NEWLINE,
} Operator;


struct Redirect {
   int fd;
   TokenType op;
    char *target;
};

typedef struct {
    ASTNode *child;
} WrapperNode;

typedef struct {
    NodeArray *array;
} ListNode;

struct LogicalNode {
    Operator op;
    ASTNode *node;
};

typedef struct {
    LogicalArray *items;
} LogicalListNode;

typedef struct {
    ArgsArray *args;
    RedirectArray *redirect;
}  SimpleCommandNode;

struct ASTNode {
    NodeType type;
    union {
        WrapperNode unary;
        ListNode list;
        SimpleCommandNode cmd;
        LogicalListNode logical;
    };
};

typedef struct {
    Token current;
    char error_msg[128];
    const char *error_pos;
} Parser;

ASTNode *run_parser();
void free_ast(ASTNode *node);

#ifdef DEBUG
    void print_ast(const ASTNode *root);
#endif

#endif