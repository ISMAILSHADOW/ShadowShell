#ifndef DYNAMIC_ARRAY_H
#define DYNAMIC_ARRAY_H

#include <stdio.h>
#include <stdlib.h>

#define DECLARE_DYNAMIC_ARRAY(T, name) \
typedef struct {                      \
    size_t size;                      \
    size_t capacity;                  \
    T *data;                          \
} name;                               \
                                      \
                                      \
name* name##_create(size_t capacity); \
                                      \
void name##_resize(name *arr, size_t new_capacity); \
                                      \
void name##_push(name *arr, T element);\
                                      \
int name##_pop(name *arr, T *element); \
                                      \
int name##_get(name *arr, T *element, size_t index); \
                                      \
void name##_destroy(name *arr);       \
                                      \
int name##_remove(name *arr, size_t index); \


// A generic implementation of a dynamic array. 
// As this is used for CLI arguments to support an arbitrary number of arguments,
// It also keeps the array NULL terminated as execve expects.

#define DEFINE_DYNAMIC_ARRAY(T, name) \
                                      \
name* name##_create(size_t capacity) { \
    name *arr = malloc(sizeof(name)); \
    if (arr == NULL) {                \
        fprintf(stderr, "Fatal Error: %s: Out of memory (malloc failed for struct).\n", #name); \
        exit(EXIT_FAILURE);           \
    }                                 \
    arr->size = 0;                    \
    arr->capacity = (capacity < 1) ? 1 : capacity;        \
    arr->data = malloc(sizeof(T) * arr->capacity);        \
    if (arr->data == NULL) {          \
        fprintf(stderr, "Fatal Error: %s: Out of memory (malloc failed for %zu bytes).\n", \
                #name, sizeof(T) * arr->capacity);        \
        exit(EXIT_FAILURE);           \
    }                                 \
                                      \
    memset(&arr->data[0], 0, sizeof(T));                  \
    return arr;                       \
}                                     \
                                      \
void name##_resize(name *arr, size_t new_capacity) { \
    if (arr == NULL) return;          \
    if (new_capacity < 1) new_capacity = 1;               \
                                      \
    T *new_data = realloc(arr->data, sizeof(T) * new_capacity); \
    if (new_data == NULL) {           \
        fprintf(stderr, "Fatal Error: %s: Out of memory (realloc failed for %zu bytes).\n", \
                #name, sizeof(T) * new_capacity);         \
        exit(EXIT_FAILURE);           \
    }                                 \
    arr->data = new_data;             \
    arr->capacity = new_capacity;     \
                                      \
    if (arr->size >= arr->capacity)   \
        arr->size = arr->capacity - 1;\
                                      \
    memset(&arr->data[arr->size], 0, sizeof(T));          \
}                                     \
                                      \
void name##_push(name *arr, T element) { \
    if (arr == NULL) return;          \
                                      \
    if (arr->size + 1 >= arr->capacity) {                 \
        size_t new_capacity = (arr->capacity < 2) ? 4 : arr->capacity * 2; \
        name##_resize(arr, new_capacity);                 \
    }                                 \
                                      \
    arr->data[arr->size++] = element; \
    memset(&arr->data[arr->size], 0, sizeof(T));          \
}                                     \
                                      \
int name##_pop(name *arr, T *element) { \
    if (arr == NULL || element == NULL || arr->size == 0) \
        return -1;                    \
                                      \
    *element = arr->data[--arr->size];\
    memset(&arr->data[arr->size], 0, sizeof(T));          \
    return 0;                         \
}                                     \
                                      \
int name##_get(name *arr, T *element, size_t index) { \
    if (arr == NULL || element == NULL || index >= arr->size) return -1; \
    *element = arr->data[index];      \
    return 0;                         \
}                                     \
                                      \
int name##_remove(name *arr, size_t index) { \
    if (arr == NULL || index >= arr->size) return -1; \
                                      \
    size_t num_elements_to_shift = arr->size - index - 1; \
    if (num_elements_to_shift > 0) {  \
        memmove(&arr->data[index], &arr->data[index + 1], sizeof(T) * num_elements_to_shift); \
    }                                 \
                                      \
    arr->size--;                      \
    memset(&arr->data[arr->size], 0, sizeof(T)); \
    return 0;                         \
}                                     \
                                      \
void name##_destroy(name *arr) {      \
    if (arr != NULL) {                \
        free(arr->data);              \
        free(arr);                    \
    }                                 \
}                                     \

#endif