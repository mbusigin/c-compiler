/**
 * ir.c - IR implementation
 */

#include "ir.h"
#include "../common/util.h"
#include <stdlib.h>
#include <stdio.h>

static const char *op_name(IROpcode op) {
    switch (op) {
        case IR_NOP: return "nop";
        case IR_LABEL: return "label";
        case IR_JMP: return "jmp";
        case IR_JMP_IF: return "jmp_if";
        case IR_RET: return "ret";
        case IR_RET_VOID: return "ret_void";
        case IR_CALL: return "call";
        case IR_LOAD: return "load";
        case IR_STORE: return "store";
        case IR_ALLOCA: return "alloca";
        case IR_ADD: return "add";
        case IR_SUB: return "sub";
        case IR_MUL: return "mul";
        case IR_DIV: return "div";
        case IR_MOD: return "mod";
        case IR_AND: return "and";
        case IR_OR: return "or";
        case IR_XOR: return "xor";
        case IR_SHL: return "shl";
        case IR_SHR: return "shr";
        case IR_NOT: return "not";
        case IR_NEG: return "neg";
        case IR_CMP: return "cmp";
        case IR_CMP_LT: return "cmp_lt";
        case IR_CMP_GT: return "cmp_gt";
        case IR_CMP_LE: return "cmp_le";
        case IR_CMP_GE: return "cmp_ge";
        case IR_CMP_EQ: return "cmp_eq";
        case IR_CMP_NE: return "cmp_ne";
        case IR_BOOL_AND: return "bool_and";
        case IR_BOOL_OR: return "bool_or";
        case IR_CONST_INT: return "const_int";
        case IR_LOAD_STACK: return "load_stack";
        case IR_STORE_STACK: return "store_stack";
        case IR_STORE_PARAM: return "store_param";
        case IR_SAVE_X8: return "save_x8";
        case IR_RESTORE_X8_RESULT: return "restore_x8_result";
        case IR_LOAD_OFFSET: return "load_offset";
        case IR_STORE_OFFSET: return "store_offset";
        case IR_STORE_INDIRECT: return "store_indirect";
        case IR_LEA: return "lea";
        case IR_ADD_X21: return "add_x21";
        case IR_ADD_IMM64: return "add_imm64";
        default: return "unknown";
    }
}

IRModule *ir_module_create(void) {
    IRModule *m = calloc(1, sizeof(IRModule));
    m->functions = list_create();
    m->strings = list_create();
    return m;
}

void ir_module_destroy(IRModule *m) {
    if (!m) return;
    list_destroy(m->functions);
    list_destroy(m->strings);
    free(m);
}

IRFunction *ir_function_create(const char *name) {
    IRFunction *f = calloc(1, sizeof(IRFunction));
    f->name = xstrdup(name);
    f->params = list_create();
    f->blocks = list_create();
    return f;
}

IRBasicBlock *ir_block_create(const char *name) {
    IRBasicBlock *b = calloc(1, sizeof(IRBasicBlock));
    b->name = xstrdup(name);
    b->instructions = list_create();
    return b;
}

void ir_module_print(const IRModule *m) {
    printf("; Module\n");
    for (size_t i = 0; i < list_size(m->functions); i++) {
        IRFunction *f = list_get(m->functions, i);
        printf("\ndefine %s @%s() {\n", f->name, f->name);
        for (size_t j = 0; j < list_size(f->blocks); j++) {
            IRBasicBlock *b = list_get(f->blocks, j);
            printf("%s:\n", b->name);
            for (size_t k = 0; k < list_size(b->instructions); k++) {
                IRInstruction *inst = list_get(b->instructions, k);
                printf("  %s\n", op_name(inst->opcode));
            }
        }
        printf("}\n");
    }
}
