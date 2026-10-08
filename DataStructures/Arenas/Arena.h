#ifndef ARENA_H
#define ARENA_H
#include <stdint.h>
#include <sys/types.h>

typedef struct {
    uint8_t *start;
    uint8_t *curr;
    size_t capacity;
} ArenaAllocator;

ArenaAllocator arena_create(size_t size);
void *arena_alloc(ArenaAllocator *arena, size_t size);
void arena_reset(ArenaAllocator *arena);
void arena_free(ArenaAllocator *arena);
char *arena_strndup(ArenaAllocator *arena, const char *s, size_t n);
#endif