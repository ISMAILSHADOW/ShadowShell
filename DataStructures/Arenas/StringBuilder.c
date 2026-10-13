#include <unistd.h>
#include <string.h>
#include "StringBuilder.h"
#include "Arena.h"

StringBuilder sb_create(ArenaAllocator *arena, size_t capacity) {
    StringBuilder sb;
    sb.buf = arena_alloc(arena, capacity + 1);
    sb.buf[capacity] = '\0';
    sb.size = 0;
    sb.capacity = capacity;
    return sb;
}

StringBuilder sb_create_cnstring(ArenaAllocator *arena, const char *c, size_t n) {
    StringBuilder sb;
    sb.buf = arena_strndup(arena, c, n);
    sb.size = sb.buf ? strlen(sb.buf) : 0;
    sb.capacity = sb.size;
    return sb;
}

StringBuilder sb_create_cstring(ArenaAllocator *arena, const char *c) {
    return sb_create_cnstring(arena, c, c ? strlen(c) : 0);
}

void sb_extend(ArenaAllocator *arena, StringBuilder *s, size_t capacity) {
    if (capacity <= s->capacity) return;

    s->buf = (char *)arena_realloc(arena, s->buf, s->capacity + 1, capacity + 1);
    s->capacity = capacity;
}


void sb_append(ArenaAllocator *arena, StringBuilder *s1, const StringBuilder *s2) {
    size_t needed = s1->size + s2->size;
    if (needed > s1->capacity) {
        sb_extend(arena, s1, needed * 2);
    }
    memcpy(s1->buf + s1->size, s2->buf, s2->size);
    s1->size = needed;
    s1->buf[s1->size] = '\0';
}