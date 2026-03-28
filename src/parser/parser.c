/**
 * parser.c - Minimal C11 recursive descent parser
 */

#include "parser.h"
#include "../common/util.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

// Simple typedef registry
typedef struct TypedefEntry {
    char *name;
    TypeKind kind;
    bool is_unsigned;
    bool is_pointer;
    Type *full_type;  // Full type info for structs
    struct TypedefEntry *next;
} TypedefEntry;

static TypedefEntry *typedef_list = NULL;

static void add_typedef_full(const char *name, Type *type) {
    TypedefEntry *entry = malloc(sizeof(TypedefEntry));
    entry->name = xstrdup(name);
    entry->kind = type ? type->kind : TYPE_INT;
    entry->is_unsigned = type ? type->is_unsigned : false;
    entry->is_pointer = type ? (type->kind == TYPE_POINTER) : false;
    entry->full_type = type;
    entry->next = typedef_list;
    typedef_list = entry;
}

static TypedefEntry *find_typedef(const char *name) {
    for (TypedefEntry *e = typedef_list; e; e = e->next) {
        if (strcmp(e->name, name) == 0) return e;
    }
    return NULL;
}

// Struct/Union registry
typedef struct StructEntry {
    char *name;
    Type *type;
    struct StructEntry *next;
} StructEntry;

static StructEntry *struct_registry = NULL;

static void add_struct(const char *name, Type *type) {
    // Remove existing entry if any
    StructEntry **prev = &struct_registry;
    while (*prev) {
        if (strcmp((*prev)->name, name) == 0) {
            StructEntry *old = *prev;
            *prev = old->next;
            // Don't free the type - it may be in use
            free(old->name);
            free(old);
            break;
        }
        prev = &(*prev)->next;
    }
    
    StructEntry *entry = malloc(sizeof(StructEntry));
    entry->name = xstrdup(name);
    entry->type = type;
    entry->next = struct_registry;
    struct_registry = entry;
}

static Type *find_struct(const char *name) {
    for (StructEntry *e = struct_registry; e; e = e->next) {
        if (strcmp(e->name, name) == 0) return e->type;
    }
    return NULL;
}

// Enum constant registry
typedef struct EnumConstant {
    char *name;
    int value;
    struct EnumConstant *next;
} EnumConstant;

static EnumConstant *enum_constants = NULL;

static void add_enum_constant(const char *name, int value) {
    EnumConstant *entry = malloc(sizeof(EnumConstant));
    entry->name = xstrdup(name);
    entry->value = value;
    entry->next = enum_constants;
    enum_constants = entry;
}

static EnumConstant *find_enum_constant(const char *name) {
    for (EnumConstant *e = enum_constants; e; e = e->next) {
        if (strcmp(e->name, name) == 0) return e;
    }
    return NULL;
}

// Forward declarations
static ASTNode *parse_declaration(Parser *parser);
static ASTNode *parse_statement(Parser *parser);
static ASTNode *parse_expression(Parser *parser);
static ASTNode *parse_assignment_expr(Parser *parser);
static ASTNode *parse_conditional_expr(Parser *parser);
static ASTNode *parse_binary_expr(Parser *parser, int min_prec);
static ASTNode *parse_cast_expr(Parser *parser);
static ASTNode *parse_unary_expr(Parser *parser);
static ASTNode *parse_postfix_expr(Parser *parser);
static ASTNode *parse_postfix_expr_with_expr(Parser *p, ASTNode *expr);
static ASTNode *parse_primary_expr(Parser *parser);
static void expect(Parser *p, TokenType t, const char *msg);

// Helper functions
static void advance_p(Parser *p) { 
    p->previous = p->current; 
    p->current = lexer_next_token(p->lexer); 
}
static TokenType peek_p(Parser *p) { return p->current.type; }
static bool check_p(Parser *p, TokenType t) { return peek_p(p) == t; }

// Look at the token after the current one without consuming it
__attribute__((unused))
static TokenType peek_next_p(Parser *p) {
    // Save lexer state
    size_t saved_pos = p->lexer->position;
    int saved_line = p->lexer->line;
    int saved_col = p->lexer->column;
    
    // Get next token
    Token next = lexer_next_token(p->lexer);
    TokenType result = next.type;
    
    // Restore lexer state
    p->lexer->position = saved_pos;
    p->lexer->line = saved_line;
    p->lexer->column = saved_col;
    
    return result;
}

static bool is_type_keyword(TokenType t) {
    return t == TOKEN_INT || t == TOKEN_CHAR || t == TOKEN_VOID || t == TOKEN_FLOAT ||
           t == TOKEN_DOUBLE || t == TOKEN_SHORT || t == TOKEN_LONG || t == TOKEN_SIGNED ||
           t == TOKEN_UNSIGNED || t == TOKEN_EXTERN || t == TOKEN_STATIC || t == TOKEN_CONST ||
           t == TOKEN_STRUCT || t == TOKEN_UNION || t == TOKEN_TYPEDEF || t == TOKEN_ENUM ||
           t == TOKEN_BOOL;
}

// Forward declaration for debug function
static void debug_typedef_check(const char *name);

// Forward declaration for token_name
static char *token_name(Parser *p);

// Check if identifier is a builtin type name
// Note: size_t, ptrdiff_t, bool, etc. are NOT builtins - they're defined by headers
// However, for self-hosting we need to handle them as builtins since we don't have standard headers
static bool is_builtin_type(const char *name) {
    return strcmp(name, "__builtin_va_list") == 0 ||
           strcmp(name, "__int128") == 0 ||
           strcmp(name, "__float128") == 0 ||
           // Standard integer types from stddef.h
           strcmp(name, "size_t") == 0 ||
           strcmp(name, "ptrdiff_t") == 0 ||
           strcmp(name, "intptr_t") == 0 ||
           strcmp(name, "uintptr_t") == 0 ||
           // Standard boolean type from stdbool.h
           strcmp(name, "bool") == 0 ||
           // Standard FILE type from stdio.h
           strcmp(name, "FILE") == 0 ||
           // va_list from stdarg.h
           strcmp(name, "va_list") == 0 ||
           // AST/Type types from our own headers
           strcmp(name, "Type") == 0 ||
           strcmp(name, "ASTNode") == 0 ||
           strcmp(name, "ASTNodeType") == 0 ||
           strcmp(name, "TypeKind") == 0 ||
           strcmp(name, "StructMember") == 0 ||
           strcmp(name, "TokenType") == 0 ||
           strcmp(name, "Token") == 0 ||
           strcmp(name, "Parser") == 0 ||
           strcmp(name, "Lexer") == 0 ||
           strcmp(name, "List") == 0 ||
           strcmp(name, "BinaryOp") == 0 ||
           // Parser internal types
           strcmp(name, "TypedefEntry") == 0 ||
           strcmp(name, "StructEntry") == 0 ||
           strcmp(name, "EnumConstant") == 0 ||
           // IR types from ir.h
           strcmp(name, "IRModule") == 0 ||
           strcmp(name, "IRFunction") == 0 ||
           strcmp(name, "IRInstruction") == 0 ||
           strcmp(name, "IRGlobal") == 0 ||
           strcmp(name, "IRBasicBlock") == 0 ||
           strcmp(name, "IROpcode") == 0 ||
           strcmp(name, "IRValue") == 0 ||
           strcmp(name, "IRValueKind") == 0 ||
           // Enum values from our headers
           strcmp(name, "TYPE_INT") == 0 ||
           strcmp(name, "TYPE_POINTER") == 0 ||
           strcmp(name, "TYPE_VOID") == 0 ||
           strcmp(name, "TYPE_CHAR") == 0 ||
           strcmp(name, "TYPE_LONG") == 0 ||
           strcmp(name, "TYPE_STRUCT") == 0 ||
           strcmp(name, "TYPE_UNION") == 0 ||
           strcmp(name, "TYPE_ARRAY") == 0 ||
           strcmp(name, "TYPE_FUNCTION") == 0 ||
           strcmp(name, "TYPE_BOOL") == 0 ||
           strcmp(name, "TYPE_FLOAT") == 0 ||
           strcmp(name, "TYPE_DOUBLE") == 0 ||
           strcmp(name, "TYPE_ENUM") == 0 ||
           strcmp(name, "AST_FUNCTION_DECL") == 0 ||
           strcmp(name, "AST_VARIABLE_DECL") == 0 ||
           strcmp(name, "AST_COMPOUND_STMT") == 0 ||
           strcmp(name, "AST_TRANSLATION_UNIT") == 0 ||
           strcmp(name, "AST_RETURN_STMT") == 0 ||
           strcmp(name, "AST_IF_STMT") == 0 ||
           strcmp(name, "AST_WHILE_STMT") == 0 ||
           strcmp(name, "AST_FOR_STMT") == 0 ||
           strcmp(name, "AST_BINARY_EXPR") == 0 ||
           strcmp(name, "AST_UNARY_EXPR") == 0 ||
           strcmp(name, "AST_CALL_EXPR") == 0 ||
           strcmp(name, "AST_IDENTIFIER_EXPR") == 0 ||
           strcmp(name, "AST_INTEGER_LITERAL_EXPR") == 0;
}

// Check if current token is a type keyword or a typedef name
static bool is_type(Parser *p) {
    // Check for 'typedef' keyword
    if (check_p(p, TOKEN_TYPEDEF)) {
        return true;
    }
    if (is_type_keyword(peek_p(p))) {
        return true;
    }
    // Check for typedef name or builtin type
    if (check_p(p, TOKEN_IDENTIFIER)) {
        char *name = token_name(p);
        TypedefEntry *entry = find_typedef(name);
        bool builtin = is_builtin_type(name);
        free(name);
        if (entry != NULL || builtin) {
            return true;
        }
    }
    return false;
}

// Copy a string from the current token's lexeme using the token's length
static char *token_name(Parser *p) {
    size_t len = p->current.length;
    char *name = xmalloc(len + 1);
    memcpy(name, p->current.lexeme, len);
    name[len] = '\0';
    return name;
}

