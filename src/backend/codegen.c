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

// Forward declaration for emit_immediate (used in prologue before definition)
static void emit_immediate(int reg, long long value);

static void prologue(int locals_size) {
    // Allocate locals aligned to 16 bytes, plus 32 bytes for saved fp/lr/x20/x21
    // Stack layout (grows down):
    //   [higher addresses]
    //   saved lr
    //   saved fp
    //   saved x21
    //   saved x20
    //   [locals...]
    //   sp -> [lower addresses]
    // 
    // x29 (frame pointer) points to the base of locals for stable addressing
    // Locals are at positive offsets from x29.
    // Stack layout (low to high addresses):
    //   sp -> [saved x20, x21]
    //         [saved x19, x22]  
    //         [saved x29, x30]
    //   x29 -> [locals...]
    // 
    // We save x19, x20, x21, x22 (callee-saved) for use in various operations
    int callee_saved = 48;  // x29, x30, x19, x20, x21, x22 (6 * 8 = 48 bytes)
    int total = locals_size + callee_saved;
    total = (total + 15) & ~15;  // Round up to 16-byte boundary
    if (total > 0)
        emit_instr("sub\tsp, sp, #%d", total);
    
    // Save callee-saved registers at the BOTTOM of the frame (lowest addresses)
    // Then set x29 to point ABOVE them, so locals don't overlap
    emit_instr("stp\tx20, x21, [sp]");           // Save x20/x21 at [sp]
    emit_instr("stp\tx19, x22, [sp, #16]");      // Save x19/x22 at [sp + 16]
    emit_instr("stp\tx29, x30, [sp, #32]");      // Save fp/lr at [sp + 32]
    // Set frame pointer to point to start of locals area (above saved registers)
    emit_instr("add\tx29, sp, #48");             // x29 = sp + 48 (past saved regs)
    
    // Frame pointer (x29) was set above to point to start of locals area
    // Locals can be addressed as [x29, #offset] where offset is 0 to locals_size-8
}

static void epilogue(int locals_size) {
    int callee_saved = 48;
    int total = locals_size + callee_saved;
    total = (total + 15) & ~15;  // Round up to 16-byte boundary
    // x29 points to start of locals area (sp + 48)
    // Saved registers are at [x29 - 48] to [x29 - 8]
    // Which is [sp] to [sp + 40]
    
    // Restore callee-saved registers
    emit_instr("ldp\tx20, x21, [sp]");           // Restore x20/x21 from [sp]
    emit_instr("ldp\tx19, x22, [sp, #16]");      // Restore x19/x22 from [sp + 16]
    emit_instr("ldp\tx29, x30, [sp, #32]");      // Restore fp/lr from [sp + 32]
    if (total > 0)
        emit_instr("add\tsp, sp, #%d", total);
    emit_instr("ret");
}

static void emit_jmp(const char *label) {
    emit_instr("b\t%s", label);
}

static void emit_call(const char *name) {
    emit_instr("bl\t_%s", name);
}

// Load an immediate value into a register
// ARM64 has limited immediate values for mov - handles large values with movz/movk
static void emit_immediate(int reg, long long value) {
    if (value >= 0 && value <= 65535) {
        // Small immediate - use mov
        emit_instr("mov\tx%d, #%lld", reg, value);
    } else if (value >= -65535 && value < 0) {
        // Negative small immediate - use movn
        emit_instr("movn\tx%d, #%lld", reg, -value);
    } else {
        // Large immediate - use movz + movk
        // Decompose into 16-bit chunks
        unsigned long long uv = (unsigned long long)value;
        // First chunk (lowest 16 bits)
        emit_instr("movz\tx%d, #%d", reg, (unsigned int)(uv & 0xFFFF));
        // Subsequent chunks
        if ((uv >> 16) & 0xFFFF) {
            emit_instr("movk\tx%d, #%d, lsl #16", reg, (unsigned int)((uv >> 16) & 0xFFFF));
        }
        if ((uv >> 32) & 0xFFFF) {
            emit_instr("movk\tx%d, #%d, lsl #32", reg, (unsigned int)((uv >> 32) & 0xFFFF));
        }
        if ((uv >> 48) & 0xFFFF) {
            emit_instr("movk\tx%d, #%d, lsl #48", reg, (unsigned int)((uv >> 48) & 0xFFFF));
        }
    }
}

