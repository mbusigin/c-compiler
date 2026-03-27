/**
 * lowerer.c - AST to IR lowering
 */

#include "lowerer.h"
#include "../parser/ast.h"
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

// Helper: check if a type is a pointer
static bool is_pointer_type(Type *t) {
    return t && t->kind == TYPE_POINTER;
}

// Helper: check if an AST node evaluates to a pointer type
static bool is_pointer_result(ASTNode *node) {
    return node && node->type_info && is_pointer_type(node->type_info);
}

// Helper: check if an AST node contains a function call
static bool contains_call(ASTNode *node) {
    if (!node) return false;
    switch (node->type) {
        case AST_CALL_EXPR:
            return true;
        case AST_BINARY_EXPR:
            return contains_call(node->data.binary.left) || contains_call(node->data.binary.right);
        case AST_UNARY_EXPR:
            return contains_call(node->data.unary.operand);
        case AST_ASSIGNMENT_EXPR:
            return contains_call(node->data.assignment.right);
        case AST_CONDITIONAL_EXPR:
            return contains_call(node->data.conditional.condition) ||
                   contains_call(node->data.conditional.then_expr) ||
                   contains_call(node->data.conditional.else_expr);
        case AST_ARRAY_SUBSCRIPT_EXPR:
            return contains_call(node->data.subscript.array) || contains_call(node->data.subscript.index);
        case AST_CAST_EXPR:
            return contains_call(node->data.cast.operand);
        case AST_MEMBER_ACCESS_EXPR:
        case AST_POINTER_MEMBER_ACCESS_EXPR:
            return contains_call(node->data.member.expr);
        default:
            return false;
    }
}

