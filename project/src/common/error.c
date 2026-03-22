/**
 * error.c - Error reporting functions
 */

#include "error.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

int error_count = 0;
int warning_count = 0;

void vreport(const char *prefix, const char *color, const char *fmt, va_list args) {
    fprintf(stderr, "%s%s:", color ? color : "", prefix ? prefix : "error");
    if (color) {
        fprintf(stderr, "\033[0m");  // Reset color
    }
    fprintf(stderr, " ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
}

void error(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vreport("error", "\033[31m", fmt, args);  // Red
    va_end(args);
    error_count++;
}

void warning(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vreport("warning", "\033[33m", fmt, args);  // Yellow
    va_end(args);
    warning_count++;
}

void note(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vreport("note", "\033[36m", fmt, args);  // Cyan
    va_end(args);
}

void fatal_error(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "\033[31mfatal error:\033[0m ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
    exit(1);
}
