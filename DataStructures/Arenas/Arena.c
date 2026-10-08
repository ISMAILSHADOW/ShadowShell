#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include "Arena.h"

ArenaAllocator arena_create(size_t size) {
    uint8_t *memory = (uint8_t *)malloc(size);
    if (memory == NULL) {           
        fprintf(stderr, "Fatal Error: ArenaAllocator: Out of memory (malloc failed for %zu bytes).\n", size);
        _exit(EXIT_FAILURE);           
    }    
    ArenaAllocator arena = {.start = memory, .curr = memory, .capacity = size};
    return arena;
}

static inline size_t arena_size(ArenaAllocator *arena) {
    return arena->curr - arena->start;
}

static inline void *align_pointer(void *ptr) {
    return (void *)(((uintptr_t)ptr + 7) & ~7ull);
}

void *arena_alloc(ArenaAllocator *arena, size_t size) {
    if (arena == NULL || arena->curr == NULL) {
        fprintf(stderr, "Fatal Error: ArenaAllocator: Tried to allocate %zu bytes from an uninitialized Arena.\n", size);
        _exit(EXIT_FAILURE);
    }

    arena->curr = (uint8_t *)align_pointer(arena->curr); // For 8 byte alignment.
    if (arena_size(arena) >= arena->capacity || size > arena->capacity - arena_size(arena)) { // We'll keep it like this for now.
        fprintf(stderr, "Fatal Error: ArenaAllocator: Arena is full. Consider increasing the size in code or putting a limit on the input.\n");
        _exit(EXIT_FAILURE);
    }

    void *ret = arena->curr;
    arena->curr += size;
    return ret;
}

char *arena_strndup(ArenaAllocator *arena, const char *s, size_t n) {
    if (s == NULL) return NULL;

    size_t len = 0;
    while (len < n && s[len] != '\0') len++;
    
    char *ret = arena_alloc(arena, len + 1);
    memcpy(ret, s, len);
    ret[len] = '\0';
    return ret;
}

void arena_reset(ArenaAllocator *arena) {
    arena->curr = arena->start;
}

void arena_free(ArenaAllocator *arena) {
    free(arena->start);
}