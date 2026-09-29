#include "tokenizer.h"
#include "globals.h"
#include <string.h>

void init_scanner() {
    scanner.start = line;
    scanner.current = line;
}

static Token make_token(TokenType type) {
    Token token;
    token.type = type;
    token.start = scanner.start;
    token.length = (uint32_t)(scanner.current - scanner.start);
    return token;
}

static Token make_error(const char *message) {
    Token token;
    token.type = TOKEN_ERROR;
    token.start = message;
    token.length = strlen(message);
    return token;
}

static bool is_at_end() {
    return *scanner.current == '\0';
}

static char peek() {
    return *scanner.current;
}

static char advance() {
    return *scanner.current++;
}

static bool match_next(const char *string) {
    size_t length = strlen(string);
    if (strncmp(scanner.current, string, length) == 0) {
        scanner.current += length;
        return true;
    }
    return false;
}

static void skip_whitespace() {
    while(1) {
        char c = peek();
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
            case '\n':
                advance();
                break;
            case '#':
                while(!is_at_end() && advance() != '\n');
                break;
            default:
                return;
        }
    }
}

static Token string(char quote_type) {
    while (peek() != quote_type && !is_at_end()) {
        advance();
    }

    if (is_at_end()) return make_error("\x1b[31mError:\x1b[0m Unterminated string"); 

    advance(); // The closing quote
    return make_token(TOKEN_STRING);
}

static bool is_operator_or_space(char c) {
    return c == ' ' || c == '\r' || c == '\t' || c == '\n' ||
           c == '|' || c == '&' || c == '<' || c == '>' || c == ';' ||
           c == '(' || c == ')' || c == '\'' || c == '"' || c == '\0';
}

static Token match_identifier() {
    while (!is_at_end() && !is_operator_or_space(peek())) advance();
    

    if (peek() == '<' || peek() == '>' || match_next("&>")) { // Checks for file descriptors before redirectors.
        bool all_digits = true;
        for (const char *p = scanner.start; p < scanner.current; p++) {
            if (*p < '0' || *p > '9') {
                all_digits = false;
                break;
            }
        }
        if (all_digits) return make_token(TOKEN_IO_NUMBER);
    }

    return make_token(TOKEN_IDENTIFIER);
}

Token scan_token() {
    skip_whitespace();
    scanner.start = scanner.current;

    if (is_at_end()) return make_token(TOKEN_EOF);

    char c = advance();

    switch (c) {
        case '(': return make_token(TOKEN_LEFT_PAREN);
        case ')': return make_token(TOKEN_RIGHT_PAREN);
        case '!': return make_token(TOKEN_BANG);
        case ';': return make_token(TOKEN_SEMICOLON);
        case '&':
            if (match_next("&"))  return make_token(TOKEN_AND_AND);
            if (match_next(">>")) return make_token(TOKEN_AND_GREATER_GREATER);
            if (match_next(">"))  return make_token(TOKEN_AND_GREATER);
            return make_token(TOKEN_AND);
        case '|':
            if (match_next("|"))  return make_token(TOKEN_OR_OR);
            return make_token(TOKEN_OR);
        case '<':
            if (match_next("<"))  return make_token(TOKEN_LESS_LESS);
            if (match_next(">"))  return make_token(TOKEN_LESS_GREATER);
            if (match_next("&"))  return make_token(TOKEN_LESS_AND);
            return make_token(TOKEN_LESS);
        case '>':
            if (match_next(">"))  return make_token(TOKEN_GREATER_GREATER);
            if (match_next("&"))  return make_token(TOKEN_GREATER_AND);
            return make_token(TOKEN_GREATER);
        case '"': return string('"');
        case '\'': return string('\'');
        default: 
            return match_identifier();
    }
}
