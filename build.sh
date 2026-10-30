#!/usr/bin/bash
CFLAGS="-O3 -I./include -std=gnu23 -flto -Wall -Wextra -Wconversion -Wsign-conversion -Wpedantic -Werror" 
DEBUG=false

TEST_FILES=$(find tests -type f -name "*.c" 2>/dev/null)

MAIN_FILES=$(find src -type f -name "*.c" | grep -v "^src/main\.c$")

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
    gcc src/main.c $MAIN_FILES $CFLAGS -o ShadowShell
elif [[ "$TYPE" == "test" ]]; then
    gcc $TEST_FILES $MAIN_FILES $CFLAGS -o ShadowShell
fi
