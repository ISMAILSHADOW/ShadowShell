#include "ast.h"
#include "globals.h"
#include "executor.h"
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>
#include <stdbool.h>
#include <ctype.h>
#include <sys/stat.h>
#include <signal.h>

OpenFDArray *process_fds;

static int execute_ast(ASTNode *node);

static inline void init_executor() {
    process_fds = OpenFDArray_create(&command_arena, 8);
    running_builtin = false;
    executing = true;
}

static inline void clean_executor() {
    executing = false;
}

static void backup_fd(int original) {
    int backup = dup(original);
    FDPair p = {.fd = {original, backup}};
    OpenFDArray_push(&command_arena, process_fds, p);
}

static void restore_fds() {
    for (ssize_t i = process_fds->size - 1;i >= 0;i--) {
        int original = process_fds->data[i].fd[0];
        int backup = process_fds->data[i].fd[1];
        if (backup == -1) {
            close(original);
        } else {
            dup2(backup, original);
            close(backup);
        }
    }
    process_fds->size = 0;
}

static bool success(int ret) {
    return ret == 0;
}


static bool redirect_file(char *path, int flags, int fd) {
    int file = open(path, flags, 0666);
    if (file == -1) {
        fprintf(stderr, "ShadowShell: %s: %s\n", path, strerror(errno));
        return false;
    }
    if (running_builtin) backup_fd(fd);
    if (dup2(file, fd) == -1) {
        fprintf(stderr, "ShadowShell: %s\n", strerror(errno));
        close(file);
        return false;
    }
    if (file != fd) close(file);
    return true;
}

static bool redirect_dup(const char *src, int fd) {
    if (running_builtin) backup_fd(fd);

    if (strcmp(src, "-") == 0) { 
        close(fd); 
        return true;
    }
    int from;
    if (!parse_int_const(src, strlen(src), &from) || dup2(from, fd) == -1) {
        fprintf(stderr, "ShadowShell: %s\n", strerror(errno));
        return false;
    }
    return true;
}

static bool handle_redirects(RedirectArray *reds) {
    for (size_t i = 0;i < reds->size;i++) {
        int fd1 = reds->data[i].fd;
        char *target = reds->data[i].target;
        bool ok;
        switch(reds->data[i].op) {
            case TOKEN_LESS:
                ok = redirect_file(target,  O_RDONLY, (fd1 == -1) ? STDIN_FILENO : fd1);
                break;
            case TOKEN_GREATER:
                ok = redirect_file(target, O_CREAT | O_WRONLY | O_TRUNC, (fd1 == -1) ? STDOUT_FILENO : fd1);
                break;
            case TOKEN_GREATER_GREATER:
                ok = redirect_file(target, O_CREAT | O_WRONLY | O_APPEND, (fd1 == -1) ? STDOUT_FILENO : fd1);
                break;
            case TOKEN_LESS_AND:
                ok = redirect_dup(target, (fd1 == -1) ? STDIN_FILENO : fd1);
                break;
            case TOKEN_GREATER_AND:
                ok = redirect_dup(target, (fd1 == -1) ? STDOUT_FILENO : fd1);
                break;
            case TOKEN_AND_GREATER: // Redirect both stdout and stderr (Overwrite)
                ok = redirect_file(target, O_CREAT | O_WRONLY | O_TRUNC, (fd1 == -1) ? STDOUT_FILENO : fd1);
                if (ok) {
                    if (running_builtin) backup_fd(STDERR_FILENO);
                    dup2(STDOUT_FILENO, STDERR_FILENO);
                }
                break;
            case TOKEN_AND_GREATER_GREATER: // Redirect both stdout and stderr (Append)
                ok = redirect_file(target, O_CREAT | O_WRONLY | O_APPEND, (fd1 == -1) ? STDOUT_FILENO : fd1);
                if (ok) {
                    if (running_builtin) backup_fd(STDERR_FILENO);
                    dup2(STDOUT_FILENO, STDERR_FILENO);
                }
                break;
            case TOKEN_LESS_GREATER:
                ok = redirect_file(target, O_CREAT | O_RDWR , (fd1 == -1) ? STDIN_FILENO : fd1);
                break;
            default:
                ok = false;
        }

        if (!ok) return false;
    }
    return true;
} 


