/**
 * dwarf.c - DWARF debug information generation
 */

#include "dwarf.h"
#include "../common/util.h"
#include <stdio.h>

void dwarf_init(FILE *out) {
    fprintf(out, "    .section .debug_info\n");
    UNUSED(out);
}

void dwarf_finish(FILE *out) {
    UNUSED(out);
}

void dwarf_function_start(FILE *out, const char *name, const char *linkage) {
    UNUSED(out);
    UNUSED(name);
    UNUSED(linkage);
}

void dwarf_function_end(FILE *out) {
    UNUSED(out);
}
