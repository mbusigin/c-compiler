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
        default: opcode = IR_ADD; break;  // TODO: handle other ops
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
            // For string literals, create a global reference
            return lower_string_literal(node);
            
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
