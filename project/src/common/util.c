/**
 * util.c - Common utility functions
 */

#include "util.h"
#include "error.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

void *xmalloc(size_t size) {
    if (size == 0) {
        return NULL;
    }
    void *ptr = malloc(size);
    if (!ptr) {
        fatal_error("Memory allocation failed (requested %zu bytes)\n", size);
    }
    return ptr;
}

void *xcalloc(size_t nmemb, size_t size) {
    if (nmemb == 0 || size == 0) {
        return NULL;
    }
    void *ptr = calloc(nmemb, size);
    if (!ptr) {
        fatal_error("Memory allocation failed (requested %zu elements of %zu bytes)\n", 
                    nmemb, size);
    }
    return ptr;
}

void *xrealloc(void *ptr, size_t size) {
    if (size == 0) {
        free(ptr);
        return NULL;
    }
    void *new_ptr = realloc(ptr, size);
    if (!new_ptr) {
        fatal_error("Memory reallocation failed (requested %zu bytes)\n", size);
    }
    return new_ptr;
}

char *xstrdup(const char *s) {
    if (!s) {
        return NULL;
    }
    char *copy = strdup(s);
    if (!copy) {
        fatal_error("Memory allocation failed for string duplication\n");
    }
    return copy;
}

char *str_concat(const char *s1, const char *s2) {
    if (!s1 && !s2) return NULL;
    if (!s1) return xstrdup(s2);
    if (!s2) return xstrdup(s1);
    
    size_t len1 = strlen(s1);
    size_t len2 = strlen(s2);
    char *result = xmalloc(len1 + len2 + 1);
    
    memcpy(result, s1, len1);
    memcpy(result + len1, s2, len2 + 1);
    
    return result;
}

char *str_printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    
    // First pass: get required size
    va_list args_copy;
    va_copy(args_copy, args);
    int size = vsnprintf(NULL, 0, fmt, args_copy);
    va_end(args_copy);
    
    if (size < 0) {
        va_end(args);
        return NULL;
    }
    
    // Allocate and format
    char *result = xmalloc((size_t)size + 1);
    vsnprintf(result, (size_t)size + 1, fmt, args);
    va_end(args);
    
    return result;
}

int str_ends_with(const char *str, const char *suffix) {
    if (!str || !suffix) {
        return 0;
    }
    size_t str_len = strlen(str);
    size_t suffix_len = strlen(suffix);
    
    if (suffix_len > str_len) {
        return 0;
    }
    
    return strcmp(str + str_len - suffix_len, suffix) == 0;
}

int str_starts_with(const char *str, const char *prefix) {
    if (!str || !prefix) {
        return 0;
    }
    size_t prefix_len = strlen(prefix);
    if (strlen(str) < prefix_len) {
        return 0;
    }
    return strncmp(str, prefix, prefix_len) == 0;
}

char *read_file(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) {
        return NULL;
    }
    
    // Get file size
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    if (size < 0) {
        fclose(f);
        return NULL;
    }
    
    // Allocate and read
    char *content = xmalloc((size_t)size + 1);
    size_t read_size = fread(content, 1, (size_t)size, f);
    fclose(f);
    
    if ((long)read_size != size) {
        free(content);
        return NULL;
    }
    
    content[size] = '\0';
    return content;
}

char *read_stdin(void) {
    // Read all input from stdin
    size_t capacity = 4096;
    size_t length = 0;
    char *buffer = xmalloc(capacity);
    
    int ch;
    while ((ch = fgetc(stdin)) != EOF) {
        if (length + 1 >= capacity) {
            capacity *= 2;
            buffer = xrealloc(buffer, capacity);
        }
        buffer[length++] = (char)ch;
    }
    
    buffer[length] = '\0';
    return buffer;
}