// Skip __attribute__((...)) if present
static void skip_attribute(Parser *p) {
    // Check if current token is __attribute__
    if (check_p(p, TOKEN_IDENTIFIER)) {
        char *name = token_name(p);
        if (strcmp(name, "__attribute__") == 0) {
            advance_p(p);  // consume __attribute__
            free(name);
            // Expect ((...))
            if (check_p(p, TOKEN_LPAREN)) {
                advance_p(p);  // consume (
                if (check_p(p, TOKEN_LPAREN)) {
                    advance_p(p);  // consume (
                    // Skip until matching ))
                    int depth = 2;
                    while (depth > 0 && !check_p(p, TOKEN_EOF)) {
                        if (check_p(p, TOKEN_LPAREN)) depth++;
                        else if (check_p(p, TOKEN_RPAREN)) depth--;
                        advance_p(p);
                    }
                }
            }
        } else {
            free(name);
        }
    }
}

// Type parsing
static Type *parse_type(Parser *p) {
    TypeKind kind = TYPE_INT;
    bool is_unsigned = false;
    
    // Handle type qualifiers (const)
    while (check_p(p, TOKEN_CONST)) {
        advance_p(p);  // skip 'const'
    }
    
    // Handle signed/unsigned modifiers
    while (check_p(p, TOKEN_SIGNED) || check_p(p, TOKEN_UNSIGNED)) {
        if (check_p(p, TOKEN_UNSIGNED)) {
            is_unsigned = true;
            advance_p(p);
        } else if (check_p(p, TOKEN_SIGNED)) {
            is_unsigned = false;
            advance_p(p);
        }
    }
    
    // Handle struct/union type reference
    if (check_p(p, TOKEN_STRUCT)) {
        advance_p(p);  // consume 'struct'
        
        char *struct_name = NULL;
        if (check_p(p, TOKEN_IDENTIFIER)) {
            // struct Name - reference to named struct
            struct_name = token_name(p);
            advance_p(p);  // consume name
        }
        
        // Check for struct definition { ... }
        if (check_p(p, TOKEN_LBRACE)) {
            // Create new struct type with definition
            Type *t = type_create(TYPE_STRUCT);
            t->is_unsigned = false;
            t->struct_name = struct_name;
            
            advance_p(p);  // consume '{'
            
            // Parse member declarations
            while (!check_p(p, TOKEN_RBRACE) && !check_p(p, TOKEN_EOF)) {
                // Parse member type
                Type *member_base_type = parse_type(p);
                if (!member_base_type) {
                    member_base_type = type_create(TYPE_INT);  // Default to int
                }
                
                // Parse member name(s) - can have multiple members of same type on one line
                while (!check_p(p, TOKEN_SEMICOLON) && !check_p(p, TOKEN_EOF)) {
                    // Handle pointer modifiers
                    Type *member_type = type_copy(member_base_type);
                    while (check_p(p, TOKEN_STAR)) {
                        advance_p(p);
                        Type *ptr = type_pointer(member_type);
                        member_type = ptr;
                    }
                    
                    if (check_p(p, TOKEN_IDENTIFIER)) {
                        char *member_name = token_name(p);
                        advance_p(p);
                        
                        // Handle array dimensions
                        if (check_p(p, TOKEN_LBRACKET)) {
                            advance_p(p);  // consume '['
                            size_t array_size = 0;
                            if (check_p(p, TOKEN_NUMBER) || check_p(p, TOKEN_INT_CONSTANT)) {
                                array_size = (size_t)p->current.value.int_val;
                                advance_p(p);
                            }
                            if (check_p(p, TOKEN_RBRACKET)) advance_p(p);
                            if (array_size > 0 && member_type) {
                                Type *arr = type_array(member_type, array_size);
                                member_type = arr;
                            }
                        }
                        
                        // Add member to struct
                        type_add_member(t, member_name, member_type);
                        free(member_name);
                    } else {
                        type_free(member_type);
                    }
                    
                    // Check for comma (multiple members of same type)
                    if (check_p(p, TOKEN_COMMA)) {
                        advance_p(p);
                        // Continue to next member
                    } else {
                        break;
                    }
                }
                
                if (check_p(p, TOKEN_SEMICOLON)) advance_p(p);
                type_free(member_base_type);
            }
            
            if (check_p(p, TOKEN_RBRACE)) advance_p(p);  // consume '}'
            
            // Finalize struct size and alignment
            t->align = type_compute_struct_alignment(t);
            t->size = type_compute_struct_size(t);
            
            // Register the struct
            if (struct_name) {
                add_struct(struct_name, t);
            }
            
            // Handle pointer suffix
            while (check_p(p, TOKEN_STAR)) {
                advance_p(p);
                Type *ptr = type_pointer(t);
                t = ptr;
            }
            
            return t;
        } else {
            // No definition - look up existing struct or create forward declaration
            Type *existing = struct_name ? find_struct(struct_name) : NULL;
            if (existing) {
                // Found existing definition - return a COPY
                free(struct_name);  // Free the name since the existing type has its own copy
                
                // Copy the existing type
                Type *t = type_copy(existing);
                
                // Handle pointer suffix
                while (check_p(p, TOKEN_STAR)) {
                    advance_p(p);
                    Type *ptr = type_pointer(t);
                    t = ptr;
                }
                return t;
            } else {
                // Forward declaration - create incomplete struct type
                Type *t = type_create(TYPE_STRUCT);
                t->is_unsigned = false;
                t->struct_name = struct_name;
                
                // Handle pointer suffix
                while (check_p(p, TOKEN_STAR)) {
                    advance_p(p);
                    Type *ptr = type_pointer(t);
                    t = ptr;
                }
                return t;
            }
        }
    }
    
    // Handle union type reference
    if (check_p(p, TOKEN_UNION)) {
        advance_p(p);  // consume 'union'
        
        char *union_name = NULL;
        if (check_p(p, TOKEN_IDENTIFIER)) {
            union_name = token_name(p);
            advance_p(p);  // consume name
        }
        
        // Check for union definition { ... }
        if (check_p(p, TOKEN_LBRACE)) {
            // Create new union type with definition
            Type *t = type_create(TYPE_UNION);
            t->is_unsigned = false;
            t->struct_name = union_name;
            
            advance_p(p);  // consume '{'
            
            // Parse member declarations (same as struct)
            while (!check_p(p, TOKEN_RBRACE) && !check_p(p, TOKEN_EOF)) {
                // Parse member type
                Type *member_base_type = parse_type(p);
                if (!member_base_type) {
                    member_base_type = type_create(TYPE_INT);
                }
                
                // Parse member name(s)
                while (!check_p(p, TOKEN_SEMICOLON) && !check_p(p, TOKEN_EOF)) {
                    Type *member_type = type_copy(member_base_type);
                    while (check_p(p, TOKEN_STAR)) {
                        advance_p(p);
                        Type *ptr = type_pointer(member_type);
                        member_type = ptr;
                    }
                    
                    if (check_p(p, TOKEN_IDENTIFIER)) {
                        char *member_name = token_name(p);
                        advance_p(p);
                        
                        // Handle array dimensions
                        if (check_p(p, TOKEN_LBRACKET)) {
                            advance_p(p);
                            size_t array_size = 0;
                            if (check_p(p, TOKEN_NUMBER)) {
                                array_size = (size_t)p->current.value.int_val;
                                advance_p(p);
                            }
                            if (check_p(p, TOKEN_RBRACKET)) advance_p(p);
                            if (array_size > 0 && member_type) {
                                Type *arr = type_array(member_type, array_size);
                                member_type = arr;
                            }
                        }
                        
                        // Add member to union
                        type_add_member(t, member_name, member_type);
                        free(member_name);
                    } else {
                        type_free(member_type);
                    }
                    
                    if (check_p(p, TOKEN_COMMA)) {
                        advance_p(p);
                    } else {
                        break;
                    }
                }
                
                if (check_p(p, TOKEN_SEMICOLON)) advance_p(p);
                type_free(member_base_type);
            }
            
            if (check_p(p, TOKEN_RBRACE)) advance_p(p);
            
            // Finalize union size and alignment
            t->align = type_compute_struct_alignment(t);
            t->size = type_compute_struct_size(t);
            
            // Register the union (use same registry as struct)
            if (union_name) {
                add_struct(union_name, t);
            }
            
            // Handle pointer suffix
            while (check_p(p, TOKEN_STAR)) {
                advance_p(p);
                Type *ptr = type_pointer(t);
                t = ptr;
            }
            
            return t;
        } else {
            // No definition - look up existing union or create forward declaration
            Type *existing = union_name ? find_struct(union_name) : NULL;
            if (existing) {
                // Found existing definition - return a COPY
                free(union_name);
                
                Type *t = type_copy(existing);
                
                while (check_p(p, TOKEN_STAR)) {
                    advance_p(p);
                    Type *ptr = type_pointer(t);
                    t = ptr;
                }
                return t;
            } else {
                // Forward declaration
                Type *t = type_create(TYPE_UNION);
                t->is_unsigned = false;
                t->struct_name = union_name;
                
                while (check_p(p, TOKEN_STAR)) {
                    advance_p(p);
                    Type *ptr = type_pointer(t);
                    t = ptr;
                }
                return t;
            }
        }
    }
    
    // Handle enum type reference
    if (check_p(p, TOKEN_ENUM)) {
        advance_p(p);  // consume 'enum'
        if (check_p(p, TOKEN_IDENTIFIER)) {
            advance_p(p);  // consume name
        }
        // Check for enum definition { ... }
        if (check_p(p, TOKEN_LBRACE)) {
            advance_p(p);  // consume '{'
            // Parse enum constants
            int next_value = 0;
            while (!check_p(p, TOKEN_RBRACE) && !check_p(p, TOKEN_EOF)) {
                // Parse constant name
                if (check_p(p, TOKEN_IDENTIFIER)) {
                    char *const_name = token_name(p);
                    advance_p(p);
                    
                    // Check for explicit value
                    int value = next_value;
                    if (check_p(p, TOKEN_ASSIGN)) {
                        advance_p(p);  // consume '='
                        // Parse the value expression (must be constant)
                        // For now, just parse simple integer literals
                        if (check_p(p, TOKEN_INT_CONSTANT) || check_p(p, TOKEN_NUMBER)) {
                            value = (int)p->current.value.int_val;
                            advance_p(p);
                        } else {
                            // Skip to next comma or brace
                            while (!check_p(p, TOKEN_COMMA) && !check_p(p, TOKEN_RBRACE) && !check_p(p, TOKEN_EOF)) {
                                advance_p(p);
                            }
                        }
                    }
                    
                    // Register the enum constant
                    add_enum_constant(const_name, value);
                    next_value = value + 1;
                    free(const_name);
                }
                
                if (check_p(p, TOKEN_COMMA)) advance_p(p);
            }
            if (check_p(p, TOKEN_RBRACE)) advance_p(p);  // consume '}'
        }
        // Create enum type (it's int-like)
        Type *t = type_create(TYPE_ENUM);
        t->is_unsigned = false;
        
        // Handle pointer suffix
        while (check_p(p, TOKEN_STAR)) {
            advance_p(p);
            Type *ptr = type_pointer(t);
            t = ptr;
        }
        
        return t;
    }
    
    // Then handle the base type
    if (check_p(p, TOKEN_VOID)) { advance_p(p); kind = TYPE_VOID; }
    else if (check_p(p, TOKEN_CHAR)) { advance_p(p); kind = TYPE_CHAR; }
    else if (check_p(p, TOKEN_FLOAT)) { advance_p(p); kind = TYPE_FLOAT; }
    else if (check_p(p, TOKEN_DOUBLE)) { advance_p(p); kind = TYPE_DOUBLE; }
    else if (check_p(p, TOKEN_SHORT)) { advance_p(p); kind = TYPE_SHORT; }
    else if (check_p(p, TOKEN_LONG)) { 
        advance_p(p); 
        if (check_p(p, TOKEN_LONG)) { advance_p(p); kind = TYPE_LONGLONG; }
        else kind = TYPE_LONG;
    }
    else if (check_p(p, TOKEN_INT)) { advance_p(p); kind = TYPE_INT; }
    else if (check_p(p, TOKEN_BOOL)) { advance_p(p); kind = TYPE_BOOL; }
    // Check for builtin types like __builtin_va_list
    else if (check_p(p, TOKEN_IDENTIFIER)) {
        char *name = token_name(p);
        
        // Check for true compiler builtin types (not library-defined types)
        if (is_builtin_type(name)) {
            // Handle standard integer types
            if (strcmp(name, "size_t") == 0 ||
                strcmp(name, "ptrdiff_t") == 0 ||
                strcmp(name, "intptr_t") == 0 ||
                strcmp(name, "uintptr_t") == 0) {
                advance_p(p);  // consume builtin type name
                free(name);
                Type *t = type_create(TYPE_LONG);
                t->is_unsigned = (strcmp(name, "size_t") == 0 || strcmp(name, "uintptr_t") == 0);
                while (check_p(p, TOKEN_STAR)) {
                    advance_p(p);
                    Type *ptr = type_pointer(t);
                    t = ptr;
                }
                return t;
            } else if (strcmp(name, "bool") == 0) {
                advance_p(p);
                free(name);
                Type *t = type_create(TYPE_BOOL);
                return t;
            } else if (strcmp(name, "FILE") == 0) {
                // FILE is a pointer type
                advance_p(p);
                free(name);
                Type *t = type_create(TYPE_POINTER);
                t->base = type_create(TYPE_VOID);
                while (check_p(p, TOKEN_STAR)) {
                    advance_p(p);
                    Type *ptr = type_create(TYPE_POINTER);
                    ptr->base = t;
                    t = ptr;
                }
                return t;
            } else if (strcmp(name, "va_list") == 0) {
                // va_list on ARM64 is a struct with 5 fields (40 bytes total):
                // void *__stack; void *__gr_top; void *__vr_top; long __gr_off; long __vr_off;
                advance_p(p);
                free(name);
                Type *t = type_create(TYPE_STRUCT);
                t->struct_name = strdup("__builtin_va_list_struct");
                t->size = 40;  // 5 fields * 8 bytes each
                t->align = 8;
                t->num_members = 0;
                t->members = NULL;
                return t;
            } else if (strcmp(name, "Type") == 0 || strcmp(name, "ASTNode") == 0 ||
                       strcmp(name, "TokenType") == 0 || strcmp(name, "Token") == 0 ||
                       strcmp(name, "Parser") == 0 || strcmp(name, "Lexer") == 0 ||
                       strcmp(name, "List") == 0 || strcmp(name, "BinaryOp") == 0 ||
                       strcmp(name, "ASTNodeType") == 0 || strcmp(name, "TypeKind") == 0 ||
                       strcmp(name, "StructMember") == 0 ||
                       strcmp(name, "TypedefEntry") == 0 || strcmp(name, "StructEntry") == 0 ||
                       strcmp(name, "EnumConstant") == 0 ||
                       // IR types from ir.h
                       strcmp(name, "IRModule") == 0 || strcmp(name, "IRFunction") == 0 ||
                       strcmp(name, "IRInstruction") == 0 || strcmp(name, "IRGlobal") == 0 ||
                       strcmp(name, "IRBasicBlock") == 0 || strcmp(name, "IROpcode") == 0 ||
                       strcmp(name, "IRValue") == 0 || strcmp(name, "IRValueKind") == 0) {
                // These are our own struct/enum types
                advance_p(p);
                free(name);
                // Create as pointer to struct type (for now, treat as pointer to incomplete struct)
                Type *t = type_create(TYPE_POINTER);
                t->base = type_create(TYPE_STRUCT);
                while (check_p(p, TOKEN_STAR)) {
                    advance_p(p);
                    Type *ptr = type_pointer(t);
                    t = ptr;
                }
                return t;
            } else {
                // Other builtin types treat as int
                advance_p(p);
                free(name);
                Type *t = type_create(TYPE_INT);
                return t;
            }
        }
        
        // Could be a typedef name
        TypedefEntry *entry = find_typedef(name);
        free(name);
        
        if (entry) {
            advance_p(p);  // consume typedef name
            
            // If we have full type info, use it
            if (entry->full_type) {
                Type *t = type_copy(entry->full_type);
                
                // Handle pointer suffix
                while (check_p(p, TOKEN_STAR)) {
                    advance_p(p);
                    Type *ptr = type_pointer(t);
                    t = ptr;
                }
                return t;
            }
            
            // Fall back to basic type info
            Type *t = type_create(entry->kind);
            t->is_unsigned = entry->is_unsigned;
            
            // If typedef was a pointer type, wrap it
            if (entry->is_pointer) {
                Type *ptr = type_pointer(t);
                t = ptr;
            }
            
            // Handle additional pointer suffix
            while (check_p(p, TOKEN_STAR)) {
                advance_p(p);
                Type *ptr = type_pointer(t);
                t = ptr;
            }
            
            return t;
        }
        
        // Not a typedef and not a builtin - don't advance, let caller handle
        // The function will return TYPE_INT as default (kind is still TYPE_INT from initialization)
    }
    // If we had unsigned/signed but no base type, default to int
    // (e.g., "unsigned x" means "unsigned int x")
    
    Type *t = type_create(kind);
    t->is_unsigned = is_unsigned;
    
    // Handle pointer suffix
    while (check_p(p, TOKEN_STAR)) {
        advance_p(p);
        Type *ptr = type_pointer(t);
        // Don't free t - the pointer type now owns it
        t = ptr;
    }
    
    return t;
}

