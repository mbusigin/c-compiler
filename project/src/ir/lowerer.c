/**
 * lowerer.c - AST to IR lowering
 */

#include "lowerer.h"
#include "../common/util.h"
#include "../sema/symtab.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static IRModule *current_module = NULL;
static IRFunction *current_function = NULL;
static IRBasicBlock *current_block = NULL;
static SymbolTable *current_symtab = NULL;
static IRValue *param_values[16];  // Simple parameter value storage (max 16 params)
static int param_count = 0;
static char *param_names[16];  // Parameter names for lookup
static int temp_counter = 0;
static int label_counter = 0;

// Forward declarations
static IRValue *lower_expression(ASTNode *node);
static void lower_statement(ASTNode *node);

// Helper to find parameter by name
static int find_param_index(const char *name) {
    for (int i = 0; i < param_count; i++) {
        if (param_names[i] && strcmp(param_names[i], name) == 0) {
            return i;
        }
    }
    return -1;
}

// Create a new temporary
static const char *new_temp(void) {
    static char buf[32];
    snprintf(buf, sizeof(buf), "t%d", temp_counter++);
    return xstrdup(buf);
}

// Create a new label
static const char *new_label(void) {
    static char buf[32];
    snprintf(buf, sizeof(buf), "L%d", label_counter++);
    return xstrdup(buf);
}

// Create an IR value
static IRValue *ir_value_create(IRValueKind kind) {
    IRValue *v = calloc(1, sizeof(IRValue));
    v->kind = kind;
    return v;
}

// Create an IR instruction
static IRInstruction *ir_instr_create(IROpcode opcode) {
    IRInstruction *instr = calloc(1, sizeof(IRInstruction));
    instr->opcode = opcode;
    return instr;
}

// Add instruction to current block
static void add_instr(IRInstruction *instr) {
    if (current_block) {
        list_push(current_block->instructions, instr);
    }
}

// Create a binary operation
static IRValue *lower_binary_op(IROpcode opcode, IRValue *left, IRValue *right) {
    IRValue *result = ir_value_create(IR_VALUE_INT);
    result->is_constant = false;
    result->is_temp = true;  // This is a temporary holding the result of this operation
    // TODO: Store the temporary name in the result
    const char *temp_name = new_temp();
    UNUSED(temp_name);
    IRInstruction *instr = ir_instr_create(opcode);
    instr->result = result;
    instr->args[0] = left;
    instr->args[1] = right;
    instr->num_args = 2;
    add_instr(instr);
    return result;
}

// Lower an identifier expression
static IRValue *lower_identifier(ASTNode *node) {
    // Check if this is a parameter
    int param_idx = find_param_index(node->data.identifier.name);
    if (param_idx >= 0) {
        return param_values[param_idx];
    }
    
    // For non-parameters, create a new value
    IRValue *result = ir_value_create(IR_VALUE_INT);
    result->is_constant = false;
    result->is_temp = false;
    result->param_reg = -1;  // Unknown register for now
    return result;
}

// Lower an integer literal
static IRValue *lower_int_literal(ASTNode *node) {
    IRValue *result = ir_value_create(IR_VALUE_INT);
    result->data.int_val = node->data.int_literal.value;
    result->is_constant = true;
    result->is_temp = false;
    IRInstruction *instr = ir_instr_create(IR_CONST_INT);
    instr->result = result;
    add_instr(instr);
    return result;
}

