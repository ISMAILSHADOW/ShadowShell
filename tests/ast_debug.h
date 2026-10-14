#ifndef AST_DEBUG_H
#define AST_DEBUG_H

#include "../DataStructures/Arenas/Arena.h"
#include "../DataStructures/Arenas/StringBuilder.h"
#include "../ast.h"
void ast_to_string(ArenaAllocator *arena, StringBuilder *sb, const ASTNode *n);
void print_ast(const ASTNode *root);
#endif