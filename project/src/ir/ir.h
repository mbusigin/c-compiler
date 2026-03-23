/**
 * ir.h - Intermediate Representation
 */

#ifndef IR_H
#define IR_H

#include "../common/list.h"

typedef enum {
    IR_NOP, IR_LABEL, IR_JMP, IR_JMP_IF, IR_RET, IR_RET_VOID, IR_CALL,
    IR_LOAD, IR_STORE, IR_ALLOCA, IR_ADD, IR_ADD_F, IR_SUB, IR_SUB_F,
    IR_MUL, IR_MUL_F, IR_DIV, IR_DIV_F, IR_MOD, IR_NEG, IR_NEG_F,
    IR_AND, IR_OR, IR_XOR, IR_SHL, IR_SHR, IR_NOT,
    IR_CMP, IR_CMP_F, IR_CMP_LT, IR_CMP_GT, IR_CMP_LE, IR_CMP_GE, IR_CMP_EQ, IR_CMP_NE,
    IR_BOOL_AND, IR_BOOL_OR,  // Logical AND/OR with short-circuit support
    IR_SEXT, IR_ZEXT, IR_TRUNC, IR_SITOFP, IR_FPTOSI,
    IR_CONST_INT, IR_CONST_FLOAT, IR_CONST_STRING,
    IR_LOAD_GLOBAL, IR_STORE_GLOBAL,
    IR_LOAD_STACK, IR_STORE_STACK,
    IR_STORE_PARAM,  // Store parameter register to stack slot
    IR_SAVE_X8,  // Save x8 to x21 (for post-increment original value)
    IR_RESTORE_X8_RESULT,  // Restore result from x21 after post-increment
    IR_LOAD_OFFSET,   // Load from [base_ptr + offset*4]
    IR_STORE_OFFSET,  // Store to [base_ptr + offset*4]
    IR_STORE_INDIRECT, // Store w8 to [x21] (address in x21, value in x8)
    IR_LEA,           // Load effective address: x8 = sp + offset
    IR_ADD_X21        // x8 = x21 + x8 (add saved address to offset)
} IROpcode;

typedef enum { IR_VALUE_INT, IR_VALUE_FLOAT, IR_VALUE_PTR, IR_VALUE_STRING } IRValueKind;

typedef struct IRValue {
    IRValueKind kind;
    union { long long int_val; double float_val; } data;
    int string_index;  // For string literals
    bool is_constant;  // True if this is a compile-time constant
    bool is_temp;     // True if this is a temporary (result of previous instruction)
    bool is_address;  // True if this value is an address (from LEA)
    int param_reg;     // For parameters: which register (x0-x3) it's in; -2 = local var
    int offset;        // For local variables: stack offset
} IRValue;

typedef struct IRInstruction {
    IROpcode opcode;
    IRValue *result;
    IRValue *args[4];
    int num_args;
    const char *label;
} IRInstruction;

typedef struct IRBasicBlock {
    char *name;
    List *instructions;
    int num_preds;
} IRBasicBlock;

typedef struct IRFunction {
    char *name;
    List *params;
    List *blocks;
} IRFunction;

typedef struct IRModule {
    List *functions;
    List *strings;
} IRModule;

IRModule *ir_module_create(void);
void ir_module_destroy(IRModule *module);
IRFunction *ir_function_create(const char *name);
IRBasicBlock *ir_block_create(const char *name);
void ir_module_print(const IRModule *module);

#endif // IR_H
