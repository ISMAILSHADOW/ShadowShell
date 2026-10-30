#ifndef EXECUTOR_H
#define EXECUTOR_H
#include "DynamicArray.h"
#include <stdbool.h>
#include <sys/types.h>

typedef struct {
    int fd[2];
} FDPair;

typedef struct {
    pid_t pid;
    pid_t pgid;
    bool running;
} Process;

DECLARE_DYNAMIC_ARRAY(FDPair, PipeArray)
DECLARE_DYNAMIC_ARRAY(Process, ProcessArray)
DECLARE_DYNAMIC_ARRAY(pid_t, PidArray)
DECLARE_DYNAMIC_ARRAY(FDPair, OpenFDArray)
int execute_command(char *line);

#endif