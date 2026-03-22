#ifndef ASM_H
#define ASM_H

#include <stdio.h>

// Assembly output utilities
void asm_print_header(FILE *out);
void asm_print_footer(FILE *out);
void asm_print_label(FILE *out, const char *label);
void asm_print_instruction(FILE *out, const char *fmt, ...);

#endif // ASM_H
