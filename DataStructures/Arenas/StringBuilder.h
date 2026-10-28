#ifndef STRING_BUILDER_H
#define STRING_BUILDER_H

#include <unistd.h>
#include <stdbool.h>
#include "Arena.h"

typedef struct {
    char *buf;
    size_t size;
    size_t capacity;
} StringBuilder;

StringBuilder sb_create(ArenaAllocator *arena, size_t capacity);
StringBuilder sb_create_cstring(ArenaAllocator *arena, const char *c);
StringBuilder sb_create_cnstring(ArenaAllocator *arena, const char *c, size_t n);
void sb_extend(ArenaAllocator *arena, StringBuilder *s, size_t capacity);
void sb_append(ArenaAllocator *arena, StringBuilder *s1, StringBuilder *s2);
void sb_append_cstring(ArenaAllocator *arena, StringBuilder *s1, const char *s2);
void sb_appendf(ArenaAllocator *arena, StringBuilder *s, const char *fmt, ...);
void sb_strip_right(StringBuilder *sb, const char *strip);
void sb_strip_left(StringBuilder *sb, const char *strip);
void sb_strip(StringBuilder *sb, const char *strip);
bool sb_cmp_ign(StringBuilder *s1, StringBuilder *s2, const char *ign);
#endif