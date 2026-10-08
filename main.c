#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include "globals.h"
#include "executor.h"
#include "DataStructures/Arenas/DynamicArray.h"

static void reap_background_processes() {
    for (int i = bg_jobs->size - 1;i >= 0;i--) {
        int status;
        pid_t pid = waitpid(bg_jobs->data[i], &status, WNOHANG);
        if (pid == bg_jobs->data[i]) {
            PidArray_remove(bg_jobs, i);
            printf(YELLOW "[%d] " RESET "DONE\n", i);
        } else if (pid == -1) {
            PidArray_remove(bg_jobs, i);
        }
    }
}

static void initialize() {
    command_arena = arena_create(MB(2));
    bgjobs_arena = arena_create(KB(2));
    bg_jobs = PidArray_create(&bgjobs_arena, 8);
    line = NULL;
    size = 0;
    interactive = isatty(STDIN_FILENO);
}

void loop_start() {
}

static void loop_end() {
    reap_background_processes();
    arena_reset(&command_arena);
}

// For later to handle running scripts.
int exec_from_file(char * filePath);

int main(int argc, char **argv) {
    if (argc > 2) 
        return -1;
    
    // if (argc == 2) 
    //     return exec_from_file(argv[1]);

    initialize();
    while (1) {
        // loop_start();

        if (interactive)
            printf("%s$ %s", prompt_color, COLOR_RESET);

        ssize_t bytes_read = getline(&line, &size, stdin);
        if (bytes_read == -1) 
            break;

        int ret = execute_command();
        if (ret == COMMAND_EXIT_SUCCESSFUL)
            break;
        set_prompt_color(ret);

        loop_end();
    }

    // clean_exit();
    return 0;
}