// Lower a binary expression
static IRValue *lower_binary_expr(ASTNode *node) {
    IRValue *left = lower_expression(node->data.binary.left);
    IRValue *right = lower_expression(node->data.binary.right);

    IROpcode opcode;
    switch (node->data.binary.op) {
        case OP_ADD: opcode = IR_ADD; break;
        case OP_SUB: opcode = IR_SUB; break;
        case OP_MUL: opcode = IR_MUL; break;
        case OP_DIV: opcode = IR_DIV; break;
        case OP_MOD: opcode = IR_MOD; break;
        case OP_LSHIFT: opcode = IR_SHL; break;
        case OP_RSHIFT: opcode = IR_SHR; break;
        case OP_BITAND: opcode = IR_AND; break;
        case OP_BITOR: opcode = IR_OR; break;
        case OP_BITXOR: opcode = IR_XOR; break;
        case OP_LT: opcode = IR_CMP_LT; break;
        case OP_GT: opcode = IR_CMP_GT; break;
        case OP_LE: opcode = IR_CMP_LE; break;
        case OP_GE: opcode = IR_CMP_GE; break;
        case OP_EQ: opcode = IR_CMP_EQ; break;
        case OP_NE: opcode = IR_CMP_NE; break;
        case OP_AND:
        case OP_OR: {
            // For logical AND/OR: we need to preserve the left result across the right evaluation.
            // Use stack to save/restore: push x8, load right, pop to x1, then op x0,x1.
            // Push left result (in x8) to stack
            IRInstruction *push_instr = ir_instr_create(IR_NOP);
            push_instr->num_args = 0;
            // We'll emit: str x8, [sp, #-8]!  (pre-index store)
            // Since we can't emit raw asm from lowerer, we use a trick:
            // Create an IR_CONST_INT with a marker, and handle it specially in codegen.
            // Actually, simpler: just use x9 as a second temp register.
            // Track x9 state similarly to x8.

            // For AND: result = (left != 0) && (right != 0)  =>  left_bit & right_bit
            // For OR:  result = (left != 0) || (right != 0)   =>  left_bit | right_bit
            // Where left_bit = (left != 0 ? 1 : 0), right_bit = (right != 0 ? 1 : 0)
            //
            // Approach: load left->x8, compare x8 to 0 -> cset to x8,
            //           load right->x0, compare x0 to 0 -> cset to x0,
            //           x0 = x0 & x8 (AND) or x0 = x0 | x8 (OR)
            // Problem: can't compare twice without saving first result.
            //
            // Solution: use x9 as temporary. Load left->x8, mov x8->x9, load right->x0,
            //           cmp x0 to 0 -> cset to x0, cmp x9 to 0 -> cset to x9,
            //           x0 = x0 & x9 or x0 = x0 | x9

            // Actually, since x9 might be used by other code, use the stack:
            // str x8, [sp, #-16]!  (save left result)
            // ... load right and compute into x0 ...
            // ldr x9, [sp], #16   (restore left result)
            // cmp/set for both, then AND/OR

            // Create a sequence of IR instructions to handle this.
            // Use a simple approach: load left (already done by lower_expression),
            // use IR_BOOL_AND/OR with right operand only.
            // IR_BOOL_AND: assumes left in x8, loads right into x0, ANDs them
            // But this doesn't work if right evaluation overwrites x8.

            // Best approach: explicit stack save/restore using emit_load_binary_args
            // but with x8 preserved. Since emit_load_binary_args doesn't support this,
            // handle it by re-emulating the load logic with x8 save.

            // Create IR instructions that will:
            // 1. Save x8 to stack (as part of loading right operand)
            // 2. Load right into x0
            // 3. AND/OR with x8 (which still has left result)
            // 4. Result in x0 and x8

            // For right operand, we need to load it and save x8.
            // Create a sequence: save x8 to stack, load right to x0, then restore x8 to x1 for AND.

            // HACK: Use IR_NOP with special handling in codegen to emit stack operations.
            // For now, use a simpler approach that works for the common case:
            // Emit: push x8, load right, pop x9, load left (from stack), compare+set both, and/x0,x9

            // Use the IR to emit a comparison for left (sets x8)
            // Then save x8 using a custom IR instruction that codegen handles specially.
            // We'll use IR_NOP as a marker... actually let's use IR_CONST_INT with a special value.

            // The cleanest fix: add a new opcode IR_SAVE_X8 that codegen handles as "str x8, [sp, #-8]!"
            // Then add IR_RESTORE_X8 that codegen handles as "ldr x9, [sp], #8"
            // And update emit_load_binary_args to use x9 instead of x8 when x8 is saved.

            // For now, let's just emit the IR operations and handle the x8 overwrite
            // by re-loading the left value in the AND/OR handler.

            // Actually the simplest fix: in IR_BOOL_AND/IR_BOOL_OR, after loading right into x0,
            // if left result was consumed, reload left from the original IRValue.
            // The issue is we don't have the original IRValue after lower_expression consumed it.

            // OK here's the actual fix: use a different temp register (x9) that we save/restore
            // around the AND/OR operation. Codegen doesn't use x9, so we can use it freely.

            // Emit: mov x9, x8  (save left result)
            //       [load right into x0 - this sets x8 to right result]
            //       cmp x9 to 0 -> cset x9
            //       cmp x0 to 0 -> cset x0
            //       x0 = x0 & x9 (or | x9)
            //       mov x8, x0

            // Since we can't emit "mov x9, x8" from lowerer, let's just use the approach
            // where we generate comparison IR for left, then comparison IR for right,
            // and have codegen handle saving/restoring x8.

            // Use IR_CMP_LT (arbitrary comparison) to reload left and save to x9,
            // then handle right and combine.

            // Simple approach: for AND/OR, use IR_ADD as a placeholder.
            // The actual fix will be in codegen to handle these cases specially.
            // Actually, let me just fix lowerer to use IR_BOOL_AND/OR and handle
            // the x8 overwrite in codegen.

            // The correct fix for IR_BOOL_AND in codegen:
            // After loading right, we need left's result. Use x9 as temp:
            // mov x9, x8   (left result is in x8)
            // [right result is now in x8 after emit_load_value(0, right)]
            // cmp x9 to 0 -> cset x9 (left boolean)
            // cmp x8 to 0 -> cset x8 (right boolean)
            // and x0, x8, x9
            // mov x8, x0

            // This requires modifying IR_BOOL_AND in codegen to:
            // 1. Save x8 to x9 first
            // 2. Load right into x0 (overwrites x8)
            // 3. Compare x9 and set it
            // 4. Compare x0 (right) and set it
            // 5. AND the two booleans
            // 6. Result in x0 and x8

            // Use IR_BOOL_AND opcode with only right operand (left assumed in x8 from previous instr)
            // Codegen IR_BOOL_AND handles the save-and-compare sequence
            IRInstruction *bool_instr = ir_instr_create(
                node->data.binary.op == OP_AND ? IR_BOOL_AND : IR_BOOL_OR);
            bool_instr->result = ir_value_create(IR_VALUE_INT);
            bool_instr->result->is_constant = false;
            bool_instr->result->is_temp = true;
            bool_instr->args[0] = right;
            bool_instr->num_args = 1;
            add_instr(bool_instr);
            return bool_instr->result;
        }
        default: opcode = IR_ADD; break;
    }

    return lower_binary_op(opcode, left, right);
}

