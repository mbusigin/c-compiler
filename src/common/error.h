#ifndef ERROR_H
#define ERROR_H

#include <stdarg.h>

// Error handling functions
void error(const char *fmt, ...);
void warning(const char *fmt, ...);
void note(const char *fmt, ...);
void fatal_error(const char *fmt, ...) __attribute__((noreturn));

// Source location for error reporting
typedef struct {
    const char *filename;
    int line;
    int column;
} SourceLocation;

// Global error count
extern int error_count;
extern int warning_count;

#endif // ERROR_H
