/**
 * preproc.c - Simple C preprocessor
 */

#include "preproc.h"
#include "../common/util.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Simple preprocessing:
// - Removes #include lines
// - Keeps other lines
// - Adds extern declarations for common functions



static bool starts_with(const char *s, const char *prefix) {
    while (*prefix) {
        if (*s != *prefix) return false;
        s++;
        prefix++;
    }
    return true;
}


char *preprocess(const char *source) {
    if (!source) return xstrdup("");
    
    size_t len = strlen(source);
    char *result = xmalloc(len + 100);  // Extra space for extern decls
    size_t result_pos = 0;
    const char *p = source;
    bool has_include = false;
    
    // First pass: check if there's a #include
    while (*p && (size_t)(p - source) < len) {
        // Skip leading whitespace to find directive
        while (*p == ' ' || *p == '\t' || *p == '\r') {
            p++;
        }
        
        if (*p == '#') {
            if (starts_with(p, "#include")) {
                has_include = true;
            }
        }
        
        // Skip to end of line
        while (*p != '\n' && *p != '\0') {
            p++;
        }
        if (*p == '\n') p++;
    }
    
    // If we found #include, add extern declarations
    if (has_include) {
        const char *extern_decls = "extern int printf();\n";
        strcpy(result, extern_decls);
        result_pos = strlen(extern_decls);
    }
    
    // Second pass: copy source, skipping #include lines
    p = source;
    while (*p && (size_t)(p - source) < len) {
        if (*p == '\n') {
            // Copy newline
            result[result_pos++] = *p;
            p++;
        } else if (*p == ' ' || *p == '\t' || *p == '\r') {
            // Copy whitespace
            result[result_pos++] = *p;
            p++;
        } else if (*p == '#') {
            // Skip preprocessor lines
            if (starts_with(p, "#include") || starts_with(p, "#define")) {
                while (*p != '\n' && *p != '\0') {
                    p++;
                }
                continue;
            }
            // Unknown directive - copy it
            result[result_pos++] = *p;
            p++;
        } else if (*p == '/' && *(p+1) == '/') {
            // Single-line comment - copy it
            result[result_pos++] = *p;
            p++;
            result[result_pos++] = *p;
            p++;
            while (*p != '\n' && *p != '\0') {
                result[result_pos++] = *p;
                p++;
            }
            continue;
        } else if (*p == '/' && *(p+1) == '*') {
            // Multi-line comment - copy it
            result[result_pos++] = *p;
            p++;
            result[result_pos++] = *p;
            p++;
            while (*p && !(p[0] == '*' && p[1] == '/')) {
                result[result_pos++] = *p;
                p++;
            }
            if (*p) {
                result[result_pos++] = *p;
                p++;
                result[result_pos++] = *p;
                p++;
            }
            continue;
        } else if (*p != '\0') {
            // Regular code
            result[result_pos++] = *p;
            p++;
        } else {
            p++;
        }
    }
    
    result[result_pos] = '\0';
    return result;
}
