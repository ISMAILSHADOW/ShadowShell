#ifndef DYNAMIC_ARRAY_H
#define DYNAMIC_ARRAY_H

#include <stdint.h>
#include <string.h>
#include "Arena.h"

#define DECLARE_DYNAMIC_ARRAY(T, name) \
typedef struct {                      \
    size_t size;                      \
    size_t capacity;                  \
    T *data;                          \
} name;                               \
                                      \
                                      \
name *name##_create(ArenaAllocator *arena, size_t capacity); \
                                      \
void name##_resize(ArenaAllocator *arena, name *arr, size_t new_capacity); \
                                      \
void name##_push(ArenaAllocator *arena, name *arr, T element);\
                                      \
int name##_pop(name *arr, T *element); \
                                      \
int name##_get(name *arr, T *element, size_t index); \
                                      \
int name##_remove(name *arr, size_t index); \
                                      \
void name##_destroy(ArenaAllocator *arena, name *arr);       \


// A generic implementation of a dynamic array using arenas for allocation. 
// It also keeps the array NULL terminated.

#define DEFINE_DYNAMIC_ARRAY(T, name)                       \
                                      \
name *name##_create(ArenaAllocator *arena, size_t capacity) { \
    name *arr = arena_alloc(arena, sizeof(name));           \
    arr->size = 0;                                          \
    arr->capacity = (capacity < 1) ? 1 : capacity;          \
    arr->data = arena_alloc(arena, sizeof(T) * arr->capacity);\
    memset(arr->data, 0, sizeof(T) * arr->capacity);          \
    return arr;                                             \
}                                                           \
                                                            \
void name##_resize(ArenaAllocator *arena, name *arr, size_t new_capacity) { \
    if (arr == NULL) return;                                \
    if (new_capacity < 1) new_capacity = 1;                 \
                                                            \
    if (arr->capacity < new_capacity) {                     \
        T *new_data = arena_alloc(arena, sizeof(T) * new_capacity); \
        memcpy(new_data, arr->data, sizeof(T) * arr->size); \
        arr->data = new_data;                               \
    }                                                       \
    arr->capacity = new_capacity;                           \
                                                            \
    if (arr->size >= arr->capacity)                         \
        arr->size = arr->capacity - 1;                      \
                                                            \
    memset(&arr->data[arr->size], 0, sizeof(T));            \
}                                                           \
                                                            \
void name##_push(ArenaAllocator *arena, name *arr, T element) { \
    if (arr == NULL) return;                                \
                                                            \
    if (arr->size + 1 >= arr->capacity) {                 \
        size_t new_capacity = (arr->capacity < 2) ? 4 : arr->capacity * 2; \
        name##_resize(arena, arr, new_capacity);                 \
    }                                                       \
                                                            \
    arr->data[arr->size++] = element;                       \
    memset(&arr->data[arr->size], 0, sizeof(T));          \
}                                                           \
                                                            \
int name##_pop(name *arr, T *element) { \
    if (arr == NULL || element == NULL || arr->size == 0) \
        return -1;                                          \
                                                            \
    *element = arr->data[--arr->size];                      \
    memset(&arr->data[arr->size], 0, sizeof(T));          \
    return 0;                                               \
}                                                           \
                                                            \
int name##_get(name *arr, T *element, size_t index) { \
    if (arr == NULL || element == NULL || index >= arr->size) return -1; \
    *element = arr->data[index];                            \
    return 0;                                               \
}                                                           \
                                                            \
int name##_remove(name *arr, size_t index) { \
    if (arr == NULL || index >= arr->size) return -1; \
                                                            \
    size_t num_elements_to_shift = arr->size - index - 1; \
    if (num_elements_to_shift > 0) {                        \
        memmove(&arr->data[index], &arr->data[index + 1], sizeof(T) * num_elements_to_shift); \
    }                                                       \
                                                            \
    arr->size--;                                            \
    memset(&arr->data[arr->size], 0, sizeof(T)); \
    return 0;                                               \
}                                                           \
                                                            \

#endif