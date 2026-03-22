/**
 * codegen.c - ARM64 code generation for Apple Silicon
 */

#include "codegen.h"
#include "../common/util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

static FILE *output;

static void emit(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(output, fmt, args);
    va_end(args);
}

static void emit_label(const char *label) {
    emit("%s:\n", label);
}

static void emit_instr(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fprintf(output, "\t");
    vfprintf(output, fmt, args);
    fprintf(output, "\n");
    va_end(args);
}

static void prologue(void) {
    emit_instr("stp\tx29, x30, [sp, -16]!");
    emit_instr("mov\tx29, sp");
}

static void epilogue(void) {
    emit_instr("ldp\tx29, x30, [sp], 16");
    emit_instr("ret");
}

static void emit_return(void) {
    emit_instr("mov\tx0, xzr");  // Default: return 0
    epilogue();
}

static void emit_jmp(const char *label) {
    emit_instr("b\t%s", label);
}

static void emit_call(const char *name) {
    emit_instr("bl\t_%s", name);
}

// Track if x8 contains a saved value for temps
static bool x8_has_temp = false;

// Load an IRValue into a specific register
static void emit_load_value(int reg, IRValue *val) {
    if (!val) {
        emit_instr("mov\tx%d, xzr", reg);
        return;
    }
    
    switch (val->kind) {
        case IR_VALUE_INT:
            if (val->is_constant) {
                // It's a compile-time constant
                emit_instr("mov\tx%d, #%lld", reg, val->data.int_val);
            } else if (val->is_temp) {
                // It's a temporary - the value might be in x0 or x8
                if (x8_has_temp) {
                    // Use the saved value from x8
                    if (reg != 0) {
                        emit_instr("mov\tx%d, x8", reg);
                    } else {
                        emit_instr("mov\tx0, x8");
                    }
                } else {
                    // Use x0 directly
                    if (reg != 0) {
                        emit_instr("mov\tx%d, x0", reg);
                    }
                    // If reg == 0, the value is already in x0
                }
            } else if (val->param_reg >= 0) {
                // It's a parameter - load from the appropriate register
                emit_instr("mov\tx%d, x%d", reg, val->param_reg);
            } else {
                // Unknown kind, default to 0
                emit_instr("mov\tx%d, xzr", reg);
            }
            break;
        case IR_VALUE_STRING:
            emit_instr("adrp\tx%d, l_.str%d@PAGE", reg, val->string_index);
            emit_instr("add\tx%d, x%d, l_.str%d@PAGEOFF", reg, reg, val->string_index);
            break;
        default:
            emit_instr("mov\tx%d, xzr", reg);
            break;
    }
}

// Load operands for a binary operation, handling the case where one is a temp
static void emit_load_binary_args(IRValue *left, IRValue *right) {
    // Check if right is a temp (references previous result in x0)
    bool right_is_temp = right && right->kind == IR_VALUE_INT && right->is_temp;
    
    if (right_is_temp) {
        // Right operand is a temp (result of previous instruction in x0)
        // We need to save it to x1 first, then load left into x0
        emit_instr("mov\tx1, x0");  // Save temp to x1
        emit_load_value(0, left);    // Load left into x0
        // Now x0 = left, x1 = temp
    } else {
        // Normal case: load left into x0, right into x1
        emit_load_value(0, left);
        emit_load_value(1, right);
    }
}

