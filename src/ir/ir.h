/**
 * ir.h - Intermediate Representation
 */

#ifndef IR_H
#define IR_H

#include "../common/list.h"
#include "../parser/ast.h"  // For Type

typedef enum {
    IR_NOP, IR_LABEL, IR_JMP, IR_JMP_IF, IR_RET, IR_RET_VOID, IR_CALL,
    IR_CALL_INDIRECT,
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
    IR_SAVE_X8,  // Save x8 to x22 (for post-increment original value)
    IR_RESTORE_X8_RESULT,  // Restore result from x22 after post-increment
    IR_SAVE_X8_TO_X20,  // Save x8 to x20 (preserve pointer across reload)
    IR_RESTORE_X8_FROM_X20,  // Restore x8 from x20
    IR_LOAD_OFFSET,   // Load from [base_ptr + offset*4]
    IR_STORE_OFFSET,  // Store to [base_ptr + offset*4]
    IR_STORE_INDIRECT, // Store x8 to [x20] (address in x20, value in x8)
    IR_STORE_INDIRECT_X22, // Store x8 to [x22] (address in x22, value in x8)
    IR_LOAD_STRING,    // Load string address into x8
    IR_SAVE_X8_TO_X22, // Save x8 to x22 (callee-saved, for struct member access)
    IR_LEA,           // Load effective address: x8 = sp + offset
    IR_ADD_X21,       // x8 = x22 + x8 (add saved address to offset)
    IR_ADD_IMM64,     // x8 = x8 + imm (64-bit add for pointer arithmetic)
    IR_LOAD_EXTERNAL,  // Load from external symbol: x8 = &symbol_name
    IR_LOAD_FUNC_ADDR  // Load function address: x8 = &func_name (for function pointers)
} IROpcode;

typedef enum { IR_VALUE_INT, IR_VALUE_FLOAT, IR_VALUE_PTR, IR_VALUE_STRING } IRValueKind;

typedef struct IRValue {
    IRValueKind kind;
    union { long long int_val; double float_val; } data;
    int string_index;  // For string literals
    bool is_constant;  // True if this is a compile-time constant
    bool is_temp;     // True if this is a temporary (result of previous instruction)
    bool is_address;  // True if this value is an address (from LEA)
    bool is_pointer;  // True if this value is a pointer (needs 8-byte load/store)
    bool is_64bit;    // True if this value is 64-bit (needs 8-byte load/store)
    bool is_byte;     // True if this value is a byte (char) - needs ldrb/strb
    bool emitted;     // True if this value has been emitted to WASM stack
    int param_reg;     // For parameters: which register (x0-x3) it's in; -2 = local var
    int offset;        // For local variables: stack offset
    int elem_size;     // Element size for array accesses (1, 2, 4, or 8 bytes)
} IRValue;

typedef struct IRInstruction {
    IROpcode opcode;
    IRValue *result;
    IRValue *args[16];  // Support up to 16 arguments (ARM64 ABI: 8 regs + stack args)
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
    bool is_static;  // True if function has static storage class
} IRFunction;

typedef struct IRGlobal {
    char *name;
    Type *type;
    IRValue *initializer;  // Initial value (can be NULL for zero-init)
    bool is_external;      // True if declared extern
    bool is_static;         // True if static (internal linkage)
} IRGlobal;

typedef struct IRModule {
    List *functions;
    List *strings;
    List *globals;         // Global variables
} IRModule;

IRModule *ir_module_create(void);
void ir_module_destroy(IRModule *module);
IRFunction *ir_function_create(const char *name);
IRBasicBlock *ir_block_create(const char *name);
IRGlobal *ir_global_create(const char *name, Type *type, IRValue *initializer);
void ir_module_add_global(IRModule *module, IRGlobal *global);
void ir_module_print(const IRModule *module);

#endif // IR_H
