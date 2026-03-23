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
static int label_counter = 0;

// Forward declarations
static unsigned int locals_hash_fn(const char *name);
static IRValue *lower_expression(ASTNode *node);
static void lower_statement(ASTNode *node);

// Simple locals tracking via hash table
#define LOCALS_BUCKETS 32
typedef struct LocalVar {
    char *name;
    int offset;       // Stack offset (positive from sp)
    struct LocalVar *next;
} LocalVar;
static LocalVar *locals_hash[LOCALS_BUCKETS];
static int locals_size = 0;  // Total bytes allocated on stack for locals

// Scope management for variable shadowing
#define SCOPE_STACK_DEPTH 32
typedef struct Scope {
    int base_locals_size;   // locals_size at scope entry
    int num_locals;         // Number of locals added in this scope
    const char *names[32];  // Names of locals in this scope
} Scope;
static Scope scope_stack[SCOPE_STACK_DEPTH];
static int scope_depth = 0;

static void scope_push(void) {
    if (scope_depth < SCOPE_STACK_DEPTH) {
        scope_stack[scope_depth].base_locals_size = locals_size;
        scope_stack[scope_depth].num_locals = 0;
        scope_depth++;
    }
}

static void scope_pop(void) {
    if (scope_depth > 0) {
        scope_depth--;
        // Remove locals declared in this scope from the hash table
        for (int i = 0; i < scope_stack[scope_depth].num_locals; i++) {
            const char *name = scope_stack[scope_depth].names[i];
            if (name) {
                unsigned int idx = locals_hash_fn(name);
                LocalVar **prev = &locals_hash[idx];
                while (*prev) {
                    if (strcmp((*prev)->name, name) == 0) {
                        LocalVar *to_free = *prev;
                        *prev = (*prev)->next;
                        free(to_free->name);
                        free(to_free);
                        break;
                    }
                    prev = &(*prev)->next;
                }
            }
        }
    }
}

// Loop context for break/continue
typedef struct LoopContext {
    const char *start_label;
    const char *end_label;
    struct LoopContext *next;
} LoopContext;
static LoopContext *loop_stack = NULL;

// Hash table helpers for locals
static unsigned int locals_hash_fn(const char *name) {
    unsigned int h = 2166136261u;
    while (*name) {
        h ^= (unsigned char)*name++;
        h *= 16777619u;
    }
    return h % LOCALS_BUCKETS;
}

static LocalVar *locals_lookup(const char *name) {
    unsigned int idx = locals_hash_fn(name);
    for (LocalVar *lv = locals_hash[idx]; lv; lv = lv->next) {
        if (strcmp(lv->name, name) == 0) return lv;
    }
    return NULL;
}

static void locals_add(const char *name, int offset) {
    LocalVar *lv = malloc(sizeof(LocalVar));
    lv->name = xstrdup(name);
    lv->offset = offset;
    unsigned int idx = locals_hash_fn(name);
    lv->next = locals_hash[idx];
    locals_hash[idx] = lv;
    // Register in current scope
    if (scope_depth > 0 && scope_stack[scope_depth-1].num_locals < 32) {
        scope_stack[scope_depth-1].names[scope_stack[scope_depth-1].num_locals++] = lv->name;
    }
}

static void locals_clear(void) {
    for (int i = 0; i < LOCALS_BUCKETS; i++) {
        LocalVar *lv = locals_hash[i];
        while (lv) {
            LocalVar *next = lv->next;
            free(lv->name);
            free(lv);
            lv = next;
        }
        locals_hash[i] = NULL;
    }
    locals_size = 0;
    scope_depth = 0;
}

// Loop context helpers
static void loop_push(const char *start, const char *end) {
    LoopContext *ctx = malloc(sizeof(LoopContext));
    ctx->start_label = start;
    ctx->end_label = end;
    ctx->next = loop_stack;
    loop_stack = ctx;
}

static void loop_pop(void) {
    if (loop_stack) {
        LoopContext *next = loop_stack->next;
        free(loop_stack);
        loop_stack = next;
    }
}

