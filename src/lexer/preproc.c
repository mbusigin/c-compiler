/**
 * preproc.c - C preprocessor implementation
 */

#include "preproc.h"
#include "../common/util.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

// Define SEEK constants if not available
#ifndef SEEK_SET
#define SEEK_SET 0
#endif
#ifndef SEEK_END
#define SEEK_END 2
#endif

// Macro definition
typedef struct Macro {
    char *name;
    char *value;
    bool is_function_like;
    char **params;  // Parameter names for function-like macros
    int num_params;
    struct Macro *next;
} Macro;

static Macro *macro_list = NULL;
static bool preproc_initialized = false;

// Include path for searching headers
#define MAX_INCLUDE_PATHS 32
static char *include_paths[MAX_INCLUDE_PATHS];
static int num_include_paths = 0;

// Stack of current file directories for relative includes
#define MAX_INCLUDE_DEPTH 32
static char *current_dirs[MAX_INCLUDE_DEPTH];
static int current_depth = 0;

void preproc_add_include_path(const char *path) {
    if (num_include_paths < MAX_INCLUDE_PATHS) {
        include_paths[num_include_paths++] = xstrdup(path);
    }
}

// Get directory from a file path
static char *get_directory(const char *filepath) {
    const char *last_slash = strrchr(filepath, '/');
    if (last_slash) {
        size_t len = last_slash - filepath;
        char *dir = xmalloc(len + 1);
        memcpy(dir, filepath, len);
        dir[len] = '\0';
        return dir;
    }
    return xstrdup(".");  // Current directory
}

// Find a macro by name
static Macro *find_macro(const char *name) {
    for (Macro *m = macro_list; m; m = m->next) {
        if (strcmp(m->name, name) == 0) return m;
    }
    return NULL;
}

// Add a macro definition
static void add_macro(const char *name, const char *value, bool is_function_like, char **params, int num_params) {
    // Remove existing macro if any - use simple pointer traversal
    Macro *curr = macro_list;
    Macro *prev = NULL;
    while (curr) {
        if (strcmp(curr->name, name) == 0) {
            // Found existing - remove it
            if (prev) {
                prev->next = curr->next;
            } else {
                macro_list = curr->next;
            }
            free(curr->name);
            free(curr->value);
            for (int i = 0; i < curr->num_params; i++) free(curr->params[i]);
            free(curr->params);
            free(curr);
            break;
        }
        prev = curr;
        curr = curr->next;
    }
    
    Macro *m = xmalloc(sizeof(Macro));
    m->name = xstrdup(name);
    m->value = value ? xstrdup(value) : xstrdup("1");
    m->is_function_like = is_function_like;
    m->num_params = num_params;
    
    // Copy parameter array
    if (num_params > 0 && params) {
        m->params = xmalloc(sizeof(char*) * num_params);
        for (int i = 0; i < num_params; i++) {
            m->params[i] = xstrdup(params[i]);
        }
    } else {
        m->params = NULL;
    }
    
    m->next = macro_list;
    macro_list = m;
}

// Check if string starts with prefix
static bool starts_with(const char *s, const char *prefix) {
    while (*prefix) {
        if (*s != *prefix) return false;
        s++;
        prefix++;
    }
    return true;
}

// Skip whitespace
static const char *skip_ws(const char *p) {
    while (*p == ' ' || *p == '\t') p++;
    return p;
}

// Read an identifier
static char *read_identifier(const char **pp) {
    const char *p = *pp;
    while (*p && (*p == '_' || isalnum(*p))) p++;
    size_t len = p - *pp;
    char *id = xmalloc(len + 1);
    memcpy(id, *pp, len);
    id[len] = '\0';
    *pp = p;
    return id;
}

