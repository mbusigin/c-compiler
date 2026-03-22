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
    IR_CMP, IR_CMP_F, IR_SEXT, IR_ZEXT, IR_TRUNC, IR_SITOFP, IR_FPTOSI,
    IR_CONST_INT, IR_CONST_FLOAT, IR_CONST_STRING,
    IR_LOAD_GLOBAL, IR_STORE_GLOBAL
} IROpcode;

typedef enum { IR_VALUE_INT, IR_VALUE_FLOAT, IR_VALUE_PTR, IR_VALUE_STRING } IRValueKind;

typedef struct IRValue {
    IRValueKind kind;
    union { long long int_val; double float_val; } data;
    int string_index;  // For string literals
    bool is_constant;  // True if this is a compile-time constant
    bool is_temp;     // True if this is a temporary (result of previous instruction)
    int param_reg;     // For parameters: which register (x0-x3) it's in
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
