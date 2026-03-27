/**
 * target.h - Abstract target interface for code generation
 * 
 * This interface provides a target-independent API for code generation.
 * Each backend (ARM64, WASM, etc.) implements this interface.
 */

#ifndef TARGET_H
#define TARGET_H

#include "../ir/ir.h"
#include "../parser/ast.h"
#include <stdio.h>
#include <stdbool.h>

// Forward declarations
typedef struct Target Target;
typedef struct TargetOptions TargetOptions;

// Target options
struct TargetOptions {
    const char *output_file;
    bool emit_debug_info;
    bool optimize;
    int optimization_level;
};

// Function pointer typedefs for target vtable
typedef const char* (*TargetGetNameFunc)(Target *target);
typedef const char* (*TargetGetTripleFunc)(Target *target);
typedef bool (*TargetInitFunc)(Target *target, FILE *output, TargetOptions *options);
typedef void (*TargetCleanupFunc)(Target *target);
typedef void (*TargetModuleFunc)(Target *target, IRModule *module);
typedef void (*TargetFunctionFunc)(Target *target, IRFunction *func);
typedef void (*TargetInstructionFunc)(Target *target, IRInstruction *instr);
typedef void (*TargetGlobalFunc)(Target *target, IRGlobal *global);
typedef void (*TargetStringFunc)(Target *target, const char *label, const char *value);
typedef void (*TargetDebugFunc)(Target *target, ASTNode *ast);
typedef size_t (*TargetSizeFunc)(Target *target);
typedef bool (*TargetFeatureFunc)(Target *target, const char *feature);

// Target interface - vtable structure
typedef struct {
    // Target identification
    TargetGetNameFunc get_name;
    TargetGetTripleFunc get_triple;
    
    // Initialization and cleanup
    TargetInitFunc init;
    TargetCleanupFunc cleanup;
    
    // Module-level emission
    TargetModuleFunc emit_module_prologue;
    TargetModuleFunc emit_module_epilogue;
    
    // Function-level emission
    TargetFunctionFunc emit_function_prologue;
    TargetFunctionFunc emit_function_epilogue;
    
    // Instruction emission
    TargetInstructionFunc emit_instruction;
    
    // Data emission
    TargetGlobalFunc emit_global_variable;
    TargetStringFunc emit_string_literal;
    
    // Debug info emission
    TargetDebugFunc emit_debug_info;
    
    // Target-specific queries
    TargetSizeFunc get_pointer_size;
    TargetSizeFunc get_stack_alignment;
    TargetFeatureFunc supports_feature;
    
} TargetVTable;

// Target structure
struct Target {
    const TargetVTable *vtable;
    void *private_data;  // Target-specific private data
    FILE *output;
    TargetOptions options;
};

// Target creation functions (implemented by each backend)
Target *target_create_arm64(void);
Target *target_create_wasm(void);

// Target interface functions (call through vtable)
static inline const char* target_get_name(struct Target *target) {
    return target->vtable->get_name(target);
}

static inline const char* target_get_triple(struct Target *target) {
    return target->vtable->get_triple(target);
}

static inline bool target_init(struct Target *target, FILE *output, TargetOptions *options) {
    return target->vtable->init(target, output, options);
}

static inline void target_cleanup(struct Target *target) {
    if (target->vtable->cleanup) {
        target->vtable->cleanup(target);
    }
}

static inline void target_emit_module_prologue(struct Target *target, IRModule *module) {
    if (target->vtable->emit_module_prologue) {
        target->vtable->emit_module_prologue(target, module);
    }
}

static inline void target_emit_module_epilogue(struct Target *target, IRModule *module) {
    if (target->vtable->emit_module_epilogue) {
        target->vtable->emit_module_epilogue(target, module);
    }
}

static inline void target_emit_function_prologue(struct Target *target, IRFunction *func) {
    if (target->vtable->emit_function_prologue) {
        target->vtable->emit_function_prologue(target, func);
    }
}

static inline void target_emit_function_epilogue(struct Target *target, IRFunction *func) {
    if (target->vtable->emit_function_epilogue) {
        target->vtable->emit_function_epilogue(target, func);
    }
}

static inline void target_emit_instruction(struct Target *target, IRInstruction *instr) {
    if (target->vtable->emit_instruction) {
        target->vtable->emit_instruction(target, instr);
    }
}

static inline void target_emit_global_variable(struct Target *target, IRGlobal *global) {
    if (target->vtable->emit_global_variable) {
        target->vtable->emit_global_variable(target, global);
    }
}

static inline void target_emit_string_literal(struct Target *target, const char *label, const char *value) {
    if (target->vtable->emit_string_literal) {
        target->vtable->emit_string_literal(target, label, value);
    }
}

static inline void target_emit_debug_info(struct Target *target, ASTNode *ast) {
    if (target->vtable->emit_debug_info) {
        target->vtable->emit_debug_info(target, ast);
    }
}

static inline size_t target_get_pointer_size(struct Target *target) {
    return target->vtable->get_pointer_size(target);
}

static inline size_t target_get_stack_alignment(struct Target *target) {
    return target->vtable->get_stack_alignment(target);
}

static inline bool target_supports_feature(struct Target *target, const char *feature) {
    return target->vtable->supports_feature(target, feature);
}

// Target destruction
void target_destroy(Target *target);

// Target registry
typedef Target* (*TargetCreator)(void);
void target_register(const char *name, TargetCreator creator);
Target *target_create_by_name(const char *name);
const char **target_get_available_names(int *count);

#endif // TARGET_H
