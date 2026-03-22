#ifndef LIST_H
#define LIST_H

#include <stddef.h>
#include <stdbool.h>

/**
 * Generic dynamic array (list) implementation
 * Provides O(1) amortized append and O(1) random access
 */
typedef struct List {
    void **data;
    size_t size;
    size_t capacity;
} List;

// Construction and destruction
List *list_create(void);
void list_destroy(List *list);
void list_clear(List *list);

// Operations
void list_push(List *list, void *item);
void *list_pop(List *list);
void *list_peek(const List *list);
void *list_get(const List *list, size_t index);
void list_set(List *list, size_t index, void *item);

// Utility
size_t list_size(const List *list);
bool list_is_empty(const List *list);
void list_reverse(List *list);
void **list_to_array(const List *list);

// Iteration macros
#define list_foreach(list, type, var) \
    for (size_t _i_ = 0, _done_ = 0; !_done_ && _i_ < (list)->size; _done_ = 1) \
        for (type var = (type)(list)->data[_i_]; !_done_; _done_ = 1)

#endif // LIST_H
