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
    // Allocate locals aligned to 16 bytes, plus 16 bytes for saved fp/lr
    // Stack grows down: [saved fp/lr at lower addresses, locals at higher addresses]
    // Layout: sp -> [locals...] -> [saved fp] -> [saved lr] -> (original sp)
    // With locals_size = 8 (one 8-byte variable), we need:
    //   8 bytes (local) + 16 bytes (fp/lr) = 24, rounded to 32
    int total = locals_size + 16;
    total = (total + 15) & ~15;  // Round up to 16-byte boundary
    if (total > 0)
        emit_instr("sub\tsp, sp, #%d", total);
    
    // For small offsets (within [-512, 504]), use direct store
    // For larger offsets, use mov + str
    if (locals_size <= 504 && locals_size >= -512) {
        emit_instr("stp\tx29, x30, [sp, #%d]", locals_size);  // Save fp/lr above locals
    } else {
        // For large offsets, save fp/lr at sp
        emit_instr("stp\tx29, x30, [sp, #0]");
    }
    emit_instr("mov\tx29, sp");
    if (locals_size <= 504 && locals_size >= -512) {
        emit_instr("add\tx29, x29, #%d", locals_size);  // x29 points to fp/lr location
    } else {
        // For large offsets, add locals_size to x29
        emit_immediate(8, locals_size);
        emit_instr("add\tx29, x29, x8");
    }
}

static void epilogue(int locals_size) {
    int total = locals_size + 16;
    total = (total + 15) & ~15;  // Round up to 16-byte boundary
    // x29 already points to [sp, #locals_size], which is where fp/lr were saved
    emit_instr("ldp\tx29, x30, [x29]");  // Restore fp/lr using x29
    if (total > 0)
        emit_instr("add\tsp, sp, #%d", total);
    emit_instr("ret");
}

