#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include "StringBuilder.h"
#include "Arena.h"

StringBuilder sb_create(ArenaAllocator *arena, size_t capacity) {
    StringBuilder sb;
    sb.buf = (char *)arena_alloc(arena, capacity + 1);
    sb.size = 0;
    sb.buf[sb.size] = '\0';
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

static inline char *align_pointer(char *ptr) {
    return (char *)(((uintptr_t)ptr + 7) & ~7ull);
}

void sb_append(ArenaAllocator *arena, StringBuilder *s1, StringBuilder *s2) {
    size_t needed = s1->size + s2->size;

    if (align_pointer(s1->buf + s1->capacity + 1) == s2->buf) { // If allocated sequantially in arena merge
        memmove(s1->buf + s1->size, s2->buf, s2->size + 1);
        s1->size = needed;
        s1->capacity = (size_t) (s2->buf - s1->buf) + s2->capacity;
        s2->buf = NULL; s2->size = 0; s2->capacity = 0;
        return;
    }

    if (needed > s1->capacity) 
        sb_extend(arena, s1, needed * 2);
    
    memcpy(s1->buf + s1->size, s2->buf, s2->size);
    s1->size = needed;
    s1->buf[s1->size] = '\0';
}


void sb_append_cstring(ArenaAllocator *arena, StringBuilder *s1, const char *s2) {
    if (!s2) return;
    size_t s2_size = strlen(s2); 
    size_t needed = s1->size + s2_size;
    if (needed > s1->capacity) 
        sb_extend(arena, s1, needed * 2);
    
    memcpy(s1->buf + s1->size, s2, s2_size);
    s1->size = needed;
    s1->buf[s1->size] = '\0';
}

void sb_appendf(ArenaAllocator *arena, StringBuilder *s, const char *fmt, ...) {
    va_list list, list2;
    va_start(list, fmt);
    va_copy(list2, list);

    int n = vsnprintf(NULL, 0, fmt, list2); // Somehow this calculates the number of bytes of the formatted string.
    va_end(list2);
    if (n <= 0) {
        va_end(list);
        return;
    }

    size_t needed = s->size + (size_t) n;
    if (needed > s->capacity)
        sb_extend(arena, s, needed * 2);

    vsnprintf(s->buf + s->size,(size_t) n + 1, fmt, list);
    s->size = needed;
    va_end(list);
}

void sb_strip_right(StringBuilder *sb, const char *strip) {
    if (!sb || !sb->buf) return;
    while (sb->size > 0 && strchr(strip, sb->buf[sb->size-1])) sb->size--;
    sb->buf[sb->size] = '\0'; 
}

void sb_strip_left(StringBuilder *sb, const char *strip) {
    if (!sb || !sb->buf) return;
    size_t leading = 0;
    while (leading < sb->size && strchr(strip, sb->buf[leading])) leading++;

    if (leading > 0) {
        memmove(sb->buf, sb->buf + leading, sb->size - leading);
        sb->size -= leading;
    }

    sb->buf[sb->size] = '\0'; 
}

void sb_strip(StringBuilder *sb, const char *strip) {
    if (!sb || !sb->buf) return;
    sb_strip_left(sb, strip);
    sb_strip_right(sb, strip);
}


// Compares two Strings ignoring any character in const char *ign.
bool sb_cmp_ign(StringBuilder *s1, StringBuilder *s2, const char *ign) {
    if (!s1 || !s1->buf || !s2 || !s2->buf) return false;
    if (!ign) ign = "";
    size_t i1 = 0, i2 = 0;
    while (true) {
        while (i1 < s1->size && strchr(ign, s1->buf[i1])) i1++;
        while (i2 < s2->size && strchr(ign, s2->buf[i2])) i2++;

        if (i1 == s1->size || i2 == s2->size) break; 
        if (s1->buf[i1] != s2->buf[i2]) return false;
        i1++; i2++;
    }

    return i1 == s1->size && i2 == s2->size;
} 