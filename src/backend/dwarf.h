#ifndef DWARF_H
#define DWARF_H

#include <stdio.h>

// DWARF debug information
void dwarf_init(FILE *out);
void dwarf_finish(FILE *out);
void dwarf_function_start(FILE *out, const char *name, const char *linkage);
void dwarf_function_end(FILE *out);

#endif // DWARF_H
