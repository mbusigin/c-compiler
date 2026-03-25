/**
 * asm.c - Assembly output utilities
 */

#include "asm.h"
#include "../common/util.h"
#include <stdio.h>
#include <stdarg.h>

void asm_print_header(FILE *out) {
    fprintf(out, "    .file   \"<stdin>\"\n");
    fprintf(out, "    .text\n");
}

void asm_print_footer(FILE *out) {
    UNUSED(out);
}

void asm_print_label(FILE *out, const char *label) {
    fprintf(out, "%s:\n", label);
}

void asm_print_instruction(FILE *out, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fprintf(out, "    ");
    vfprintf(out, fmt, args);
    fprintf(out, "\n");
    va_end(args);
}
