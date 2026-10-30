/*
    The on-demand scanner used by the parser in ast.c
    init_scanner must be called with a pointer to the line to be tokenized as an
   argument.

    The line must not be changed while the parser is working as the tokens are
   generated in place.
*/

#include "tokenizer.h"
#include "globals.h"
#include <string.h>

void init_scanner(char *line) {
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
    token.length = (uint32_t)strlen(message);
    return token;
}

static inline bool is_at_end() { return *scanner.current == '\0'; }

static inline char peek() { return *scanner.current; }

static inline char prev() {
    return scanner.current > scanner.start ? *(scanner.current - 1) : '\0';
}

static inline char advance() { return *scanner.current++; }

static bool match_next(const char *string) {
    size_t length = strlen(string);
    if (strncmp(scanner.current, string, length) == 0) {
        scanner.current += length;
        return true;
    }
    return false;
}

static void skip_whitespace() {
    while (1) {
        char c = peek();
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
            case '\n': advance(); break;
            case '#':
                while (!is_at_end() && peek() != '\n')
                    advance();
                break;
            default: return;
        }
    }
}

static Token match_identifier() {
    bool in_quotes = is_quote(prev());
    char current_quote = is_quote(prev()) ? prev() : '\0';
    while (!is_at_end() && (in_quotes || (!is_operator(peek()) && !is_space(peek())))) {
        if (is_quote(peek())) {
            if (peek() == current_quote) {
                in_quotes = false;
                current_quote = '\0';
            } else if (current_quote == '\0') {
                in_quotes = true;
                current_quote = peek();
            }
        }
        advance();
    }

    if (in_quotes) return make_error("\x1b[31mError:\x1b[0m Unterminated string");

    if (peek() == '<' || peek() == '>') { // Checks for file descriptors before redirectors.
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
            if (match_next("&")) return make_token(TOKEN_AND_AND);
            if (match_next(">>")) return make_token(TOKEN_AND_GREATER_GREATER);
            if (match_next(">")) return make_token(TOKEN_AND_GREATER);
            return make_token(TOKEN_AND);
        case '|':
            if (match_next("|")) return make_token(TOKEN_OR_OR);
            return make_token(TOKEN_OR);
        case '<':
            if (match_next("<")) return make_token(TOKEN_LESS_LESS);
            if (match_next(">")) return make_token(TOKEN_LESS_GREATER);
            if (match_next("&")) return make_token(TOKEN_LESS_AND);
            return make_token(TOKEN_LESS);
        case '>':
            if (match_next(">")) return make_token(TOKEN_GREATER_GREATER);
            if (match_next("&")) return make_token(TOKEN_GREATER_AND);
            return make_token(TOKEN_GREATER);
        default: return match_identifier();
    }
}