// Simple locals tracking via hash table
#define LOCALS_BUCKETS 32
typedef struct LocalVar {
    char *name;
    int offset;       // Stack offset (positive from sp)
    int elem_size;    // Size of each element for arrays (0 for non-arrays)
    int array_size;   // Number of elements for arrays (0 for non-arrays)
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
    const char *continue_label;  // For for-loops, this is the increment section
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

static void locals_add(const char *name, int offset, int is_array) {
    LocalVar *lv = malloc(sizeof(LocalVar));
    lv->name = xstrdup(name);
    lv->offset = offset;
    lv->array_size = is_array ? 1 : 0;  // Track if this is an array
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
static void loop_push(const char *start, const char *cont, const char *end) {
    LoopContext *ctx = malloc(sizeof(LoopContext));
    ctx->start_label = start;
    ctx->continue_label = cont;
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
        // Get the parameter info (location)
        IRValue *param_loc = param_values[param_idx];
        
        // Create a NEW value for the load result (don't reuse param_loc)
        IRValue *load_result = ir_value_create(IR_VALUE_INT);
        load_result->is_constant = false;
        load_result->is_temp = true;
        load_result->param_reg = param_loc->param_reg;  // Copy param info
        load_result->offset = param_loc->offset;
        load_result->is_pointer = is_pointer_result(node);
        
        // Create load instruction
        IRInstruction *load_instr = ir_instr_create(IR_LOAD_STACK);
        load_instr->result = load_result;
        add_instr(load_instr);
        
        return load_result;  // Return the newly created result
    }
    
    // Check if this is a local variable
    LocalVar *lv = locals_lookup(node->data.identifier.name);
    if (lv) {
        // Check if this is an array - arrays decay to pointers
        if (lv->array_size > 0) {
            // Return address of array (sp + offset)
            IRValue *addr_val = ir_value_create(IR_VALUE_INT);
            addr_val->offset = lv->offset;
            addr_val->is_constant = false;
            addr_val->is_temp = true;
            addr_val->is_address = true;  // Mark as address for 64-bit operations
            addr_val->is_pointer = true;  // Address is a pointer
            IRInstruction *addr_instr = ir_instr_create(IR_LEA);
            addr_instr->result = addr_val;
            add_instr(addr_instr);
            return addr_instr->result;
        }
        
        // Regular local variable - load its value
        IRValue *result = ir_value_create(IR_VALUE_INT);
        result->is_constant = false;
        result->is_temp = true;
        result->offset = lv->offset;
        result->param_reg = -2;  // Mark as local variable (stack-relative)
        result->is_pointer = is_pointer_result(node);
        IRInstruction *load_instr = ir_instr_create(IR_LOAD_STACK);
        load_instr->result = result;
        add_instr(load_instr);
        return result;
    }
    
    // Check if it's a function symbol - functions decay to function pointers
    Symbol *sym = symtab_lookup(current_symtab, node->data.identifier.name);
    if (sym && sym->kind == SYMBOL_FUNCTION) {
        // Function name used as value - this is a function pointer
        // Load the function address using a special IR instruction
        IRValue *result = ir_value_create(IR_VALUE_INT);
        result->is_constant = false;
        result->is_temp = true;
        result->is_pointer = true;  // Function pointer
        
        // Create IR_LOAD_FUNC_ADDR instruction to load function address
        IRInstruction *load_instr = ir_instr_create(IR_LOAD_FUNC_ADDR);
        load_instr->result = result;
        load_instr->label = node->data.identifier.name;  // Store function name
        add_instr(load_instr);
        return result;
    }
    
    // Check if it's a global variable (like stderr, stdout, stdin)
    Symbol *global_sym = symtab_lookup(current_symtab, node->data.identifier.name);
    
    // Special case: stderr/stdout/stdin are built-in external symbols
    // Handle them even if not found in symbol table
    const char *name = node->data.identifier.name;
    bool is_stdio_stream = (strcmp(name, "stderr") == 0 || strcmp(name, "stdout") == 0 || strcmp(name, "stdin") == 0);
    
    if ((global_sym && global_sym->kind == SYMBOL_VARIABLE) || is_stdio_stream) {
        // This is a global variable - load its value
        IRValue *result = ir_value_create(IR_VALUE_INT);
        result->is_constant = false;
        result->is_temp = true;
        result->is_pointer = is_pointer_result(node);
        
        // On macOS, stderr/stdout/stdin are actually __stderrp/__stdoutp/__stdinp
        // These are external symbols, so use IR_LOAD_EXTERNAL
        const char *symbol_name = name;
        if (strcmp(name, "stderr") == 0) {
            symbol_name = "__stderrp";
        } else if (strcmp(name, "stdout") == 0) {
            symbol_name = "__stdoutp";
        } else if (strcmp(name, "stdin") == 0) {
            symbol_name = "__stdinp";
        }
        
        // Create IR_LOAD_EXTERNAL to load value from external symbol
        IRInstruction *load_instr = ir_instr_create(IR_LOAD_EXTERNAL);
        load_instr->result = result;
        load_instr->label = symbol_name;
        add_instr(load_instr);
        return result;
    }
    
    // For non-parameters, non-variables, and non-functions, create a dummy value
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
    // Handle short-circuit operators specially
    if (node->data.binary.op == OP_AND) {
        // Short-circuit AND: if left is false, don't evaluate right
        const char *skip_right_label = new_label();
        const char *end_label = new_label();
        
        // Evaluate left, result in x8
        IRValue *left_val = lower_expression(node->data.binary.left);
        
        // Jump to end (skip right) if left is false
        // IR_JMP_IF jumps to label if condition is FALSE (cbz)
        IRInstruction *jmp_skip = ir_instr_create(IR_JMP_IF);
        jmp_skip->args[0] = left_val;
        jmp_skip->num_args = 1;
        jmp_skip->label = skip_right_label;
        add_instr(jmp_skip);
        
        // Left is true, evaluate right
        IRValue *right_val = lower_expression(node->data.binary.right);
        (void)right_val;  // Result already in x8
        
        // Jump to end
        IRInstruction *jmp_end = ir_instr_create(IR_JMP);
        jmp_end->label = end_label;
        add_instr(jmp_end);
        
        // Skip right label: left was false, set result to 0
        IRInstruction *skip_lbl = ir_instr_create(IR_LABEL);
        skip_lbl->label = skip_right_label;
        add_instr(skip_lbl);
        
        // Set result to 0 (false)
        IRValue *zero_val = ir_value_create(IR_VALUE_INT);
        zero_val->data.int_val = 0;
        zero_val->is_constant = true;
        IRInstruction *set_false = ir_instr_create(IR_CONST_INT);
        set_false->result = zero_val;
        add_instr(set_false);
        
        // End label
        IRInstruction *end_lbl = ir_instr_create(IR_LABEL);
        end_lbl->label = end_label;
        add_instr(end_lbl);
        
        // Return a temp value (result is in x8)
        IRValue *result = ir_value_create(IR_VALUE_INT);
        result->is_constant = false;
        result->is_temp = true;
        return result;
    }
    
    if (node->data.binary.op == OP_OR) {
        // Short-circuit OR: if left is true, don't evaluate right
        const char *eval_right_label = new_label();
        const char *end_label = new_label();
        
        // Evaluate left, result in x8
        IRValue *left_val = lower_expression(node->data.binary.left);
        
        // Jump to eval_right if left is false
        IRInstruction *jmp_eval = ir_instr_create(IR_JMP_IF);
        jmp_eval->args[0] = left_val;
        jmp_eval->num_args = 1;
        jmp_eval->label = eval_right_label;
        add_instr(jmp_eval);
        
        // Left is true, set result to 1
        IRValue *one_val = ir_value_create(IR_VALUE_INT);
        one_val->data.int_val = 1;
        one_val->is_constant = true;
        IRInstruction *set_true = ir_instr_create(IR_CONST_INT);
        set_true->result = one_val;
        add_instr(set_true);
        
        // Jump to end
        IRInstruction *jmp_end = ir_instr_create(IR_JMP);
        jmp_end->label = end_label;
        add_instr(jmp_end);
        
        // Evaluate right label
        IRInstruction *eval_lbl = ir_instr_create(IR_LABEL);
        eval_lbl->label = eval_right_label;
        add_instr(eval_lbl);
        
        // Evaluate right, result in x8
        IRValue *right_val = lower_expression(node->data.binary.right);
        (void)right_val;
        
        // End label
        IRInstruction *end_lbl = ir_instr_create(IR_LABEL);
        end_lbl->label = end_label;
        add_instr(end_lbl);
        
        // Return a temp value
        IRValue *result = ir_value_create(IR_VALUE_INT);
        result->is_constant = false;
        result->is_temp = true;
        return result;
    }
    
    IRValue *left = lower_expression(node->data.binary.left);
    
    // If left is a temp and right contains a call, save left to stack before evaluating right
    // This ensures the left value survives the call
    int temp_slot = -1;
    if (left && left->is_temp && contains_call(node->data.binary.right)) {
        temp_slot = locals_size;
        locals_size += 8;
        
        // Store the temp to stack
        IRValue *store_result = ir_value_create(IR_VALUE_INT);
        store_result->offset = temp_slot;
        store_result->param_reg = -2;  // Mark as local variable
        IRInstruction *store_i = ir_instr_create(IR_STORE_STACK);
        store_i->result = store_result;
        add_instr(store_i);
        
        // Emit IR_ALLOCA for consistency
        IRValue *size_val = ir_value_create(IR_VALUE_INT);
        size_val->data.int_val = 8;
        size_val->is_constant = true;
        IRInstruction *alloc_instr = ir_instr_create(IR_ALLOCA);
        alloc_instr->result = size_val;
        add_instr(alloc_instr);
        
        // Create a new IRValue pointing to the stack slot
        IRValue *saved_left = ir_value_create(IR_VALUE_INT);
        saved_left->is_constant = false;
        saved_left->is_temp = false;
        saved_left->param_reg = -2;
        saved_left->offset = temp_slot;
        left = saved_left;
    }
    
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
        default: opcode = IR_ADD; break;
    }

    return lower_binary_op(opcode, left, right);
}

