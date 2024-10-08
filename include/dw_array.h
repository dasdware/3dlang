#ifndef __DW_ARRAY_H
#define __DW_ARRAY_H

#include <stdlib.h>

#ifndef DA_INITIAL_CAPACITY
#   define DA_INITIAL_CAPACITY 8
#endif

#ifndef DA_MALLOC
#  define DA_REALLOC realloc
#endif

#ifndef DA_FREE
#  define DA_FREE free
#endif

#define da_array(type) type*

typedef struct {
    size_t size;
    size_t capacity;
    char data[];
} DA_Header;

size_t da_size(void* array);
size_t da_capacity(void *array);

void* _da_check_capacity(void *array, size_t element_size, size_t n);

#define da_add(array, element)                                              \
    do {                                                                    \
        size_t array_size = da_size((array));                               \
        (array) = _da_check_capacity((array), sizeof(*(array)), 1);         \
        (array)[array_size] = element;                                      \
        ((DA_Header*) array)[-1].size++;                                    \
    } while(false)

#define da_addn(array, elements, count)                                     \
    do {                                                                    \
        size_t array_size = da_size((array));                               \
        (array) = _da_check_capacity((array), sizeof(*(array)), count);     \
        memcpy((array) + array_size, (elements), count * sizeof(*(array))); \
        ((DA_Header*) array)[-1].size += count;                             \
    } while(false)

#define da_clear(array) (((DA_Header*) array)[-1].size = 0)

#define da_free(array) DA_FREE(((DA_Header*) array) - 1)

#endif // __DW_ARRAY_H

#ifdef DW_ARRAY_IMPLEMENTATION
#undef DW_ARRAY_IMPLEMENTATION

size_t da_size(void* array) {
    if (!array) {
        return 0;
    }
    return ((DA_Header*) array)[-1].size;
}

size_t da_capacity(void *array) {
    if (!array) {
        return 0;
    }
    return ((DA_Header*) array)[-1].capacity;
}

void* _da_check_capacity(void *array, size_t element_size, size_t n) {
    bool old_exists = (array != NULL);
    size_t old_capacity = da_capacity(array);

    size_t new_size = da_size(array) + n;
    size_t new_capacity = (old_exists) ? old_capacity : DA_INITIAL_CAPACITY;
    while (new_size > new_capacity) {
        new_capacity *= 2;
    }

    if (new_capacity > old_capacity) {
        DA_Header* old_header = (old_exists) ? ((DA_Header*) (array)) - 1 : NULL;
        DA_Header* new_header = DA_REALLOC(old_header, sizeof(DA_Header) + new_capacity * element_size);
        if (!old_exists) {
            new_header->size = 0;
        }
        new_header->capacity = new_capacity;
        return &(new_header[1]);
    }

    return array;
}

#endif // DW_ARRAY_IMPLEMENTATION