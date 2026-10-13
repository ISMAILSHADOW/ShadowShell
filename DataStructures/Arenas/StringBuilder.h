#include <unistd.h>
#include "Arena.h"

typedef struct {
    char *buf;
    size_t size;
    size_t capacity;
} StringBuilder;

StringBuilder sb_create(ArenaAllocator *arena, size_t capacity);
StringBuilder sb_create_cstring(ArenaAllocator *arena, char *c);
StringBuilder sb_create_cnstring(ArenaAllocator *arena, char *c, size_t n);
void sb_extend(ArenaAllocator *arena, StringBuilder *s, size_t capacity);
void sb_append(ArenaAllocator *arena, StringBuilder *s1, StringBuilder *s2);