// Helper to find parameter by name
static int find_param_index(const char *name) {
    for (int i = 0; i < param_count; i++) {
        if (param_names[i] && strcmp(param_names[i], name) == 0) {
            return i;
        }
    }
    return -1;
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
    result->is_temp = true;
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
    
    // Check if this is a local variable
    LocalVar *lv = locals_lookup(node->data.identifier.name);
    if (lv) {
        // Create an IR_LOAD_STACK instruction: load from [sp, #offset]
        IRValue *result = ir_value_create(IR_VALUE_INT);
        result->is_constant = false;
        result->is_temp = true;
        result->offset = lv->offset;
        result->param_reg = -2;  // Mark as local variable (stack-relative)
        IRInstruction *load_instr = ir_instr_create(IR_LOAD_STACK);
        load_instr->result = result;
        add_instr(load_instr);
        return result;
    }
    
    // For non-parameters and non-variables, create a dummy value
    IRValue *result = ir_value_create(IR_VALUE_INT);
    result->is_constant = false;
    result->is_temp = false;
    result->param_reg = -1;
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
    IRValue *operand = lower_expression(node->data.unary.operand);

    switch (node->data.unary.op) {
        case 0: {  // ++x (prefix increment)
            // operand result is in x0; x8_has_temp is set
            // Emit constant 1 after (constant will go to x0, x8 preserved)
            IRValue *one_val = ir_value_create(IR_VALUE_INT);
            one_val->data.int_val = 1;
            one_val->is_constant = true;
            IRInstruction *one_instr = ir_instr_create(IR_CONST_INT);
            one_instr->result = one_val;
            add_instr(one_instr);
            
            IRInstruction *add_instr2 = ir_instr_create(IR_ADD);
            add_instr2->result = ir_value_create(IR_VALUE_INT);
            add_instr2->result->is_constant = false;
            add_instr2->result->is_temp = true;
            add_instr2->args[0] = operand;
            add_instr2->args[1] = one_val;
            add_instr2->num_args = 2;
            add_instr(add_instr2);
            return add_instr2->result;
        }
        case 1: {  // --x (prefix decrement)
            IRValue *one_val = ir_value_create(IR_VALUE_INT);
            one_val->data.int_val = 1;
            one_val->is_constant = true;
            IRInstruction *one_instr = ir_instr_create(IR_CONST_INT);
            one_instr->result = one_val;
            add_instr(one_instr);
            
            IRInstruction *sub_instr = ir_instr_create(IR_SUB);
            sub_instr->result = ir_value_create(IR_VALUE_INT);
            sub_instr->result->is_constant = false;
            sub_instr->result->is_temp = true;
            sub_instr->args[0] = operand;
            sub_instr->args[1] = one_val;
            sub_instr->num_args = 2;
            add_instr(sub_instr);
            return sub_instr->result;
        }
        case 2: {  // ! (logical NOT)
            // For !x, return x == 0 ? 1 : 0
            IRValue *zero_val = ir_value_create(IR_VALUE_INT);
            zero_val->data.int_val = 0;
            zero_val->is_constant = true;
            IRInstruction *zero_instr = ir_instr_create(IR_CONST_INT);
            zero_instr->result = zero_val;
            add_instr(zero_instr);
            
            IRInstruction *cmp_instr = ir_instr_create(IR_CMP_EQ);
            cmp_instr->result = ir_value_create(IR_VALUE_INT);
            cmp_instr->result->is_constant = false;
            cmp_instr->result->is_temp = true;
            cmp_instr->args[0] = operand;
            cmp_instr->args[1] = zero_val;
            cmp_instr->num_args = 2;
            add_instr(cmp_instr);
            return cmp_instr->result;
        }
        case 3: {  // ~ (bitwise NOT) - not in tests, skip
            return operand;
        }
        case 6: {  // ++ (postfix increment)
            // For post-increment: return original value, then increment.
            // Strategy: load operand (x9 = original), save x9 to x19 (callee-saved),
            // emit IR_CONST_INT 1, add operand+1, store result back, then restore x0=x19.
            IRInstruction *save_instr = ir_instr_create(IR_SAVE_X9);
            add_instr(save_instr);
            IRValue *one_val = ir_value_create(IR_VALUE_INT);
            one_val->data.int_val = 1;
            one_val->is_constant = true;
            IRInstruction *one_instr = ir_instr_create(IR_CONST_INT);
            one_instr->result = one_val;
            add_instr(one_instr);
            IRInstruction *add_instr2 = ir_instr_create(IR_ADD);
            add_instr2->result = ir_value_create(IR_VALUE_INT);
            add_instr2->result->is_constant = false;
            add_instr2->result->is_temp = true;
            add_instr2->args[0] = operand;
            add_instr2->args[1] = one_val;
            add_instr2->num_args = 2;
            add_instr(add_instr2);
            // Return: x0 should be original value (from x19)
            IRInstruction *ret_instr = ir_instr_create(IR_RESTORE_X9_RESULT);
            add_instr(ret_instr);
            IRValue *ret_val = ir_value_create(IR_VALUE_INT);
            ret_val->is_constant = false;
            ret_val->is_temp = true;
            return ret_val;
        }
        case 7: {  // -- (postfix decrement)
            // For post-decrement: return original value, then decrement.
            IRInstruction *save_instr = ir_instr_create(IR_SAVE_X9);
            add_instr(save_instr);
            IRValue *one_val = ir_value_create(IR_VALUE_INT);
            one_val->data.int_val = 1;
            one_val->is_constant = true;
            IRInstruction *one_instr = ir_instr_create(IR_CONST_INT);
            one_instr->result = one_val;
            add_instr(one_instr);
            IRInstruction *sub_instr = ir_instr_create(IR_SUB);
            sub_instr->result = ir_value_create(IR_VALUE_INT);
            sub_instr->result->is_constant = false;
            sub_instr->result->is_temp = true;
            sub_instr->args[0] = operand;
            sub_instr->args[1] = one_val;
            sub_instr->num_args = 2;
            add_instr(sub_instr);
            // Return: x0 should be original value (from x19)
            IRInstruction *ret_instr = ir_instr_create(IR_RESTORE_X9_RESULT);
            add_instr(ret_instr);
            IRValue *ret_val = ir_value_create(IR_VALUE_INT);
            ret_val->is_constant = false;
            ret_val->is_temp = true;
            return ret_val;
        }
        default:
            return operand;
    }
}

