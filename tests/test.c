#include <stdio.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include "ast_debug.h"
#include "../globals.h"
#include "../DataStructures/Arenas/StringBuilder.h"

#define CHECK(c) do { if (!(c)) { \
    fprintf(stderr, "  %s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #c); \
    _exit(1); } } while (0)
    
#define CHECK_STR(got, want) do { if (strcmp((got), (want)) != 0) { \
    fprintf(stderr, "  %s:%d: expected \"%s\" but got \"%s\"\n", __FILE__, __LINE__, (want), (got)); \
    _exit(1); } } while (0)
    
// #include "../tokenizer.h"
// static void test_tokenizer_manually(char *line) {
//     while (1) {
//         printf("$ ");

//         ssize_t bytes_read = getline(&line, &size, stdin);
//         if (bytes_read == -1) 
//             break;
            
//         Token token;
//         init_scanner(line);
//         do {
//             token = scan_token();
//             if (token.type == TOKEN_ERROR) 
//                 printf("%s -> %s", token.start, line);
//             printf("Type: %d | %.*s\n", token.type, token.length, token.start);
//         } while(token.type != TOKEN_EOF);

//         line = NULL;
//     }
// }

// static void test_parser_manually(char *line) {
//     while(1) {
//         printf("$ ");

//         ssize_t bytes_read = getline(&line, &size, stdin);
//         if (bytes_read == -1) 
//             break;

//         ASTNode* node = run_parser(line);
// #ifdef DEBUG
//         print_ast(node);
// #endif
//     }
// }

typedef struct {
    const char *name;
    uint32_t timeout;
    void (*fn)(void);
} TestCase;

static bool run_test(TestCase *t) {
    pid_t pid = fork();
    if (pid == 0) {
        int fd = open("./test.log", O_WRONLY | O_CLOEXEC | O_CREAT | O_TRUNC, 0666);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        alarm(t->timeout);
        t->fn();
        _exit(0);
    }
    int wstatus;
    if (waitpid(pid, &wstatus, 0) == -1) 
        return false;

    if (WIFEXITED(wstatus) && WEXITSTATUS(wstatus) == 0) {
        printf(GREEN "[ ok ] %s\n" RESET, t->name);
        return true;
    } else if (WIFSIGNALED(wstatus)) {
        if (WTERMSIG(wstatus) == SIGALRM) printf(RED "[FAIL] %s: Process Timed Out After %d Seconds\n" RESET, t->name, t->timeout);
        else printf(RED "[FAIL] Signaled %s: %s\n" RESET, t->name, strsignal(WTERMSIG(wstatus)));
    } 
    else printf(RED "[FAIL] %s\n" RESET, t->name);

    return false;
}

void ParserTest1() {
    command_arena = arena_create(KB(2));
    ASTNode *node = run_parser("! a | b && c || sleep 1");
    
    StringBuilder sb = sb_create(&command_arena, 16);
    ast_to_string(&command_arena, &sb, node);
    
    StringBuilder expected = sb_create_cstring(&command_arena, 
    "LOGICAL ( NOT ( PIPELINE ( COMMAND ( \"a\" ) |"
    "COMMAND ( \"b\" ) ) ) && COMMAND ( \"c\" ) || COMMAND ( \"sleep\" \"1\" ) )");
    CHECK(sb_cmp_ign(&sb, &expected, " "));
    _exit(0);
}

void ParserTest2() {
    command_arena = arena_create(KB(1));
    ASTNode *node = run_parser("(");
    CHECK(node == NULL);
    _exit(0);
}

void ParserTest3() {
    command_arena = arena_create(KB(2));
    ASTNode *node = run_parser("ls -l | grep txt | wc -l");
    
    StringBuilder sb = sb_create(&command_arena, 16);
    StringBuilder expected = sb_create_cstring(&command_arena, 
        "PIPELINE ( COMMAND ( \"ls\" \"-l\" ) | "
        "COMMAND ( \"grep\" \"txt\" ) | "
        "COMMAND ( \"wc\" \"-l\" ) )");
    
    ast_to_string(&command_arena, &sb, node);
    CHECK(sb_cmp_ign(&sb, &expected, " "));
    _exit(0);
}