// Read a quoted string token (helper for future use)
__attribute__((unused))
static char *read_quoted_string(const char **pp) {
    const char *p = *pp;
    char quote = *p++;
    const char *start = p;
    while (*p && *p != quote) {
        if (*p == '\\' && *(p+1)) p++;
        p++;
    }
    size_t len = p - start;
    char *str = xmalloc(len + 1);
    memcpy(str, start, len);
    str[len] = '\0';
    if (*p == quote) p++;
    *pp = p;
    return str;
}

// Read a file's contents (helper for future use)
__attribute__((unused))
static char *read_file_contents(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return NULL;
    
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char *content = xmalloc(size + 1);
    size_t read_size = fread(content, 1, size, f);
    content[read_size] = '\0';
    fclose(f);
    
    return content;
}

// Try to find and include a file
// Sets found_path to the full path if found (caller must free)
static char *try_include(const char *filename, bool is_system, char **found_path) {
    if (found_path) *found_path = NULL;
    // For "file", first try directory of current file
    if (!is_system && current_depth > 0 && current_dirs[current_depth - 1]) {
        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", current_dirs[current_depth - 1], filename);
        char *content = read_file(path);
        if (content) {
            if (found_path) *found_path = xstrdup(path);
            return content;
        }
    }
    
    // For "file", also try current working directory
    if (!is_system) {
        char *content = read_file(filename);
        if (content) {
            if (found_path) *found_path = xstrdup(filename);
            return content;
        }
    }
    
    // Try include paths
    for (int i = 0; i < num_include_paths; i++) {
        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", include_paths[i], filename);
        char *content = read_file(path);
        if (content) {
            if (found_path) *found_path = xstrdup(path);
            return content;
        }
    }
    
    return NULL;
}

// Forward declaration for recursive preprocessing
static char *preprocess_source(const char *source, int depth, const char *filename);

// Expand macros in a string
static char *expand_macros(const char *input) {
    if (!input) return xstrdup("");
    
    size_t len = strlen(input);
    char *result = xmalloc(len * 4 + 1);  // Allow for expansion
    size_t result_pos = 0;
    const char *p = input;
    
    while (*p) {
        // Check if this is an identifier
        if (*p == '_' || isalpha(*p)) {
            char *id = read_identifier(&p);
            Macro *m = find_macro(id);
            
            if (m && !m->is_function_like) {
                // Expand object-like macro
                const char *value = m->value;
                while (*value) {
                    result[result_pos++] = *value++;
                }
            } else if (m && m->is_function_like && *p == '(') {
                // Function-like macro expansion
                p++;  // skip '('
                
                // Collect arguments
                char *args[16];
                int num_args = 0;
                const char *arg_start = p;
                int paren_depth = 1;
                
                while (*p && paren_depth > 0) {
                    if (*p == '(') paren_depth++;
                    else if (*p == ')') paren_depth--;
                    else if (*p == ',' && paren_depth == 1) {
                        size_t arg_len = p - arg_start;
                        args[num_args] = xmalloc(arg_len + 1);
                        memcpy(args[num_args], arg_start, arg_len);
                        args[num_args][arg_len] = '\0';
                        num_args++;
                        arg_start = p + 1;
                    }
                    p++;
                }
                
                // Last argument
                if (num_args < 16) {
                    size_t arg_len = p - arg_start - 1;  // -1 for ')'
                    if (arg_len > 0) {
                        args[num_args] = xmalloc(arg_len + 1);
                        memcpy(args[num_args], arg_start, arg_len);
                        args[num_args][arg_len] = '\0';
                        num_args++;
                    }
                }
                
                // Substitute parameters
                const char *value = m->value;
                while (*value) {
                    if (*value == '_' || isalpha(*value)) {
                        const char *id_start = value;
                        char *param_id = read_identifier(&value);
                        
                        // Check if this is a parameter
                        int param_idx = -1;
                        for (int i = 0; i < m->num_params; i++) {
                            if (strcmp(m->params[i], param_id) == 0) {
                                param_idx = i;
                                break;
                            }
                        }
                        
                        if (param_idx >= 0 && param_idx < num_args && args[param_idx]) {
                            const char *arg = args[param_idx];
                            while (*arg) {
                                result[result_pos++] = *arg++;
                            }
                        } else {
                            const char *id_copy = id_start;
                            while (id_copy < value) {
                                result[result_pos++] = *id_copy++;
                            }
                        }
                        free(param_id);
                    } else {
                        result[result_pos++] = *value++;
                    }
                }
                
                // Free args
                for (int i = 0; i < num_args; i++) free(args[i]);
            } else {
                // Not a macro, copy identifier
                const char *id_copy = id;
                while (*id_copy) {
                    result[result_pos++] = *id_copy++;
                }
            }
            free(id);
        } else {
            result[result_pos++] = *p++;
        }
    }
    
    result[result_pos] = '\0';
    return result;
}