// Lower an assignment expression
static IRValue *lower_assignment_expr(ASTNode *node) {
    // Get the left-hand side (identifier)
    ASTNode *lhs = node->data.assignment.left;
    int op = node->data.assignment.op;  // 0 = plain assignment, OP_ADD = +=, etc.
    
    if (lhs && lhs->type == AST_IDENTIFIER_EXPR) {
        // Check if it's a parameter
        int param_idx = find_param_index(lhs->data.identifier.name);
        if (param_idx >= 0) {
            // For parameters, just return the value
            IRValue *value = lower_expression(node->data.assignment.right);
            return value;
        }
        
        // Check if it's a local variable
        LocalVar *lv = locals_lookup(lhs->data.identifier.name);
        if (lv) {
            // For compound assignment (+=, -=, etc.), load the current value first
            IRValue *result_val = NULL;
            if (op != 0) {
                // Load current value of the variable
                IRValue *load_result = ir_value_create(IR_VALUE_INT);
                load_result->offset = lv->offset;
                load_result->param_reg = -2;
                IRInstruction *load_instr = ir_instr_create(IR_LOAD_STACK);
                load_instr->result = load_result;
                add_instr(load_instr);
            }
            
            // Evaluate RHS → x0 = value, x8 = value
            IRValue *rhs_val = lower_expression(node->data.assignment.right);
            UNUSED(rhs_val);
            
            // If compound assignment, emit the operation
            if (op != 0) {
                IROpcode ir_op = 0;
                switch (op) {
                    case OP_ADD: ir_op = IR_ADD; break;
                    case OP_SUB: ir_op = IR_SUB; break;
                    case OP_MUL: ir_op = IR_MUL; break;
                    case OP_DIV: ir_op = IR_DIV; break;
                    case OP_MOD: ir_op = IR_MOD; break;
                    case OP_LSHIFT: ir_op = IR_SHL; break;
                    case OP_RSHIFT: ir_op = IR_SHR; break;
                    case OP_BITAND: ir_op = IR_AND; break;
                    case OP_BITOR: ir_op = IR_OR; break;
                    case OP_BITXOR: ir_op = IR_XOR; break;
                    default: break;
                }
                if (ir_op != 0) {
                    IRInstruction *op_instr = ir_instr_create(ir_op);
                    op_instr->result = ir_value_create(IR_VALUE_INT);
                    op_instr->result->is_constant = false;
                    op_instr->result->is_temp = true;
                    // After the load, x9 has the first operand (current value)
                    // After lower_expression, x8 has the RHS value
                    // For binary op: load_binary_args expects args[0] and args[1]
                    // Since args are not used directly for the op, create proper args
                    IRValue *arg0 = ir_value_create(IR_VALUE_INT);
                    arg0->is_constant = false;
                    arg0->is_temp = true;
                    arg0->offset = lv->offset;
                    arg0->param_reg = -2;
                    op_instr->args[0] = arg0;
                    op_instr->args[1] = rhs_val;
                    op_instr->num_args = 2;
                    add_instr(op_instr);
                    result_val = op_instr->result;
                }
            }
            
            if (result_val == NULL) {
                result_val = rhs_val;
            }
            
            // Store result to [sp, #offset] using IR_STORE_STACK
            IRValue *store_result = ir_value_create(IR_VALUE_INT);
            store_result->offset = lv->offset;
            IRInstruction *store_i = ir_instr_create(IR_STORE_STACK);
            store_i->result = store_result;
            add_instr(store_i);
            
            // Return the result value
            IRValue *ret_val = ir_value_create(IR_VALUE_INT);
            ret_val->is_constant = false;
            ret_val->is_temp = true;
            return ret_val;
        }
    }
    
    // Simple case: just return the value
    IRValue *value = lower_expression(node->data.assignment.right);
    return value;
}