static int cd_builtin(ArgsArray *args) {
    if (args->size > 2) {
        fprintf(stderr, "ShadowShell: cd: too many arguments\n");
        return COMMAND_FAILED;
    }

    if (args->size == 1) { // TODO: Look at this after understanding env vars
        const char *home = getenv("HOME");
        if (home && chdir(home) == -1) {
            fprintf(stderr, "ShadowShell: cd: %s: %s\n", home, strerror(errno));
            return COMMAND_FAILED;
        }
        return COMMAND_SUCCESSFUL;
    }

    if (chdir(args->data[1]) == -1) {
        fprintf(stderr, "ShadowShell: cd: %s: %s\n", args->data[1], strerror(errno));
        return COMMAND_FAILED;
    }
    return COMMAND_SUCCESSFUL;
}

static void exit_builtin() {
    exit(0);
}

static inline void take_terminal_control() {
    signal(SIGTTOU, SIG_IGN);
    tcsetpgrp(STDIN_FILENO, getpid());
    signal(SIGTTOU, SIG_DFL);
}

static inline void give_terminal_control(pid_t pid) {
    tcsetpgrp(STDIN_FILENO, pid);
}

static int handle_child_status(pid_t pid, pid_t pgid, int status) {
    int ret = -1;
    if (WIFEXITED(status)) {
        ret = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        int sig = WTERMSIG(status);
        if (sig == SIGINT) executing = false;
        fprintf(stderr, "%s\n", strsignal(sig));  
        ret = 128 + sig;  // Convention for termination by signals
    } else if (WIFSTOPPED(status)) {
        executing = false;
        printf("\nShadowShell: suspended: " YELLOW "[%d] " RESET "\n", jobs->size);
        ProcessArray_push(&jobs_arena, jobs, (Process){.pid=pid, .pgid=pgid, .running=false});
    }
    return ret;
}

static int wait_for_child(pid_t pid, pid_t pgid) {
    int status;
    if (waitpid(pid, &status, WUNTRACED) == -1) {
        if (!job_background) take_terminal_control();
        fprintf(stderr, "ShadowShell: waitpid: %s\n", strerror(errno));
        return COMMAND_FAILED;
    }

    if (!job_background) take_terminal_control();
    return handle_child_status(pid, pgid, status);
}

static int fg(ArgsArray *args) {
    if (jobs->size == 0) return 0;
    if (args->size > 2) {
        fprintf(stderr, "ShadowShell: fg: Expected 0 or 1 arguments (Job ID).\n");
        return COMMAND_FAILED;
    }

    int id = jobs->size - 1;
    if (args->size == 2) {
        if (!parse_int_const(args->data[1], strlen(args->data[1]), &id) || id < 0 || id >= jobs->size){
            fprintf(stderr, "ShadowShell: fg: Invalid Job ID [%d]\n", id);
            return COMMAND_FAILED;
        }
    }

    Process proc;
    ProcessArray_get(jobs, &proc, id);
    ProcessArray_remove(jobs, id);

    give_terminal_control(proc.pgid);
    // Negative is used to wake entire group. I think otherwize pipelines may break.
    if (!proc.running) kill(-proc.pgid, SIGCONT); 

    int ret = wait_for_child(proc.pid, proc.pgid);
    return ret;
}

static int bg(ArgsArray *args) {
    if (jobs->size == 0) return 0;
    if (args->size > 2) {
        fprintf(stderr, "ShadowShell: bg: Expected 0 or 1 arguments (Job ID).\n");
        return COMMAND_FAILED;
    }

    int id = jobs->size - 1;
    if (args->size == 2) {
        if (!parse_int_const(args->data[1], strlen(args->data[1]), &id) || id < 0 || id >= jobs->size){
            fprintf(stderr, "ShadowShell: bg: Invalid Job ID [%d]\n", id);
            return COMMAND_FAILED;
        }
    }

    if (jobs->data[id].running) {
        printf("bg: job already in background.\n", id);
        return 0;
    }

    kill(-jobs->data[id].pgid, SIGCONT);
    for (size_t i = 0;i < jobs->size;i++) {
        if (jobs->data[i].pgid == jobs->data[id].pgid)
            jobs->data[i].running = true;
    }

    printf("ShadowShell: continued: " YELLOW "[%d] " RESET "\n", id);
    return 0;
}

