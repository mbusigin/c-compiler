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

static void prologue(int locals_size) {
    // Allocate locals aligned to 16 bytes, plus 16 bytes for saved fp/lr
    // Stack grows down: [saved fp/lr at lower addresses, locals at higher addresses]
    // Layout: sp -> [locals...] -> [saved fp] -> [saved lr] -> (original sp)
    // With locals_size = 8 (one 8-byte variable), we need:
    //   8 bytes (local) + 16 bytes (fp/lr) = 24, rounded to 32
    int total = locals_size + 16;
    total = (total + 15) & ~15;  // Round up to 16-byte boundary
    if (total > 0)
        emit_instr("sub\tsp, sp, #%d", total);
    emit_instr("stp\tx29, x30, [sp, #%d]", locals_size);  // Save fp/lr above locals
    emit_instr("mov\tx29, sp");
    emit_instr("add\tx29, x29, #%d", locals_size);  // x29 points to fp/lr location
}

static void epilogue(int locals_size) {
    emit_instr("ldp\tx29, x30, [sp, #%d]", locals_size);
    int total = locals_size + 16;
    total = (total + 15) & ~15;  // Round up to 16-byte boundary
    if (total > 0)
        emit_instr("add\tsp, sp, #%d", total);
    emit_instr("ret");
}

static void emit_return(void) {
    emit_instr("mov\tx0, xzr");  // Default: return 0
    epilogue(0);
}

static void emit_jmp(const char *label) {
    emit_instr("b\t%s", label);
}

static void emit_call(const char *name) {
    emit_instr("bl\t_%s", name);
}

// Track if x8/x9 contain saved values for temps
// x8_temp_type: 0 = empty, 1 = first_binary_op_arg, 2 = const
// x9_temp_type: 0 = empty, 1 = first_binary_op_arg, 2 = const
static int x8_temp_type = 0;
static int x9_temp_type = 0;
static int current_locals_size = 0;