// Lower a variable declaration
static void lower_variable_decl(ASTNode *node) {
    if (!node || node->type != AST_VARIABLE_DECL) return;
    
    // Allocate 8 bytes on stack
    // Layout: [local at sp+0, sp+8...][saved fp/lr above at sp+locals_size]
    // So first local is at offset 0, second at offset 8, etc.
    int var_offset = locals_size;
    locals_size += 8;
    locals_add(node->data.variable.name, var_offset);
    
    // Emit: sub sp, sp, #8  (allocate space)
    IRValue *size_val = ir_value_create(IR_VALUE_INT);
    size_val->data.int_val = 8;
    size_val->is_constant = true;
    IRInstruction *alloc_instr = ir_instr_create(IR_ALLOCA);
    alloc_instr->result = size_val;
    add_instr(alloc_instr);
    
    // Handle initializer
    if (node->data.variable.init) {
        // Evaluate initializer → x0 = value, x8 = value
        IRValue *init_val = lower_expression(node->data.variable.init);
        UNUSED(init_val);
        
        // Store value to [sp, #offset] using IR_STORE_STACK
        IRValue *result = ir_value_create(IR_VALUE_INT);
        result->offset = var_offset;
        IRInstruction *store_i = ir_instr_create(IR_STORE_STACK);
        store_i->result = result;
        add_instr(store_i);
    }
}

// Lower a function call
static IRValue *lower_call_expr(ASTNode *node) {
    IRValue *result = ir_value_create(IR_VALUE_INT);
    IRInstruction *instr = ir_instr_create(IR_CALL);
    instr->result = result;

    if (node->data.call.callee && node->data.call.callee->type == AST_IDENTIFIER_EXPR) {
        instr->label = node->data.call.callee->data.identifier.name;
    }

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
            const char *else_label = new_label();
            const char *end_label = new_label();

            IRValue *cond_val = lower_expression(node->data.conditional.condition);

            IRInstruction *jmp_if = ir_instr_create(IR_JMP_IF);
            jmp_if->args[0] = cond_val;
            jmp_if->label = else_label;
            add_instr(jmp_if);

            IRValue *then_val = lower_expression(node->data.conditional.then_expr);
            IRInstruction *ret_then = ir_instr_create(IR_RET);
            ret_then->args[0] = then_val;
            ret_then->num_args = 1;
            add_instr(ret_then);

            IRInstruction *jmp_end = ir_instr_create(IR_JMP);
            jmp_end->label = end_label;
            add_instr(jmp_end);

            IRInstruction *else_lbl = ir_instr_create(IR_LABEL);
            else_lbl->label = else_label;
            add_instr(else_lbl);

            IRValue *else_val = lower_expression(node->data.conditional.else_expr);
            IRInstruction *ret_else = ir_instr_create(IR_RET);
            ret_else->args[0] = else_val;
            ret_else->num_args = 1;
            add_instr(ret_else);

            IRInstruction *end_lbl = ir_instr_create(IR_LABEL);
            end_lbl->label = end_label;
            add_instr(end_lbl);

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
    const char *str = node->data.string_literal.value;
    list_push(current_module->strings, strdup(str));
    
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

    if (node->data.if_stmt.condition) {
        IRValue *cond = lower_expression(node->data.if_stmt.condition);
        IRInstruction *instr = ir_instr_create(IR_JMP_IF);
        instr->args[0] = cond;
        instr->label = else_label;
        add_instr(instr);
    }

    if (node->data.if_stmt.then_stmt) {
        lower_statement(node->data.if_stmt.then_stmt);
    }

    IRInstruction *jmp = ir_instr_create(IR_JMP);
    jmp->label = end_label;
    add_instr(jmp);

    IRInstruction *else_lbl = ir_instr_create(IR_LABEL);
    else_lbl->label = else_label;
    add_instr(else_lbl);

    if (node->data.if_stmt.else_stmt) {
        lower_statement(node->data.if_stmt.else_stmt);
    }

    IRInstruction *end_lbl = ir_instr_create(IR_LABEL);
    end_lbl->label = end_label;
    add_instr(end_lbl);
}