// Function parameter parsing
static ASTNode *parse_parameter(Parser *p) {
    Type *param_type = parse_type(p);
    char *param_name = NULL;
    
    // Check for function pointer parameter: Type (*name)(params)
    if (check_p(p, TOKEN_LPAREN)) {
        advance_p(p);  // consume '('
        if (check_p(p, TOKEN_STAR)) {
            // This is a function pointer: Type (*name)(params)
            advance_p(p);  // consume '*'
            if (check_p(p, TOKEN_IDENTIFIER)) {
                param_name = token_name(p);
                advance_p(p);  // consume name
            }
            if (check_p(p, TOKEN_RPAREN)) {
                advance_p(p);  // consume ')'
            }
            
            // Create function pointer type
            Type *func_type = type_create(TYPE_FUNCTION);
            func_type->return_type = param_type;  // The parsed type is the return type
            func_type->param_types = NULL;
            func_type->num_params = 0;
            func_type->is_variadic = false;
            
            // Parse parameter list: (param1, param2, ...)
            if (check_p(p, TOKEN_LPAREN)) {
                advance_p(p);  // consume '('
                
                // Parse parameters
                Type **params = NULL;
                size_t num_params = 0;
                size_t param_capacity = 0;
                
                while (!check_p(p, TOKEN_RPAREN) && !check_p(p, TOKEN_EOF)) {
                    Type *p_type = parse_type(p);
                    if (!p_type) {
                        p_type = type_create(TYPE_INT);
                    }
                    
                    // Skip pointer modifiers
                    while (check_p(p, TOKEN_STAR)) {
                        advance_p(p);
                        Type *ptr = type_pointer(p_type);
                        p_type = ptr;
                    }
                    
                    // Skip parameter name if present
                    if (check_p(p, TOKEN_IDENTIFIER)) {
                        advance_p(p);
                    }
                    
                    // Skip array brackets if present
                    while (check_p(p, TOKEN_LBRACKET)) {
                        advance_p(p);
                        while (!check_p(p, TOKEN_RBRACKET) && !check_p(p, TOKEN_EOF)) advance_p(p);
                        if (check_p(p, TOKEN_RBRACKET)) advance_p(p);
                        Type *ptr = type_pointer(p_type);
                        p_type = ptr;
                    }
                    
                    // Add to parameter list
                    if (num_params >= param_capacity) {
                        param_capacity = param_capacity ? param_capacity * 2 : 4;
                        params = realloc(params, sizeof(Type*) * param_capacity);
                    }
                    params[num_params++] = p_type;
                    
                    // Skip comma
                    if (check_p(p, TOKEN_COMMA)) {
                        advance_p(p);
                    }
                }
                
                if (check_p(p, TOKEN_RPAREN)) {
                    advance_p(p);  // consume ')'
                }
                
                func_type->param_types = params;
                func_type->num_params = num_params;
            }
            
            // Wrap function type in pointer type (function pointer)
            Type *fp_type = type_pointer(func_type);
            param_type = fp_type;
        } else {
            // Just a parenthesized name, put it back
            // Unconsume the '(' - we can't actually do this, so skip to matching ')'
            int depth = 1;
            while (depth > 0 && !check_p(p, TOKEN_EOF)) {
                if (check_p(p, TOKEN_LPAREN)) depth++;
                else if (check_p(p, TOKEN_RPAREN)) depth--;
                advance_p(p);
            }
        }
    }
    
    ASTNode *param = ast_create(AST_PARAMETER_DECL);
    param->data.parameter.param_type = param_type;
    
    if (param_name) {
        param->data.parameter.name = param_name;
    } else if (check_p(p, TOKEN_IDENTIFIER)) {
        param->data.parameter.name = token_name(p);
        advance_p(p);
    } else {
        param->data.parameter.name = xstrdup("");
    }
    
    // Array parameters become pointers
    while (check_p(p, TOKEN_LBRACKET)) {
        advance_p(p);
        while (!check_p(p, TOKEN_RBRACKET) && !check_p(p, TOKEN_EOF)) advance_p(p);
        if (check_p(p, TOKEN_RBRACKET)) advance_p(p);
        Type *ptr = type_pointer(param_type);
        // Don't free param_type - the pointer type now owns it
        param_type = ptr;
        param->data.parameter.param_type = param_type;
    }
    
    return param;
}

