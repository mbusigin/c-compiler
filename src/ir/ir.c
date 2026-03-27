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
        case IR_CALL_INDIRECT: return "call_indirect";
        case IR_LOAD: return "load";
        case IR_STORE: return "store";
        case IR_ALLOCA: return "alloca";
        case IR_ADD: return "add";
        case IR_ADD_F: return "add_f";
        case IR_SUB: return "sub";
        case IR_SUB_F: return "sub_f";
        case IR_MUL: return "mul";
        case IR_MUL_F: return "mul_f";
        case IR_DIV: return "div";
        case IR_DIV_F: return "div_f";
        case IR_MOD: return "mod";
        case IR_NEG: return "neg";
        case IR_NEG_F: return "neg_f";
        case IR_AND: return "and";
        case IR_OR: return "or";
        case IR_XOR: return "xor";
        case IR_SHL: return "shl";
        case IR_SHR: return "shr";
        case IR_NOT: return "not";
        case IR_CMP: return "cmp";
        case IR_CMP_F: return "cmp_f";
        case IR_CMP_LT: return "cmp_lt";
        case IR_CMP_GT: return "cmp_gt";
        case IR_CMP_LE: return "cmp_le";
        case IR_CMP_GE: return "cmp_ge";
        case IR_CMP_EQ: return "cmp_eq";
        case IR_CMP_NE: return "cmp_ne";
        case IR_BOOL_AND: return "bool_and";
        case IR_BOOL_OR: return "bool_or";
        case IR_CONST_INT: return "const_int";
        case IR_CONST_FLOAT: return "const_float";
        case IR_CONST_STRING: return "const_string";
        case IR_LOAD_STACK: return "load_stack";
        case IR_STORE_STACK: return "store_stack";
        case IR_STORE_PARAM: return "store_param";
        case IR_SAVE_X8: return "save_x8";
        case IR_RESTORE_X8_RESULT: return "restore_x8_result";
        case IR_SAVE_X8_TO_X20: return "save_x8_to_x20";
        case IR_RESTORE_X8_FROM_X20: return "restore_x8_from_x20";
        case IR_LOAD_OFFSET: return "load_offset";
        case IR_STORE_OFFSET: return "store_offset";
        case IR_STORE_INDIRECT: return "store_indirect";
        case IR_LEA: return "lea";
        case IR_ADD_X21: return "add_x21";
        case IR_ADD_IMM64: return "add_imm64";
        case IR_LOAD_EXTERNAL: return "load_external";
        case IR_LOAD_FUNC_ADDR: return "load_func_addr";
        case IR_SEXT: return "sext";
        case IR_ZEXT: return "zext";
        case IR_TRUNC: return "trunc";
        case IR_SITOFP: return "sitofp";
        case IR_FPTOSI: return "fptosi";
        case IR_LOAD_GLOBAL: return "load_global";
        case IR_STORE_GLOBAL: return "store_global";
        case IR_SAVE_X8_TO_X22: return "save_x8_to_x22";
        case IR_STORE_INDIRECT_X22: return "store_indirect_x22";
        default: return "unknown";
    }
}

IRModule *ir_module_create(void) {
    IRModule *m = xcalloc(1, sizeof(IRModule));
    m->functions = list_create();
    m->strings = list_create();
    m->globals = list_create();
    return m;
}

void ir_module_destroy(IRModule *m) {
    if (!m) return;
    list_destroy(m->functions);
    list_destroy(m->strings);
    list_destroy(m->globals);
    free(m);
}

IRFunction *ir_function_create(const char *name) {
    IRFunction *f = xcalloc(1, sizeof(IRFunction));
    f->name = xstrdup(name);
    f->params = list_create();
    f->blocks = list_create();
    f->is_static = false;  // Default to false
    return f;
}

IRBasicBlock *ir_block_create(const char *name) {
    IRBasicBlock *b = xcalloc(1, sizeof(IRBasicBlock));
    b->name = xstrdup(name);
    b->instructions = list_create();
    return b;
}

IRGlobal *ir_global_create(const char *name, Type *type, IRValue *initializer) {
    IRGlobal *g = xcalloc(1, sizeof(IRGlobal));
    g->name = xstrdup(name);
    g->type = type;
    g->initializer = initializer;
    g->is_external = false;
    return g;
}

void ir_module_add_global(IRModule *module, IRGlobal *global) {
    list_push(module->globals, global);
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
