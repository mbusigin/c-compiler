#include "symtab.h"
#include "../common/util.h"
#include <stdlib.h>
#include <string.h>

#define HASH_SIZE 256

// Simple hash function
static unsigned int hash(const char *name) {
    unsigned int hash = 5381;
    int c;
    while ((c = *name++)) {
        hash = ((hash << 5) + hash) + c;  // hash * 33 + c
    }
    return hash % HASH_SIZE;
}

SymbolTable *symtab_create(void) {
    SymbolTable *table = calloc(1, sizeof(SymbolTable));
    table->buckets = calloc(HASH_SIZE, sizeof(Symbol*));
    table->num_buckets = HASH_SIZE;
    table->current_scope = 0;
    table->parent = NULL;
    return table;
}

SymbolTable *symtab_create_child(SymbolTable *parent) {
    SymbolTable *table = symtab_create();
    table->parent = parent;
    table->current_scope = parent->current_scope + 1;
    return table;
}

void symtab_destroy(SymbolTable *table) {
    if (!table) return;
    
    // Free all symbols
    for (size_t i = 0; i < table->num_buckets; i++) {
        Symbol *sym = table->buckets[i];
        while (sym) {
            Symbol *next = sym->next;
            free(sym->name);
            free(sym);
            sym = next;
        }
    }
    
    free(table->buckets);
    free(table);
}

Symbol *symtab_add(SymbolTable *table, const char *name, SymbolKind kind, Type *type) {
    // Check if already defined in current scope
    Symbol *existing = symtab_lookup_current_scope(table, name);
    if (existing) {
        return NULL;  // Already defined
    }
    
    Symbol *sym = calloc(1, sizeof(Symbol));
    sym->name = xstrdup(name);
    sym->kind = kind;
    sym->type = type;
    sym->scope_level = table->current_scope;
    sym->is_defined = false;
    sym->is_used = false;
    
    unsigned int idx = hash(name);
    sym->next = table->buckets[idx];
    table->buckets[idx] = sym;
    
    return sym;
}

Symbol *symtab_lookup(SymbolTable *table, const char *name) {
    SymbolTable *current = table;
    while (current) {
        unsigned int idx = hash(name);
        Symbol *sym = current->buckets[idx];
        while (sym) {
            if (strcmp(sym->name, name) == 0) {
                return sym;
            }
            sym = sym->next;
        }
        current = current->parent;
    }
    return NULL;
}

Symbol *symtab_lookup_current_scope(SymbolTable *table, const char *name) {
    unsigned int idx = hash(name);
    Symbol *sym = table->buckets[idx];
    while (sym) {
        if (strcmp(sym->name, name) == 0 && sym->scope_level == table->current_scope) {
            return sym;
        }
        sym = sym->next;
    }
    return NULL;
}

void symtab_enter_scope(SymbolTable *table) {
    table->current_scope++;
}

void symtab_exit_scope(SymbolTable *table) {
    if (table->current_scope > 0) {
        table->current_scope--;
        // Remove symbols from current scope
        for (size_t i = 0; i < table->num_buckets; i++) {
            Symbol **p = &table->buckets[i];
            while (*p && (*p)->scope_level > table->current_scope) {
                Symbol *old = *p;
                *p = (*p)->next;
                free(old->name);
                free(old);
            }
        }
    }
}

int symtab_get_scope_level(SymbolTable *table) {
    return table->current_scope;
}