// Parse function body
static ASTNode *parse_function_body(Parser *p) {
    // Already at LBRACE
    ASTNode *node = ast_create(AST_COMPOUND_STMT);
    node->data.compound.stmts = list_create();
    advance_p(p); // consume LBRACE
    
    while (!check_p(p, TOKEN_RBRACE) && !check_p(p, TOKEN_EOF)) {
        if (is_type(p)) {
            ASTNode *decl = parse_declaration(p);
            // Handle compound statement wrapper for multiple declarators
            if (decl && decl->type == AST_COMPOUND_STMT) {
                // Unwrap - add each declaration to function scope
                for (size_t i = 0; i < list_size(decl->data.compound.stmts); i++) {
                    list_push(node->data.compound.stmts, list_get(decl->data.compound.stmts, i));
                }
                // Free the wrapper but not the children
                decl->data.compound.stmts = NULL;
                ast_free(decl);
            } else {
                list_push(node->data.compound.stmts, decl);
            }
        } else {
            list_push(node->data.compound.stmts, parse_statement(p));
        }
    }
    
    expect(p, TOKEN_RBRACE, "expected '}'");
    advance_p(p); // consume RBRACE
    return node;
}

// Parse function definition
static ASTNode *parse_function_definition(Parser *p, Type *return_type, const char *name) {
    ASTNode *func = ast_create(AST_FUNCTION_DECL);
    func->data.function.name = xstrdup(name);
    func->data.function.func_type = return_type;
    func->data.function.params = list_create();
    
    // Parse parameters (already consumed '(')
    while (!check_p(p, TOKEN_RPAREN) && !check_p(p, TOKEN_EOF)) {
        // Check for variadic argument ...
        if (check_p(p, TOKEN_ELLIPSIS)) {
            advance_p(p);  // consume ...
            // Mark function as variadic
            if (func->data.function.func_type) {
                func->data.function.func_type->is_variadic = true;
            }
            break;
        }
        ASTNode *param = parse_parameter(p);
        
        // In C, (void) means no parameters, not a parameter of type void
        // Check if this is a void parameter with no name
        if (param->data.parameter.param_type && 
            param->data.parameter.param_type->kind == TYPE_VOID &&
            list_size(func->data.function.params) == 0 &&
            (!param->data.parameter.name || param->data.parameter.name[0] == '\0')) {
            // This is (void) - skip adding this parameter
            // Free the parameter node
            free((void*)param->data.parameter.name);
            free(param);
            break;
        }
        
        list_push(func->data.function.params, param);
        if (check_p(p, TOKEN_COMMA)) {
            advance_p(p);
        } else {
            break;
        }
    }
    expect(p, TOKEN_RPAREN, "expected ')'");
    advance_p(p); // consume RPAREN
    
    // Skip __attribute__((...)) if present
    skip_attribute(p);
    
    // Check for function body
    if (check_p(p, TOKEN_LBRACE)) {
        func->data.function.body = parse_function_body(p);
    }
    return func;
}

