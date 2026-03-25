/**
 * list.c - Dynamic array implementation
 */

#include "list.h"
#include "util.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define INITIAL_CAPACITY 16
#define GROWTH_FACTOR 2

List *list_create(void) {
    List *list = xmalloc(sizeof(List));
    list->capacity = INITIAL_CAPACITY;
    list->size = 0;
    list->data = xcalloc(INITIAL_CAPACITY, sizeof(void *));
    return list;
}

void list_destroy(List *list) {
    if (!list) return;
    free(list->data);
    free(list);
}

void list_clear(List *list) {
    if (!list) return;
    list->size = 0;
}

void list_push(List *list, void *item) {
    if (!list) return;
    
    // Grow if needed
    if (list->size >= list->capacity) {
        list->capacity *= GROWTH_FACTOR;
        list->data = xrealloc(list->data, list->capacity * sizeof(void *));
    }
    
    list->data[list->size++] = item;
}

void *list_pop(List *list) {
    if (!list || list->size == 0) {
        return NULL;
    }
    return list->data[--list->size];
}

void *list_peek(const List *list) {
    if (!list || list->size == 0) {
        return NULL;
    }
    return list->data[list->size - 1];
}

void *list_get(const List *list, size_t index) {
    if (!list || index >= list->size) {
        return NULL;
    }
    return list->data[index];
}

void list_set(List *list, size_t index, void *item) {
    if (!list || index >= list->size) {
        return;
    }
    list->data[index] = item;
}

size_t list_size(const List *list) {
    return list ? list->size : 0;
}

bool list_is_empty(const List *list) {
    return !list || list->size == 0;
}

void list_reverse(List *list) {
    if (!list || list->size <= 1) {
        return;
    }
    
    size_t i, j;
    for (i = 0, j = list->size - 1; i < j; i++, j--) {
        void *tmp = list->data[i];
        list->data[i] = list->data[j];
        list->data[j] = tmp;
    }
}

void **list_to_array(const List *list) {
    if (!list || list->size == 0) {
        return NULL;
    }
    
    void **array = xmalloc(list->size * sizeof(void *));
    memcpy(array, list->data, list->size * sizeof(void *));
    return array;
}