// Generate code for an IR instruction
static void gen_instr(IRInstruction *instr) {
    // Clear x0 restore flag at start of each instruction
    // (Each instruction will set it if needed)
    // x0_needs_restore is managed by emit_call_arg
    
    switch (instr->opcode) {
        case IR_NOP:
            break;
            
        case IR_LABEL:
            if (instr->label) {
                emit_label(instr->label);
            }
            break;
            
        case IR_JMP:
            if (instr->label) {
                emit_jmp(instr->label);
            }
            break;
            
        case IR_JMP_IF: {
            const char *label = instr->label ? instr->label : ".Lend";
            emit_instr("cbnz\tw0, %s", label);
            break;
        }
            
        case IR_RET:
            // If there's a return value argument, use it
            if (instr->num_args > 0 && instr->args[0]) {
                // Load the return value into x0
                emit_load_value(0, instr->args[0]);
            } else {
                emit_instr("mov\tx0, xzr");  // Return 0 if no value
            }
            epilogue();
            break;
            
        case IR_RET_VOID:
            emit_return();
            break;
            
        case IR_CALL: {
            // For function calls, pass arguments in registers (x0-x3)
            // For variadic functions (printf), also put args on stack
            int num_args = instr->num_args;
            bool is_printf = instr->label && strcmp(instr->label, "printf") == 0;
            
            // Save any existing temp value first
            if (x8_has_temp) {
                emit_instr("mov\tx8, x0");  // Save return value
            }
            
            if (is_printf && num_args >= 2) {
                // For printf:
                // x0 = format string (first argument)
                // [sp] = first variadic argument (second IR argument)
                
                // Allocate stack
                emit_instr("sub\tsp, sp, #32");
                
                // Load format string into x0
                if (instr->args[0]) {
                    emit_load_value(0, instr->args[0]);
                }
                
                // Load first variadic arg into x8 and store at [sp]
                if (instr->args[1]) {
                    emit_load_value(8, instr->args[1]);
                    emit_instr("mov\tx9, sp");
                    emit_instr("str\tx8, [x9]");
                }
                
                if (instr->label) {
                    emit_call(instr->label);
                }
                
                emit_instr("add\tsp, sp, #32");
            } else {
                // Normal register-based calling
                for (int i = 0; i < num_args && i < 4; i++) {
                    emit_load_value(i, instr->args[i]);
                }
                
                if (instr->label) {
                    emit_call(instr->label);
                }
            }
            
            // Save return value to x8 for future use
            emit_instr("mov\tx8, x0");
            
            // The return value is in x0/w0
            if (instr->result) {
                instr->result->is_temp = true;
            }
            x8_has_temp = true;
            break;
        }
            
        case IR_ADD: {
            emit_load_binary_args(instr->args[0], instr->args[1]);
            emit_instr("add\tw0, w0, w1");
            break;
        }
            
        case IR_SUB: {
            emit_load_binary_args(instr->args[0], instr->args[1]);
            emit_instr("sub\tw0, w0, w1");
            break;
        }
            
        case IR_MUL: {
            emit_load_binary_args(instr->args[0], instr->args[1]);
            emit_instr("mul\tw0, w0, w1");
            break;
        }
            
        case IR_DIV: {
            emit_load_binary_args(instr->args[0], instr->args[1]);
            emit_instr("sdiv\tw0, w0, w1");
            break;
        }
            
        case IR_MOD:
            emit_instr("sdiv\tw2, w0, w1");
            emit_instr("msub\tw0, w2, w1, w0");
            break;
            
        case IR_AND:
            emit_instr("and\tw0, w0, w1");
            break;
            
        case IR_OR: {
            emit_load_binary_args(instr->args[0], instr->args[1]);
            emit_instr("orr\tw0, w0, w1");
            break;
        }
            
        case IR_XOR: {
            emit_load_binary_args(instr->args[0], instr->args[1]);
            emit_instr("eor\tw0, w0, w1");
            break;
        }
            
        case IR_SHL: {
            emit_load_binary_args(instr->args[0], instr->args[1]);
            emit_instr("lslv\tw0, w0, w1");
            break;
        }
            
        case IR_SHR: {
            emit_load_binary_args(instr->args[0], instr->args[1]);
            emit_instr("lsrv\tw0, w0, w1");
            break;
        }
            
        case IR_NOT:
            emit_instr("mvn\tw0, w0");
            break;
            
        case IR_NEG: {
            emit_load_binary_args(instr->args[0], NULL);
            emit_instr("neg\tw0, w0");
            break;
        }
            
        case IR_CMP: {
            emit_load_binary_args(instr->args[0], instr->args[1]);
            emit_instr("cmp\tw0, w1");
            break;
        }
            
        case IR_LOAD:
            emit_instr("ldr\tw0, [x0]");
            break;
            
        case IR_STORE:
            emit_instr("str\tw0, [x0]");
            break;
            
        case IR_ALLOCA:
            break;
            
        case IR_CONST_INT:
            // Don't emit code here - constants are loaded on-demand by instructions that need them
            // The result value will be used by the next instruction via emit_load_value
            break;
            
        default:
            break;
    }
}

// Generate code for a basic block
static void gen_block(IRBasicBlock *block) {
    for (size_t i = 0; i < list_size(block->instructions); i++) {
        gen_instr(list_get(block->instructions, i));
    }
}

// Generate code for a function
static void gen_function(IRFunction *func) {
    // Use underscore prefix for all functions (macOS convention)
    emit("\n\t.globl\t_%s\n", func->name);
    emit("\t.p2align\t2\n");
    emit("\t_%s:\n", func->name);
    
    // For main function, also create a non-underscored alias
    if (strcmp(func->name, "main") == 0) {
        emit("\t.globl\t%s\n", func->name);
        emit("\t.set\t%s, _%s\n", func->name, func->name);
    }
    
    prologue();
    
    for (size_t i = 0; i < list_size(func->blocks); i++) {
        gen_block(list_get(func->blocks, i));
    }
}

// Generate code for the entire module

// Generate code for the entire module
static void gen_module(IRModule *module) {
    emit("\t.file\t\"<stdin>\"\n");
    emit("\t.text\n");
    
    // Emit string literals in .cstring section (macOS)
    if (list_size(module->strings) > 0) {
        emit("\t.section\t__TEXT,__cstring,cstring_literals\n");
        for (size_t i = 0; i < list_size(module->strings); i++) {
            const char *str = list_get(module->strings, i);
            emit("l_.str%d:\n", (int)i);
            emit("\t.asciz\t%s\n", str);
        }
        emit("\t.text\n");
    }
    
    for (size_t i = 0; i < list_size(module->functions); i++) {
        gen_function(list_get(module->functions, i));
    }
}

void codegen_generate(IRModule *module, FILE *out) {
    if (!module || !out) return;
    
    output = out;
    
    gen_module(module);
}

void codegen_generate_to_file(IRModule *module, const char *filename) {
    FILE *f = fopen(filename, "w");
    if (f) {
        codegen_generate(module, f);
        fclose(f);
    }
}