void ParserTest4() {
    command_arena = arena_create(KB(2));
    ASTNode *node = run_parser("(cd /tmp && ls) | grep foo");
    
    StringBuilder sb = sb_create(&command_arena, 16);
    StringBuilder expected = sb_create_cstring(&command_arena, 
        "PIPELINE ( SUBSHELL ( LOGICAL ( COMMAND ( \"cd\" \"/tmp\" ) && COMMAND ( \"ls\" ) ) ) | "
        "COMMAND ( \"grep\" \"foo\" ) )");
    
    ast_to_string(&command_arena, &sb, node);
    CHECK(sb_cmp_ign(&sb, &expected, " "));
    _exit(0);
}

void ParserTest5() {
    command_arena = arena_create(KB(2));
    ASTNode *node = run_parser("cat < input.txt > output.txt");
    
    StringBuilder sb = sb_create(&command_arena, 16);
    StringBuilder expected = sb_create_cstring(&command_arena, 
        "COMMAND ( \"cat\" [< input.txt] [> output.txt] )");
    
    ast_to_string(&command_arena, &sb, node);
    CHECK(sb_cmp_ign(&sb, &expected, " "));
    _exit(0);
}

void ParserTest6() {
    command_arena = arena_create(KB(2));
    ASTNode *node = run_parser("sleep 1 & echo done");
    
    StringBuilder sb = sb_create(&command_arena, 16);
    StringBuilder expected = sb_create_cstring(&command_arena, 
        "COMMAND_LIST ( BACKGROUND ( COMMAND ( \"sleep\" \"1\" ) ) ; COMMAND ( \"echo\" \"done\" ) )");
    
    ast_to_string(&command_arena, &sb, node);
    CHECK(sb_cmp_ign(&sb, &expected, " "));
    _exit(0);
}

void ParserTest7() {
    command_arena = arena_create(KB(2));
    ASTNode *node = run_parser("ls | ");
    
    CHECK(node == NULL);
    _exit(0);
}



static void SBTest1() {
    command_arena = arena_create(KB(2));
    StringBuilder sb = sb_create(&command_arena, 8);
    CHECK(sb.size == 0);
    CHECK(sb.buf[0] == '\0'); 
}
 
static void SBTest2() {
    command_arena = arena_create(KB(2));
    StringBuilder sb = sb_create(&command_arena, 1);
    for (int i = 0; i < 100; i++)
        sb_append_cstring(&command_arena, &sb, "ab");
    CHECK(sb.size == 200);
    CHECK(strlen(sb.buf) == 200);
    CHECK(strncmp(sb.buf, "abab", 4) == 0);
    CHECK(sb.buf[198] == 'a' && sb.buf[199] == 'b');
    CHECK(sb.buf[200] == '\0');
}
 
static void SBTest3() {
    command_arena = arena_create(KB(2));
    StringBuilder sb = sb_create(&command_arena, 2);   
    sb_appendf(&command_arena, &sb, "%d-%s", 42, "xyz");
    sb_appendf(&command_arena, &sb, "[%5.2f]", 3.14159);
    sb_appendf(&command_arena, &sb, "%s", "");    
    
    CHECK_STR(sb.buf, "42-xyz[ 3.14]");
    CHECK(sb.size == 13);
}
 
static void SBTest4() {
    command_arena = arena_create(KB(2));
    StringBuilder a = sb_create_cstring(&command_arena, "ab");
    arena_alloc(&command_arena, 100);                
    StringBuilder b = sb_create_cstring(&command_arena, "cd");
    sb_append(&command_arena, &a, &b);
    CHECK_STR(a.buf, "abcd");
    CHECK(a.size == 4);
    CHECK_STR(b.buf, "cd");
}
 
// sb_append has a shortcut when the second builder sits right after the first in the arena. They should merge.
static void SBTest5() {
    command_arena = arena_create(KB(2));
    StringBuilder a = sb_create(&command_arena, 8);
    sb_append_cstring(&command_arena, &a, "ab");
    StringBuilder b = sb_create_cstring(&command_arena, "xyz");  
    sb_append(&command_arena, &a, &b);
 
    CHECK_STR(a.buf, "abxyz");
    CHECK(a.size == 5);
    CHECK(b.buf == NULL);                                        
 
    void *next = arena_alloc(&command_arena, 8);
    CHECK((char *)next >= a.buf + a.capacity + 1);              
 
    sb_append_cstring(&command_arena, &a, "123456789012");
    CHECK_STR(a.buf, "abxyz123456789012");
}
 
