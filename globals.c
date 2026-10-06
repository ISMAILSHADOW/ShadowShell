#include "DynamicArray.h"
#include "globals.h"
#include "tokenizer.h"
#include "ast.h"
#include "executor.h"
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>


const char *const COLOR_RED   = "\x1b[31m";
const char *const COLOR_GREEN = "\x1b[32m";
const char *const COLOR_BLUE  = "\x1b[34m";
const char *const COLOR_CYAN  = "\x1b[36m";
const char *const COLOR_RESET = "\x1b[0m";

DEFINE_DYNAMIC_ARRAY(char *, ArgsArray)
DEFINE_DYNAMIC_ARRAY(ASTNode *, NodeArray)
DEFINE_DYNAMIC_ARRAY(Redirect, RedirectArray)
DEFINE_DYNAMIC_ARRAY(LogicalNode, LogicalArray)
DEFINE_DYNAMIC_ARRAY(FDPair, PipeArray);
DEFINE_DYNAMIC_ARRAY(pid_t, PidArray);
DEFINE_DYNAMIC_ARRAY(FDPair, OpenFDArray)

char *line;
Scanner scanner;
Parser parser;
PidArray *bg_jobs;
size_t size;
const char *prompt_color = "\x1b[36m";
bool running_builtin, interactive;

void set_prompt_color(int ret) {
    if (ret == COMMAND_EMPTY)
        prompt_color = COLOR_CYAN;
    else if (ret != COMMAND_SUCCESSFUL)
        prompt_color = COLOR_RED;
    else
        prompt_color = COLOR_GREEN;
}

// To parse integers from token strings.
bool parse_int_const(const char *str, size_t len, int *val) {
    char buf[32]; 
    if (len >= sizeof(buf)) 
        return false;

    memcpy(buf, str, len);
    buf[len] = '\0';

    char *endptr; 
    errno = 0; // errno must be zeroed as the man page says strtol doesn't reset it on success
    long result = strtol(buf, &endptr, 10);

    if (endptr == buf || *endptr != '\0') return false;
    
    // Check overflow, underflow and int range
    if (result < INT32_MIN || result > INT32_MAX || errno == ERANGE) return false;
    *val = (int)result;
    return true;
}