#ifndef CODEGEN_H
#define CODEGEN_H

#include "../ir/ir.h"
#include <stdio.h>

// Generate x86-64 assembly from IR
void codegen_generate(IRModule *module, FILE *out);
void codegen_generate_to_file(IRModule *module, const char *filename);

#endif // CODEGEN_H