static void SBTest6() {
    command_arena = arena_create(KB(2));
    StringBuilder a = sb_create_cstring(&command_arena, " \t abc \n");
    sb_strip(&a, " \t\n");
    CHECK_STR(a.buf, "abc");
    CHECK(a.size == 3);
 
    StringBuilder b = sb_create_cstring(&command_arena, " \t\n");
    sb_strip(&b, " \t\n");
    CHECK_STR(b.buf, "");
    CHECK(b.size == 0);
 
    StringBuilder c = sb_create_cstring(&command_arena, "  x  ");
    sb_strip_left(&c, " ");
    CHECK_STR(c.buf, "x  ");
    sb_strip_right(&c, " ");
    CHECK_STR(c.buf, "x");
 
    StringBuilder d = sb_create_cstring(&command_arena, "keep");
    sb_strip(&d, "");
    CHECK_STR(d.buf, "keep");
}
 
static void SBTest7() {
    command_arena = arena_create(KB(2));
    StringBuilder a   = sb_create_cstring(&command_arena, "a b  c");
    StringBuilder abc = sb_create_cstring(&command_arena, "abc");
    StringBuilder abd = sb_create_cstring(&command_arena, "abd");
    StringBuilder ab  = sb_create_cstring(&command_arena, "ab");
    StringBuilder blank = sb_create_cstring(&command_arena, "   ");
    StringBuilder none  = sb_create_cstring(&command_arena, "");
 
    CHECK(sb_cmp_ign(&a, &abc, " "));
    CHECK(!sb_cmp_ign(&a, &abc, ""));
    CHECK(!sb_cmp_ign(&a, &abc, NULL)); 
    CHECK(!sb_cmp_ign(&abc, &abd, " "));
    CHECK(!sb_cmp_ign(&abc, &ab, " ")); 
    CHECK(!sb_cmp_ign(&ab, &abc, " "));
    CHECK(sb_cmp_ign(&blank, &none, " "));
 
    // Known blind spot: spaces are ignored everywhere, including inside quotes, so these compare equal.
    // Tests that care about spaces inside a word must inspect the AST directly
    StringBuilder q1 = sb_create_cstring(&command_arena, "\"a b\"");
    StringBuilder q2 = sb_create_cstring(&command_arena, "\"ab\"");
    CHECK(sb_cmp_ign(&q1, &q2, " "));
}
 
static void SBTest8() {
    command_arena = arena_create(KB(2));
    StringBuilder sb = sb_create_cstring(&command_arena, NULL);
    CHECK(sb.size == 0);
    sb_append_cstring(&command_arena, &sb, "x"); 
    CHECK_STR(sb.buf, "x");
}

#define NUM_TESTS 15

TestCase tests[NUM_TESTS] = {
    {.name="Parser 1: (Logical + Negation)",        .timeout=2, .fn=ParserTest1},
    {.name="Parser 2: (Unclosed Subshell)",         .timeout=2, .fn=ParserTest2},
    {.name="Parser 3: (Multi-Pipeline)",            .timeout=2, .fn=ParserTest3},
    {.name="Parser 4: (Subshell Pipeline)",         .timeout=2, .fn=ParserTest4},
    {.name="Parser 5: (Redirections)",              .timeout=2, .fn=ParserTest5},
    {.name="Parser 6: (Background List)",           .timeout=2, .fn=ParserTest6},
    {.name="Parser 7: (Hanging Pipe Error)",        .timeout=2, .fn=ParserTest7},
    {.name="SB 1: empty builder is terminated",     .timeout=2, .fn=SBTest1,},
    {.name="SB 2: append grows",                    .timeout=2, .fn=SBTest2,},
    {.name="SB 3: appendf formats and grows",       .timeout=2, .fn=SBTest3,},
    {.name="SB 4: append builder, separate buffers",.timeout=2, .fn=SBTest4,},
    {.name="SB 5: append builder, adjacent buffers",.timeout=2, .fn=SBTest5,},
    {.name="SB 6: strip",                           .timeout=2, .fn=SBTest6,},
    {.name="SB 7: compare ignoring characters",     .timeout=2, .fn=SBTest7,},
    {.name="SB 8: NULL input",                      .timeout=2, .fn=SBTest8,},
};

int main(void) {
    int failures = 0;
    for (int i = 0; i < NUM_TESTS; i++) {
        if (!run_test(&tests[i]))
            failures++;
    }
    printf("Tests: %d passed, %d failed, %d total\n", NUM_TESTS - failures, failures, NUM_TESTS);
    return failures ? 1 : 0;
}