// Parse declaration
static ASTNode *parse_declaration(Parser *p) {
    // Handle storage class specifiers (extern, static)
    if (check_p(p, TOKEN_EXTERN)) {
        advance_p(p); // consume 'extern'
        // Parse the type and function name
        Type *base_type = parse_type(p);
        
        if (check_p(p, TOKEN_IDENTIFIER)) {
            char *name = token_name(p);
            advance_p(p);
            
            // Check if it's a function declaration
            if (check_p(p, TOKEN_LPAREN)) {
                // Skip to matching RPAREN
                advance_p(p); // consume '('
                int depth = 1;
                while (depth > 0 && !check_p(p, TOKEN_EOF)) {
                    if (check_p(p, TOKEN_LPAREN)) depth++;
                    else if (check_p(p, TOKEN_RPAREN)) depth--;
                    advance_p(p);
                }
            }
            
            free(name);
        }
        
        type_free(base_type);
        
        // Skip to semicolon
        while (!check_p(p, TOKEN_SEMICOLON) && !check_p(p, TOKEN_EOF)) {
            advance_p(p);
        }
        if (check_p(p, TOKEN_SEMICOLON)) advance_p(p);
        return ast_create(AST_NULL_STMT);
    }
    
    // Handle storage class specifiers and function specifiers
    bool is_static = false;
    bool is_inline = false;
    
    // Handle inline keyword (can appear before or after static)
    if (check_p(p, TOKEN_INLINE)) {
        advance_p(p); // consume 'inline'
        is_inline = true;
    }
    
    // Handle static keyword
    if (check_p(p, TOKEN_STATIC)) {
        advance_p(p); // consume 'static'
        is_static = true;
        // Handle inline after static: static inline
        if (check_p(p, TOKEN_INLINE)) {
            advance_p(p); // consume 'inline'
            is_inline = true;
        }
    }
    
    // Skip inline if it appears again (e.g., inline static - non-standard but handle it)
    if (!is_inline && check_p(p, TOKEN_INLINE)) {
        advance_p(p); // consume 'inline'
        is_inline = true;
    }
    
    // Handle typedef
    if (check_p(p, TOKEN_TYPEDEF)) {
        advance_p(p);  // consume 'typedef'
        // Parse the underlying type
        Type *base_type = parse_type(p);
        
        // Handle function pointer typedef: typedef ret (*name)(params);
        // Or regular typedef: typedef int name;
        char *typedef_name = NULL;
        
        if (check_p(p, TOKEN_LPAREN)) {
            // Could be function pointer: (*name)(params)
            advance_p(p);  // consume '('
            if (check_p(p, TOKEN_STAR)) {
                advance_p(p);  // consume '*'
                if (check_p(p, TOKEN_IDENTIFIER)) {
                    typedef_name = token_name(p);
                    advance_p(p);  // consume name
                }
                // Skip to matching ')'
                if (check_p(p, TOKEN_RPAREN)) advance_p(p);  // consume ')'
                // Skip function params: (params)
                if (check_p(p, TOKEN_LPAREN)) {
                    advance_p(p);  // consume '('
                    int depth = 1;
                    while (depth > 0 && !check_p(p, TOKEN_EOF)) {
                        if (check_p(p, TOKEN_LPAREN)) depth++;
                        else if (check_p(p, TOKEN_RPAREN)) depth--;
                        advance_p(p);
                    }
                }
            } else {
                // Regular parenthesized declarator - skip to find name
                int depth = 1;
                while (depth > 0 && !check_p(p, TOKEN_EOF)) {
                    if (check_p(p, TOKEN_LPAREN)) depth++;
                    else if (check_p(p, TOKEN_RPAREN)) depth--;
                    else if (check_p(p, TOKEN_IDENTIFIER) && depth == 1) {
                        if (!typedef_name) {
                            typedef_name = token_name(p);
                        }
                    }
                    advance_p(p);
                }
            }
        } else if (check_p(p, TOKEN_IDENTIFIER)) {
            typedef_name = token_name(p);
            advance_p(p);
        }
        
        if (typedef_name) {
            // Register the typedef with full type info
            add_typedef_full(typedef_name, base_type);
            free(typedef_name);
        }
        
        // Don't free base_type - it's now stored in the typedef entry
        
        expect(p, TOKEN_SEMICOLON, "expected ';'");
        advance_p(p);
        return ast_create(AST_TYPEDEF_DECL);
    }
    
    // Parse the type (this now handles struct/union types properly)
    Type *base_type = parse_type(p);
    
    // If we just parsed a struct definition (no variable name), check for variable
    // The type parser consumed the struct definition, now we need the variable name
    
    // Get identifier
    char *name = NULL;
    
    // Check for function pointer declarator: (*name)(params) or *name(params)
    // This must be checked BEFORE looking for a simple identifier
    if (check_p(p, TOKEN_LPAREN)) {
        // Might be a function pointer: (*name)(params)
        // Save position and try to parse it
        size_t saved_pos = p->lexer->position;
        int saved_line = p->lexer->line;
        int saved_col = p->lexer->column;
        Token saved_previous = p->previous;
        Token saved_current = p->current;
        
        advance_p(p); // consume '('
        
        if (check_p(p, TOKEN_STAR)) {
            // This is a function pointer: (*name)(params)
            advance_p(p); // consume '*'
            if (check_p(p, TOKEN_IDENTIFIER)) {
                name = token_name(p);
                advance_p(p); // consume name
                
                // Expect ')' followed by '(' for parameter list
                if (check_p(p, TOKEN_RPAREN)) {
                    advance_p(p); // consume ')'
                    // We've consumed (*name) - now we have the function pointer type
                    // Create the function type with base_type as return type
                    Type *func_type = type_create(TYPE_FUNCTION);
                    func_type->return_type = base_type;
                    func_type->param_types = NULL;
                    func_type->num_params = 0;
                    func_type->is_variadic = false;
                    
                    // Parse parameter list if present: (params)
                    if (check_p(p, TOKEN_LPAREN)) {
                        advance_p(p); // consume '('
                        
                        // Parse parameters
                        Type **params = NULL;
                        size_t num_params = 0;
                        size_t param_capacity = 0;
                        
                        while (!check_p(p, TOKEN_RPAREN) && !check_p(p, TOKEN_EOF)) {
                            // Skip ellipsis for now
                            if (check_p(p, TOKEN_ELLIPSIS)) {
                                func_type->is_variadic = true;
                                advance_p(p);
                                break;
                            }
                            
                            Type *p_type = parse_type(p);
                            if (!p_type) {
                                p_type = type_create(TYPE_INT);
                            }
                            
                            // Skip pointer modifiers
                            while (check_p(p, TOKEN_STAR)) {
                                advance_p(p);
                                Type *ptr = type_pointer(p_type);
                                p_type = ptr;
                            }
                            
                            // Skip parameter name if present (for abstract declarators)
                            if (check_p(p, TOKEN_IDENTIFIER)) {
                                advance_p(p);
                            }
                            
                            // Skip array brackets
                            while (check_p(p, TOKEN_LBRACKET)) {
                                advance_p(p);
                                while (!check_p(p, TOKEN_RBRACKET) && !check_p(p, TOKEN_EOF)) advance_p(p);
                                if (check_p(p, TOKEN_RBRACKET)) advance_p(p);
                                Type *ptr = type_pointer(p_type);
                                p_type = ptr;
                            }
                            
                            // Add to parameter list
                            if (num_params >= param_capacity) {
                                param_capacity = param_capacity ? param_capacity * 2 : 4;
                                params = realloc(params, sizeof(Type*) * param_capacity);
                            }
                            params[num_params++] = p_type;
                            
                            if (check_p(p, TOKEN_COMMA)) {
                                advance_p(p);
                            }
                        }
                        
                        if (check_p(p, TOKEN_RPAREN)) {
                            advance_p(p); // consume ')'
                        }
                        
                        func_type->param_types = params;
                        func_type->num_params = num_params;
                    }
                    
                    // Function pointer type is complete
                    // IMPORTANT: A function pointer is a POINTER to a FUNCTION type
                    Type *func_ptr_type = type_pointer(func_type);
                    
                    // This is a variable declaration with function pointer type
                    // Check for initializer or semicolon
                    ASTNode *decl = ast_create(AST_VARIABLE_DECL);
                    decl->data.variable.var_type = func_ptr_type;
                    decl->data.variable.name = name;
                    decl->data.variable.init = NULL;
                    
                    // Initializer
                    if (check_p(p, TOKEN_ASSIGN)) {
                        advance_p(p);
                        decl->data.variable.init = parse_assignment_expr(p);
                    }
                    
                    // Expect semicolon
                    expect(p, TOKEN_SEMICOLON, "expected ';'");
                    advance_p(p);
                    
                    return decl;
                }
            }
        }
        
        // Not a function pointer - restore position
        p->lexer->position = saved_pos;
        p->lexer->line = saved_line;
        p->lexer->column = saved_col;
        p->previous = saved_previous;
        p->current = saved_current;
    }
    
    // Check for simple identifier
    if (check_p(p, TOKEN_IDENTIFIER)) {
        // Use token length to copy exactly the right number of characters
        name = token_name(p);
        advance_p(p);
    } else {
        name = xstrdup("");
    }
    
    // Skip __attribute__((...)) if present
    skip_attribute(p);
    
    // Check for function definition
    if (check_p(p, TOKEN_LPAREN)) {
        advance_p(p); // consume '('
        ASTNode *func = parse_function_definition(p, base_type, name);
        func->is_static = is_static;  // Preserve static storage class
        free(name);
        return func;
    }
    
    // Handle array declarator: int arr[5]
    while (check_p(p, TOKEN_LBRACKET)) {
        advance_p(p);  // consume '['
        // Skip the array size expression (we don't support it yet, just skip to ']')
        while (!check_p(p, TOKEN_RBRACKET) && !check_p(p, TOKEN_EOF)) {
            advance_p(p);
        }
        if (check_p(p, TOKEN_RBRACKET)) advance_p(p);  // consume ']'
        // Convert to pointer type
        Type *ptr = type_pointer(base_type);
        base_type = ptr;
    }
    
    // Variable declaration
    ASTNode *decl = ast_create(AST_VARIABLE_DECL);
    decl->is_static = is_static;  // Preserve static storage class
    decl->data.variable.var_type = base_type;
    decl->data.variable.name = name;
    decl->data.variable.init = NULL;
    
    // Initializer
    if (check_p(p, TOKEN_ASSIGN)) {
        advance_p(p);
        if (check_p(p, TOKEN_LBRACE)) {
            // Parse initializer list: { expr1, expr2, ... }
            advance_p(p);  // consume '{'
            ASTNode *init_list = ast_create(AST_INITIALIZER_LIST);
            init_list->data.init_list.elements = list_create();
            while (!check_p(p, TOKEN_RBRACE) && !check_p(p, TOKEN_EOF)) {
                // Parse an element (could be a nested initializer list)
                if (check_p(p, TOKEN_LBRACE)) {
                    // Nested initializer - for now, skip it
                    advance_p(p);  // consume '{'
                    int depth = 1;
                    while (depth > 0 && !check_p(p, TOKEN_EOF)) {
                        if (check_p(p, TOKEN_LBRACE)) depth++;
                        else if (check_p(p, TOKEN_RBRACE)) depth--;
                        advance_p(p);
                    }
                } else {
                    ASTNode *elem = parse_assignment_expr(p);
                    if (elem) list_push(init_list->data.init_list.elements, elem);
                }
                if (check_p(p, TOKEN_COMMA)) advance_p(p);
            }
            if (check_p(p, TOKEN_RBRACE)) advance_p(p);  // consume '}'
            decl->data.variable.init = init_list;
            
            // Set array size from initializer count if this is an array
            if (base_type && base_type->kind == TYPE_ARRAY && base_type->array_size == 0) {
                base_type->array_size = list_size(init_list->data.init_list.elements);
                // Recompute size
                if (base_type->base) {
                    base_type->size = base_type->array_size * base_type->base->size;
                }
            }
        } else {
            decl->data.variable.init = parse_assignment_expr(p);
        }
    }
    
    // Handle multiple declarators: int a, b, c;
    // Check for comma before semicolon
    if (check_p(p, TOKEN_COMMA)) {
        // Create a compound statement to hold multiple declarations
        ASTNode *compound = ast_create(AST_COMPOUND_STMT);
        compound->data.compound.stmts = list_create();
        list_push(compound->data.compound.stmts, decl);
        
        while (check_p(p, TOKEN_COMMA)) {
            advance_p(p);  // consume ','
            
            // Parse next declarator - handle pointer declarator
            Type *next_type = type_copy(base_type);
            
            // Handle pointer declarator(s) for subsequent variables
            while (check_p(p, TOKEN_STAR)) {
                advance_p(p);  // consume '*'
                Type *ptr = type_pointer(next_type);
                next_type = ptr;
            }
            
            // Parse name
            char *next_name = NULL;
            if (check_p(p, TOKEN_IDENTIFIER)) {
                next_name = token_name(p);
                advance_p(p);
            } else {
                next_name = xstrdup("");
            }
            
            // Handle array declarator for subsequent variables
            while (check_p(p, TOKEN_LBRACKET)) {
                advance_p(p);  // consume '['
                // Skip the array size expression
                while (!check_p(p, TOKEN_RBRACKET) && !check_p(p, TOKEN_EOF)) {
                    advance_p(p);
                }
                if (check_p(p, TOKEN_RBRACKET)) advance_p(p);  // consume ']'
                // Convert to pointer type
                Type *ptr = type_pointer(next_type);
                next_type = ptr;
            }
            
            // Skip __attribute__ if present
            skip_attribute(p);
            
            // Create the declaration
            ASTNode *next_decl = ast_create(AST_VARIABLE_DECL);
            next_decl->data.variable.var_type = next_type;
            next_decl->data.variable.name = next_name;
            next_decl->data.variable.init = NULL;
            
            // Handle initializer
            if (check_p(p, TOKEN_ASSIGN)) {
                advance_p(p);
                next_decl->data.variable.init = parse_assignment_expr(p);
            }
            
            list_push(compound->data.compound.stmts, next_decl);
        }
        
        expect(p, TOKEN_SEMICOLON, "expected ';'");
        advance_p(p); // consume SEMICOLON
        
        return compound;
    }

    expect(p, TOKEN_SEMICOLON, "expected ';'");
    advance_p(p); // consume SEMICOLON
    return decl;
}

