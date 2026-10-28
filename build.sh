#!/usr/bin/bash
CFLAGS="-O3 -std=gnu23 -flto -Wall -Wextra -Wconversion -Wsign-conversion -Wpedantic -Werror" 
DEBUG=false

TEST_FILES="tests/test.c tests/ast_debug.c"
MAIN_FILES="ast.c executor.c globals.c tokenizer.c DataStructures/Arenas/Arena.c DataStructures/Arenas/StringBuilder.c"

for arg in "$@"; do
    if [[ "$arg" == "-d" || "$arg" == "-debug" ]]; then
        DEBUG=true
    fi
done

TYPE=""
for arg in "$@"; do
    if [[ "$arg" != "-d" && "$arg" != "-debug" ]]; then
        TYPE="$arg"
    fi
done

if [[ "$DEBUG" == true ]] || [[ "$TYPE" == "test" ]]; then
    CFLAGS="$CFLAGS -ggdb -DDEBUG"
fi

if [[ "$TYPE" == "release" ]] || [[ -z "$TYPE" ]]; then
    gcc main.c $MAIN_FILES $CFLAGS -o ShadowShell
elif [[ "$TYPE" == "test" ]]; then
    gcc $TEST_FILES $MAIN_FILES $CFLAGS -o ShadowShell
fi
