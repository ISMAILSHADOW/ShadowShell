#ifndef EXECUTOR_H
#define EXECUTOR_H

typedef struct {
    int fd[2];
} Pipe;

DECLARE_DYNAMIC_ARRAY(Pipe, PipeArray);
DECLARE_DYNAMIC_ARRAY(pid_t, PidArray);
int execute_command();

#endif