static ASTNode *parse_statement(Parser *p) {
    if (check_p(p, TOKEN_LBRACE)) {
        ASTNode *node = ast_create(AST_COMPOUND_STMT);
        node->data.compound.stmts = list_create();
        advance_p(p); // consume LBRACE
        while (!check_p(p, TOKEN_RBRACE) && !check_p(p, TOKEN_EOF)) {
            if (is_type(p)) {
                ASTNode *decl = parse_declaration(p);
                // Handle compound statement wrapper for multiple declarators
                if (decl && decl->type == AST_COMPOUND_STMT) {
                    // Unwrap - add each declaration to outer scope
                    for (size_t i = 0; i < list_size(decl->data.compound.stmts); i++) {
                        list_push(node->data.compound.stmts, list_get(decl->data.compound.stmts, i));
                    }
                    // Free the wrapper but not the children
                    decl->data.compound.stmts = NULL;
                    ast_free(decl);
                } else {
                    list_push(node->data.compound.stmts, decl);
                }
            } else {
                list_push(node->data.compound.stmts, parse_statement(p));
            }
        }
        expect(p, TOKEN_RBRACE, "expected '}'");
        advance_p(p); // consume RBRACE
        return node;
    }
    
    if (check_p(p, TOKEN_IF)) {
        advance_p(p); // consume IF
        expect(p, TOKEN_LPAREN, "expected '('");
        advance_p(p); // consume '('
        ASTNode *node = ast_create(AST_IF_STMT);
        node->data.if_stmt.condition = parse_expression(p);
        expect(p, TOKEN_RPAREN, "expected ')'");
        advance_p(p); // consume ')'
        node->data.if_stmt.then_stmt = parse_statement(p);
        if (check_p(p, TOKEN_ELSE)) {
            advance_p(p); // consume ELSE
            node->data.if_stmt.else_stmt = parse_statement(p);
        }
        return node;
    }
    
    if (check_p(p, TOKEN_WHILE)) {
        advance_p(p); // consume WHILE
        expect(p, TOKEN_LPAREN, "expected '('");
        advance_p(p); // consume '('
        ASTNode *node = ast_create(AST_WHILE_STMT);
        node->data.while_stmt.condition = parse_expression(p);
        expect(p, TOKEN_RPAREN, "expected ')'");
        advance_p(p); // consume ')'
        node->data.while_stmt.body = parse_statement(p);
        return node;
    }
    
    if (check_p(p, TOKEN_FOR)) {
        advance_p(p); // consume FOR
        expect(p, TOKEN_LPAREN, "expected '('");
        advance_p(p); // consume '('
        ASTNode *node = ast_create(AST_FOR_STMT);
        bool is_decl = is_type(p);
        if (!check_p(p, TOKEN_SEMICOLON))
            node->data.for_stmt.init = is_decl ? parse_declaration(p) : parse_expression(p);
        // If init was an expression, consume the semicolon. Declarations already consume it.
        if (!is_decl || check_p(p, TOKEN_SEMICOLON)) {
            expect(p, TOKEN_SEMICOLON, "expected ';'");
            advance_p(p); // consume ';'
        }
        if (!check_p(p, TOKEN_SEMICOLON))
            node->data.for_stmt.condition = parse_expression(p);
        expect(p, TOKEN_SEMICOLON, "expected ';'");
        advance_p(p); // consume ';'
        if (!check_p(p, TOKEN_RPAREN))
            node->data.for_stmt.increment = parse_expression(p);
        expect(p, TOKEN_RPAREN, "expected ')'");
        advance_p(p); // consume ')'
        node->data.for_stmt.body = parse_statement(p);
        return node;
    }
    
    if (check_p(p, TOKEN_SWITCH)) {
        advance_p(p); // consume SWITCH
        expect(p, TOKEN_LPAREN, "expected '('");
        advance_p(p); // consume '('
        ASTNode *node = ast_create(AST_SWITCH_STMT);
        node->data.switch_stmt.expr = parse_expression(p);
        expect(p, TOKEN_RPAREN, "expected ')'");
        advance_p(p); // consume ')'
        node->data.switch_stmt.body = parse_statement(p);
        return node;
    }
    
    if (check_p(p, TOKEN_RETURN)) {
        advance_p(p); // consume RETURN
        ASTNode *node = ast_create(AST_RETURN_STMT);
        if (!check_p(p, TOKEN_SEMICOLON))
            node->data.return_stmt.expr = parse_expression(p);
        expect(p, TOKEN_SEMICOLON, "expected ';'");
        advance_p(p); // consume ';'
        return node;
    }
    
    if (check_p(p, TOKEN_BREAK)) {
        advance_p(p); // consume BREAK
        expect(p, TOKEN_SEMICOLON, "expected ';'");
        advance_p(p); // consume ';'
        return ast_create(AST_BREAK_STMT);
    }
    
    if (check_p(p, TOKEN_CONTINUE)) {
        advance_p(p); // consume CONTINUE
        expect(p, TOKEN_SEMICOLON, "expected ';'");
        advance_p(p); // consume ';'
        return ast_create(AST_CONTINUE_STMT);
    }
    
    if (check_p(p, TOKEN_CASE)) {
        advance_p(p); // consume CASE
        ASTNode *node = ast_create(AST_CASE_STMT);
        node->data.case_stmt.case_expr = parse_expression(p);
        expect(p, TOKEN_COLON, "expected ':'");
        advance_p(p); // consume ':'
        node->data.case_stmt.stmt = parse_statement(p);
        return node;
    }
    
    if (check_p(p, TOKEN_DEFAULT)) {
        advance_p(p); // consume DEFAULT
        expect(p, TOKEN_COLON, "expected ':'");
        advance_p(p); // consume ':'
        ASTNode *node = ast_create(AST_DEFAULT_STMT);
        node->data.default_stmt.stmt = parse_statement(p);
        return node;
    }
    
    if (check_p(p, TOKEN_SEMICOLON)) {
        advance_p(p); // consume ';'
        return ast_create(AST_NULL_STMT);
    }
    
    ASTNode *expr = parse_expression(p);
    expect(p, TOKEN_SEMICOLON, "expected ';'");
    advance_p(p); // consume ';'
    ASTNode *stmt = ast_create(AST_EXPRESSION_STMT);
    stmt->data.expr_stmt.expr = expr;
    return stmt;
}

static ASTNode *parse_expression(Parser *p) {
    ASTNode *left = parse_assignment_expr(p);
    
    // Handle comma operator (lowest precedence)
    while (check_p(p, TOKEN_COMMA)) {
        advance_p(p);  // consume ','
        ASTNode *right = parse_assignment_expr(p);
        ASTNode *comma = ast_create(AST_COMMA_EXPR);
        comma->data.comma.left = left;
        comma->data.comma.right = right;
        left = comma;
    }
    
    return left;
}

static ASTNode *parse_assignment_expr(Parser *p) {
    ASTNode *left = parse_conditional_expr(p);
    
    if (check_p(p, TOKEN_ASSIGN)) {
        advance_p(p);
        ASTNode *node = ast_create(AST_ASSIGNMENT_EXPR);
        node->data.assignment.op = -1;  // Plain assignment (0 is OP_ADD)
        node->data.assignment.left = left;
        node->data.assignment.right = parse_assignment_expr(p);
        return node;
    }
    if (check_p(p, TOKEN_PLUS_EQ)) {
        advance_p(p);
        ASTNode *node = ast_create(AST_ASSIGNMENT_EXPR);
        node->data.assignment.op = OP_ADD;
        node->data.assignment.left = left;
        node->data.assignment.right = parse_assignment_expr(p);
        return node;
    }
    if (check_p(p, TOKEN_MINUS_EQ)) {
        advance_p(p);
        ASTNode *node = ast_create(AST_ASSIGNMENT_EXPR);
        node->data.assignment.op = OP_SUB;
        node->data.assignment.left = left;
        node->data.assignment.right = parse_assignment_expr(p);
        return node;
    }
    if (check_p(p, TOKEN_STAR_EQ)) {
        advance_p(p);
        ASTNode *node = ast_create(AST_ASSIGNMENT_EXPR);
        node->data.assignment.op = OP_MUL;
        node->data.assignment.left = left;
        node->data.assignment.right = parse_assignment_expr(p);
        return node;
    }
    if (check_p(p, TOKEN_SLASH_EQ)) {
        advance_p(p);
        ASTNode *node = ast_create(AST_ASSIGNMENT_EXPR);
        node->data.assignment.op = OP_DIV;
        node->data.assignment.left = left;
        node->data.assignment.right = parse_assignment_expr(p);
        return node;
    }
    if (check_p(p, TOKEN_PERCENT_EQ)) {
        advance_p(p);
        ASTNode *node = ast_create(AST_ASSIGNMENT_EXPR);
        node->data.assignment.op = OP_MOD;
        node->data.assignment.left = left;
        node->data.assignment.right = parse_assignment_expr(p);
        return node;
    }
    if (check_p(p, TOKEN_LSHIFT_EQ)) {
        advance_p(p);
        ASTNode *node = ast_create(AST_ASSIGNMENT_EXPR);
        node->data.assignment.op = OP_LSHIFT;
        node->data.assignment.left = left;
        node->data.assignment.right = parse_assignment_expr(p);
        return node;
    }
    if (check_p(p, TOKEN_RSHIFT_EQ)) {
        advance_p(p);
        ASTNode *node = ast_create(AST_ASSIGNMENT_EXPR);
        node->data.assignment.op = OP_RSHIFT;
        node->data.assignment.left = left;
        node->data.assignment.right = parse_assignment_expr(p);
        return node;
    }
    if (check_p(p, TOKEN_AMP_EQ)) {
        advance_p(p);
        ASTNode *node = ast_create(AST_ASSIGNMENT_EXPR);
        node->data.assignment.op = OP_BITAND;
        node->data.assignment.left = left;
        node->data.assignment.right = parse_assignment_expr(p);
        return node;
    }
    if (check_p(p, TOKEN_PIPE_EQ)) {
        advance_p(p);
        ASTNode *node = ast_create(AST_ASSIGNMENT_EXPR);
        node->data.assignment.op = OP_BITOR;
        node->data.assignment.left = left;
        node->data.assignment.right = parse_assignment_expr(p);
        return node;
    }
    if (check_p(p, TOKEN_CARET_EQ)) {
        advance_p(p);
        ASTNode *node = ast_create(AST_ASSIGNMENT_EXPR);
        node->data.assignment.op = OP_BITXOR;
        node->data.assignment.left = left;
        node->data.assignment.right = parse_assignment_expr(p);
        return node;
    }
    
    return left;
}

static ASTNode *parse_conditional_expr(Parser *p) {
    ASTNode *cond = parse_binary_expr(p, 0);
    
    if (check_p(p, TOKEN_QUESTION)) {
        advance_p(p);
        ASTNode *node = ast_create(AST_CONDITIONAL_EXPR);
        node->data.conditional.condition = cond;
        node->data.conditional.then_expr = parse_expression(p);
        expect(p, TOKEN_COLON, "expected ':'");
        advance_p(p);
        node->data.conditional.else_expr = parse_conditional_expr(p);
        return node;
    }
    
    return cond;
}

static int prec(TokenType t) {
    switch (t) {
        case TOKEN_PIPE_PIPE: return 4;
        case TOKEN_AMP_AMP: return 5;
        case TOKEN_PIPE: return 6;
        case TOKEN_CARET: return 7;
        case TOKEN_AMPERSAND: return 8;
        case TOKEN_EQ_EQ: case TOKEN_EXCLAIM_EQ: return 9;
        case TOKEN_LESS: case TOKEN_GREATER: case TOKEN_LESS_EQ: case TOKEN_GREATER_EQ: return 10;
        case TOKEN_LSHIFT: case TOKEN_RSHIFT: return 11;
        case TOKEN_PLUS: case TOKEN_MINUS: return 12;
        case TOKEN_STAR: case TOKEN_SLASH: case TOKEN_PERCENT: return 13;
        default: return -1;
    }
}

