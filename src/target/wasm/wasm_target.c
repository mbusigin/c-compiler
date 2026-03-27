/**
 * wasm_target.c - WASM target implementation
 *
 * This module implements the Target interface for WASM code generation.
 * It wraps the existing WASM backend in src/backend/wasm_codegen.c
 */

#include "wasm_target.h"
#include "../../backend/wasm_codegen.h"
#include "../../common/util.h"
#include <stdlib.h>
#include <string.h>

// WASM target private data
typedef struct {
    IRModule *current_module;
    int label_counter;
} WASMPrivateData;

// Target identification
static const char* wasm_get_name(struct Target *target) {
    (void)target;
    return "wasm";
}

static const char* wasm_get_triple(struct Target *target) {
    (void)target;
    return "wasm32-unknown-unknown";
}

// Initialization and cleanup
static bool wasm_init(struct Target *target, FILE *output, TargetOptions *options) {
    WASMPrivateData *data = xcalloc(1, sizeof(WASMPrivateData));
    target->private_data = data;
    target->output = output;
    
    if (options) {
        target->options = *options;
    }
    
    data->current_module = NULL;
    data->label_counter = 0;
    
    return true;
}

static void wasm_cleanup(struct Target *target) {
    if (target->private_data) {
        WASMPrivateData *data = (WASMPrivateData*)target->private_data;
        free(data);
        target->private_data = NULL;
    }
}

// Module-level emission
static void wasm_emit_module_prologue(struct Target *target, IRModule *module) {
    WASMPrivateData *data = (WASMPrivateData*)target->private_data;
    data->current_module = module;
    // Prologue is handled by wasm_codegen
}

static void wasm_emit_module_epilogue(struct Target *target, IRModule *module) {
    WASMPrivateData *data = (WASMPrivateData*)target->private_data;
    (void)module;
    data->current_module = NULL;
    // Epilogue is handled by wasm_codegen
}

// Function-level emission (delegated to wasm_codegen)
static void wasm_emit_function_prologue(struct Target *target, IRFunction *func) {
    (void)target;
    (void)func;
    // Handled by wasm_codegen
}

static void wasm_emit_function_epilogue(struct Target *target, IRFunction *func) {
    (void)target;
    (void)func;
    // Handled by wasm_codegen
}

// Instruction emission (delegated to wasm_codegen)
static void wasm_target_emit_instruction(struct Target *target, IRInstruction *instr) {
    (void)target;
    (void)instr;
    // Handled by wasm_codegen
}

// Data emission (delegated to wasm_codegen)
static void wasm_target_emit_global_variable(struct Target *target, IRGlobal *global) {
    (void)target;
    (void)global;
    // Handled by wasm_codegen
}

static void wasm_target_emit_string_literal(struct Target *target, const char *label, const char *value) {
    (void)target;
    (void)label;
    (void)value;
    // Handled by wasm_codegen
}

// Debug info emission (delegated to wasm_codegen)
static void wasm_target_emit_debug_info(struct Target *target, ASTNode *ast) {
    (void)target;
    (void)ast;
    // Not yet implemented for WASM
}

// Target-specific queries
static size_t wasm_get_pointer_size(struct Target *target) {
    (void)target;
    return 4;  // 32-bit pointers in wasm32
}

static size_t wasm_get_stack_alignment(struct Target *target) {
    (void)target;
    return 16;  // WASM stack alignment
}

static bool wasm_supports_feature(struct Target *target, const char *feature) {
    (void)target;
    // WASM feature support
    if (strcmp(feature, "simd") == 0) return true;
    if (strcmp(feature, "bulk-memory") == 0) return true;
    if (strcmp(feature, "mutable-globals") == 0) return true;
    return false;
}

// WASM vtable
static const TargetVTable wasm_vtable = {
    .get_name = wasm_get_name,
    .get_triple = wasm_get_triple,
    .init = wasm_init,
    .cleanup = wasm_cleanup,
    .emit_module_prologue = wasm_emit_module_prologue,
    .emit_module_epilogue = wasm_emit_module_epilogue,
    .emit_function_prologue = wasm_emit_function_prologue,
    .emit_function_epilogue = wasm_emit_function_epilogue,
    .emit_instruction = wasm_target_emit_instruction,
    .emit_global_variable = wasm_target_emit_global_variable,
    .emit_string_literal = wasm_target_emit_string_literal,
    .emit_debug_info = wasm_target_emit_debug_info,
    .get_pointer_size = wasm_get_pointer_size,
    .get_stack_alignment = wasm_get_stack_alignment,
    .supports_feature = wasm_supports_feature,
};

// Target creation function
struct Target *target_create_wasm(void) {
    struct Target *target = xcalloc(1, sizeof(struct Target));
    target->vtable = &wasm_vtable;
    target->private_data = NULL;
    target->output = NULL;
    return target;
}

// WASM target initialization (registers with target registry)
bool wasm_target_init(void) {
    target_register("wasm", target_create_wasm);
    target_register("wasm32", target_create_wasm);
    return true;
}
