/**
 * arm64_target.c - ARM64 target implementation
 *
 * This module implements the Target interface for ARM64 code generation.
 * It wraps the existing ARM64 backend in src/backend/codegen.c
 */

#include "arm64_target.h"
#include "../../backend/codegen.h"
#include "../../common/util.h"
#include <stdlib.h>
#include <string.h>

// ARM64 target private data
typedef struct {
    IRModule *current_module;
    int stack_size;
    int label_counter;
} ARM64PrivateData;

// Target identification
static const char* arm64_get_name(struct Target *target) {
    (void)target;
    return "arm64";
}

static const char* arm64_get_triple(struct Target *target) {
    (void)target;
    return "aarch64-unknown-linux-gnu";
}

// Initialization and cleanup
static bool arm64_init(struct Target *target, FILE *output, TargetOptions *options) {
    ARM64PrivateData *data = xcalloc(1, sizeof(ARM64PrivateData));
    target->private_data = data;
    target->output = output;
    
    if (options) {
        target->options = *options;
    }
    
    data->current_module = NULL;
    data->stack_size = 0;
    data->label_counter = 0;
    
    return true;
}

static void arm64_cleanup(struct Target *target) {
    if (target->private_data) {
        ARM64PrivateData *data = (ARM64PrivateData*)target->private_data;
        free(data);
        target->private_data = NULL;
    }
}

// Module-level emission
static void arm64_emit_module_prologue(struct Target *target, IRModule *module) {
    ARM64PrivateData *data = (ARM64PrivateData*)target->private_data;
    data->current_module = module;
    // Prologue is handled by codegen_generate
}

static void arm64_emit_module_epilogue(struct Target *target, IRModule *module) {
    ARM64PrivateData *data = (ARM64PrivateData*)target->private_data;
    (void)module;
    data->current_module = NULL;
    // Epilogue is handled by codegen_generate
}

// Function-level emission (delegated to codegen_generate)
static void arm64_emit_function_prologue(struct Target *target, IRFunction *func) {
    (void)target;
    (void)func;
    // Handled by codegen_generate
}

static void arm64_emit_function_epilogue(struct Target *target, IRFunction *func) {
    (void)target;
    (void)func;
    // Handled by codegen_generate
}

// Instruction emission (delegated to codegen_generate)
static void arm64_emit_instruction(struct Target *target, IRInstruction *instr) {
    (void)target;
    (void)instr;
    // Handled by codegen_generate
}

// Data emission (delegated to codegen_generate)
static void arm64_emit_global_variable(struct Target *target, IRGlobal *global) {
    (void)target;
    (void)global;
    // Handled by codegen_generate
}

static void arm64_emit_string_literal(struct Target *target, const char *label, const char *value) {
    (void)target;
    (void)label;
    (void)value;
    // Handled by codegen_generate
}

// Debug info emission (delegated to codegen_generate)
static void arm64_emit_debug_info(struct Target *target, ASTNode *ast) {
    (void)target;
    (void)ast;
    // Not yet implemented
}

// Target-specific queries
static size_t arm64_get_pointer_size(struct Target *target) {
    (void)target;
    return 8;  // 64-bit pointers
}

static size_t arm64_get_stack_alignment(struct Target *target) {
    (void)target;
    return 16;  // ARM64 requires 16-byte stack alignment
}

static bool arm64_supports_feature(struct Target *target, const char *feature) {
    (void)target;
    // ARM64 feature support
    if (strcmp(feature, "neon") == 0) return true;
    if (strcmp(feature, "vfp") == 0) return true;
    if (strcmp(feature, "fp") == 0) return true;
    return false;
}

// ARM64 vtable
static const TargetVTable arm64_vtable = {
    .get_name = arm64_get_name,
    .get_triple = arm64_get_triple,
    .init = arm64_init,
    .cleanup = arm64_cleanup,
    .emit_module_prologue = arm64_emit_module_prologue,
    .emit_module_epilogue = arm64_emit_module_epilogue,
    .emit_function_prologue = arm64_emit_function_prologue,
    .emit_function_epilogue = arm64_emit_function_epilogue,
    .emit_instruction = arm64_emit_instruction,
    .emit_global_variable = arm64_emit_global_variable,
    .emit_string_literal = arm64_emit_string_literal,
    .emit_debug_info = arm64_emit_debug_info,
    .get_pointer_size = arm64_get_pointer_size,
    .get_stack_alignment = arm64_get_stack_alignment,
    .supports_feature = arm64_supports_feature,
};

// Target creation function
struct Target *target_create_arm64(void) {
    struct Target *target = xcalloc(1, sizeof(struct Target));
    target->vtable = &arm64_vtable;
    target->private_data = NULL;
    target->output = NULL;
    return target;
}

// ARM64 target initialization (registers with target registry)
bool arm64_target_init(void) {
    target_register("arm64", target_create_arm64);
    target_register("aarch64", target_create_arm64);
    return true;
}