// Forward declaration
static int current_locals_size;

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
                // It's a local variable on the stack - load from [sp, #offset]
                // val->offset contains the stack offset
                // Use 64-bit load for pointers, 32-bit for integers
                if (val->is_pointer) {
                    emit_instr("ldr\tx%d, [sp, #%d]", reg, val->offset);
                } else {
                    emit_instr("ldr\tw%d, [sp, #%d]", reg, val->offset);
                }
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
// - Previous value (first operand or temp) is saved in x10 by IR_LOAD_STACK/IR_CONST_INT
// - Constants are loaded to x8
// - For binary ops: x0 = left, x1 = right
static void emit_load_binary_args(IRValue *left, IRValue *right) {
    bool left_is_temp = left && left->kind == IR_VALUE_INT && left->is_temp;
    bool right_is_temp = right && right->kind == IR_VALUE_INT && right->is_temp;
    bool right_is_const = right && right->kind == IR_VALUE_INT && right->is_constant;
    bool left_is_const = left && left->kind == IR_VALUE_INT && left->is_constant;
    
    if (left_is_temp && right_is_temp) {
        // Both temps: x10 = left (saved by previous instruction), x8 = right
        emit_instr("mov\tx0, x10");   // x0 = left
        emit_instr("mov\tx1, x8");    // x1 = right
    } else if (left_is_temp && right_is_const) {
        // Left in x10 (saved by previous instruction), right is constant in x8
        emit_instr("mov\tx0, x10");   // x0 = left
        emit_instr("mov\tx1, x8");    // x1 = constant
    } else if (left_is_temp && !right_is_temp && !right_is_const) {
        // Left in x10, right is something else (e.g., parameter or stack load)
        emit_instr("mov\tx0, x10");   // x0 = left
        emit_load_value(1, right);    // x1 = right
    } else if (right_is_temp && !left_is_temp) {
        // Right in x8, left is something else
        emit_instr("mov\tx1, x8");    // x1 = right
        emit_load_value(0, left);     // x0 = left
    } else if (right_is_const && !left_is_temp) {
        // Right is constant (in x8), left is something else
        emit_instr("mov\tx1, x8");    // x1 = constant
        emit_load_value(0, left);     // x0 = left
    } else if (left_is_const && !right_is_temp) {
        // Left is constant (in x8), right is something else
        emit_instr("mov\tx0, x8");    // x0 = constant
        emit_load_value(1, right);    // x1 = right
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
            // Load the condition value
            if (instr->num_args > 0 && instr->args[0]) {
                emit_load_value(0, instr->args[0]);
            }
            // cbz jumps if condition is FALSE (zero)
            emit_instr("cbz\tw0, %s", label);
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
        
        case IR_CALL_INDIRECT: {
            // Indirect call through function pointer
            // The callee (fn) is in x0, and arguments are in their original registers (x1, x2, x3)
            // We just need to call through x0
            
            // Indirect call through x0
            emit_instr("blr\tx0");
            
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
            // Use 64-bit multiplication for pointer arithmetic
            emit_instr("mul\tx0, x0, x1");
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
            // Load from address
            // If args[0] is NULL, load from [x8] (for array subscript)
            // Otherwise, x0 contains the address
            if (instr->num_args == 0) {
                // Load from address in x8
                // Use 8-byte load for pointers, 4-byte for integers
                if (instr->result && instr->result->is_pointer) {
                    emit_instr("ldr\tx8, [x8]");  // 8-byte load for pointers
                } else {
                    emit_instr("ldr\tw8, [x8]");  // 4-byte load for integers
                }
                // x8 now has the loaded value - don't set temp type here
                // as the caller (e.g., IR_CALL) should not save this value
            } else {
                // Legacy behavior: x0 contains the address
                if (instr->result && instr->result->is_pointer) {
                    emit_instr("ldr\tx0, [x0]");
                    emit_instr("mov\tx8, x0");
                } else {
                    emit_instr("ldr\tw0, [x0]");
                    emit_instr("mov\tx8, x0");
                }
            }
            break;
            
        case IR_STORE:
            // x0 contains the address; store x0's value to it
            emit_instr("str\tw0, [x0]");
            break;
            
        case IR_LOAD_STACK: {
            // Load from [sp, #offset]
            // Strategy: if we have a pending value in x8, save it to x10 first.
            // Also save x9 to x21 (it may have the first operand or return value from previous ops).
            if (instr->result && instr->result->kind == IR_VALUE_INT) {
                int old_x9_type = x9_temp_type;
                int offset = (int)instr->result->offset;
                if (old_x9_type == 1 || old_x9_type == 2) {
                    emit_instr("mov\tx21, x9");  // Save x9 before it gets clobbered
                }
                emit_instr("mov\tx10, x8");  // Always save x8 first (will be garbage if first load)
                // Use 64-bit load for pointers or parameters, 32-bit for integers
                if (instr->result->is_pointer || instr->result->param_reg == -2) {
                    emit_instr("ldr\tx0, [sp, #%d]", offset);  // 64-bit load for pointers
                } else {
                    emit_instr("ldr\tw0, [sp, #%d]", offset);  // 32-bit load for integers
                }
                emit_instr("mov\tx8, x0");  // Save loaded value to x8
                if (old_x9_type == 1 || old_x9_type == 2) {
                    emit_instr("mov\tx9, x21");  // Restore x9
                }
            }
            x8_temp_type = 1;  // x8 now has the loaded value
            x9_temp_type = 1;  // x9 has the previous x8 value (first operand)
            break;
        }
            
        case IR_STORE_STACK: {
            // Store value to [sp, #offset]
            // The value is in x8 (from the previous load or const_int)
            // result->offset = stack offset
            // Use 8-byte store by default for all values to avoid truncation issues
            if (instr->result && instr->result->kind == IR_VALUE_INT) {
                int offset = (int)instr->result->offset;
                // Use 8-byte store for all values to avoid pointer truncation
                emit_instr("str\tx8, [sp, #%d]", offset);
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
            if (instr->result && instr->result->kind == IR_VALUE_INT) {
                int param_reg = (int)instr->result->data.int_val;
                int offset = (int)instr->result->offset;
                // Store parameter register to [sp, #offset]
                emit_instr("str\tx%d, [sp, #%d]", param_reg, offset);
            }
            break;
        }
            
        case IR_CONST_INT:
            // Emit the constant load
            // Strategy: load constant directly to x8/x9 without touching x0 (which may hold parameters)
            // x20: save area for preserving x8 (return value or first operand)
            // x19: save area for preserving x9 when it had a return value (temp_type == 1)
            if (instr->result && instr->result->kind == IR_VALUE_INT) {
                int old_x8_type = x8_temp_type;
                int old_x9_type = x9_temp_type;
                if (old_x8_type == 1 || old_x8_type == 2) {
                    emit_instr("mov\tx20, x8");  // Save x8 (return value or first operand)
                }
                if (old_x9_type == 1) {
                    emit_instr("mov\tx19, x9");  // Save x9 when it had value
                }
                emit_instr("mov\tx10, x8");  // Save previous x8 to x10 (for binary ops to access)
                // Load constant directly to x8, NOT to x0 (preserve parameters)
                emit_immediate(8, instr->result->data.int_val);
                emit_instr("mov\tx9, x8");   // Copy constant to x9
                // DON'T restore x8 here - the constant should stay in x8 for the next operation
                // The saved value in x20 can be restored later if needed by emit_load_binary_args
                x8_temp_type = 2;  // x8 now has a constant
                x9_temp_type = 2;  // x9 has the constant
            }
            break;

        case IR_SAVE_X8:
            // Save x8 to x23 (callee-saved) for later use (e.g., post-increment original value, or array address)
            emit_instr("mov\tx23, x8");
            break;

        case IR_RESTORE_X8_RESULT:
            // For post-increment: result should be original value (in x23)
            emit_instr("mov\tx0, x23");  // Return original value
            emit_instr("mov\tx8, x23");  // x8 also has original value
            x8_temp_type = 1;  // x8 has temp result
            x9_temp_type = 1;  // x9 has temp result
            break;
            
        case IR_SAVE_X8_TO_X20:
            // Save x8 to x20 (callee-saved) to preserve across reload
            emit_instr("mov\tx20, x8");
            break;
            
        case IR_RESTORE_X8_FROM_X20:
            // Restore x8 from x20
            emit_instr("mov\tx8, x20");
            x8_temp_type = 1;  // x8 has the restored value
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
            // Store w8 to [x22]
            // x22 has the address (saved earlier with IR_SAVE_X8)
            // x8 has the value to store
            emit_instr("str\tw8, [x22]");
            break;
            
        case IR_LEA:
            // Load effective address: x8 = sp + offset
            // result->offset = stack offset
            if (instr->result && instr->result->kind == IR_VALUE_INT) {
                int offset = (int)instr->result->offset;
                emit_instr("mov\tx8, sp");
                emit_instr("add\tx8, x8, #%d", offset);
                x8_temp_type = 1;  // x8 now has an address
                x9_temp_type = 1;
            }
            break;
            
        case IR_ADD_X21:
            // x8 = x20 + x8 (add saved pointer in x20 to offset in x8)
            emit_instr("add\tx8, x20, x8");
            x8_temp_type = 1;
            x9_temp_type = 1;
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
