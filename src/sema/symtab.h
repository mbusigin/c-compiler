#ifndef SYMTAB_H
#define SYMTAB_H

#include "../parser/ast.h"
#include "../common/list.h"
#include <stdbool.h>

// Symbol kinds
typedef enum {
    SYMBOL_VARIABLE,
    SYMBOL_FUNCTION,
    SYMBOL_PARAMETER,
    SYMBOL_TYPE,
    SYMBOL_ENUM_CONSTANT,
    SYMBOL_LABEL
} SymbolKind;

// Symbol information
typedef struct Symbol {
    char *name;
    SymbolKind kind;
    Type *type;
    int scope_level;
    bool is_defined;
    bool is_used;
    bool is_static;  // True if declared with 'static' keyword
    struct Symbol *next;  // For hash table chaining
} Symbol;

// Symbol table with scopes
typedef struct SymbolTable {
    Symbol **buckets;
    size_t num_buckets;
    int current_scope;
    struct SymbolTable *parent;
} SymbolTable;

// Symbol table functions
SymbolTable *symtab_create(void);
SymbolTable *symtab_create_child(SymbolTable *parent);
void symtab_destroy(SymbolTable *table);

Symbol *symtab_add(SymbolTable *table, const char *name, SymbolKind kind, Type *type);
Symbol *symtab_lookup(SymbolTable *table, const char *name);
Symbol *symtab_lookup_current_scope(SymbolTable *table, const char *name);
void symtab_enter_scope(SymbolTable *table);
void symtab_exit_scope(SymbolTable *table);

int symtab_get_scope_level(SymbolTable *table);

#endif // SYMTAB_H