static int jobs_cmd() {
    for (size_t i = 0;i < jobs->size;i++) {
        Process proc = jobs->data[i];
        if (jobs->data[i].running)
            printf(YELLOW "[%d] is running. PID: %d PGID: %d" RESET "\n", i, proc.pid, proc.pgid);
        else
            printf(YELLOW "[%d] is suspended. PID: %d PGID: %d" RESET "\n", i, proc.pid, proc.pgid);
    }
    return 0;
}


static bool is_builtin(ASTNode *node) {
    if (node->type != NODE_COMMAND || node->cmd.args->size == 0) return false;
    const char *cmd = node->cmd.args->data[0];
    return strcmp(cmd, "cd") == 0 || strcmp(cmd, "exit") == 0 || strcmp(cmd, "true") == 0 || strcmp(cmd, "false") == 0
            || strcmp(cmd, "fg") == 0 || strcmp(cmd, "bg") == 0 || strcmp(cmd, "jobs") == 0;
}

static int run_builtin(ASTNode *node) {
    ArgsArray *args = node->cmd.args;
    const char *cmd = args->data[0];

    if (node->cmd.redirect->size != 0 && !handle_redirects(node->cmd.redirect)) {
        fprintf(stderr, "ShadowShell: %s: redirection failed\n", cmd);
        return COMMAND_FAILED;
    }

    if (strcmp(cmd, "cd") == 0) return cd_builtin(args);
    if (strcmp(cmd, "exit") == 0) exit_builtin();
    if (strcmp(cmd, "true") == 0) return 0;
    if (strcmp(cmd, "false") == 0) return 1;
    if (strcmp(cmd, "fg") == 0) return fg(args);
    if (strcmp(cmd, "bg") == 0) return bg(args);
    if (strcmp(cmd, "jobs") == 0) return jobs_cmd();

    return COMMAND_FAILED;
}

static int run_subprocess(ASTNode *node, bool background, bool already_forked) {
    if (is_builtin(node) && !background)  {
        running_builtin = true;
        int ret = run_builtin(node);
        restore_fds(); running_builtin = false;
        return ret;
    }

    int ret = -1; pid_t pid = -1;
    if (!already_forked) {
        pid = fork();

        if (background && pid == 0) {
            job_background = true;
            job_pgid = getpid();
        }

        setpgid(pid, job_background ? job_pgid : pid);
        if (!job_background && !background) give_terminal_control(pid);
    }
    
    if (pid == 0 || already_forked) {
        signal(SIGTSTP, SIG_DFL);
        if (node->type == NODE_COMMAND) {
            if (node->cmd.redirect->size != 0)
                if (!handle_redirects(node->cmd.redirect)) 
                    _exit(1);

            if (node->cmd.args->size == 0) _exit(0); // For redirect only commands

            ret = execvp(node->cmd.args->data[0], node->cmd.args->data);
            fprintf(stderr, "ShadowShell: %s: %s\n", node->cmd.args->data[0], strerror(errno));
        }
        else if (node->type == NODE_SUBSHELL) {
            ret = execute_ast(node->unary.child);
        }
        else {
            ret = execute_ast(node);
        } 
        _exit(ret);
    }

    if (pid == -1) {
        fprintf(stderr, "ShadowShell: fork: %s\n", strerror(errno));
        return COMMAND_FAILED;
    }

    if (background) {
        ProcessArray_push(&jobs_arena, jobs, (Process){.pid=pid, .pgid=pid, .running=true});
        printf(YELLOW "[%zu] %d" RESET "\n" , jobs->size - 1, pid);
        return 0;
    }

    return wait_for_child(pid, pid);
}



static void close_pipe_array(PipeArray *pipes) {
    for (size_t i = 0;i < pipes->size;i++) {
        close(pipes->data[i].fd[0]);
        close(pipes->data[i].fd[1]);
    }
}

