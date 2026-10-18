#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include "globals.h"
#include "executor.h"
#include "DataStructures/Arenas/DynamicArray.h"

static char *line;

static void reap_background_processes() {
    for (int i = jobs->size - 1;i >= 0;i--) {
        int status;
        pid_t pid = waitpid(jobs->data[i].pid, &status, WNOHANG);
        if (pid == jobs->data[i].pid) {
            ProcessArray_remove(jobs, i);
            printf(YELLOW "[%d] " RESET "DONE\n", i);
        } else if (pid == -1) {
            ProcessArray_remove(jobs, i);
        }
    }
}

void sigint_handler(int signum) {
    (void) signum;
    if (!executing) write(STDOUT_FILENO, "\n" YELLOW "$ " RESET, 12);
}

static void handle_signals() {
    struct sigaction sa = {.sa_handler=sigint_handler, .sa_flags=SA_RESTART};
    sigaction(SIGINT, &sa, NULL);
    signal(SIGTSTP, SIG_IGN);
}

static void initialize() {
    command_arena = arena_create(MB(40));
    jobs_arena = arena_create(MB(20));
    jobs = ProcessArray_create(&jobs_arena, 8);
    jobs = ProcessArray_create(&jobs_arena, 8);
    job_pgid = -1;
    job_background = false;
    line = NULL;
    size = 0;
    interactive = isatty(STDIN_FILENO);
    handle_signals();
}

void loop_start() {
}

static void loop_end() {
    reap_background_processes();
    arena_reset(&command_arena);
}

// For later to handle running scripts.
int exec_from_file(char * filePath) {
    fprintf(stderr, "exec_from_file IS NOT IMPLEMENTED YET!");
    _exit(1);
}

void clean_exit() {
    for (size_t i = 0;i < jobs->size;i++) 
        kill(jobs->data[i].pid, SIGHUP);
}

int main(int argc, char **argv) {
    if (argc > 2) 
        return -1;
    
    if (argc == 2) 
        return exec_from_file(argv[1]);
    initialize();
    int last_status = 0;
    while (1) {
        // loop_start();

        if (interactive)
            printf("%s$ %s", prompt_color, COLOR_RESET);

        ssize_t bytes_read = getline(&line, &size, stdin);
        if (bytes_read == -1) 
            break;
        else if (bytes_read > (ssize_t)MAX_LINE) {
            fprintf(stderr, "ShadowShell: Command is too long. Maximum command size is %ld", MAX_LINE);
            continue;
        }

        int ret = execute_command(line);
        if (ret != COMMAND_EMPTY)
            last_status = ret;
        set_prompt_color(ret);

        loop_end();
    }

    clean_exit();
    return last_status;
}