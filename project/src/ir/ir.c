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
        case IR_CMP: return "cmp";
        case IR_CONST_INT: return "const_int";
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