// Preprocess source with include depth tracking
static char *preprocess_source(const char *source, int depth, const char *filename) {
    if (!source) return xstrdup("");
    if (depth > 16) return xstrdup("");  // Prevent infinite recursion
    
    // Push current directory onto stack
    char *current_dir = NULL;
    if (filename && current_depth < MAX_INCLUDE_DEPTH) {
        current_dir = get_directory(filename);
        current_dirs[current_depth++] = current_dir;
    }
    
    size_t len = strlen(source);
    char *result = xmalloc(len * 4 + 10000);
    size_t result_pos = 0;
    const char *p = source;
    
    // Conditional compilation stack
    bool cond_stack[32];
    int cond_depth = 0;
    bool skipping = false;
    
    while (*p) {
        // Check for preprocessor directive at start of line
        const char *line_start = p;
        
        // Skip leading whitespace to detect preprocessor directives
        while (*p == ' ' || *p == '\t') p++;
        
        if (*p == '#') {
            // Preprocessor directive - don't copy the skipped whitespace
            p++;
            p = skip_ws(p);
            
            if (starts_with(p, "include")) {
                p += 7;
                p = skip_ws(p);
                
                if (!skipping) {
                    bool is_system = (*p == '<');
                    char quote = is_system ? '>' : '"';
                    p++;  // skip < or "
                    
                    const char *file_start = p;
                    while (*p && *p != quote) p++;
                    size_t file_len = p - file_start;
                    char *inc_filename = xmalloc(file_len + 1);
                    memcpy(inc_filename, file_start, file_len);
                    inc_filename[file_len] = '\0';
                    if (*p == quote) p++;
                    
                    // Try to include the file
                    char *included_path = NULL;
                    char *included = try_include(inc_filename, is_system, &included_path);
                    if (included) {
                        char *processed = preprocess_source(included, depth + 1, included_path ? included_path : inc_filename);
                        free(included);
                        if (included_path) free(included_path);
                        
                        // Copy processed content
                        const char *inc_p = processed;
                        while (*inc_p) {
                            result[result_pos++] = *inc_p++;
                        }
                        free(processed);
                    }
                    free(inc_filename);
                }
            } else if (starts_with(p, "define")) {
                p += 6;
                p = skip_ws(p);
                
                if (!skipping) {
                    char *name = read_identifier(&p);
                    
                    // Check for function-like macro: name immediately followed by '('
                    bool is_function_like = false;
                    char *params[16];
                    int num_params = 0;
                    
                    // Function-like macro: name immediately followed by '(' (no space)
                    if (*p == '(') {
                        // No space - function-like macro
                        is_function_like = true;
                        p++;  // skip '('
                        
                        while (*p && *p != ')') {
                            p = skip_ws(p);
                            if (*p && *p != ',' && *p != ')') {
                                params[num_params++] = read_identifier(&p);
                                p = skip_ws(p);
                                if (*p == ',') p++;
                            }
                        }
                        if (*p == ')') p++;
                    }
                    
                    p = skip_ws(p);
                    
                    // Read value (rest of line, handling backslash continuation)
                    size_t value_capacity = 256;
                    size_t value_len = 0;
                    char *value = xmalloc(value_capacity);
                    
                    while (*p) {
                        if (*p == '\\' && *(p+1) == '\n') {
                            // Backslash-newline continuation
                            value[value_len++] = ' ';  // Replace with space
                            p += 2;  // Skip backslash and newline
                            // Skip leading whitespace on next line
                            while (*p == ' ' || *p == '\t') p++;
                            continue;
                        }
                        if (*p == '\n') break;
                        if (value_len + 1 >= value_capacity) {
                            value_capacity *= 2;
                            value = xrealloc(value, value_capacity);
                        }
                        value[value_len++] = *p++;
                    }
                    value[value_len] = '\0';
                    
                    // Trim trailing whitespace
                    while (value_len > 0 && (value[value_len-1] == ' ' || value[value_len-1] == '\t')) {
                        value[--value_len] = '\0';
                    }
                    
                    add_macro(name, value, is_function_like, params, num_params);
                    free(name);
                    free(value);
                }
            } else if (starts_with(p, "ifdef")) {
                p += 5;
                p = skip_ws(p);
                
                char *name = read_identifier(&p);
                bool defined = find_macro(name) != NULL;
                free(name);
                
                if (cond_depth < 32) {
                    cond_stack[cond_depth++] = skipping;
                    skipping = skipping || !defined;
                }
            } else if (starts_with(p, "ifndef")) {
                p += 6;
                p = skip_ws(p);
                
                char *name = read_identifier(&p);
                bool defined = find_macro(name) != NULL;
                free(name);
                
                if (cond_depth < 32) {
                    cond_stack[cond_depth++] = skipping;
                    skipping = skipping || defined;
                }
            } else if (starts_with(p, "endif")) {
                p += 5;
                
                if (cond_depth > 0) {
                    cond_depth--;
                    skipping = cond_depth > 0 ? cond_stack[cond_depth - 1] : false;
                }
            } else if (starts_with(p, "else")) {
                p += 4;
                
                if (cond_depth > 0) {
                    skipping = !skipping;
                }
            } else if (starts_with(p, "elif")) {
                p += 4;
                // For simplicity, treat elif like else if
                // Full implementation would evaluate the condition
                if (cond_depth > 0) {
                    skipping = !skipping;
                }
            } else if (starts_with(p, "undef")) {
                p += 5;
                p = skip_ws(p);
                
                if (!skipping) {
                    char *name = read_identifier(&p);
                    // Remove macro
                    Macro **prev = &macro_list;
                    while (*prev) {
                        if (strcmp((*prev)->name, name) == 0) {
                            Macro *old = *prev;
                            *prev = old->next;
                            free(old->name);
                            free(old->value);
                            free(old);
                            break;
                        }
                        prev = &(*prev)->next;
                    }
                    free(name);
                }
            }
            // Skip to end of line
            while (*p && *p != '\n') p++;
            if (*p == '\n') {
                result[result_pos++] = '\n';
                p++;
            }
        } else if (!skipping) {
            // Regular code - start from the beginning of the line
            // (not from after whitespace, to preserve leading whitespace)
            p = line_start;
            
            if (*p == '\n') {
                result[result_pos++] = *p++;
            } else if (*p == '"' || *p == '\'') {
                // String/char literal - copy without expansion
                char quote = *p;
                result[result_pos++] = *p++;
                while (*p && *p != quote) {
                    if (*p == '\\' && *(p+1)) {
                        result[result_pos++] = *p++;
                    }
                    result[result_pos++] = *p++;
                }
                if (*p == quote) result[result_pos++] = *p++;
            } else if (*p == '/' && *(p+1) == '/') {
                // Single-line comment
                while (*p && *p != '\n') p++;
            } else if (*p == '/' && *(p+1) == '*') {
                // Multi-line comment
                p += 2;
                while (*p && !(p[0] == '*' && p[1] == '/')) p++;
                if (*p) p += 2;
            } else if (*p == '_' || isalpha(*p)) {
                // Identifier - might be a macro
                char *id = read_identifier(&p);
                Macro *m = find_macro(id);
                
                if (m && !m->is_function_like) {
                    // Add space before macro expansion to preserve token boundary
                    result[result_pos++] = ' ';
                    const char *value = m->value;
                    while (*value) {
                        result[result_pos++] = *value++;
                    }
                    // Add space after to preserve next token boundary
                    result[result_pos++] = ' ';
                } else {
                    // Not a simple macro, just copy the identifier
                    const char *id_copy = id;
                    while (*id_copy) {
                        result[result_pos++] = *id_copy++;
                    }
                }
                free(id);
            } else {
                result[result_pos++] = *p++;
            }
        } else {
            // Skipping due to conditional
            if (*p == '\n') {
                result[result_pos++] = *p++;
            } else {
                p++;
            }
        }
    }
    
    result[result_pos] = '\0';
    
    // Pop directory from stack
    if (current_dir) {
        current_depth--;
        free(current_dir);
    }
    
    // Final macro expansion pass
    char *expanded = expand_macros(result);
    free(result);
    
    return expanded;
}

