/**
 * wasm_codegen.h - WebAssembly code generation interface
 */

#ifndef WASM_CODEGEN_H
#define WASM_CODEGEN_H

#include "../ir/ir.h"
#include <stdio.h>
#include <stdbool.h>

// WASM code generation context
typedef struct WasmContext {
    FILE *out;
    int func_index;
    int string_index;
    int local_index;
    int label_count;
    int memory_pages;
    bool has_printf;
    bool has_putchar;
} WasmContext;

// Main entry point - generate WAT from IR module
void wasm_codegen_generate(IRModule *module, FILE *out);
void wasm_codegen_generate_to_file(IRModule *module, const char *filename);

// Context management
WasmContext *wasm_context_create(FILE *out);
void wasm_context_destroy(WasmContext *ctx);

// Module-level emission
void wasm_emit_module_header(WasmContext *ctx);
void wasm_emit_module_footer(WasmContext *ctx);
void wasm_emit_memory(WasmContext *ctx);
void wasm_emit_imports(WasmContext *ctx);
void wasm_emit_exports(WasmContext *ctx, IRModule *module);
void wasm_emit_data_section(WasmContext *ctx, IRModule *module);

// Function-level emission
void wasm_emit_function(WasmContext *ctx, IRFunction *func);
void wasm_emit_function_header(WasmContext *ctx, IRFunction *func);
void wasm_emit_function_footer(WasmContext *ctx);
void wasm_emit_locals(WasmContext *ctx, IRFunction *func);
void wasm_emit_function_body(WasmContext *ctx, IRFunction *func);

// Instruction emission
void wasm_emit_instruction(WasmContext *ctx, IRInstruction *instr);

// Value emission
void wasm_emit_value(WasmContext *ctx, IRValue *val);
void wasm_emit_const_int(WasmContext *ctx, long long value);
void wasm_emit_const_float(WasmContext *ctx, double value);

// Type helpers
const char *wasm_type_for_ir_value(IRValue *val);

#endif // WASM_CODEGEN_H
