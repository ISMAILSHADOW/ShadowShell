#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include "tokenizer.h"
#include "globals.h"


void test_tokenizer() {
    while (1) {
        printf("$ ");

        ssize_t bytes_read = getline(&line, &size, stdin);
        if (bytes_read == -1) 
            break;
            
        Token token;
        init_scanner();
        do {
            token = scan_token();
            if (token.type == TOKEN_ERROR) 
                printf("%s -> %s", token.start, line);
            printf("Type: %d | %.*s\n", token.type, token.length, token.start);
        } while(token.type != TOKEN_EOF);

        free(line);
        line = NULL;
    }
}

void test_parser() {
    while(1) {
        printf("$ ");

        ssize_t bytes_read = getline(&line, &size, stdin);
        if (bytes_read == -1) 
            break;

        ASTNode* node = run_parser();
#ifdef DEBUG
        print_ast(node);
#endif
        free(line);
        line = NULL;
    }
}

int main() {
    test_parser();
    return 0;
}