static int run_pipeline(ASTNode *node) {
    size_t pline_size = node->list.array->size;
    PidArray *pids = PidArray_create(&command_arena, pline_size);
    PipeArray *pipes = PipeArray_create(&command_arena, pline_size - 1);
    
    for (size_t i = 1;i < pline_size;i++) {
        FDPair p;
        if (pipe(p.fd) == -1) {
            fprintf(stderr, "ShadowShell: pipe: %s\n", strerror(errno));
            close_pipe_array(pipes);
            return COMMAND_FAILED;
        }
        PipeArray_push(&command_arena, pipes, p);
    }

    pid_t group_id = -1;
    for (size_t i = 0;i < pline_size;i++) {
        ASTNode *cur = node->list.array->data[i];
        NodeType type = cur->type;

        pid_t pid = fork();
        if (group_id == -1) group_id = job_background ? job_pgid : pid;
        
        setpgid(pid, group_id);
        if (i == 0 && !job_background) give_terminal_control(group_id); // Call only for first process in the group. It's redundant.

        if (pid == 0) {
            signal(SIGTSTP, SIG_DFL);

            if (i > 0)  // We can read from i - 1
                dup2(pipes->data[i-1].fd[0], STDIN_FILENO);
            if (i < (pline_size - 1)) // We can write to i
                dup2(pipes->data[i].fd[1], STDOUT_FILENO);
            close_pipe_array(pipes);

            if (type == NODE_COMMAND || type == NODE_SUBSHELL) // We avoid forking twice in row a pipeline
                _exit(run_subprocess(cur, false, true));
            _exit(execute_ast(cur));
        } else if (pid == -1) {
            if (!job_background) take_terminal_control();
            for (size_t j = 0; j < pids->size; j++) {
                kill(pids->data[j], SIGKILL);
                waitpid(pids->data[j], NULL, 0);
            }
            close_pipe_array(pipes);
            return COMMAND_FAILED;
        }
        PidArray_push(&command_arena, pids, pid);
    }
    
    close_pipe_array(pipes);
    
    int status, ret = COMMAND_FAILED;
    for (size_t i = 0;i < pids->size;i++)  {
        waitpid(pids->data[i], &status, WUNTRACED);
        if (i == 0 && !job_background) take_terminal_control();
        ret = handle_child_status(pids->data[i], pids->data[0], status);
    }

    
    return ret;
}

static int execute_ast(ASTNode *node) {
    int ret = COMMAND_FAILED;

    if (node == NULL) return ret;
    switch (node->type) {
        case NODE_COMMAND_LIST:
            for (size_t i = 0;i < node->list.array->size;i++) {
                ret = execute_ast(node->list.array->data[i]);
                if (!executing) return ret;
            } 
                
            return ret;
        case NODE_LOGICAL_LIST:
            ret = execute_ast(node->logical.items->data[0].node);
            if (!executing) return ret;
            for (size_t i = 1;i < node->logical.items->size;i++) {
                Operator op = node->logical.items->data[i].op;
                if ((op == OP_AND_AND && success(ret)) || (op == OP_OR_OR && !success(ret)))
                    ret = execute_ast(node->logical.items->data[i].node);
                if (!executing) return ret;
            }
            return ret;
        case NODE_NEGATION:
            ret = execute_ast(node->unary.child);
            return !ret;
        case NODE_PIPELINE:
            ret = run_pipeline(node);
            return ret;
        case NODE_BACKGROUND:
            ret = run_subprocess(node->unary.child, true, false);
            return ret;
        case NODE_SUBSHELL:
        case NODE_COMMAND:
            ret = run_subprocess(node, false, false);
            return ret;
        default: 
            fprintf(stderr, "Maybe you added a new command without updating the executor?");
            _exit(1);
    }
}

int execute_command(char *line) {
    init_executor();
    ASTNode *root = run_parser(line);
    if (root == NULL) {
        clean_executor();
        return parser.error_pos ? COMMAND_FAILED : COMMAND_EMPTY;
    }
    int ret = execute_ast(root);
    clean_executor();
    return ret;
}