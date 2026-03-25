#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>
#include <stdbool.h>

// Memory allocation with error handling
void *xmalloc(size_t size);
void *xcalloc(size_t nmemb, size_t size);
void *xrealloc(void *ptr, size_t size);
char *xstrdup(const char *s);

// String utilities
char *str_concat(const char *s1, const char *s2);
char *str_printf(const char *fmt, ...);
int str_ends_with(const char *str, const char *suffix);
int str_starts_with(const char *str, const char *prefix);

// File utilities
char *read_file(const char *filename);
char *read_stdin(void);

// Utility macros
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define CLAMP(val, min, max) (MAX((min), MIN((max), (val))))

// Unused parameter macro
#define UNUSED(x) (void)(x)

#endif // UTIL_H
