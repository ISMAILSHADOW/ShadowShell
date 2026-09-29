#ifndef TOKENIZER_H
#define TOKENIZER_H
#include <stdint.h>

typedef enum {
    // Single character tokens
    // TOKEN_SLASH,
    TOKEN_LEFT_PAREN, 
    TOKEN_RIGHT_PAREN, 
    TOKEN_SEMICOLON,
    TOKEN_BANG,
    // One or more
    TOKEN_AND,
    TOKEN_AND_AND,
    TOKEN_AND_GREATER,
    TOKEN_AND_GREATER_GREATER,
    TOKEN_GREATER,
    TOKEN_OR,
    TOKEN_OR_OR,
    TOKEN_LESS,
    TOKEN_LESS_GREATER,
    TOKEN_LESS_AND,
    TOKEN_LESS_LESS,
    TOKEN_GREATER_GREATER,
    TOKEN_GREATER_AND,
    // Arbitrary number of characters
    TOKEN_IDENTIFIER,
    TOKEN_IO_NUMBER, // Numbers before redirectors
    TOKEN_STRING, // String is in quotes identifier is not.

    TOKEN_ERROR,
    TOKEN_EOF
} TokenType;


typedef struct {
    TokenType type;
    const char* start;
    uint32_t length;
} Token;

typedef struct {
    const char *start;
    const char *current;
} Scanner;

void init_scanner();
Token scan_token();
#endif