// Track if x8/x9 contain saved values for temps
// x8_temp_type: 0 = empty, 1 = return_value_or_first_op, 2 = const
// x9_temp_type: 0 = empty, 1 = first_op_or_return_value, 2 = const
// x10: used to save the "previous x8" value for binary ops
// x19: callee-saved register used to preserve x9 (first operand) across const loads
// x20: callee-saved register used to preserve x8 (return value or first operand) across const loads
// x21: callee-saved register used by IR_SAVE_X9 for post-increment and IR_LOAD_STACK
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
                // It's a compile-time constant - use emit_immediate for large value support
                emit_immediate(reg, val->data.int_val);
            } else if (val->is_temp) {
                // It's a temporary - the value is in x8 (from previous instruction)
                if (reg != 8) {
                    emit_instr("mov\tx%d, x8", reg);
                }
            } else if (val->param_reg >= 0) {
                // It's a parameter - load from the appropriate register
                emit_instr("mov\tx%d, x%d", reg, val->param_reg);
            } else if (val->param_reg == -2 || val->param_reg == -3) {
                // It's a local variable on the stack - load from [x29, #offset] (frame pointer relative)
                // val->offset contains the stack offset
                // IMPORTANT: Default to 64-bit load for all stack values on ARM64
                // Addresses (from LEA) must be loaded as 64-bit, and it's safer to use 64-bit
                // for all local variables to avoid truncation issues.
                emit_instr("ldr\tx%d, [x29, #%d]", reg, val->offset);
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
// Optimization: Try to use operands directly from their registers without extra moves
// Strategy:
// - Previous value (first operand or temp) is saved in x10 by IR_LOAD_STACK/IR_CONST_INT
// - Constants are loaded to x8
// - For binary ops: try to use registers directly, minimize moves
static void emit_load_binary_args(IRValue *left, IRValue *right) {
    bool left_is_temp = left && left->kind == IR_VALUE_INT && left->is_temp;
    bool right_is_temp = right && right->kind == IR_VALUE_INT && right->is_temp;
    bool right_is_const = right && right->kind == IR_VALUE_INT && right->is_constant;
    bool left_is_const = left && left->kind == IR_VALUE_INT && left->is_constant;
    
    // Optimization: For operations that write to x8, we can use operands directly
    // instead of moving them to x0/x1 first
    // This is handled by the caller - we just ensure operands are in predictable registers
    
    if (left_is_temp && right_is_temp) {
        // Both temps: x10 = left (saved by previous instruction), x8 = right
        // Operation will be: op x8, x10, x8 (or similar)
        // Just ensure both values are in their expected registers - no moves needed
    } else if (left_is_temp && right_is_const) {
        // Left in x10, right is constant in x8
        // Operation will be: op x8, x10, x8
        // No moves needed
    } else if (left_is_temp && !right_is_temp && !right_is_const) {
        // Left in x10, right is something else (e.g., parameter or stack load)
        // Need to load right - x10 already has left
        emit_load_value(0, right);    // Load right into x0 temporarily
        emit_instr("mov\tx10, x0");   // Move to x10 so operation can use it
    } else if (right_is_temp && !left_is_temp) {
        // Right in x8, left is something else
        // Need to load left into x10
        emit_load_value(10, left);     // Load left directly into x10
        // x8 already has right
    } else if (right_is_const && !left_is_temp) {
        // Right is constant (in x8), left is something else
        // Load left into x10, keep right in x8
        emit_load_value(10, left);
    } else if (left_is_const && !right_is_temp) {
        // Left is constant (in x8), right is something else
        // Need to swap: move left to x10, load right to x8
        emit_instr("mov\tx10, x8");    // Save left to x10
        emit_load_value(8, right);     // Load right to x8
        // Now x10 = left, x8 = right (swapped)
    } else {
        // Default: load left into x10, right into x8
        emit_load_value(10, left);
        emit_load_value(8, right);
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
            // Load the condition value
            if (instr->num_args > 0 && instr->args[0]) {
                emit_load_value(0, instr->args[0]);
            }
            // Jump to label if condition is FALSE (zero)
            // This is the correct behavior for if/while statements
            // Use x0 for 64-bit pointer comparison (was w0, which is wrong)
            emit_instr("cbz\tx0, %s", label);
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
            epilogue(current_locals_size);
            break;
            
        case IR_CALL: {
            // For function calls, pass arguments in registers (x0-x3)
            // For variadic functions (printf, fprintf), also put args on stack
            int num_args = instr->num_args;
            bool is_printf = instr->label && strcmp(instr->label, "printf") == 0;
            bool is_fprintf = instr->label && strcmp(instr->label, "fprintf") == 0;
            
            // NOTE: We do NOT save x8 here before the call - the caller should have
            // already dealt with preserving any needed values. x0-x3 are used for args.
            
            if (is_printf && num_args >= 2) {
                // For printf:
                // x0 = format string (first argument)
                // Additional args go on stack (in reverse order for varargs)
                
                // Count variadic args (all args after format string)
                int num_varargs = num_args - 1;
                
                // Allocate stack for variadic args (8 bytes each, padded to 32)
                int stack_size = (num_varargs * 8 + 15) & ~15;
                if (stack_size < 32) stack_size = 32;
                emit_instr("sub\tsp, sp, #%d", stack_size);
                
                // IMPORTANT: If args[num_args-1] is a temp, it's already in x8.
                // We need to store it FIRST before loading other args.
                // Store variadic args on stack (in reverse order to handle temps correctly)
                for (int i = num_args - 1; i >= 1; i--) {
                    if (instr->args[i]) {
                        // For the last arg (i == num_args-1), the value is already in x8 if it's a temp
                        // from the previous expression evaluation
                        if (i == num_args - 1 && instr->args[i]->is_temp) {
                            // x8 already contains this value, no need to load
                        } else {
                            emit_load_value(8, instr->args[i]);
                        }
                        int offset = (i - 1) * 8;
                        if (offset == 0) {
                            emit_instr("mov\tx9, sp");
                            emit_instr("str\tx8, [x9]");
                        } else {
                            emit_instr("str\tx8, [sp, #%d]", offset);
                        }
                    }
                }
                
                // Load format string into x0 (this may clobber x8)
                if (instr->args[0]) {
                    emit_load_value(0, instr->args[0]);
                }
                
                if (instr->label) {
                    emit_call(instr->label);
                }
                
                emit_instr("add\tsp, sp, #%d", stack_size);
            } else if (is_fprintf && num_args >= 3) {
                // For fprintf:
                // x0 = FILE* (first argument, e.g., stderr)
                // x1 = format string (second argument)
                // Additional args go on stack (in reverse order for varargs)
                
                // Count variadic args (all args after format string)
                int num_varargs = num_args - 2;
                
                // Allocate stack for variadic args (8 bytes each, padded to 32)
                int stack_size = (num_varargs * 8 + 15) & ~15;
                if (stack_size < 32) stack_size = 32;
                emit_instr("sub\tsp, sp, #%d", stack_size);
                
                // Store variadic args on stack (in reverse order to handle temps correctly)
                for (int i = num_args - 1; i >= 2; i--) {
                    if (instr->args[i]) {
                        // For the last arg (i == num_args-1), the value is already in x8 if it's a temp
                        if (i == num_args - 1 && instr->args[i]->is_temp) {
                            // x8 already contains this value, no need to load
                        } else {
                            emit_load_value(8, instr->args[i]);
                        }
                        int offset = (i - 2) * 8;
                        if (offset == 0) {
                            emit_instr("mov\tx9, sp");
                            emit_instr("str\tx8, [x9]");
                        } else {
                            emit_instr("str\tx8, [sp, #%d]", offset);
                        }
                    }
                }
                
                // Load format string into x1
                if (instr->args[1]) {
                    emit_load_value(1, instr->args[1]);
                }
                
                // Load FILE* into x0 (this may clobber x8)
                if (instr->args[0]) {
                    emit_load_value(0, instr->args[0]);
                }
                
                if (instr->label) {
                    emit_call(instr->label);
                }
                
                emit_instr("add\tsp, sp, #%d", stack_size);
            } else {
                // Normal register-based calling
                // ARM64 passes first 8 args in registers (x0-x7), rest on stack
                
                // Calculate stack space needed for args beyond 8
                int stack_args = num_args > 8 ? num_args - 8 : 0;
                int stack_size = (stack_args * 8 + 15) & ~15;
                if (stack_size > 0) {
                    emit_instr("sub\tsp, sp, #%d", stack_size);
                }
                
                // Store args beyond 8 on the stack (in reverse order for consistency)
                for (int i = num_args - 1; i >= 8; i--) {
                    if (instr->args[i]) {
                        emit_load_value(8, instr->args[i]);
                        int offset = (i - 8) * 8;
                        if (offset == 0) {
                            emit_instr("mov\tx9, sp");
                            emit_instr("str\tx8, [x9]");
                        } else {
                            emit_instr("str\tx8, [sp, #%d]", offset);
                        }
                    }
                }
                
                // Pass first 8 args in registers x0-x7
                for (int i = 0; i < num_args && i < 8; i++) {
                    emit_load_value(i, instr->args[i]);
                }
                
                if (instr->label) {
                    emit_call(instr->label);
                }
                
                // Restore stack if we allocated for stack args
                if (stack_size > 0) {
                    emit_instr("add\tsp, sp, #%d", stack_size);
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
        
        case IR_CALL_INDIRECT: {
            // Indirect call through function pointer
            // args[0] = function pointer, args[1-3] = function arguments
            
            // The function pointer was saved to x22 by IR_LOAD_STACK (if it's a pointer)
            // Use x22 for the indirect call (callee-saved register, won't be clobbered by args)
            
            // Load arguments into x0, x1, x2, x3 (standard calling convention)
            // Skip args[0] which is the function pointer (already in x22)
            for (int i = 1; i < instr->num_args && i < 4; i++) {
                if (instr->args[i]) {
                    emit_load_value(i - 1, instr->args[i]);
                }
            }
            
            // Indirect call through x22 (function pointer)
            emit_instr("blr\tx22");
            
            // Save return value to x8 for future use
            emit_instr("mov\tx8, x0");
            
            if (instr->result) {
                instr->result->is_temp = true;
            }
            x8_temp_type = 1;
            break;
        }
            
        case IR_ADD: {
            emit_load_binary_args(instr->args[0], instr->args[1]);
            // Use 64-bit add for both integers and addresses (works for both)
            // Operands are in x10 (left) and x8 (right), result goes to x8
            emit_instr("add\tx8, x10, x8");
            x8_temp_type = 1;
            break;
        }

        case IR_SUB: {
            emit_load_binary_args(instr->args[0], instr->args[1]);
            // Operands are in x10 (left) and x8 (right), result goes to x8
            emit_instr("sub\tx8, x10, x8");
            x8_temp_type = 1;
            break;
        }

        case IR_MUL: {
            emit_load_binary_args(instr->args[0], instr->args[1]);
            // Use 64-bit multiplication for pointer arithmetic
            emit_instr("mul\tx8, x10, x8");
            x8_temp_type = 1;
            break;
        }

        case IR_DIV: {
            emit_load_binary_args(instr->args[0], instr->args[1]);
            emit_instr("sdiv\tx8, x10, x8");
            x8_temp_type = 1;
            break;
        }

        case IR_MOD: {
            emit_load_binary_args(instr->args[0], instr->args[1]);
            emit_instr("sdiv\tx2, x10, x8");
            emit_instr("msub\tx8, x2, x8, x10");
            x8_temp_type = 1;
            break;
        }

        case IR_AND: {
            emit_load_binary_args(instr->args[0], instr->args[1]);
            emit_instr("and\tx8, x10, x8");
            x8_temp_type = 1;
            break;
        }

        case IR_OR: {
            emit_load_binary_args(instr->args[0], instr->args[1]);
            emit_instr("orr\tx8, x10, x8");
            x8_temp_type = 1;
            break;
        }

        case IR_XOR: {
            emit_load_binary_args(instr->args[0], instr->args[1]);
            emit_instr("eor\tx8, x10, x8");
            x8_temp_type = 1;
            break;
        }

        case IR_SHL: {
            emit_load_binary_args(instr->args[0], instr->args[1]);
            emit_instr("lslv\tx8, x10, x8");
            x8_temp_type = 1;
            break;
        }

        case IR_SHR: {
            emit_load_binary_args(instr->args[0], instr->args[1]);
            emit_instr("lsrv\tx8, x10, x8");
            x8_temp_type = 1;
            break;
        }
            
        case IR_NOT:
            emit_load_value(0, instr->args[0]);
            emit_instr("cmp\tx0, xzr");
            emit_instr("cset\tx8, eq");  // 1 if x0==0 (was false), 0 if x0!=0 (was true)
            x8_temp_type = 1;
            break;
            
        case IR_NEG: {
            // Load operand into x0, then negate
            emit_load_value(0, instr->args[0]);
            emit_instr("neg\tx8, x0");
            x8_temp_type = 1;
            break;
        }
            
        case IR_CMP: {
            emit_load_binary_args(instr->args[0], instr->args[1]);
            emit_instr("cmp\tx10, x8");
            break;
        }

        // Comparison ops: load args, compare, set result to 1 or 0, save to x8
        case IR_CMP_LT: {
            emit_load_binary_args(instr->args[0], instr->args[1]);
            emit_instr("cmp\tx10, x8");
            emit_instr("cset\tx8, lt");
            x8_temp_type = 1;
            break;
        }

        case IR_CMP_GT: {
            emit_load_binary_args(instr->args[0], instr->args[1]);
            emit_instr("cmp\tx10, x8");
            emit_instr("cset\tx8, gt");
            x8_temp_type = 1;
            break;
        }

        case IR_CMP_LE: {
            emit_load_binary_args(instr->args[0], instr->args[1]);
            emit_instr("cmp\tx10, x8");
            emit_instr("cset\tx8, le");
            x8_temp_type = 1;
            break;
        }

        case IR_CMP_GE: {
            emit_load_binary_args(instr->args[0], instr->args[1]);
            emit_instr("cmp\tx10, x8");
            emit_instr("cset\tx8, ge");
            x8_temp_type = 1;
            break;
        }

        case IR_CMP_EQ: {
            emit_load_binary_args(instr->args[0], instr->args[1]);
            emit_instr("cmp\tx10, x8");
            emit_instr("cset\tx8, eq");
            x8_temp_type = 1;
            break;
        }

        case IR_CMP_NE: {
            emit_load_binary_args(instr->args[0], instr->args[1]);
            emit_instr("cmp\tx10, x8");
            emit_instr("cset\tx8, ne");
            x8_temp_type = 1;
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
            // Load from address in x8
            // If args[0] is NULL, load from [x8] (for array subscript/struct member)
            // Otherwise, x0 contains the address
            if (instr->num_args == 0) {
                // Save current x8 value to x10 only if it has a temp value
                // (not if it's just an address from LEA)
                if (x8_temp_type == 1 || x8_temp_type == 2) {
                    emit_instr("mov\tx10, x8");
                }
                
                // Load from address in x8
                // Use 8-byte load for pointers and 64-bit values, ldrb for bytes (char)
                if (instr->result && (instr->result->is_pointer || instr->result->is_64bit)) {
                    emit_instr("ldr\tx8, [x8]");  // 8-byte load for pointers/64-bit
                } else if (instr->result && instr->result->is_byte) {
                    emit_instr("ldrb\tw8, [x8]");  // 1-byte load for char
                } else {
                    emit_instr("ldr\tw8, [x8]");  // 4-byte load for 32-bit integers
                }
            } else {
                // Legacy behavior: x0 contains the address
                // Save current x8 value to x10 only if it has a temp value
                if (x8_temp_type == 1 || x8_temp_type == 2) {
                    emit_instr("mov\tx10, x8");
                }
                
                if (instr->result && (instr->result->is_pointer || instr->result->is_64bit)) {
                    emit_instr("ldr\tx0, [x0]");
                    emit_instr("mov\tx8, x0");
                } else if (instr->result && instr->result->is_byte) {
                    emit_instr("ldrb\tw0, [x0]");
                    emit_instr("mov\tx8, x0");
                } else {
                    emit_instr("ldr\tw0, [x0]");
                    emit_instr("mov\tx8, x0");
                }
            }
            x8_temp_type = 1;  // x8 now has a temp value (loaded value)
            x9_temp_type = 1;
            break;
            
        case IR_STORE:
            // x0 contains the address; store x0's value to it
            emit_instr("str\tw0, [x0]");
            break;
            
        case IR_LOAD_STACK: {
            // Load from [x29, #offset]
            if (instr->result && instr->result->kind == IR_VALUE_INT) {
                int old_x9_type = x9_temp_type;
                int offset = (int)instr->result->offset;
                
                if (old_x9_type == 1 || old_x9_type == 2) {
                    emit_instr("mov\tx21, x9");
                }
                if (x8_temp_type == 1 || x8_temp_type == 2) {
                    emit_instr("mov\tx10, x8");
                }
                // Use 64-bit load for all stack values (safer for addresses)
                emit_instr("ldr\tx0, [x29, #%d]", offset);
                emit_instr("mov\tx8, x0");
                if (instr->result->is_pointer) {
                    emit_instr("mov\tx22, x8");
                }
                if (old_x9_type == 1 || old_x9_type == 2) {
                    emit_instr("mov\tx9, x21");
                }
            }
            x8_temp_type = 1;
            x9_temp_type = 1;
            break;
        }
            
        case IR_STORE_STACK: {
            // Store value to [sp, #offset]
            // The value is in x8 (from the previous load or const_int)
            // result->offset = stack offset
            // NOTE: We use x29 (frame pointer) for addressing to handle stack adjustments during calls
            if (instr->result && instr->result->kind == IR_VALUE_INT) {
                int offset = (int)instr->result->offset;
                // For large offsets (> 504), use indirect addressing
                if (offset > 504 || offset < -512) {
                    emit_instr("add\tx9, x29, #%d", offset);
                    emit_instr("str\tx8, [x9]");  // Store via computed address
                } else {
                    // Use 8-byte store for all values to avoid pointer truncation, use x29
                    emit_instr("str\tx8, [x29, #%d]", offset);
                }
            }
            break;
        }
            
        case IR_ALLOCA:
            // All locals are allocated in the prologue; don't emit sub sp here
            break;
            
        case IR_STORE_PARAM: {
            // Store parameter register to stack slot
            // result->data.int_val = parameter register number (0-3 for x0-x3)
            // result->offset = stack offset
            // NOTE: We use x29 (frame pointer) for addressing to handle stack adjustments during calls
            if (instr->result && instr->result->kind == IR_VALUE_INT) {
                int param_reg = (int)instr->result->data.int_val;
                int offset = (int)instr->result->offset;
                // For large offsets (> 504), use indirect addressing
                if (offset > 504 || offset < -512) {
                    emit_instr("add\tx9, x29, #%d", offset);
                    emit_instr("str\tx%d, [x9]", param_reg);  // Store via computed address
                } else {
                    // Store parameter register to [x29, #offset]
                    emit_instr("str\tx%d, [x29, #%d]", param_reg, offset);
                }
            }
            break;
        }
            
        case IR_CONST_INT:
            // Emit the constant load
            // Strategy: load constant directly to x8/x9 without touching x0 (which may hold parameters)
            // x10: used to preserve previous x8 value for binary ops
            // x20: reserved for explicit saves (IR_SAVE_X8_TO_X20) - don't use here
            // x19: save area for preserving x9 when it had a return value (temp_type == 1)
            if (instr->result && instr->result->kind == IR_VALUE_INT) {
                int old_x9_type = x9_temp_type;
                // Note: We save to x10, NOT x20. x20 is for explicit saves via IR_SAVE_X8_TO_X20.
                // This prevents overwriting values that were explicitly saved for array indexing.
                if (old_x9_type == 1) {
                    emit_instr("mov\tx19, x9");  // Save x9 when it had value
                }
                // Only save x8 to x10 if it contains a value that might be used
                if (x8_temp_type == 1 || x8_temp_type == 2) {
                    emit_instr("mov\tx10, x8");  // Save previous x8 to x10 (for binary ops to access)
                }
                // Load constant directly to x8, NOT to x0 (preserve parameters)
                emit_immediate(8, instr->result->data.int_val);
                emit_instr("mov\tx9, x8");   // Copy constant to x9
                // DON'T restore x8 here - the constant should stay in x8 for the next operation
                x8_temp_type = 2;  // x8 now has a constant
                x9_temp_type = 2;  // x9 has the constant
            }
            break;

        case IR_CONST_STRING:
            // Load a string literal address into x8
            // result->string_index contains the string index
            if (instr->result) {
                emit_instr("mov\tx10, x8");  // Save previous x8
                emit_instr("adrp\tx8, l_.str%d@PAGE", instr->result->string_index);
                emit_instr("add\tx8, x8, l_.str%d@PAGEOFF", instr->result->string_index);
                emit_instr("mov\tx9, x8");   // Copy to x9
                x8_temp_type = 2;  // x8 has a string address
                x9_temp_type = 2;
            }
            break;

        case IR_LOAD_OFFSET:
            // Load from [base_ptr + offset*4]
            // args[0] = base pointer value (in x8)
            // args[1] = offset/index
            // The address is computed as: x8 + args[1]*4
            // For now, we assume the index was already multiplied by 4
            // x8 has the base address, compute final address and load
            if (instr->num_args >= 2 && instr->args[1]) {
                // args[1] contains the byte offset (index * 4)
                // Add to base (in x8) and load
                emit_instr("mov\tx0, x8");  // base address to x0
                // The offset should have been computed by previous instructions
                // For now, assume offset is in x8 after a sequence of operations
                // We need to emit: ldr w8, [x0 + offset] but we don't have the offset as immediate
                // Instead, use: ldr w8, [x0, x8, lsl #2] if x8 is the index
                // But that's complex - let's use a simpler approach:
                // The previous ADD should have put address in x8, just load from it
                emit_instr("ldr\tw8, [x8]");  // Load from address in x8
                x8_temp_type = 1;
                x9_temp_type = 1;
            }
            break;
            
        case IR_STORE_OFFSET:
            // Store to [base_ptr + offset*4]
            // args[0] = base pointer
            // args[1] = offset/index  
            // result = value to store
            // The address was computed in x8 by previous instructions
            // The value to store is in result (which should be in x8)
            // But we need both - this is tricky
            // For now, emit: str w_val, [x8] where x8 has address
            // We need the value somewhere else...
            // Actually, this needs redesign. For now, do a simple store through x8
            emit_instr("str\tw8, [x8]");  // This is wrong - stores address to itself
            break;
            
        case IR_STORE_INDIRECT:
            // Store w8 to [x8] (address in x8, value needs to be saved first)
            // Save the value to a temp register
            emit_instr("mov\tw9, w8");  // Save value
            // But wait - we need to reload the address first
            // The pattern is: reload address from temp slot, then store
            // For now, assume address is in x8 and we need an extra register
            // This is a temporary fix - proper fix needs better temp management
            break;
            
        case IR_LEA:
            // Load effective address: x8 = x29 + offset (frame pointer relative)
            // result->offset = stack offset relative to frame pointer
            // Save current x8 value to x10 before computing new address
            if (instr->result && instr->result->kind == IR_VALUE_INT) {
                int offset = (int)instr->result->offset;
                // Save x8 to x10 if it has a temp value (for binary operations)
                if (x8_temp_type == 1 || x8_temp_type == 2) {
                    emit_instr("mov\tx10, x8");
                }
                // Use frame pointer (x29) for addressing locals
                emit_instr("add\tx8, x29, #%d", offset);
                // Mark x8 as containing an address (for subsequent operations)
                x8_temp_type = 0;
            }
            break;
            
        case IR_ADD_IMM64:
            // x8 = x8 + imm (64-bit add for pointer arithmetic)
            // args[1] contains the immediate offset
            if (instr->num_args >= 2 && instr->args[1] && instr->args[1]->is_constant) {
                long long offset = instr->args[1]->data.int_val;
                if (offset != 0) {
                    // Load offset into a temp register, then add
                    emit_immediate(9, offset);  // x9 = offset
                    emit_instr("add\tx8, x8, x9");  // x8 = x8 + x9
                }
            }
            x8_temp_type = 1;
            x9_temp_type = 1;
            break;
            
        case IR_LOAD_EXTERNAL:
            // Load value from external symbol (like stderr, stdout)
            // For external symbols, we need to load from GOT
            // result->is_temp should be true, result->is_pointer should be true
            if (instr->label) {
                const char *name = instr->label;
                // Load the pointer value from GOT
                // adrp + ldr gives us the address of the variable
                // then we need another ldr to get the actual value
                emit_instr("adrp\tx8, _%s@GOTPAGE", name);
                emit_instr("ldr\tx8, [x8, _%s@GOTPAGEOFF]", name);
                // For FILE* like stderr, we need to dereference once more
                // The GOT entry contains the address of stderr,
                // and stderr itself is a FILE*
                emit_instr("ldr\tx8, [x8]");
            }
            x8_temp_type = 1;
            if (instr->result) {
                instr->result->is_temp = true;
            }
            break;
            
        case IR_LOAD_FUNC_ADDR:
        case IR_LOAD_GLOBAL:
            // Load address of function or static global variable
            // result->is_temp should be true, result->is_pointer should be true
            // instr->label contains the symbol name
            if (instr->label) {
                // Load address using adrp + add
                // For Mach-O: use _name for C functions
                const char *name = instr->label;
                emit_instr("adrp\tx8, _%s@PAGE", name);
                emit_instr("add\tx8, x8, _%s@PAGEOFF", name);
            }
            x8_temp_type = 1;
            if (instr->result) {
                instr->result->is_temp = true;
            }
            break;
            
        case IR_SAVE_X8:
            // Save x8 to x22 (for post-increment original value)
            emit_instr("mov\tx22, x8");
            break;
            
        case IR_SAVE_X8_TO_X20:
            // Save x8 to x20 (preserve pointer across reload)
            emit_instr("mov\tx20, x8");
            break;
            
        case IR_SAVE_X8_TO_X22:
            // Save x8 to x22 (callee-saved, for struct member access)
            emit_instr("mov\tx22, x8");
            break;
            
        case IR_RESTORE_X8_RESULT:
            // Restore x8 from x21 (for post-increment result)
            emit_instr("mov\tx8, x21");
            break;
            
        case IR_RESTORE_X8_FROM_X20:
            // Restore x8 from x20
            emit_instr("mov\tx8, x20");
            break;
            
        case IR_ADD_X21:
            // x8 = x20 + x8 (add saved address to offset)
            emit_instr("add\tx8, x20, x8");
            x8_temp_type = 1;
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
    // Need to count both IR_ALLOCA instructions AND parameter stack slots
    int locals_size = 0;
    int max_param_offset = 0;
    for (size_t i = 0; i < list_size(func->blocks); i++) {
        IRBasicBlock *block = list_get(func->blocks, i);
        for (size_t j = 0; j < list_size(block->instructions); j++) {
            IRInstruction *instr = list_get(block->instructions, j);
            if (instr->opcode == IR_ALLOCA && instr->result && instr->result->is_constant) {
                locals_size += (int)instr->result->data.int_val;
            }
            // Also track parameter offsets for IR_STORE_PARAM
            if (instr->opcode == IR_STORE_PARAM && instr->result) {
                int offset = (int)instr->result->offset;
                if (offset > max_param_offset) {
                    max_param_offset = offset;
                }
            }
        }
    }
    // Add parameter stack space (max_param_offset + 8 for the last parameter's slot)
    // Note: We use >= 0 because offset 0 means 1 parameter at that offset
    if (max_param_offset >= 0) {
        int param_space = max_param_offset + 8;
        if (param_space > locals_size) {
            locals_size = param_space;
        }
    }
    
    // Use underscore prefix for all functions (macOS convention)
    // Only emit .globl for non-static functions (static functions have internal linkage)
    if (!func->is_static) {
        emit("\n\t.globl\t_%s\n", func->name);
    } else {
        emit("\n");  // Just a newline for static functions
    }
    emit("\t.p2align\t2\n");
    emit("\t_%s:\n", func->name);
    
    // For main function, also create a non-underscored alias
    if (strcmp(func->name, "main") == 0 && !func->is_static) {
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
    
    // Emit global variables in data section
    if (list_size(module->globals) > 0) {
        emit("\t.section\t__DATA,__data\n");
        for (size_t i = 0; i < list_size(module->globals); i++) {
            IRGlobal *g = list_get(module->globals, i);
            emit("\t.p2align\t3\n");
            // Static variables have internal linkage - don't emit .globl
            if (!g->is_static) {
                emit("\t.globl\t_%s\n", g->name);
            }
            emit("_%s:\n", g->name);
            if (g->initializer && g->initializer->is_constant) {
                // Check if initializer is a string literal
                if (g->initializer->kind == IR_VALUE_STRING) {
                    // Emit reference to string literal label
                    emit("\t.quad\tl_.str%d\n", g->initializer->string_index);
                } else {
                    // Emit the integer initializer value
                    emit("\t.quad\t%lld\n", (long long)g->initializer->data.int_val);
                }
            } else {
                // Zero-initialize
                emit("\t.quad\t0\n");
            }
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