// Lower a unary expression
static IRValue *lower_unary_expr(ASTNode *node) {
    UNUSED(node);
    IRValue *operand = lower_expression(node->data.unary.operand);

    IROpcode opcode;
    switch (node->data.unary.op) {
        case 6:  // ++ (postfix, treated as unary)
            // For now, just return the operand
            return operand;
        case 7:  // --
            return operand;
        default:
            opcode = IR_ADD;  // Suppress warning
            UNUSED(opcode);
            return operand;
    }
}

// Lower an assignment expression
static IRValue *lower_assignment_expr(ASTNode *node) {
    IRValue *value = lower_expression(node->data.assignment.right);
    // For now, just return the value
    return value;
}

// Lower a function call
static IRValue *lower_call_expr(ASTNode *node) {
    IRValue *result = ir_value_create(IR_VALUE_INT);
    IRInstruction *instr = ir_instr_create(IR_CALL);
    instr->result = result;

    // Add callee
    if (node->data.call.callee && node->data.call.callee->type == AST_IDENTIFIER_EXPR) {
        instr->label = node->data.call.callee->data.identifier.name;
    }

    // Add arguments
    for (size_t i = 0; i < list_size(node->data.call.args); i++) {
        IRValue *arg = lower_expression(list_get(node->data.call.args, i));
        if (instr->num_args < 4) {
            instr->args[instr->num_args++] = arg;
        }
    }

    add_instr(instr);
    return result;
}

// Forward declaration
static IRValue *lower_string_literal(ASTNode *node);

// Lower any expression
static IRValue *lower_expression(ASTNode *node) {
    if (!node) return NULL;

    switch (node->type) {
        case AST_INTEGER_LITERAL_EXPR:
            return lower_int_literal(node);

        case AST_IDENTIFIER_EXPR:
            return lower_identifier(node);

        case AST_BINARY_EXPR:
            return lower_binary_expr(node);

        case AST_UNARY_EXPR:
            return lower_unary_expr(node);

        case AST_ASSIGNMENT_EXPR:
            return lower_assignment_expr(node);

        case AST_CALL_EXPR:
            return lower_call_expr(node);

        case AST_STRING_LITERAL_EXPR:
            return lower_string_literal(node);

        case AST_CONDITIONAL_EXPR: {
            // Ternary: condition ? then_expr : else_expr
            // For use in return statements, we generate:
            //   lower condition (result in x0/x8)
            //   IR_JMP_IF cond, else_label  (jump if false)
            //   lower then_expr, IR_RET
            //   IR_JMP end_label
            //   else_label: lower else_expr, IR_RET
            //   end_label:
            const char *else_label = new_label();
            const char *end_label = new_label();

            // Lower condition
            IRValue *cond_val = lower_expression(node->data.conditional.condition);

            // Jump to else if condition is false
            IRInstruction *jmp_if = ir_instr_create(IR_JMP_IF);
            jmp_if->args[0] = cond_val;
            jmp_if->label = else_label;
            add_instr(jmp_if);

            // Then branch: lower then_expr and return it
            IRValue *then_val = lower_expression(node->data.conditional.then_expr);
            IRInstruction *ret_then = ir_instr_create(IR_RET);
            ret_then->args[0] = then_val;
            ret_then->num_args = 1;
            add_instr(ret_then);

            // Jump to end
            IRInstruction *jmp_end = ir_instr_create(IR_JMP);
            jmp_end->label = end_label;
            add_instr(jmp_end);

            // Else label
            IRInstruction *else_lbl = ir_instr_create(IR_LABEL);
            else_lbl->label = else_label;
            add_instr(else_lbl);

            // Else branch: lower else_expr and return it
            IRValue *else_val = lower_expression(node->data.conditional.else_expr);
            IRInstruction *ret_else = ir_instr_create(IR_RET);
            ret_else->args[0] = else_val;
            ret_else->num_args = 1;
            add_instr(ret_else);

            // End label
            IRInstruction *end_lbl = ir_instr_create(IR_LABEL);
            end_lbl->label = end_label;
            add_instr(end_lbl);

            // Return a dummy value (the actual returns are above)
            IRValue *result = ir_value_create(IR_VALUE_INT);
            result->is_constant = false;
            result->is_temp = true;
            return result;
        }

        default:
            return ir_value_create(IR_VALUE_INT);
    }
}