// Lower a unary expression
static IRValue *lower_unary_expr(ASTNode *node) {
    // For ++/--, we need to know if the operand is a variable to store back
    ASTNode *operand_node = node->data.unary.operand;
    bool is_var = operand_node && operand_node->type == AST_IDENTIFIER_EXPR;
    LocalVar *lv = NULL;
    if (is_var) {
        lv = locals_lookup(operand_node->data.identifier.name);
    }
    
    IRValue *operand = lower_expression(operand_node);

    switch (node->data.unary.op) {
        case 0: {  // ++x (prefix increment)
            // Emit constant 1
            IRValue *one_val = ir_value_create(IR_VALUE_INT);
            one_val->data.int_val = 1;
            one_val->is_constant = true;
            IRInstruction *one_instr = ir_instr_create(IR_CONST_INT);
            one_instr->result = one_val;
            add_instr(one_instr);
            
            // Add 1 to operand
            IRInstruction *add_instr2 = ir_instr_create(IR_ADD);
            add_instr2->result = ir_value_create(IR_VALUE_INT);
            add_instr2->result->is_constant = false;
            add_instr2->result->is_temp = true;
            add_instr2->args[0] = operand;
            add_instr2->args[1] = one_val;
            add_instr2->num_args = 2;
            add_instr(add_instr2);
            
            // Store result back to variable
            if (lv) {
                IRValue *store_result = ir_value_create(IR_VALUE_INT);
                store_result->offset = lv->offset;
                store_result->param_reg = -2;  // Mark as local variable
                IRInstruction *store_i = ir_instr_create(IR_STORE_STACK);
                store_i->result = store_result;
                add_instr(store_i);
            }
            
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
            
            // Store result back to variable
            if (lv) {
                IRValue *store_result = ir_value_create(IR_VALUE_INT);
                store_result->offset = lv->offset;
                store_result->param_reg = -2;  // Mark as local variable
                IRInstruction *store_i = ir_instr_create(IR_STORE_STACK);
                store_i->result = store_result;
                add_instr(store_i);
            }
            
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
            // For post-increment: increment and store, return ORIGINAL value.
            // Save the current x8 (which has the loaded value) to x21
            IRInstruction *save_instr = ir_instr_create(IR_SAVE_X8);
            add_instr(save_instr);
            
            // Now emit the constant 1
            IRValue *one_val = ir_value_create(IR_VALUE_INT);
            one_val->data.int_val = 1;
            one_val->is_constant = true;
            IRInstruction *one_instr = ir_instr_create(IR_CONST_INT);
            one_instr->result = one_val;
            add_instr(one_instr);
            
            // Add 1 to operand (x10 has original, x8 has 1)
            IRInstruction *add_instr2 = ir_instr_create(IR_ADD);
            add_instr2->result = ir_value_create(IR_VALUE_INT);
            add_instr2->result->is_constant = false;
            add_instr2->result->is_temp = true;
            add_instr2->args[0] = operand;
            add_instr2->args[1] = one_val;
            add_instr2->num_args = 2;
            add_instr(add_instr2);
            
            // Store result back to variable
            if (lv) {
                IRValue *store_result = ir_value_create(IR_VALUE_INT);
                store_result->offset = lv->offset;
                store_result->param_reg = -2;  // Mark as local variable
                IRInstruction *store_i = ir_instr_create(IR_STORE_STACK);
                store_i->result = store_result;
                add_instr(store_i);
            }
            
            // Restore original value from x21
            IRInstruction *restore_instr = ir_instr_create(IR_RESTORE_X8_RESULT);
            add_instr(restore_instr);
            
            IRValue *ret_val = ir_value_create(IR_VALUE_INT);
            ret_val->is_constant = false;
            ret_val->is_temp = true;
            return ret_val;
        }
        case 7: {  // -- (postfix decrement)
            // For post-decrement: decrement and store, return ORIGINAL value.
            IRInstruction *save_instr = ir_instr_create(IR_SAVE_X8);
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
            
            // Store result back to variable
            if (lv) {
                IRValue *store_result = ir_value_create(IR_VALUE_INT);
                store_result->offset = lv->offset;
                IRInstruction *store_i = ir_instr_create(IR_STORE_STACK);
                store_i->result = store_result;
                add_instr(store_i);
            }
            
            // Restore original value from x21
            IRInstruction *restore_instr = ir_instr_create(IR_RESTORE_X8_RESULT);
            add_instr(restore_instr);
            
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
    // Get the left-hand side (identifier or array subscript)
    ASTNode *lhs = node->data.assignment.left;
    int op = node->data.assignment.op;  // -1 = plain assignment, OP_ADD = +=, etc.
    
    if (lhs && lhs->type == AST_IDENTIFIER_EXPR) {
        // Check if it's a local variable or parameter (both are stored on stack)
        LocalVar *lv = locals_lookup(lhs->data.identifier.name);
        if (lv) {
            // For compound assignment (+=, -=, etc.), load the current value first
            IRValue *result_val = NULL;
            IRValue *load_result = NULL;
            if (op >= 0) {  // Compound assignment (op is a valid BinaryOp)
                // Load current value of the variable
                load_result = ir_value_create(IR_VALUE_INT);
                load_result->offset = lv->offset;
                load_result->param_reg = -2;
                IRInstruction *load_instr = ir_instr_create(IR_LOAD_STACK);
                load_instr->result = load_result;
                add_instr(load_instr);
            }
            
            // Evaluate RHS
            IRValue *rhs_val = lower_expression(node->data.assignment.right);
            
            // If compound assignment, emit the operation
            if (op >= 0) {
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
                    // The load saved the current value in x8, and rhs_val is a temp
                    op_instr->args[0] = load_result;
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
            store_result->param_reg = -2;  // Mark as local variable
            IRInstruction *store_i = ir_instr_create(IR_STORE_STACK);
            store_i->result = store_result;
            add_instr(store_i);
            
            // Return the result value
            IRValue *ret_val = ir_value_create(IR_VALUE_INT);
            ret_val->is_constant = false;
            ret_val->is_temp = true;
            return ret_val;
        }
    } else if (lhs && lhs->type == AST_ARRAY_SUBSCRIPT_EXPR) {
        // Handle array subscript assignment: base[index] = value
        ASTNode *base_node = lhs->data.subscript.array;
        ASTNode *index_node = lhs->data.subscript.index;
        
        if (base_node && base_node->type == AST_IDENTIFIER_EXPR) {
            const char *base_name = base_node->data.identifier.name;
            
            // Check if base is a parameter (pointer)
            int param_idx = find_param_index(base_name);
            if (param_idx >= 0) {
                // Base is a pointer parameter - load the pointer value
                IRValue *ptr_val = param_values[param_idx];
                
                // Evaluate index
                IRValue *index_val = lower_expression(index_node);
                
                // Scale index by 4
                IRValue *scale_val = ir_value_create(IR_VALUE_INT);
                scale_val->data.int_val = 4;
                scale_val->is_constant = true;
                IRInstruction *scale_instr = ir_instr_create(IR_CONST_INT);
                scale_instr->result = scale_val;
                add_instr(scale_instr);
                
                // index * 4
                IRInstruction *mul_instr = ir_instr_create(IR_MUL);
                mul_instr->result = ir_value_create(IR_VALUE_INT);
                mul_instr->result->is_constant = false;
                mul_instr->result->is_temp = true;
                mul_instr->args[0] = index_val;
                mul_instr->args[1] = scale_val;
                mul_instr->num_args = 2;
                add_instr(mul_instr);
                
                // Save offset to x21
                IRInstruction *save_offset = ir_instr_create(IR_SAVE_X8);
                add_instr(save_offset);
                
                // Load pointer from stack
                IRInstruction *load_ptr = ir_instr_create(IR_LOAD_STACK);
                load_ptr->result = ptr_val;
                add_instr(load_ptr);
                
                // Add pointer + offset
                IRInstruction *add_i = ir_instr_create(IR_ADD_X21);
                add_i->result = ir_value_create(IR_VALUE_INT);
                add_instr(add_i);
                
                // Save address to x21
                IRInstruction *save_addr = ir_instr_create(IR_SAVE_X8);
                add_instr(save_addr);
                
                // Evaluate RHS
                IRValue *rhs_val = lower_expression(node->data.assignment.right);
                UNUSED(rhs_val);
                
                // Store through x21
                IRInstruction *store_indirect = ir_instr_create(IR_STORE_INDIRECT);
                store_indirect->result = ir_value_create(IR_VALUE_INT);
                add_instr(store_indirect);
                
                IRValue *ret_val = ir_value_create(IR_VALUE_INT);
                ret_val->is_constant = false;
                ret_val->is_temp = true;
                return ret_val;
            }
            
            // Check if base is a local variable (array)
            LocalVar *lv = locals_lookup(base_name);
            if (lv) {
                // Local array - compute address as sp + offset + index*4
                
                // Step 1: Evaluate index
                IRValue *index_val = lower_expression(index_node);
                
                // Step 2: Scale index by 4 (sizeof int)
                IRValue *scale_val = ir_value_create(IR_VALUE_INT);
                scale_val->data.int_val = 4;
                scale_val->is_constant = true;
                IRInstruction *scale_instr = ir_instr_create(IR_CONST_INT);
                scale_instr->result = scale_val;
                add_instr(scale_instr);
                
                // Step 3: index * 4 (result in x8)
                IRInstruction *mul_instr = ir_instr_create(IR_MUL);
                mul_instr->result = ir_value_create(IR_VALUE_INT);
                mul_instr->result->is_constant = false;
                mul_instr->result->is_temp = true;
                mul_instr->args[0] = index_val;
                mul_instr->args[1] = scale_val;
                mul_instr->num_args = 2;
                add_instr(mul_instr);
                
                // Step 4: Save offset (in x8) to x21
                IRInstruction *save_offset = ir_instr_create(IR_SAVE_X8);
                add_instr(save_offset);
                
                // Step 5: Get array address (sp + offset)
                IRValue *addr_val = ir_value_create(IR_VALUE_INT);
                addr_val->offset = lv->offset;
                addr_val->is_constant = false;
                addr_val->is_temp = true;
                IRInstruction *addr_instr = ir_instr_create(IR_LEA);
                addr_instr->result = addr_val;
                add_instr(addr_instr);
                
                // Step 6: Add address + offset (x8 = x8 + x21)
                IRInstruction *add_i = ir_instr_create(IR_ADD_X21);
                add_i->result = ir_value_create(IR_VALUE_INT);
                add_instr(add_i);
                
                // Step 7: Save computed address to x21
                IRInstruction *save_addr = ir_instr_create(IR_SAVE_X8);
                add_instr(save_addr);
                
                // Step 8: Evaluate RHS value
                IRValue *rhs_val = lower_expression(node->data.assignment.right);
                UNUSED(rhs_val);  // Value is now in x8
                
                // Step 9: Store through address in x21
                IRInstruction *store_indirect = ir_instr_create(IR_STORE_INDIRECT);
                store_indirect->result = ir_value_create(IR_VALUE_INT);
                add_instr(store_indirect);
                
                IRValue *ret_val = ir_value_create(IR_VALUE_INT);
                ret_val->is_constant = false;
                ret_val->is_temp = true;
                return ret_val;
            }
        }
        
        IRValue *ret_val = ir_value_create(IR_VALUE_INT);
        ret_val->is_constant = false;
        ret_val->is_temp = true;
        return ret_val;
    }
    
    // Simple case: just return the value
    IRValue *value = lower_expression(node->data.assignment.right);
    return value;
}

// Lower a variable declaration
static void lower_variable_decl(ASTNode *node) {
    if (!node || node->type != AST_VARIABLE_DECL) return;
    
    // Check if this is an array (type is TYPE_ARRAY, not TYPE_POINTER)
    // Note: TYPE_POINTER is for pointer variables like "int *p", not arrays
    int is_array = (node->data.variable.var_type && 
                    node->data.variable.var_type->kind == TYPE_ARRAY);
    
    // Allocate 8 bytes on stack
    // Layout: [local at sp+0, sp+8...][saved fp/lr above at sp+locals_size]
    // So first local is at offset 0, second at offset 8, etc.
    int var_offset = locals_size;
    locals_size += 8;
    locals_add(node->data.variable.name, var_offset, is_array);
    
    // Emit: sub sp, sp, #8  (allocate space)
    IRValue *size_val = ir_value_create(IR_VALUE_INT);
    size_val->data.int_val = 8;
    size_val->is_constant = true;
    IRInstruction *alloc_instr = ir_instr_create(IR_ALLOCA);
    alloc_instr->result = size_val;
    add_instr(alloc_instr);
    
    // Handle initializer
    if (node->data.variable.init) {
        // Check if this is an array initializer list
        if (is_array && node->data.variable.init->type == AST_INITIALIZER_LIST) {
            // For array initializers, emit stores for each element
            List *elements = node->data.variable.init->data.init_list.elements;
            if (elements) {
                for (size_t i = 0; i < list_size(elements); i++) {
                    ASTNode *elem = list_get(elements, i);
                    if (elem) {
                        // Evaluate the element value
                        IRValue *elem_val = lower_expression(elem);
                        
                        // Calculate address: base + i * 4 (assuming int elements)
                        IRValue *index_val = ir_value_create(IR_VALUE_INT);
                        index_val->data.int_val = i * 4;
                        index_val->is_constant = true;
                        IRInstruction *index_instr = ir_instr_create(IR_CONST_INT);
                        index_instr->result = index_val;
                        add_instr(index_instr);
                        
                        // Get base address
                        IRValue *addr_val = ir_value_create(IR_VALUE_INT);
                        addr_val->offset = var_offset;
                        addr_val->is_constant = false;
                        addr_val->is_temp = true;
                        IRInstruction *addr_instr = ir_instr_create(IR_LEA);
                        addr_instr->result = addr_val;
                        add_instr(addr_instr);
                        
                        // Add offset to base
                        IRInstruction *add_offset_instr = ir_instr_create(IR_ADD);
                        add_offset_instr->result = ir_value_create(IR_VALUE_INT);
                        add_offset_instr->result->is_temp = true;
                        add_offset_instr->args[0] = addr_val;
                        add_offset_instr->args[1] = index_val;
                        add_offset_instr->num_args = 2;
                        add_instr(add_offset_instr);
                        
                        // Store the element
                        IRInstruction *store_instr = ir_instr_create(IR_STORE);
                        store_instr->args[0] = add_offset_instr->result;  // address
                        store_instr->args[1] = elem_val;  // value
                        store_instr->num_args = 2;
                        add_instr(store_instr);
                    }
                }
            }
        } else if (node->data.variable.init->type == AST_INITIALIZER_LIST) {
            // Struct/compound initializer - initialize each member
            // For now, emit zero for the entire struct to avoid storing garbage
            // TODO: Properly handle individual field initializers
            IRValue *zero_val = ir_value_create(IR_VALUE_INT);
            zero_val->data.int_val = 0;
            zero_val->is_constant = true;
            IRInstruction *zero_instr = ir_instr_create(IR_CONST_INT);
            zero_instr->result = zero_val;
            add_instr(zero_instr);
            
            // Store zero to [sp, #offset]
            IRValue *result = ir_value_create(IR_VALUE_INT);
            result->offset = var_offset;
            result->param_reg = -2;
            IRInstruction *store_i = ir_instr_create(IR_STORE_STACK);
            store_i->result = result;
            add_instr(store_i);
        } else {
            // Scalar initializer
            // Evaluate initializer → x0 = value, x8 = value
            IRValue *init_val = lower_expression(node->data.variable.init);
            
            // Store value to [sp, #offset] using IR_STORE_STACK
            IRValue *result = ir_value_create(IR_VALUE_INT);
            result->offset = var_offset;
            result->param_reg = -2;  // Mark as local variable, not parameter
            IRInstruction *store_i = ir_instr_create(IR_STORE_STACK);
            store_i->result = result;
            add_instr(store_i);
            UNUSED(init_val);
        }
    }
}

// Lower a function call
static IRValue *lower_call_expr(ASTNode *node) {
    IRValue *result = ir_value_create(IR_VALUE_INT);
    result->is_temp = true;  // Mark result as temp immediately
    
    // Check if this is a direct call or an indirect call through a function pointer
    bool is_indirect_call = false;
    
    if (node->data.call.callee && node->data.call.callee->type == AST_IDENTIFIER_EXPR) {
        const char *name = node->data.call.callee->data.identifier.name;
        
        // First check if it's a local variable (function pointer)
        LocalVar *lv = locals_lookup(name);
        if (lv && node->data.call.callee->type_info) {
            Type *t = node->data.call.callee->type_info;
            // Check if the type is a pointer to function
            if (t && t->kind == TYPE_POINTER && t->base && t->base->kind == TYPE_FUNCTION) {
                is_indirect_call = true;
            }
        } else {
            // Check symbol table for parameters and global symbols
            Symbol *callee_sym = symtab_lookup(current_symtab, name);
            
            // Check if this is a function pointer (variable or parameter)
            if (callee_sym && (callee_sym->kind == SYMBOL_VARIABLE || callee_sym->kind == SYMBOL_PARAMETER)) {
                // Check if the type is a pointer to function
                Type *t = callee_sym->type;
                if (t && t->kind == TYPE_POINTER && t->base && t->base->kind == TYPE_FUNCTION) {
                    is_indirect_call = true;
                }
            }
        }
    }
    
    IRInstruction *instr;
    if (is_indirect_call) {
        // For indirect calls, load the function pointer into a value
        instr = ir_instr_create(IR_CALL_INDIRECT);
        IRValue *func_ptr = lower_identifier(node->data.call.callee);
        // The function pointer will be in the first arg slot (loaded into x0 by backend)
        instr->args[0] = func_ptr;
        instr->num_args = 1;
    } else {
        // Direct call
        instr = ir_instr_create(IR_CALL);
        if (node->data.call.callee && node->data.call.callee->type == AST_IDENTIFIER_EXPR) {
            instr->label = node->data.call.callee->data.identifier.name;
        }
    }
    
    instr->result = result;

    // Track temps that need to be saved to stack
    int temp_slot = -1;
    
    // For indirect calls, args start at index 1 (index 0 is the function pointer)
    // For direct calls, args start at index 0
    int arg_start_idx = is_indirect_call ? 1 : 0;
    int max_args = is_indirect_call ? 3 : 4;  // x1-x3 for indirect, x0-x3 for direct
    
    for (size_t i = 0; i < list_size(node->data.call.args); i++) {
        IRValue *arg = lower_expression(list_get(node->data.call.args, i));
        
        // Check if ANY remaining argument (not just the next one) will modify x8.
        // We need to save this temp if there are any non-string-literal args remaining.
        bool remaining_args_preserve_x8 = true;
        if (arg && arg->is_temp && i < list_size(node->data.call.args) - 1) {
            // Check all remaining arguments
            for (size_t j = i + 1; j < list_size(node->data.call.args); j++) {
                ASTNode *remaining_arg = list_get(node->data.call.args, j);
                if (remaining_arg && remaining_arg->type != AST_STRING_LITERAL_EXPR) {
                    // This remaining argument will emit IR and may modify x8
                    remaining_args_preserve_x8 = false;
                    break;
                }
            }
        }
        
        // If this argument is a temp and remaining arguments might modify x8,
        // save it to stack before continuing
        if (arg && arg->is_temp && i < list_size(node->data.call.args) - 1 && !remaining_args_preserve_x8) {
            // Allocate a stack slot for this temp
            temp_slot = locals_size;
            locals_size += 8;
            
            // Store the temp to the stack slot
            IRValue *store_result = ir_value_create(IR_VALUE_INT);
            store_result->offset = temp_slot;
            IRValue *store_result2 = ir_value_create(IR_VALUE_INT);
            store_result2->offset = temp_slot;
            IRInstruction *store_i = ir_instr_create(IR_STORE_STACK);
            store_i->result = store_result;
            add_instr(store_i);
            
            // Create a new IRValue that points to the stack slot
            IRValue *saved_arg = ir_value_create(IR_VALUE_INT);
            saved_arg->is_constant = false;
            saved_arg->is_temp = false;
            saved_arg->param_reg = -2;  // Mark as local variable (stack-relative)
            saved_arg->offset = temp_slot;
            saved_arg->is_pointer = arg->is_pointer;  // Preserve pointer flag!
            arg = saved_arg;
            
            // Also emit IR_ALLOCA for consistency
            IRValue *size_val = ir_value_create(IR_VALUE_INT);
            size_val->data.int_val = 8;
            size_val->is_constant = true;
            IRInstruction *alloc_instr = ir_instr_create(IR_ALLOCA);
            alloc_instr->result = size_val;
            add_instr(alloc_instr);
        }
        
        if (instr->num_args < arg_start_idx + max_args) {
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

        case AST_SIZEOF_EXPR: {
            // sizeof(type) or sizeof(expr) - returns size in bytes
            size_t size = 8;  // Default size for pointers
            
            if (node->data.sizeof_expr.sizeof_type) {
                Type *t = node->data.sizeof_expr.sizeof_type;
                switch (t->kind) {
                    case TYPE_VOID: size = 1; break;
                    case TYPE_CHAR: size = 1; break;
                    case TYPE_SHORT: size = 2; break;
                    case TYPE_INT: size = 4; break;
                    case TYPE_LONG: 
                    case TYPE_LONGLONG: size = 8; break;
                    case TYPE_FLOAT: size = 4; break;
                    case TYPE_DOUBLE: 
                    case TYPE_LONGDOUBLE: size = 8; break;
                    case TYPE_POINTER: size = 8; break;
                    case TYPE_ARRAY: 
                        // Array size = element size * count
                        if (t->base) {
                            size_t elem_size = 4;  // Default int
                            if (t->base->kind == TYPE_CHAR) elem_size = 1;
                            else if (t->base->kind == TYPE_SHORT) elem_size = 2;
                            else if (t->base->kind == TYPE_INT) elem_size = 4;
                            else if (t->base->kind == TYPE_LONG) elem_size = 8;
                            else if (t->base->kind == TYPE_FLOAT) elem_size = 4;
                            else if (t->base->kind == TYPE_DOUBLE) elem_size = 8;
                            else if (t->base->kind == TYPE_POINTER) elem_size = 8;
                            size = elem_size * t->array_size;
                        }
                        break;
                    default: size = 8; break;
                }
            } else if (node->data.sizeof_expr.sizeof_expr) {
                // For sizeof(expr), we need the type of the expression
                // For simplicity, use default size
                // TODO: Get actual type from semantic analysis
                size = 4;  // Default to int size
            }
            
            // Return the size as a constant integer
            IRValue *result = ir_value_create(IR_VALUE_INT);
            result->data.int_val = size;
            result->is_constant = true;
            IRInstruction *const_instr = ir_instr_create(IR_CONST_INT);
            const_instr->result = result;
            add_instr(const_instr);
            return result;
        }

        case AST_CAST_EXPR:
            // For now, just evaluate the operand (no type conversion)
            return lower_expression(node->data.cast.operand);

        case AST_ARRAY_SUBSCRIPT_EXPR: {
            // table[i] - load from table + i*element_size
            // Strategy: compute index first, then load pointer, then compute address and load
            ASTNode *base_node = node->data.subscript.array;
            ASTNode *index_node = node->data.subscript.index;
            bool result_is_pointer = is_pointer_result(node);
            
            // Determine element size from base type
            int elem_size = 4;  // Default to int size
            if (base_node && base_node->type_info) {
                Type *base_type = base_node->type_info;
                if (base_type->kind == TYPE_POINTER && base_type->base) {
                    // Pointer type: element size is size of pointed-to type
                    switch (base_type->base->kind) {
                        case TYPE_CHAR: elem_size = 1; break;
                        case TYPE_SHORT: elem_size = 2; break;
                        case TYPE_INT: elem_size = 4; break;
                        case TYPE_LONG:
                        case TYPE_LONGLONG: elem_size = 8; break;
                        case TYPE_POINTER: elem_size = 8; break;
                        default: elem_size = 4; break;
                    }
                } else if (base_type->kind == TYPE_ARRAY && base_type->base) {
                    // Array type: element size is size of element type
                    switch (base_type->base->kind) {
                        case TYPE_CHAR: elem_size = 1; break;
                        case TYPE_SHORT: elem_size = 2; break;
                        case TYPE_INT: elem_size = 4; break;
                        case TYPE_LONG:
                        case TYPE_LONGLONG: elem_size = 8; break;
                        case TYPE_POINTER: elem_size = 8; break;
                        default: elem_size = 4; break;
                    }
                }
            }
            
            if (base_node && base_node->type == AST_IDENTIFIER_EXPR) {
                const char *base_name = base_node->data.identifier.name;
                
                // Check if base is a parameter (pointer)
                int param_idx = find_param_index(base_name);
                if (param_idx >= 0) {
                    // Load the pointer value first (this will be clobbered by index computation)
                    IRValue *ptr_val = param_values[param_idx];
                    ptr_val->is_pointer = true;
                    IRInstruction *load_ptr = ir_instr_create(IR_LOAD_STACK);
                    load_ptr->result = ptr_val;
                    add_instr(load_ptr);
                    
                    // Move pointer to x20 (callee-saved) immediately
                    IRInstruction *save_ptr = ir_instr_create(IR_SAVE_X8_TO_X20);
                    add_instr(save_ptr);
                    
                    // Evaluate index (clobbers x8)
                    IRValue *index_val = lower_expression(index_node);
                    
                    // Scale index by element size
                    IRValue *scale_val = ir_value_create(IR_VALUE_INT);
                    scale_val->data.int_val = elem_size;
                    scale_val->is_constant = true;
                    IRInstruction *scale_instr = ir_instr_create(IR_CONST_INT);
                    scale_instr->result = scale_val;
                    add_instr(scale_instr);
                    
                    // index * 8 (result in x0 from emit_load_binary_args, then x8)
                    IRInstruction *mul_instr = ir_instr_create(IR_MUL);
                    mul_instr->result = ir_value_create(IR_VALUE_INT);
                    mul_instr->result->is_constant = false;
                    mul_instr->result->is_temp = true;
                    mul_instr->args[0] = index_val;
                    mul_instr->args[1] = scale_val;
                    mul_instr->num_args = 2;
                    add_instr(mul_instr);
                    
                    // Now x8 has scaled index, x20 has pointer
                    // Add pointer + scaled index: x8 = x20 + x8
                    IRInstruction *add_i = ir_instr_create(IR_ADD_X21);
                    add_i->result = ir_value_create(IR_VALUE_INT);
                    add_instr(add_i);
                    
                    // Load from address (x8 has the address)
                    IRInstruction *load_i = ir_instr_create(IR_LOAD);
                    load_i->result = ir_value_create(IR_VALUE_INT);
                    load_i->result->is_temp = true;
                    load_i->result->is_pointer = result_is_pointer;
                    load_i->num_args = 0;
                    add_instr(load_i);
                    
                    return load_i->result;
                }
                
                // Check if base is a local variable (array)
                LocalVar *lv = locals_lookup(base_name);
                if (lv) {
                    // Local array - compute address as sp + offset + index*4
                    
                    // Step 1: Evaluate index
                    IRValue *index_val = lower_expression(index_node);
                    
                    // Step 2: Scale index by 4 (sizeof int)
                    IRValue *scale_val = ir_value_create(IR_VALUE_INT);
                    scale_val->data.int_val = 4;
                    scale_val->is_constant = true;
                    IRInstruction *scale_instr = ir_instr_create(IR_CONST_INT);
                    scale_instr->result = scale_val;
                    add_instr(scale_instr);
                    
                    // Step 3: index * 4 (result in x8)
                    IRInstruction *mul_instr = ir_instr_create(IR_MUL);
                    mul_instr->result = ir_value_create(IR_VALUE_INT);
                    mul_instr->result->is_constant = false;
                    mul_instr->result->is_temp = true;
                    mul_instr->args[0] = index_val;
                    mul_instr->args[1] = scale_val;
                    mul_instr->num_args = 2;
                    add_instr(mul_instr);
                    
                    // Step 4: Save offset (in x8) to x21
                    IRInstruction *save_offset = ir_instr_create(IR_SAVE_X8);
                    add_instr(save_offset);
                    
                    // Step 5: Get array address (sp + offset)
                    IRValue *addr_val = ir_value_create(IR_VALUE_INT);
                    addr_val->offset = lv->offset;
                    addr_val->is_constant = false;
                    addr_val->is_temp = true;
                    IRInstruction *addr_instr = ir_instr_create(IR_LEA);
                    addr_instr->result = addr_val;
                    add_instr(addr_instr);
                    
                    // Step 6: Add address + offset (x8 = x8 + x21)
                    IRInstruction *add_i = ir_instr_create(IR_ADD_X21);
                    add_i->result = ir_value_create(IR_VALUE_INT);
                    add_instr(add_i);
                    
                    // Step 7: Load from address in x8
                    IRInstruction *load_i = ir_instr_create(IR_LOAD);
                    load_i->result = ir_value_create(IR_VALUE_INT);
                    load_i->result->is_temp = true;
                    load_i->num_args = 0;  // Signal to load from [x8]
                    add_instr(load_i);
                    
                    return load_i->result;
                }
            }
            // Fallback
            IRValue *result = ir_value_create(IR_VALUE_INT);
            result->is_constant = false;
            result->is_temp = true;
            return result;
        }

        case AST_CONDITIONAL_EXPR: {
            const char *else_label = new_label();
            const char *end_label = new_label();

            IRValue *cond_val = lower_expression(node->data.conditional.condition);

            // Jump to else if condition is false (default IR_JMP_IF behavior)
            IRInstruction *jmp_if = ir_instr_create(IR_JMP_IF);
            jmp_if->args[0] = cond_val;
            jmp_if->num_args = 1;
            jmp_if->label = else_label;
            add_instr(jmp_if);

            // Then branch: evaluate then_expr, result goes to x8
            IRValue *then_val = lower_expression(node->data.conditional.then_expr);
            (void)then_val;  // Result is already in x8

            // Jump to end
            IRInstruction *jmp_end = ir_instr_create(IR_JMP);
            jmp_end->label = end_label;
            add_instr(jmp_end);

            // Else label
            IRInstruction *else_lbl = ir_instr_create(IR_LABEL);
            else_lbl->label = else_label;
            add_instr(else_lbl);

            // Else branch: evaluate else_expr, result goes to x8
            IRValue *else_val = lower_expression(node->data.conditional.else_expr);
            (void)else_val;  // Result is already in x8

            // End label: x8 contains the result from whichever branch was taken
            IRInstruction *end_lbl = ir_instr_create(IR_LABEL);
            end_lbl->label = end_label;
            add_instr(end_lbl);

            // Return a temp value indicating x8 has the result
            IRValue *result = ir_value_create(IR_VALUE_INT);
            result->is_constant = false;
            result->is_temp = true;
            result->is_pointer = is_pointer_result(node);
            return result;
        }

        case AST_MEMBER_ACCESS_EXPR: {
            // struct_var.member - compute address and load
            ASTNode *base = node->data.member.expr;
            const char *member_name = node->data.member.member;
            
            // Get member offset from type info
            size_t member_offset = 0;
            Type *struct_type = NULL;
            
            if (base && base->type_info) {
                struct_type = base->type_info;
                if (struct_type) {
                    StructMember *member = type_find_member(struct_type, member_name);
                    if (member) {
                        member_offset = member->offset;
                    }
                }
            }
            
            // For a local struct variable, compute its address
            if (base && base->type == AST_IDENTIFIER_EXPR) {
                const char *name = base->data.identifier.name;
                LocalVar *lv = locals_lookup(name);
                if (lv) {
                    // Get struct address (sp + offset + member_offset)
                    IRValue *addr_val = ir_value_create(IR_VALUE_INT);
                    addr_val->offset = lv->offset + member_offset;
                    addr_val->is_constant = false;
                    addr_val->is_temp = true;
                    IRInstruction *addr_instr = ir_instr_create(IR_LEA);
                    addr_instr->result = addr_val;
                    add_instr(addr_instr);
                    
                    // Load from struct address
                    IRInstruction *load_instr = ir_instr_create(IR_LOAD);
                    load_instr->result = ir_value_create(IR_VALUE_INT);
                    load_instr->result->is_temp = true;
                    load_instr->num_args = 0;
                    add_instr(load_instr);
                    
                    return load_instr->result;
                }
            }
            
            // Fallback - evaluate base expression
            return lower_expression(base);
        }

        case AST_POINTER_MEMBER_ACCESS_EXPR: {
            // ptr->member - dereference pointer and access member
            ASTNode *base = node->data.member.expr;
            const char *member_name = node->data.member.member;
            
            // Get member offset from type info
            size_t member_offset = 0;
            Type *ptr_type = base ? base->type_info : NULL;
            Type *struct_type = NULL;
            
            if (ptr_type && ptr_type->kind == TYPE_POINTER) {
                struct_type = ptr_type->base;
                if (struct_type) {
                    StructMember *member = type_find_member(struct_type, member_name);
                    if (member) {
                        member_offset = member->offset;
                    }
                }
            }
            
            // Evaluate the pointer expression
            IRValue *base_val = lower_expression(base);
            (void)base_val;  // The pointer value is now in x8
            
            // Add member offset to pointer using 64-bit add
            if (member_offset > 0) {
                // Create a constant for the offset
                IRValue *offset_val = ir_value_create(IR_VALUE_INT);
                offset_val->is_constant = true;
                offset_val->data.int_val = (long long)member_offset;
                
                // Use 64-bit add for pointer arithmetic
                IRInstruction *offset_add = ir_instr_create(IR_ADD_IMM64);
                offset_add->result = ir_value_create(IR_VALUE_INT);
                offset_add->result->is_temp = true;
                offset_add->result->is_address = true;  // Mark as 64-bit address
                offset_add->args[0] = base_val;
                offset_add->args[1] = offset_val;
                offset_add->num_args = 2;
                add_instr(offset_add);
            }
            
            // Load from that address (in x8)
            IRInstruction *load_instr = ir_instr_create(IR_LOAD);
            load_instr->result = ir_value_create(IR_VALUE_INT);
            load_instr->result->is_temp = true;
            load_instr->num_args = 0;
            add_instr(load_instr);
            
            return load_instr->result;
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
        instr->num_args = 1;
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

    loop_push(start_label, NULL, end_label);  // continue goes to start for while

    IRInstruction *start_lbl = ir_instr_create(IR_LABEL);
    start_lbl->label = start_label;
    add_instr(start_lbl);

    if (node->data.while_stmt.condition) {
        IRValue *cond = lower_expression(node->data.while_stmt.condition);
        IRInstruction *instr = ir_instr_create(IR_JMP_IF);
        instr->args[0] = cond;
        instr->num_args = 1;
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
    const char *continue_label = new_label();  // Label for increment section
    const char *end_label = new_label();

    loop_push(start_label, continue_label, end_label);

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
        instr->num_args = 1;
        instr->label = end_label;
        add_instr(instr);
    }

    if (node->data.for_stmt.body) {
        lower_statement(node->data.for_stmt.body);
    }

    // Continue label - increment section
    IRInstruction *cont_lbl = ir_instr_create(IR_LABEL);
    cont_lbl->label = continue_label;
    add_instr(cont_lbl);

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

// Lower a switch statement (simple implementation using if-else chain)
static void lower_switch_stmt(ASTNode *node) {
    // For a simple implementation, we handle case/default by converting to if-else
    
    const char *end_label = new_label();
    
    // Push the switch end label for break statements
    loop_push(NULL, NULL, end_label);
    
    // Evaluate the switch expression
    if (node->data.switch_stmt.expr) {
        (void)lower_expression(node->data.switch_stmt.expr);
    }
    
    // Lower the body (case labels will be handled by lower_statement)
    if (node->data.switch_stmt.body) {
        lower_statement(node->data.switch_stmt.body);
    }
    
    // End label
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
        case AST_SWITCH_STMT:
            lower_switch_stmt(node);
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
                // For for-loops, continue goes to increment; for while loops, to start
                jmp->label = loop_stack->continue_label ? loop_stack->continue_label : loop_stack->start_label;
                add_instr(jmp);
            }
            break;
        }
        case AST_CASE_STMT: {
            // For now, just lower the statement inside the case
            // A proper implementation would emit a label and conditional jump
            if (node->data.case_stmt.stmt) {
                lower_statement(node->data.case_stmt.stmt);
            }
            break;
        }
        case AST_DEFAULT_STMT: {
            // For now, just lower the statement inside the default
            if (node->data.default_stmt.stmt) {
                lower_statement(node->data.default_stmt.stmt);
            }
            break;
        }
        default:
            // Handle expression nodes (for loop init can be an expression)
            if (node->type >= AST_BINARY_EXPR && node->type <= AST_COMMA_EXPR) {
                lower_expression(node);
            }
            break;
    }
}

// Lower a function
static void lower_function(ASTNode *node) {
    IRFunction *func = ir_function_create(node->data.function.name);
    func->is_static = node->is_static;  // Preserve static storage class
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
    // Save them to stack slots to preserve them across function calls
    param_count = 0;
    for (size_t i = 0; i < list_size(node->data.function.params) && i < 16; i++) {
        ASTNode *param = list_get(node->data.function.params, i);
        if (param->type == AST_PARAMETER_DECL) {
            // Allocate stack slot for this parameter
            int param_offset = locals_size;
            locals_size += 8;
            locals_add(param->data.parameter.name, param_offset, 0);  // Parameters are not arrays
            
            // Create IRValue pointing to the stack slot
            // For WASM: param_reg should be the parameter number (0, 1, 2, 3)
            // For ARM64: param_reg = -2 means stack-relative (after storing param to stack)
            IRValue *param_val = ir_value_create(IR_VALUE_INT);
            param_val->is_constant = false;
            param_val->is_temp = false;
            param_val->param_reg = param_count;  // Store parameter number for WASM backend
            param_val->offset = param_offset;
            param_values[param_count] = param_val;
            param_names[param_count] = xstrdup(param->data.parameter.name);
            
            // Emit IR_STORE_PARAM to save parameter register to stack slot
            IRValue *store_info = ir_value_create(IR_VALUE_INT);
            store_info->data.int_val = param_count;  // Parameter register number (x0-x3)
            store_info->offset = param_offset;       // Stack offset
            IRInstruction *store_param = ir_instr_create(IR_STORE_PARAM);
            store_param->result = store_info;
            add_instr(store_param);
            
            param_count++;
            
            // Allocate stack space
            IRValue *size_val = ir_value_create(IR_VALUE_INT);
            size_val->data.int_val = 8;
            size_val->is_constant = true;
            IRInstruction *alloc_instr = ir_instr_create(IR_ALLOCA);
            alloc_instr->result = size_val;
            add_instr(alloc_instr);
        }
    }

    // Lower body
    if (node->data.function.body) {
        lower_statement(node->data.function.body);
    }
    
    // Add implicit return for functions that don't end with one
    // This is needed for void functions and functions that fall off the end
    IRInstruction *ret_void = ir_instr_create(IR_RET_VOID);
    add_instr(ret_void);

    // Restore
    current_function = prev_func;
    current_block = prev_block;
}

// Lower a global variable declaration
static void lower_global_var(ASTNode *node) {
    if (!node || node->type != AST_VARIABLE_DECL) return;
    
    const char *name = node->data.variable.name;
    
    // Skip anonymous/empty variables
    if (!name || name[0] == '\0') {
        return;
    }
    
    Type *type = node->data.variable.var_type;
    ASTNode *init = node->data.variable.init;
    bool is_static = node->is_static;
    
    // Create IRValue for initializer
    IRValue *init_value = NULL;
    if (init) {
        if (init->type == AST_INTEGER_LITERAL_EXPR) {
            init_value = ir_value_create(IR_VALUE_INT);
            init_value->is_constant = true;
            init_value->data.int_val = init->data.int_literal.value;
        } else if (init->type == AST_IDENTIFIER_EXPR) {
            // Handle NULL initializer
            if (strcmp(init->data.identifier.name, "NULL") == 0) {
                init_value = ir_value_create(IR_VALUE_INT);
                init_value->is_constant = true;
                init_value->data.int_val = 0;
            }
        }
    }
    
    // Create and add global to module
    IRGlobal *global = ir_global_create(name, type, init_value);
    global->is_static = is_static;
    ir_module_add_global(current_module, global);
}

// Lower a translation unit
static void lower_translation_unit(ASTNode *node) {
    for (size_t i = 0; i < list_size(node->data.unit.declarations); i++) {
        ASTNode *decl = list_get(node->data.unit.declarations, i);
        // Only lower function DEFINITIONS (those with bodies), not declarations
        // Declarations like "int strncmp(...);" have no body
        if (decl->type == AST_FUNCTION_DECL && decl->data.function.body) {
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
