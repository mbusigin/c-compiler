/**
 * ir.c - IR implementation
 */

#include "ir.h"
#include "../common/util.h"
#include <stdlib.h>
#include <stdio.h>

static const char *op_name(IROpcode op) {
    if (op == IR_NOP) {

        return "nop";

    } else if (op == IR_LABEL) {

        return "label";

    } else if (op == IR_JMP) {

        return "jmp";

    } else if (op == IR_JMP_IF) {

        return "jmp_if";

    } else if (op == IR_JMP_IF_TRUE) {

        return "jmp_if_true";

    } else if (op == IR_RET) {

        return "ret";

    } else if (op == IR_RET_VOID) {

        return "ret_void";

    } else if (op == IR_CALL) {

        return "call";

    } else if (op == IR_CALL_INDIRECT) {

        return "call_indirect";

    } else if (op == IR_LOAD) {

        return "load";

    } else if (op == IR_STORE) {

        return "store";

    } else if (op == IR_ALLOCA) {

        return "alloca";

    } else if (op == IR_ADD) {

        return "add";

    } else if (op == IR_ADD_F) {

        return "add_f";

    } else if (op == IR_SUB) {

        return "sub";

    } else if (op == IR_SUB_F) {

        return "sub_f";

    } else if (op == IR_MUL) {

        return "mul";

    } else if (op == IR_MUL_F) {

        return "mul_f";

    } else if (op == IR_DIV) {

        return "div";

    } else if (op == IR_DIV_F) {

        return "div_f";

    } else if (op == IR_MOD) {

        return "mod";

    } else if (op == IR_NEG) {

        return "neg";

    } else if (op == IR_NEG_F) {

        return "neg_f";

    } else if (op == IR_AND) {

        return "and";

    } else if (op == IR_OR) {

        return "or";

    } else if (op == IR_XOR) {

        return "xor";

    } else if (op == IR_SHL) {

        return "shl";

    } else if (op == IR_SHR) {

        return "shr";

    } else if (op == IR_NOT) {

        return "not";

    } else if (op == IR_CMP) {

        return "cmp";

    } else if (op == IR_CMP_F) {

        return "cmp_f";

    } else if (op == IR_CMP_LT) {

        return "cmp_lt";

    } else if (op == IR_CMP_GT) {

        return "cmp_gt";

    } else if (op == IR_CMP_LE) {

        return "cmp_le";

    } else if (op == IR_CMP_GE) {

        return "cmp_ge";

    } else if (op == IR_CMP_EQ) {

        return "cmp_eq";

    } else if (op == IR_CMP_NE) {

        return "cmp_ne";

    } else if (op == IR_BOOL_AND) {

        return "bool_and";

    } else if (op == IR_BOOL_OR) {

        return "bool_or";

    } else if (op == IR_CONST_INT) {

        return "const_int";

    } else if (op == IR_CONST_FLOAT) {

        return "const_float";

    } else if (op == IR_CONST_STRING) {

        return "const_string";

    } else if (op == IR_LOAD_STACK) {

        return "load_stack";

    } else if (op == IR_STORE_STACK) {

        return "store_stack";

    } else if (op == IR_STORE_PARAM) {

        return "store_param";

    } else if (op == IR_SAVE_X8) {

        return "save_x8";

    } else if (op == IR_RESTORE_X8_RESULT) {

        return "restore_x8_result";

    } else if (op == IR_RESTORE_X8_FROM_X22) {

        return "restore_x8_from_x22";

    } else if (op == IR_SAVE_X8_TO_X20) {

        return "save_x8_to_x20";

    } else if (op == IR_RESTORE_X8_FROM_X20) {

        return "restore_x8_from_x20";

    } else if (op == IR_LOAD_OFFSET) {

        return "load_offset";

    } else if (op == IR_STORE_OFFSET) {

        return "store_offset";

    } else if (op == IR_STORE_INDIRECT) {

        return "store_indirect";

    } else if (op == IR_LEA) {

        return "lea";

    } else if (op == IR_ADD_X21) {

        return "add_x21";

    } else if (op == IR_ADD_X22) {

        return "add_x22";

    } else if (op == IR_ADD_IMM64) {

        return "add_imm64";

    } else if (op == IR_LOAD_EXTERNAL) {

        return "load_external";

    } else if (op == IR_LOAD_FUNC_ADDR) {

        return "load_func_addr";

    } else if (op == IR_SEXT) {

        return "sext";

    } else if (op == IR_ZEXT) {

        return "zext";

    } else if (op == IR_TRUNC) {

        return "trunc";

    } else if (op == IR_SITOFP) {

        return "sitofp";

    } else if (op == IR_FPTOSI) {

        return "fptosi";

    } else if (op == IR_LOAD_GLOBAL) {

        return "load_global";

    } else if (op == IR_STORE_GLOBAL) {

        return "store_global";

    } else if (op == IR_SAVE_X8_TO_X22) {

        return "save_x8_to_x22";

    } else if (op == IR_STORE_INDIRECT_X20) {

        return "store_indirect_x20";

    } else {

        return "unknown";

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