// Lower a string literal
static IRValue *lower_string_literal(ASTNode *node) {
    // Add string to module's string list
    const char *str = node->data.string_literal.value;
    list_push(current_module->strings, strdup(str));
    
    // Return an IR value that references the string (we'll handle this in codegen)
    IRValue *val = ir_value_create(IR_VALUE_STRING);
    val->string_index = list_size(current_module->strings) - 1;
    return val;
}

// Lower a return statement
static void lower_return_stmt(ASTNode *node) {
    if (node->data.return_stmt.expr) {
        IRValue *value = lower_expression(node->data.return_stmt.expr);
        if (value) {
            IRInstruction *instr = ir_instr_create(IR_RET);
            instr->args[0] = value;
            instr->num_args = 1;
            add_instr(instr);
        } else {
            // No value, just return void
            IRInstruction *instr = ir_instr_create(IR_RET_VOID);
            add_instr(instr);
        }
    } else {
        IRInstruction *instr = ir_instr_create(IR_RET_VOID);
        add_instr(instr);
    }
}

// Lower an expression statement
static void lower_expr_stmt(ASTNode *node) {
    if (node->data.expr_stmt.expr) {
        lower_expression(node->data.expr_stmt.expr);
    }
}

// Lower an if statement
static void lower_if_stmt(ASTNode *node) {
    const char *else_label = new_label();
    const char *end_label = new_label();

    // Condition
    if (node->data.if_stmt.condition) {
        IRValue *cond = lower_expression(node->data.if_stmt.condition);
        IRInstruction *instr = ir_instr_create(IR_JMP_IF);
        instr->args[0] = cond;
        instr->label = else_label;
        add_instr(instr);
    }

    // Then branch
    if (node->data.if_stmt.then_stmt) {
        lower_statement(node->data.if_stmt.then_stmt);
    }

    // Jump to end
    IRInstruction *jmp = ir_instr_create(IR_JMP);
    jmp->label = end_label;
    add_instr(jmp);

    // Else label
    IRInstruction *else_lbl = ir_instr_create(IR_LABEL);
    else_lbl->label = else_label;
    add_instr(else_lbl);

    // Else branch
    if (node->data.if_stmt.else_stmt) {
        lower_statement(node->data.if_stmt.else_stmt);
    }

    // End label
    IRInstruction *end_lbl = ir_instr_create(IR_LABEL);
    end_lbl->label = end_label;
    add_instr(end_lbl);
}

// Lower a while statement
static void lower_while_stmt(ASTNode *node) {
    const char *start_label = new_label();
    const char *end_label = new_label();

    // Start label
    IRInstruction *start_lbl = ir_instr_create(IR_LABEL);
    start_lbl->label = start_label;
    add_instr(start_lbl);

    // Condition
    if (node->data.while_stmt.condition) {
        IRValue *cond = lower_expression(node->data.while_stmt.condition);
        IRInstruction *instr = ir_instr_create(IR_JMP_IF);
        instr->args[0] = cond;
        instr->label = end_label;
        add_instr(instr);
    }

    // Body
    if (node->data.while_stmt.body) {
        lower_statement(node->data.while_stmt.body);
    }

    // Jump back to start
    IRInstruction *jmp = ir_instr_create(IR_JMP);
    jmp->label = start_label;
    add_instr(jmp);

    // End label
    IRInstruction *end_lbl = ir_instr_create(IR_LABEL);
    end_lbl->label = end_label;
    add_instr(end_lbl);
}