// Initialize preprocessor with system include paths and built-in defines
static void preproc_init(void) {
    if (preproc_initialized) return;
    preproc_initialized = true;
    
    // Add built-in defines
    add_macro("__STDC__", "1", false, NULL, 0);
    add_macro("__STDC_VERSION__", "201112L", false, NULL, 0);
    add_macro("__STDC_HOSTED__", "1", false, NULL, 0);
    add_macro("__apple__", "1", false, NULL, 0);
    add_macro("__APPLE__", "1", false, NULL, 0);
    add_macro("__MACH__", "1", false, NULL, 0);
    add_macro("__aarch64__", "1", false, NULL, 0);
    add_macro("__ARM64_ARCH_8__", "1", false, NULL, 0);
    add_macro("__LITTLE_ENDIAN__", "1", false, NULL, 0);
    add_macro("__BYTE_ORDER__", "1234", false, NULL, 0);
    add_macro("__ORDER_LITTLE_ENDIAN__", "1234", false, NULL, 0);
    add_macro("__ORDER_BIG_ENDIAN__", "4321", false, NULL, 0);
    add_macro("__SIZE_TYPE__", "unsigned long", false, NULL, 0);
    add_macro("__PTRDIFF_TYPE__", "long", false, NULL, 0);
    add_macro("__INTPTR_TYPE__", "long", false, NULL, 0);
    add_macro("__UINTPTR_TYPE__", "unsigned long", false, NULL, 0);
    add_macro("__SIZEOF_INT__", "4", false, NULL, 0);
    add_macro("__SIZEOF_LONG__", "8", false, NULL, 0);
    add_macro("__SIZEOF_POINTER__", "8", false, NULL, 0);
    add_macro("__SIZEOF_LONG_LONG__", "8", false, NULL, 0);
    add_macro("__SIZEOF_SHORT__", "2", false, NULL, 0);
    add_macro("__SIZEOF_FLOAT__", "4", false, NULL, 0);
    add_macro("__SIZEOF_DOUBLE__", "8", false, NULL, 0);
    add_macro("__CHAR_BIT__", "8", false, NULL, 0);
    
    // Add sysincludes as include path
    preproc_add_include_path("sysincludes");
}

// Main preprocessing entry point
char *preprocess(const char *source) {
    preproc_init();
    return preprocess_source(source, 0, NULL);
}

// Preprocess a file
char *preprocess_file(const char *filename) {
    preproc_init();
    char *content = read_file(filename);
    if (!content) return NULL;
    
    char *result = preprocess_source(content, 0, filename);
    free(content);
    return result;
}
