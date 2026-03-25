/**
 * wasm_emit.h - WAT text emission utilities
 */

#ifndef WASM_EMIT_H
#define WASM_EMIT_H

#include "wasm_codegen.h"
#include <stdarg.h>

// Emit formatted text to output
void wasm_emit(WasmContext *ctx, const char *fmt, ...);

// Emit with indentation
void wasm_emit_indented(WasmContext *ctx, int indent, const char *fmt, ...);

// Emit a label
void wasm_emit_label(WasmContext *ctx, const char *label);

// Emit instruction (with indentation)
void wasm_emit_instr(WasmContext *ctx, const char *fmt, ...);

// Generate unique label names
const char *wasm_generate_label(WasmContext *ctx, const char *prefix);

// Get current indentation level
int wasm_get_indent(WasmContext *ctx);

// Set indentation level
void wasm_set_indent(WasmContext *ctx, int indent);

// Emit comment
void wasm_emit_comment(WasmContext *ctx, const char *fmt, ...);

// Emit string literal in data section format
void wasm_emit_string_literal(WasmContext *ctx, const char *str, int index);

// Escape string for WAT output
char *wasm_escape_string(const char *str);

#endif // WASM_EMIT_H