// Load an IRValue into a specific register
// For param_reg=-2, this loads from the variable's stack slot (not just the address)
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
                // It's a temporary - the value is in x0 or x8
                if (x8_temp_type == 1) {
                    if (reg != 0) {
                        emit_instr("mov\tx%d, x8", reg);
                    }
                    // If reg == 0, the value is already in x0
                } else {
                    // Value is in x0
                    if (reg != 0) {
                        emit_instr("mov\tx%d, x0", reg);
                    }
                    // If reg == 0, the value is already in x0
                }
            } else if (val->param_reg >= 0) {
                // It's a parameter - load from the appropriate register
                emit_instr("mov\tx%d, x%d", reg, val->param_reg);
            } else if (val->param_reg == -2 || val->param_reg == -3) {
                // It's a local variable on the stack - load from [sp, #offset]
                // val->offset contains the stack offset
                emit_instr("ldr\tw%d, [sp, #%d]", reg, val->offset);
            } else if (val->param_reg == -4) {
                // Special: indicates this is a LOAD_STACK result that needs the offset from result field
                // This case should not normally be reached
                emit_instr("mov\tx%d, xzr", reg);
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

// Load operands for a binary operation
// Strategy:
// - IR_LOAD_STACK saves previous x8 to x10, loads new value to x0, saves to x8
//   After first load: x10 = garbage, x8 = value, x9 = garbage
//   After second load: x10 = first value, x8 = second value, x9 = second value
// - IR_CONST_INT saves previous x8 to x10, loads constant to x0, saves to x9
//   After const: x10 = previous x8, x8 = garbage (not set), x9 = constant
// We need x0 = left, x1 = right for the operation
static void emit_load_binary_args(IRValue *left, IRValue *right) {
    bool left_is_temp = left && left->kind == IR_VALUE_INT && left->is_temp;
    bool right_is_temp = right && right->kind == IR_VALUE_INT && right->is_temp;
    bool right_is_const = right && right->kind == IR_VALUE_INT && right->is_constant;
    
    if (left_is_temp && right_is_temp) {
        // Both temps: x10 = left, x8 = right
        emit_instr("mov\tx0, x10");   // x0 = left
        emit_instr("mov\tx1, x8");    // x1 = right
    } else if (left_is_temp && right_is_const) {
        // Left in x10, right is constant in x9
        emit_instr("mov\tx0, x10");   // x0 = left
        emit_instr("mov\tx1, x9");    // x1 = constant
    } else if (left_is_temp && !right_is_temp && !right_is_const) {
        // Left in x10, right is something else (e.g., parameter)
        emit_instr("mov\tx0, x10");   // x0 = left
        emit_load_value(1, right);    // x1 = right
    } else if (right_is_temp && !left_is_temp) {
        // Right in x8, left is something else
        emit_instr("mov\tx1, x8");    // x1 = right
        emit_load_value(0, left);     // x0 = left
    } else if (right_is_const && !left_is_temp) {
        // Right is constant, left is something else
        emit_instr("mov\tx1, x0");    // Save constant to x1
        emit_load_value(0, left);     // x0 = left
    } else {
        // Default: load left into x0, right into x1
        emit_load_value(0, left);
        emit_load_value(1, right);
    }
    
    // Reset temp tracking after binary op args are prepared
    x8_temp_type = 0;
    x9_temp_type = 0;
}

// Generate code for an IR instruction
static void gen_instr(IRInstruction *instr) {
    // Clear x0 restore flag at start of each instruction
    // (Each instruction will set it if needed)
    // x0_needs_restore is managed by emit_call_arg
    
    switch (instr->opcode) {
        case IR_NOP:
            // Marker instruction - no code emitted
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
            emit_instr("cbz\tw0, %s", label);  // Jump to label if condition is FALSE (zero)
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
            epilogue(current_locals_size);
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
            if (x8_temp_type == 1) {
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
            x8_temp_type = 1;  // x8 has the return value
            break;
        }
            
        case IR_ADD: {
            emit_load_binary_args(instr->args[0], instr->args[1]);
            emit_instr("add\tw0, w0, w1");
            emit_instr("mov\tx8, x0");
            break;
        }

        case IR_SUB: {
            emit_load_binary_args(instr->args[0], instr->args[1]);
            emit_instr("sub\tw0, w0, w1");
            emit_instr("mov\tx8, x0");
            break;
        }

        case IR_MUL: {
            emit_load_binary_args(instr->args[0], instr->args[1]);
            emit_instr("mul\tw0, w0, w1");
            emit_instr("mov\tx8, x0");
            break;
        }

        case IR_DIV: {
            emit_load_binary_args(instr->args[0], instr->args[1]);
            emit_instr("sdiv\tw0, w0, w1");
            emit_instr("mov\tx8, x0");
            break;
        }

        case IR_MOD: {
            emit_load_binary_args(instr->args[0], instr->args[1]);
            emit_instr("sdiv\tw2, w0, w1");
            emit_instr("msub\tw0, w2, w1, w0");
            emit_instr("mov\tx8, x0");
            break;
        }

        case IR_AND: {
            emit_load_binary_args(instr->args[0], instr->args[1]);
            emit_instr("and\tw0, w0, w1");
            emit_instr("mov\tx8, x0");
            break;
        }

        case IR_OR: {
            emit_load_binary_args(instr->args[0], instr->args[1]);
            emit_instr("orr\tw0, w0, w1");
            emit_instr("mov\tx8, x0");
            break;
        }

        case IR_XOR: {
            emit_load_binary_args(instr->args[0], instr->args[1]);
            emit_instr("eor\tw0, w0, w1");
            emit_instr("mov\tx8, x0");
            break;
        }

        case IR_SHL: {
            emit_load_binary_args(instr->args[0], instr->args[1]);
            emit_instr("lslv\tw0, w0, w1");
            emit_instr("mov\tx8, x0");
            break;
        }

        case IR_SHR: {
            emit_load_binary_args(instr->args[0], instr->args[1]);
            emit_instr("lsrv\tw0, w0, w1");
            emit_instr("mov\tx8, x0");
            break;
        }
            
        case IR_NOT:
            emit_load_value(0, instr->args[0]);
            emit_instr("cmp\tx0, xzr");
            emit_instr("cset\tw0, eq");  // 1 if x0==0 (was false), 0 if x0!=0 (was true)
            emit_instr("mov\tx8, x0");
            break;
            
        case IR_NEG: {
            // Load operand into x0, then negate
            emit_load_value(0, instr->args[0]);
            emit_instr("neg\tw0, w0");
            emit_instr("mov\tx8, x0");
            break;
        }
            
        case IR_CMP: {
            emit_load_binary_args(instr->args[0], instr->args[1]);
            emit_instr("cmp\tw0, w1");
            break;
        }

        // Comparison ops: load args, compare, set result to 1 or 0, save to x8
        case IR_CMP_LT: {
            emit_load_binary_args(instr->args[0], instr->args[1]);
            emit_instr("cmp\tw0, w1");
            emit_instr("cset\tw0, lt");
            emit_instr("mov\tx8, x0");
            break;
        }

        case IR_CMP_GT: {
            emit_load_binary_args(instr->args[0], instr->args[1]);
            emit_instr("cmp\tw0, w1");
            emit_instr("cset\tw0, gt");
            emit_instr("mov\tx8, x0");
            break;
        }

        case IR_CMP_LE: {
            emit_load_binary_args(instr->args[0], instr->args[1]);
            emit_instr("cmp\tw0, w1");
            emit_instr("cset\tw0, le");
            emit_instr("mov\tx8, x0");
            break;
        }

        case IR_CMP_GE: {
            emit_load_binary_args(instr->args[0], instr->args[1]);
            emit_instr("cmp\tw0, w1");
            emit_instr("cset\tw0, ge");
            emit_instr("mov\tx8, x0");
            break;
        }

        case IR_CMP_EQ: {
            emit_load_binary_args(instr->args[0], instr->args[1]);
            emit_instr("cmp\tw0, w1");
            emit_instr("cset\tw0, eq");
            emit_instr("mov\tx8, x0");
            break;
        }

        case IR_CMP_NE: {
            emit_load_binary_args(instr->args[0], instr->args[1]);
            emit_instr("cmp\tw0, w1");
            emit_instr("cset\tw0, ne");
            emit_instr("mov\tx8, x0");
            break;
        }

        // IR_BOOL_AND: logical AND with short-circuit evaluation
        // left result is in x8 (from previous instruction), right is in args[0]
        // Strategy: save x8 to x9, load right into x0 (overwrites x8),
        //           compare both to 0 (setting booleans), AND them
        case IR_BOOL_AND: {
            // Save left result (in x8) to x9 before loading right
            emit_instr("mov\tx9, x8");  // save left to x9
            // Load right into x0 (may overwrite x8)
            emit_load_value(0, instr->args[0]);
            // Now: x9 has left result, x0 has right result
            // Normalize both to 0/1 booleans, then AND
            emit_instr("cmp\tx9, xzr");  // left == 0?
            emit_instr("cset\tx9, ne");   // x9 = (left != 0)
            emit_instr("cmp\tx0, xzr");   // right == 0?
            emit_instr("cset\tx0, ne");   // x0 = (right != 0)
            emit_instr("and\tw0, w0, w9"); // x0 = left_bool & right_bool
            emit_instr("mov\tx8, x0");    // save result to x8
            break;
        }

        // IR_BOOL_OR: logical OR with short-circuit evaluation
        // left result is in x8, right is in args[0]
        // Strategy: save x8 to x9, load right into x0, 
        //           result = (left != 0) ? 1 : right_bool
        case IR_BOOL_OR: {
            // Save left result to x9
            emit_instr("mov\tx9, x8");  // save left to x9
            // Load right into x0 (overwrites x8)
            emit_load_value(0, instr->args[0]);
            // Normalize right to boolean
            emit_instr("cmp\tx0, xzr");
            emit_instr("cset\tx0, ne");  // x0 = right_bool
            // If left != 0 (x9 != 0): result = 1; else result = right_bool
            emit_instr("mov\tx8, #1");    // x8 = 1 (the "true" value)
            emit_instr("csel\tx0, x8, x0, ne"); // if x9 != 0 (ne), x0 = 1, else x0 = right_bool
            emit_instr("mov\tx8, x0");    // save result to x8
            break;
        }
            
        case IR_LOAD:
            // x0 contains the address; load from it
            emit_instr("ldr\tw0, [x0]");
            emit_instr("mov\tx8, x0");
            break;
            
        case IR_STORE:
            // x0 contains the address; store x0's value to it
            emit_instr("str\tw0, [x0]");
            break;
            
        case IR_LOAD_STACK: {
            // Load from [sp, #offset]
            // Strategy: if we have a pending value in x8, save it to x10 first
            if (instr->result && instr->result->kind == IR_VALUE_INT) {
                int offset = (int)instr->result->offset;
                emit_instr("mov\tx10, x8");  // Always save x8 first (will be garbage if first load)
                emit_instr("ldr\tw0, [sp, #%d]", offset);
                emit_instr("mov\tx8, x0");  // Save loaded value to x8
            }
            x8_temp_type = 1;  // x8 now has the loaded value
            x9_temp_type = 1;  // x9 has the previous x8 value (first operand)
            break;
        }
            
        case IR_STORE_STACK: {
            // Store x0 to [sp, #offset] where offset is in the previous const_int
            // We look at the previous instruction in the block to get the offset
            // Actually, simpler: store to [sp, #offset] where offset was emitted by the previous const_int
            // But we need to get the offset from somewhere...
            // 
            // Alternative approach: have IR_STORE_STACK carry the offset in its result
            // For now, we use a hack: the previous instruction's result's data.int_val is the offset
            // Since IR_CONST_INT is emitted right before IR_STORE_STACK with the offset value,
            // we look at the previous instruction
            // 
            // This is a hack - the clean approach would be to have the store carry the offset
            // For now, we use the convention: IR_STORE_STACK expects the offset in x8 (from the const_int)
            // But x8 has the value to store, not the offset.
            //
            // NEW APPROACH: embed the offset in the IR_STORE_STACK instruction's result
            // Lowerer already puts the offset in the result. But we need to pass it.
            //
            // Actually, the cleanest approach: have the previous const_int emit the offset to x9,
            // then IR_STORE_STACK emits: str w0, [sp, x9]
            //
            // But IR_CONST_INT already emits x8 = value. 
            //
            // SIMPLEST: have IR_STORE_STACK emit: str w0, [sp, #N] where N comes from the const_int
            // Since we don't have easy access to the previous instruction in codegen, use a different approach:
            // Emit the constant to x9 as well (add x9, x0).
            // But IR_CONST_INT doesn't know to emit to x9.
            //
            // NEW PLAN: change IR_CONST_INT to also save to x9 when followed by LOAD_STACK or STORE_STACK.
            // But that's complex to detect.
            //
            // SIMPLEST EVER: just look at the previous instruction's result's data.int_val.
            // We can access it through the IRInstruction's args array... but IR_STORE_STACK has no args.
            //
            // OK FINAL APPROACH: modify IR_STORE_STACK to have an offset in its result field,
            // just like IR_LOAD_STACK. The lowerer already sets this.
            // So IR_STORE_STACK's result->offset contains the stack offset.
            if (instr->result && instr->result->kind == IR_VALUE_INT) {
                int offset = (int)instr->result->offset;
                emit_instr("str\tw0, [sp, #%d]", offset);
            }
            break;
        }
            
        case IR_ALLOCA:
            // All locals are allocated in the prologue; don't emit sub sp here
            break;
            
        case IR_CONST_INT:
            // Emit the constant load
            // Strategy: if we have a pending value in x8, save it to x10 first
            if (instr->result && instr->result->kind == IR_VALUE_INT) {
                emit_instr("mov\tx10, x8");  // Always save x8 first (will be garbage if first const)
                emit_instr("mov\tx0, #%lld", instr->result->data.int_val);
                emit_instr("mov\tx8, x0");   // Also save to x8
                emit_instr("mov\tx9, x0");   // Save constant to x9
            }
            x8_temp_type = 1;  // x8 now has the constant
            x9_temp_type = 2;  // x9 has the constant
            break;
            
        default:
            break;
    }
}

// Generate code for a basic block
static void gen_block(IRBasicBlock *block) {
    x8_temp_type = 0;  // Reset at start of each block
    x9_temp_type = 0;
    for (size_t i = 0; i < list_size(block->instructions); i++) {
        gen_instr(list_get(block->instructions, i));
    }
}

// Generate code for a function
static void gen_function(IRFunction *func) {
    // Calculate total locals size
    int locals_size = 0;
    for (size_t i = 0; i < list_size(func->blocks); i++) {
        IRBasicBlock *block = list_get(func->blocks, i);
        for (size_t j = 0; j < list_size(block->instructions); j++) {
            IRInstruction *instr = list_get(block->instructions, j);
            if (instr->opcode == IR_ALLOCA && instr->result && instr->result->is_constant) {
                locals_size += (int)instr->result->data.int_val;
            }
        }
    }
    
    // Use underscore prefix for all functions (macOS convention)
    emit("\n\t.globl\t_%s\n", func->name);
    emit("\t.p2align\t2\n");
    emit("\t_%s:\n", func->name);
    
    // For main function, also create a non-underscored alias
    if (strcmp(func->name, "main") == 0) {
        emit("\t.globl\t%s\n", func->name);
        emit("\t.set\t%s, _%s\n", func->name, func->name);
    }
    
    // Round locals_size up to 16-byte boundary for alignment
    int aligned_locals = (locals_size + 15) & ~15;
    current_locals_size = aligned_locals;
    prologue(aligned_locals);
    
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