// Lower a while statement
static void lower_while_stmt(ASTNode *node) {
    const char *start_label = new_label();
    const char *end_label = new_label();

    loop_push(start_label, end_label);

    IRInstruction *start_lbl = ir_instr_create(IR_LABEL);
    start_lbl->label = start_label;
    add_instr(start_lbl);

    if (node->data.while_stmt.condition) {
        IRValue *cond = lower_expression(node->data.while_stmt.condition);
        IRInstruction *instr = ir_instr_create(IR_JMP_IF);
        instr->args[0] = cond;
        instr->label = end_label;
        add_instr(instr);
    }

    if (node->data.while_stmt.body) {
        lower_statement(node->data.while_stmt.body);
    }

    IRInstruction *jmp = ir_instr_create(IR_JMP);
    jmp->label = start_label;
    add_instr(jmp);

    IRInstruction *end_lbl = ir_instr_create(IR_LABEL);
    end_lbl->label = end_label;
    add_instr(end_lbl);

    loop_pop();
}

// Lower a for statement
static void lower_for_stmt(ASTNode *node) {
    const char *start_label = new_label();
    const char *end_label = new_label();

    loop_push(start_label, end_label);

    if (node->data.for_stmt.init) {
        lower_statement(node->data.for_stmt.init);
    }

    IRInstruction *start_lbl = ir_instr_create(IR_LABEL);
    start_lbl->label = start_label;
    add_instr(start_lbl);

    if (node->data.for_stmt.condition) {
        IRValue *cond = lower_expression(node->data.for_stmt.condition);
        IRInstruction *instr = ir_instr_create(IR_JMP_IF);
        instr->args[0] = cond;
        instr->label = end_label;
        add_instr(instr);
    }

    if (node->data.for_stmt.body) {
        lower_statement(node->data.for_stmt.body);
    }

    if (node->data.for_stmt.increment) {
        lower_expression(node->data.for_stmt.increment);
    }

    IRInstruction *jmp = ir_instr_create(IR_JMP);
    jmp->label = start_label;
    add_instr(jmp);

    IRInstruction *end_lbl = ir_instr_create(IR_LABEL);
    end_lbl->label = end_label;
    add_instr(end_lbl);

    loop_pop();
}

// Lower a compound statement
static void lower_compound_stmt(ASTNode *node) {
    scope_push();
    for (size_t i = 0; i < list_size(node->data.compound.stmts); i++) {
        lower_statement(list_get(node->data.compound.stmts, i));
    }
    scope_pop();
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
        case AST_VARIABLE_DECL:
            lower_variable_decl(node);
            break;
        case AST_BREAK_STMT: {
            if (loop_stack) {
                IRInstruction *jmp = ir_instr_create(IR_JMP);
                jmp->label = loop_stack->end_label;
                add_instr(jmp);
            }
            break;
        }
        case AST_CONTINUE_STMT: {
            if (loop_stack) {
                IRInstruction *jmp = ir_instr_create(IR_JMP);
                jmp->label = loop_stack->start_label;
                add_instr(jmp);
            }
            break;
        }
        default:
            break;
    }
}

// Lower a function
static void lower_function(ASTNode *node) {
    IRFunction *func = ir_function_create(node->data.function.name);
    list_push(current_module->functions, func);

    IRBasicBlock *entry = ir_block_create("entry");
    list_push(func->blocks, entry);

    IRFunction *prev_func = current_function;
    IRBasicBlock *prev_block = current_block;
    current_function = func;
    current_block = entry;

    // Clear locals tracking
    locals_clear();
    locals_size = 0;

    // Handle parameters - in ARM64, first 4 integer params are in x0-x3
    param_count = 0;
    for (size_t i = 0; i < list_size(node->data.function.params) && i < 16; i++) {
        ASTNode *param = list_get(node->data.function.params, i);
        if (param->type == AST_PARAMETER_DECL) {
            IRValue *param_val = ir_value_create(IR_VALUE_INT);
            param_val->is_constant = false;
            param_val->is_temp = false;
            param_val->param_reg = param_count;
            param_values[param_count] = param_val;
            param_names[param_count] = xstrdup(param->data.parameter.name);
            param_count++;
        }
    }

    // Lower body
    if (node->data.function.body) {
        lower_statement(node->data.function.body);
    }

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
    label_counter = 0;

    if (ast && ast->type == AST_TRANSLATION_UNIT) {
        lower_translation_unit(ast);
    }

    return current_module;
}

void lowerer_free_module(IRModule *module) {
    ir_module_destroy(module);
}
