/**
 * wasm_emit.c - WAT text emission utilities
 */

#include "wasm_emit.h"
#include "../common/util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int current_indent = 0;

// WASM stack pointer for local arrays - starts at 65536 (64KB)
int wasm_stack_ptr = 65536;

void wasm_emit(WasmContext *ctx, const char *fmt, ...) {
    if (!ctx || !ctx->out || !fmt) return;
    
    va_list args;
    va_start(args, fmt);
    vfprintf(ctx->out, fmt, args);
    va_end(args);
}

void wasm_emit_indented(WasmContext *ctx, int indent, const char *fmt, ...) {
    if (!ctx || !ctx->out || !fmt) return;
    
    // Print indentation
    for (int i = 0; i < indent; i++) {
        fprintf(ctx->out, "  ");
    }
    
    va_list args;
    va_start(args, fmt);
    vfprintf(ctx->out, fmt, args);
    va_end(args);
}

void wasm_emit_label(WasmContext *ctx, const char *label) {
    if (!ctx || !ctx->out || !label) return;
    fprintf(ctx->out, "%s:\n", label);
}

void wasm_emit_instr(WasmContext *ctx, const char *fmt, ...) {
    if (!ctx || !ctx->out || !fmt) return;
    
    // Print indentation (use current_indent + 1 for instructions inside blocks)
    int indent = current_indent > 0 ? current_indent : 2;
    for (int i = 0; i < indent; i++) {
        fprintf(ctx->out, "  ");
    }
    
    va_list args;
    va_start(args, fmt);
    vfprintf(ctx->out, fmt, args);
    va_end(args);
    fprintf(ctx->out, "\n");
}

const char *wasm_generate_label(WasmContext *ctx, const char *prefix) {
    static char label_buf[64];
    if (!ctx || !prefix) return ".Lunknown";
    
    snprintf(label_buf, sizeof(label_buf), "$%s_%d", prefix, ctx->label_count++);
    return label_buf;
}

int wasm_get_indent(WasmContext *ctx) {
    (void)ctx;
    return current_indent;
}

void wasm_set_indent(WasmContext *ctx, int indent) {
    (void)ctx;
    current_indent = indent;
}

void wasm_emit_comment(WasmContext *ctx, const char *fmt, ...) {
    if (!ctx || !ctx->out || !fmt) return;
    
    // Print indentation
    for (int i = 0; i < current_indent; i++) {
        fprintf(ctx->out, "  ");
    }
    
    fprintf(ctx->out, ";; ");
    
    va_list args;
    va_start(args, fmt);
    vfprintf(ctx->out, fmt, args);
    va_end(args);
    fprintf(ctx->out, "\n");
}

void wasm_emit_string_literal(WasmContext *ctx, const char *str, int index) {
    if (!ctx || !ctx->out || !str) return;
    
    // Escape the string for WAT
    char *escaped = wasm_escape_string(str);
    wasm_emit_indented(ctx, 1, "(data (i32.const %d) \"%s\")\n", index * 256, escaped);
    free(escaped);
}

char *wasm_escape_string(const char *str) {
    if (!str) return xstrdup("");
    
    // Allocate buffer for escaped string (worst case: 2x + quotes + null)
    size_t len = strlen(str);
    char *escaped = xmalloc(len * 2 + 3);
    char *p = escaped;
    
    *p++ = '\\';
    *p++ = '"';
    
    while (*str) {
        switch (*str) {
            case '"':
                *p++ = '\\';
                *p++ = '"';
                break;
            case '\\':
                *p++ = '\\';
                *p++ = '\\';
                break;
            case '\n':
                *p++ = '\\';
                *p++ = 'n';
                break;
            case '\r':
                *p++ = '\\';
                *p++ = 'r';
                break;
            case '\t':
                *p++ = '\\';
                *p++ = 't';
                break;
            default:
                *p++ = *str;
                break;
        }
        str++;
    }
    
    *p++ = '\\';
    *p++ = '"';
    *p++ = '\0';
    
    return escaped;
}