// Lower a for statement
static void lower_for_stmt(ASTNode *node) {
    const char *start_label = new_label();
    const char *end_label = new_label();

    // Init
    if (node->data.for_stmt.init) {
        lower_statement(node->data.for_stmt.init);
    }

    // Start label
    IRInstruction *start_lbl = ir_instr_create(IR_LABEL);
    start_lbl->label = start_label;
    add_instr(start_lbl);

    // Condition
    if (node->data.for_stmt.condition) {
        IRValue *cond = lower_expression(node->data.for_stmt.condition);
        IRInstruction *instr = ir_instr_create(IR_JMP_IF);
        instr->args[0] = cond;
        instr->label = end_label;
        add_instr(instr);
    }

    // Body
    if (node->data.for_stmt.body) {
        lower_statement(node->data.for_stmt.body);
    }

    // Increment
    if (node->data.for_stmt.increment) {
        lower_expression(node->data.for_stmt.increment);
    }

    // Jump back to start
    IRInstruction *jmp = ir_instr_create(IR_JMP);
    jmp->label = start_label;
    add_instr(jmp);

    // End label
    IRInstruction *end_lbl = ir_instr_create(IR_LABEL);
    end_lbl->label = end_label;
    add_instr(end_lbl);
}

// Lower a compound statement
static void lower_compound_stmt(ASTNode *node) {
    for (size_t i = 0; i < list_size(node->data.compound.stmts); i++) {
        lower_statement(list_get(node->data.compound.stmts, i));
    }
}

// Lower a statement
static void lower_statement(ASTNode *node) {
    if (!node) return;

    switch (node->type) {
        case AST_COMPOUND_STMT:
            lower_compound_stmt(node);
            break;
        case AST_RETURN_STMT:
            lower_return_stmt(node);
            break;
        case AST_EXPRESSION_STMT:
            lower_expr_stmt(node);
            break;
        case AST_IF_STMT:
            lower_if_stmt(node);
            break;
        case AST_WHILE_STMT:
            lower_while_stmt(node);
            break;
        case AST_FOR_STMT:
            lower_for_stmt(node);
            break;
        default:
            break;
    }
}

// Lower a function
static void lower_function(ASTNode *node) {
    IRFunction *func = ir_function_create(node->data.function.name);
    list_push(current_module->functions, func);

    // Create entry block
    IRBasicBlock *entry = ir_block_create("entry");
    list_push(func->blocks, entry);

    // Set current function and block
    IRFunction *prev_func = current_function;
    IRBasicBlock *prev_block = current_block;
    current_function = func;
    current_block = entry;

    // Handle parameters - in ARM64, first 4 integer params are in x0-x3
    param_count = 0;
    for (size_t i = 0; i < list_size(node->data.function.params) && i < 16; i++) {
        ASTNode *param = list_get(node->data.function.params, i);
        if (param->type == AST_PARAMETER_DECL) {
            // Create a value for this parameter (it's in a register)
            IRValue *param_val = ir_value_create(IR_VALUE_INT);
            param_val->is_constant = false;
            param_val->is_temp = false;
            param_val->param_reg = param_count;  // x0, x1, x2, x3
            param_values[param_count] = param_val;
            param_names[param_count] = xstrdup(param->data.parameter.name);
            param_count++;
        }
    }

    // Lower body
    if (node->data.function.body) {
        lower_statement(node->data.function.body);
    }

    // Add implicit return if needed
    // (For now, assume the last statement is a return)

    // Restore
    current_function = prev_func;
    current_block = prev_block;
}

// Lower a global variable declaration
static void lower_global_var(ASTNode *node) {
    UNUSED(node);
    // TODO: Handle global variables
}

// Lower a translation unit
static void lower_translation_unit(ASTNode *node) {
    for (size_t i = 0; i < list_size(node->data.unit.declarations); i++) {
        ASTNode *decl = list_get(node->data.unit.declarations, i);
        if (decl->type == AST_FUNCTION_DECL) {
            lower_function(decl);
        } else if (decl->type == AST_VARIABLE_DECL) {
            lower_global_var(decl);
        }
    }
}

IRModule *lowerer_lower(ASTNode *ast, SymbolTable *symtab) {
    current_module = ir_module_create();
    current_symtab = symtab;
    temp_counter = 0;
    label_counter = 0;

    if (ast && ast->type == AST_TRANSLATION_UNIT) {
        lower_translation_unit(ast);
    }

    return current_module;
}

void lowerer_free_module(IRModule *module) {
    ir_module_destroy(module);
}