static BinaryOp binop(TokenType t) {
    switch (t) {
        case TOKEN_PLUS: return OP_ADD;
        case TOKEN_MINUS: return OP_SUB;
        case TOKEN_STAR: return OP_MUL;
        case TOKEN_SLASH: return OP_DIV;
        case TOKEN_PERCENT: return OP_MOD;
        case TOKEN_LSHIFT: return OP_LSHIFT;
        case TOKEN_RSHIFT: return OP_RSHIFT;
        case TOKEN_LESS: return OP_LT;
        case TOKEN_GREATER: return OP_GT;
        case TOKEN_LESS_EQ: return OP_LE;
        case TOKEN_GREATER_EQ: return OP_GE;
        case TOKEN_EQ_EQ: return OP_EQ;
        case TOKEN_EXCLAIM_EQ: return OP_NE;
        case TOKEN_AMPERSAND: return OP_BITAND;
        case TOKEN_CARET: return OP_BITXOR;
        case TOKEN_PIPE: return OP_BITOR;
        case TOKEN_AMP_AMP: return OP_AND;
        case TOKEN_PIPE_PIPE: return OP_OR;
        default: return 0;
    }
}

static ASTNode *parse_binary_expr(Parser *p, int min_prec) {
    ASTNode *left = parse_cast_expr(p);
    
    while (prec(peek_p(p)) >= min_prec) {
        BinaryOp op = binop(peek_p(p));
        int assoc_prec = prec(peek_p(p));
        advance_p(p);
        
        ASTNode *node = ast_create(AST_BINARY_EXPR);
        node->data.binary.op = op;
        node->data.binary.left = left;
        node->data.binary.right = parse_binary_expr(p, assoc_prec + 1);
        left = node;
    }
    
    return left;
}

static ASTNode *parse_cast_expr(Parser *p) {
    // Check for cast expression: (type)expr
    if (check_p(p, TOKEN_LPAREN)) {
        // Peek ahead to see if this looks like a cast
        // A cast has (type) where type is a type keyword or typedef name
        advance_p(p); // consume '('
        
        if (is_type(p)) {
            // This is a cast expression
            Type *t = parse_type(p);
            expect(p, TOKEN_RPAREN, "expected ')' after type in cast");
            advance_p(p); // consume ')'
            ASTNode *node = ast_create(AST_CAST_EXPR);
            node->data.cast.cast_type = t;
            node->data.cast.operand = parse_cast_expr(p);
            return node;
        }
        
        // Not a cast - this is a parenthesized expression
        // We already consumed '(', so parse the expression inside
        ASTNode *expr = parse_expression(p);
        expect(p, TOKEN_RPAREN, "expected ')'");
        advance_p(p); // consume ')'
        // After a parenthesized expression, we need to check for postfix operators
        // Call parse_postfix_expr to handle ->, ., [], etc.
        return parse_postfix_expr_with_expr(p, expr);
    }
    return parse_unary_expr(p);
}

static ASTNode *parse_unary_expr(Parser *p) {
    // Handle sizeof
    if (check_p(p, TOKEN_SIZEOF)) {
        advance_p(p);  // consume 'sizeof'
        ASTNode *node = ast_create(AST_SIZEOF_EXPR);
        
        if (check_p(p, TOKEN_LPAREN)) {
            // Could be sizeof(type) or sizeof(expr)
            advance_p(p);  // consume '('
            if (is_type(p)) {
                // sizeof(type)
                Type *t = parse_type(p);
                node->data.sizeof_expr.sizeof_type = t;
                node->data.sizeof_expr.sizeof_expr = NULL;
                expect(p, TOKEN_RPAREN, "expected ')' after type in sizeof");
                advance_p(p);  // consume ')'
            } else {
                // sizeof(expr)
                node->data.sizeof_expr.sizeof_type = NULL;
                node->data.sizeof_expr.sizeof_expr = parse_expression(p);
                expect(p, TOKEN_RPAREN, "expected ')' after expression in sizeof");
                advance_p(p);  // consume ')'
            }
        } else {
            // sizeof expr without parens
            node->data.sizeof_expr.sizeof_type = NULL;
            node->data.sizeof_expr.sizeof_expr = parse_unary_expr(p);
        }
        return node;
    }
    
    if (check_p(p, TOKEN_PLUS_PLUS)) {
        advance_p(p);
        ASTNode *node = ast_create(AST_UNARY_EXPR);
        node->data.unary.op = 0;
        node->data.unary.operand = parse_unary_expr(p);
        return node;
    }
    if (check_p(p, TOKEN_MINUS_MINUS)) {
        advance_p(p);
        ASTNode *node = ast_create(AST_UNARY_EXPR);
        node->data.unary.op = 1;
        node->data.unary.operand = parse_unary_expr(p);
        return node;
    }
    if (check_p(p, TOKEN_STAR)) {
        advance_p(p);
        ASTNode *node = ast_create(AST_UNARY_EXPR);
        node->data.unary.op = 4;
        node->data.unary.operand = parse_postfix_expr(p);
        return node;
    }
    if (check_p(p, TOKEN_AMPERSAND)) {
        advance_p(p);
        ASTNode *node = ast_create(AST_UNARY_EXPR);
        node->data.unary.op = 5;
        node->data.unary.operand = parse_postfix_expr(p);
        return node;
    }
    if (check_p(p, TOKEN_MINUS)) {
        advance_p(p);
        ASTNode *node = ast_create(AST_UNARY_EXPR);
        node->data.unary.op = 1;  // Unary minus
        node->data.unary.operand = parse_postfix_expr(p);
        return node;
    }
    if (check_p(p, TOKEN_PLUS)) {
        advance_p(p);
        ASTNode *node = ast_create(AST_UNARY_EXPR);
        node->data.unary.op = 0;  // Unary plus
        node->data.unary.operand = parse_postfix_expr(p);
        return node;
    }
    if (check_p(p, TOKEN_EXCLAIM)) {
        advance_p(p);
        ASTNode *node = ast_create(AST_UNARY_EXPR);
        node->data.unary.op = 2;
        node->data.unary.operand = parse_postfix_expr(p);
        return node;
    }
    if (check_p(p, TOKEN_TILDE)) {
        advance_p(p);
        ASTNode *node = ast_create(AST_UNARY_EXPR);
        node->data.unary.op = 3;
        node->data.unary.operand = parse_postfix_expr(p);
        return node;
    }
    return parse_postfix_expr(p);
}

// Helper function: continue parsing postfix operators on an already-parsed expression
static ASTNode *parse_postfix_expr_with_expr(Parser *p, ASTNode *expr) {
    while (true) {
        if (check_p(p, TOKEN_LBRACKET)) {
            advance_p(p);
            ASTNode *node = ast_create(AST_ARRAY_SUBSCRIPT_EXPR);
            node->data.subscript.array = expr;
            node->data.subscript.index = parse_expression(p);
            expect(p, TOKEN_RBRACKET, "expected ']'");
            advance_p(p);
            expr = node;
        }
        else if (check_p(p, TOKEN_LPAREN)) {
            advance_p(p);
            // Check if callee is a builtin va function
            bool is_va_start = false;
            if (expr && expr->type == AST_IDENTIFIER_EXPR) {
                const char *name = expr->data.identifier.name;
                if (strcmp(name, "va_start") == 0 || strcmp(name, "__builtin_va_start") == 0) {
                    expr->data.identifier.name = strdup("___builtin_va_start");
                    is_va_start = true;
                } else if (strcmp(name, "va_end") == 0 || strcmp(name, "__builtin_va_end") == 0) {
                    expr->data.identifier.name = strdup("___builtin_va_end");
                } else if (strcmp(name, "va_copy") == 0 || strcmp(name, "__builtin_va_copy") == 0) {
                    expr->data.identifier.name = strdup("___builtin_va_copy");
                }
            }
            ASTNode *node = ast_create(AST_CALL_EXPR);
            node->data.call.callee = expr;
            node->data.call.args = list_create();
            int arg_idx = 0;
            while (!check_p(p, TOKEN_RPAREN) && !check_p(p, TOKEN_EOF)) {
                ASTNode *arg = parse_assignment_expr(p);
                // For va_start, the first argument must be the address-of the va_list
                if (is_va_start && arg_idx == 0 && arg->type == AST_IDENTIFIER_EXPR) {
                    // Wrap in address-of operator (unary op 5)
                    ASTNode *addr_of = ast_create(AST_UNARY_EXPR);
                    addr_of->data.unary.op = 5;  // Address-of operator
                    addr_of->data.unary.operand = arg;
                    arg = addr_of;
                }
                list_push(node->data.call.args, arg);
                arg_idx++;
                if (check_p(p, TOKEN_COMMA)) advance_p(p);
                else break;
            }
            expect(p, TOKEN_RPAREN, "expected ')'");
            advance_p(p);
            expr = node;
        }
        else if (check_p(p, TOKEN_DOT)) {
            advance_p(p);
            ASTNode *node = ast_create(AST_MEMBER_ACCESS_EXPR);
            node->data.member.expr = expr;
            expect(p, TOKEN_IDENTIFIER, "expected member name");
            node->data.member.member = token_name(p);
            advance_p(p);
            node->data.member.is_arrow = false;
            expr = node;
        }
        else if (check_p(p, TOKEN_ARROW)) {
            advance_p(p);
            ASTNode *node = ast_create(AST_POINTER_MEMBER_ACCESS_EXPR);
            node->data.member.expr = expr;
            expect(p, TOKEN_IDENTIFIER, "expected member name");
            node->data.member.member = token_name(p);
            advance_p(p);
            node->data.member.is_arrow = true;
            expr = node;
        }
        else if (check_p(p, TOKEN_PLUS_PLUS)) {
            advance_p(p);
            ASTNode *node = ast_create(AST_UNARY_EXPR);
            node->data.unary.op = 6;
            node->data.unary.operand = expr;
            expr = node;
        }
        else if (check_p(p, TOKEN_MINUS_MINUS)) {
            advance_p(p);
            ASTNode *node = ast_create(AST_UNARY_EXPR);
            node->data.unary.op = 7;
            node->data.unary.operand = expr;
            expr = node;
        }
        else break;
    }
    
    return expr;
}

