#ifndef GLOBALS_H
#define GLOBALS_H

#include "DataStructures/Arenas/DynamicArray.h"
#include "tokenizer.h"
#include "ast.h"
#include "executor.h"
#include <stdbool.h>
#include <stddef.h>

#define RED    "\x1b[31m"
#define GREEN  "\x1b[32m"
#define YELLOW "\x1b[33m"
#define BLUE   "\x1b[34m"
#define CYAN   "\x1b[36m"
#define RESET  "\x1b[0m"

#define KB(NUM) ((NUM) * 1024UL)
#define MB(NUM) ((NUM) * 1024UL * 1024UL)

extern char *line;
// extern char *command;
extern Parser parser;
extern Scanner scanner;
extern PidArray *bg_jobs;
extern size_t size;
extern ArenaAllocator bgjobs_arena, command_arena;
extern bool running_builtin, interactive;

extern const char *const COLOR_RED;
extern const char *const COLOR_GREEN;
extern const char *const COLOR_BLUE;
extern const char *const COLOR_CYAN;
extern const char *const COLOR_RESET;
extern const char *prompt_color;
void set_prompt_color(int ret);

typedef enum {
    COMMAND_UNKNOWN,
    ECHO,
    EXIT,
    CD
} CommandType;

typedef enum {
    INT_OVERFLOW,
} IntParseStatus;

typedef enum {
    COMMAND_EMPTY = -3,
    COMMAND_EXIT_SUCCESSFUL = -2,
    COMMAND_FAILED = -1,
    COMMAND_SUCCESSFUL = 0,
} CommandResult;

// Flags for command prefixes.
typedef struct {
    bool negate_ret_val;
    bool time_command;
} PrefixFlags;

extern PrefixFlags prefix_flags;


bool parse_int_const(const char *str, size_t len, int *val);


#endif