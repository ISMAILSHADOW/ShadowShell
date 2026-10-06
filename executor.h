#ifndef EXECUTOR_H
#define EXECUTOR_H

typedef struct {
    int fd[2];
} FDPair;

DECLARE_DYNAMIC_ARRAY(FDPair, PipeArray);
DECLARE_DYNAMIC_ARRAY(pid_t, PidArray);
DECLARE_DYNAMIC_ARRAY(FDPair, OpenFDArray)
int execute_command();

#endif