static ASTNode *parse_postfix_expr(Parser *p) {
    ASTNode *expr = parse_primary_expr(p);
    
    while (true) {
        if (check_p(p, TOKEN_LBRACKET)) {
            advance_p(p);
            ASTNode *node = ast_create(AST_ARRAY_SUBSCRIPT_EXPR);
            node->data.subscript.array = expr;
            node->data.subscript.index = parse_expression(p);
            expect(p, TOKEN_RBRACKET, "expected ']'");
            advance_p(p);
            expr = node;
        }
        else if (check_p(p, TOKEN_LPAREN)) {
            advance_p(p);
            // Check if callee is a builtin va function
            bool is_va_start = false;
            if (expr && expr->type == AST_IDENTIFIER_EXPR) {
                const char *name = expr->data.identifier.name;
                if (strcmp(name, "va_start") == 0 || strcmp(name, "__builtin_va_start") == 0) {
                    expr->data.identifier.name = strdup("___builtin_va_start");
                    is_va_start = true;
                } else if (strcmp(name, "va_end") == 0 || strcmp(name, "__builtin_va_end") == 0) {
                    expr->data.identifier.name = strdup("___builtin_va_end");
                } else if (strcmp(name, "va_copy") == 0 || strcmp(name, "__builtin_va_copy") == 0) {
                    expr->data.identifier.name = strdup("___builtin_va_copy");
                }
            }
            ASTNode *node = ast_create(AST_CALL_EXPR);
            node->data.call.callee = expr;
            node->data.call.args = list_create();
            int arg_idx = 0;
            while (!check_p(p, TOKEN_RPAREN) && !check_p(p, TOKEN_EOF)) {
                ASTNode *arg = parse_assignment_expr(p);
                // For va_start, the first argument must be the address-of the va_list
                if (is_va_start && arg_idx == 0 && arg->type == AST_IDENTIFIER_EXPR) {
                    // Wrap in address-of operator (unary op 5)
                    ASTNode *addr_of = ast_create(AST_UNARY_EXPR);
                    addr_of->data.unary.op = 5;  // Address-of operator
                    addr_of->data.unary.operand = arg;
                    arg = addr_of;
                }
                list_push(node->data.call.args, arg);
                arg_idx++;
                if (check_p(p, TOKEN_COMMA)) advance_p(p);
                else break;
            }
            expect(p, TOKEN_RPAREN, "expected ')'");
            advance_p(p);
            expr = node;
        }
        else if (check_p(p, TOKEN_DOT)) {
            advance_p(p);
            ASTNode *node = ast_create(AST_MEMBER_ACCESS_EXPR);
            node->data.member.expr = expr;
            expect(p, TOKEN_IDENTIFIER, "expected member name");
            node->data.member.member = token_name(p);
            advance_p(p);
            node->data.member.is_arrow = false;
            expr = node;
        }
        else if (check_p(p, TOKEN_ARROW)) {
            advance_p(p);
            ASTNode *node = ast_create(AST_POINTER_MEMBER_ACCESS_EXPR);
            node->data.member.expr = expr;
            expect(p, TOKEN_IDENTIFIER, "expected member name");
            node->data.member.member = token_name(p);
            advance_p(p);
            node->data.member.is_arrow = true;
            expr = node;
        }
        else if (check_p(p, TOKEN_PLUS_PLUS)) {
            advance_p(p);
            ASTNode *node = ast_create(AST_UNARY_EXPR);
            node->data.unary.op = 6;
            node->data.unary.operand = expr;
            expr = node;
        }
        else if (check_p(p, TOKEN_MINUS_MINUS)) {
            advance_p(p);
            ASTNode *node = ast_create(AST_UNARY_EXPR);
            node->data.unary.op = 7;
            node->data.unary.operand = expr;
            expr = node;
        }
        else break;
    }
    
    return expr;
}

static ASTNode *parse_primary_expr(Parser *p) {
    if (check_p(p, TOKEN_IDENTIFIER)) {
        char *name = token_name(p);
        
        // Check if this is NULL
        if (strcmp(name, "NULL") == 0) {
            free(name);
            advance_p(p);
            // NULL is the null pointer constant - treat as integer 0
            ASTNode *node = ast_create(AST_INTEGER_LITERAL_EXPR);
            node->data.int_literal.value = 0;
            return node;
        }
        
        // Check if this is an enum constant
        EnumConstant *ec = find_enum_constant(name);
        if (ec) {
            free(name);
            advance_p(p);
            ASTNode *node = ast_create(AST_INTEGER_LITERAL_EXPR);
            node->data.int_literal.value = ec->value;
            return node;
        }
        
        // Check if this is a builtin constant (true, false)
        if (strcmp(name, "true") == 0) {
            free(name);
            advance_p(p);
            ASTNode *node = ast_create(AST_INTEGER_LITERAL_EXPR);
            node->data.int_literal.value = 1;
            return node;
        }
        if (strcmp(name, "false") == 0) {
            free(name);
            advance_p(p);
            ASTNode *node = ast_create(AST_INTEGER_LITERAL_EXPR);
            node->data.int_literal.value = 0;
            return node;
        }
        
        // Regular identifier (including stdin, stdout, stderr as global variables)
        ASTNode *node = ast_create(AST_IDENTIFIER_EXPR);
        node->data.identifier.name = name;
        advance_p(p);
        return node;
    }
    
    if (check_p(p, TOKEN_INT_CONSTANT) || check_p(p, TOKEN_NUMBER)) {
        ASTNode *node = ast_create(AST_INTEGER_LITERAL_EXPR);
        node->data.int_literal.value = p->current.value.int_val;
        advance_p(p);
        return node;
    }
    
    if (check_p(p, TOKEN_FLOAT_CONSTANT)) {
        ASTNode *node = ast_create(AST_FLOAT_LITERAL_EXPR);
        node->data.float_literal.value = p->current.value.float_val;
        advance_p(p);
        return node;
    }
    
    if (check_p(p, TOKEN_STRING_LITERAL)) {
        ASTNode *node = ast_create(AST_STRING_LITERAL_EXPR);
        node->data.string_literal.value = token_name(p);
        advance_p(p);
        return node;
    }
    
    if (check_p(p, TOKEN_CHAR_CONSTANT)) {
        ASTNode *node = ast_create(AST_INTEGER_LITERAL_EXPR);
        node->data.int_literal.value = p->current.value.int_val;
        advance_p(p);
        return node;
    }
    
    if (check_p(p, TOKEN_LPAREN)) {
        advance_p(p);
        ASTNode *expr = parse_expression(p);
        expect(p, TOKEN_RPAREN, "expected ')'");
        advance_p(p);
        return expr;
    }
    
    return ast_create(AST_INTEGER_LITERAL_EXPR);
}

static void expect(Parser *p, TokenType t, const char *msg) {
    if (check_p(p, t)) return;
    fprintf(stderr, "error: [%d:%d] %s\n", p->current.line, p->current.column, msg);
}

Parser *parser_create(Lexer *lexer) {
    Parser *p = calloc(1, sizeof(Parser));
    p->lexer = lexer;
    p->translation_unit = ast_create(AST_TRANSLATION_UNIT);
    p->translation_unit->data.unit.declarations = list_create();
    advance_p(p); // Get first token
    return p;
}

void parser_destroy(Parser *p) {
    if (p) {
        lexer_destroy(p->lexer);
        ast_free(p->translation_unit);
        free(p);
    }
}

ASTNode *parse(Parser *p) {
    while (!check_p(p, TOKEN_EOF)) {
        // Skip error tokens
        while (check_p(p, TOKEN_ERROR)) {
            advance_p(p);
        }
        if (check_p(p, TOKEN_EOF)) break;

        if (is_type(p)) {
            ASTNode *decl = parse_declaration(p);
            if (decl) list_push(p->translation_unit->data.unit.declarations, decl);
        } else if (check_p(p, TOKEN_IDENTIFIER)) {
            // Check if this is an implicit int function definition: name() { ... }
            // Save position and check if identifier is followed by '('
            size_t saved_pos = p->lexer->position;
            int saved_line = p->lexer->line;
            int saved_col = p->lexer->column;
            
            char *name = token_name(p);
            advance_p(p);  // consume identifier
            
            if (check_p(p, TOKEN_LPAREN)) {
                // This is a function definition with implicit int return type
                advance_p(p); // consume '('
                Type *int_type = type_create(TYPE_INT);
                ASTNode *func = parse_function_definition(p, int_type, name);
                free(name);
                if (func) list_push(p->translation_unit->data.unit.declarations, func);
            } else {
                // Not a function definition, restore position and parse as expression
                p->lexer->position = saved_pos;
                p->lexer->line = saved_line;
                p->lexer->column = saved_col;
                free(name);
                
                ASTNode *stmt = parse_statement(p);
                if (stmt) list_push(p->translation_unit->data.unit.declarations, stmt);
            }
        } else {
            ASTNode *stmt = parse_statement(p);
            if (stmt) list_push(p->translation_unit->data.unit.declarations, stmt);
        }
    }
    return p->translation_unit;
}

// Debug function - can be called from is_type
__attribute__((unused))
static void debug_typedef_check(const char *name) {
    fprintf(stderr, "DEBUG: Checking typedef '%s'\n", name);
    TypedefEntry *e = find_typedef(name);
    if (e) {
        fprintf(stderr, "DEBUG: Found typedef '%s' with kind %d\n", e->name, e->kind);
    } else {
        fprintf(stderr, "DEBUG: Typedef '%s' NOT FOUND\n", name);
        fprintf(stderr, "DEBUG: Current typedefs:\n");
        for (TypedefEntry *t = typedef_list; t; t = t->next) {
            fprintf(stderr, "  - %s\n", t->name);
        }
    }
}
