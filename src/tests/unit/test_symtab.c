/**
 * test_symtab.c - Symbol table unit tests
 */

#include "../../common/test_framework.h"
#include "../../common/util.h"
#include "../../sema/symtab.h"
#include "../../parser/ast.h"
#include <stdio.h>
#include <string.h>

static bool test_symtab_create(void) {
    TEST_START("Test Symbol Table Creation");
    
    SymbolTable *table = symtab_create();
    TEST_ASSERT(table != NULL, "Table should not be NULL");
    TEST_ASSERT(table->current_scope != NULL, "Should have global scope");
    TEST_ASSERT(table->current_scope->parent == NULL, "Global scope should have no parent");
    
    symtab_destroy(table);
    TEST_PASS();
    return true;
}

static bool test_symtab_insert_lookup(void) {
    TEST_START("Test Insert and Lookup");
    
    SymbolTable *table = symtab_create();
    
    Type *int_type = type_int();
    
    // Insert a symbol
    Symbol *sym = symtab_insert(table, "x", int_type, SYMBOL_VARIABLE);
    TEST_ASSERT(sym != NULL, "Insert should succeed");
    TEST_ASSERT_STR_EQ(sym->name, "x", "Symbol name should be 'x'");
    
    // Lookup the symbol
    Symbol *found = symtab_lookup(table, "x");
    TEST_ASSERT(found != NULL, "Lookup should find symbol");
    TEST_ASSERT(found == sym, "Found symbol should be same as inserted");
    
    symtab_destroy(table);
    TEST_PASS();
    return true;
}

static bool test_symtab_scope(void) {
    TEST_START("Test Scoping");
    
    SymbolTable *table = symtab_create();
    
    Type *int_type = type_int();
    Type *char_type = type_char();
    
    // Insert in global scope
    symtab_insert(table, "global_var", int_type, SYMBOL_VARIABLE);
    
    // Enter new scope
    symtab_enter_scope(table);
    
    // Insert in local scope
    symtab_insert(table, "local_var", char_type, SYMBOL_VARIABLE);
    
    // Should find both
    TEST_ASSERT(symtab_lookup(table, "global_var") != NULL, "Should find global var");
    TEST_ASSERT(symtab_lookup(table, "local_var") != NULL, "Should find local var");
    
    // Exit scope
    symtab_exit_scope(table);
    
    // Should only find global now
    TEST_ASSERT(symtab_lookup(table, "global_var") != NULL, "Should find global var");
    TEST_ASSERT(symtab_lookup(table, "local_var") == NULL, "Should NOT find local var (out of scope)");
    
    symtab_destroy(table);
    TEST_PASS();
    return true;
}

static bool test_symtab_redefinition(void) {
    TEST_START("Test Redefinition Prevention");
    
    SymbolTable *table = symtab_create();
    
    Type *int_type = type_int();
    Type *char_type = type_char();
    
    // Insert first definition
    Symbol *first = symtab_insert(table, "x", int_type, SYMBOL_VARIABLE);
    TEST_ASSERT(first != NULL, "First insert should succeed");
    
    // Try to insert again (should fail)
    Symbol *second = symtab_insert(table, "x", char_type, SYMBOL_VARIABLE);
    TEST_ASSERT(second == NULL, "Second insert should fail");
    
    symtab_destroy(table);
    TEST_PASS();
    return true;
}

int main(void) {
    TestCase tests[] = {
        {"Symbol Table Creation", test_symtab_create},
        {"Insert and Lookup", test_symtab_insert_lookup},
        {"Scoping", test_symtab_scope},
        {"Redefinition Prevention", test_symtab_redefinition}
    };
    
    int result = run_tests(tests, sizeof(tests) / sizeof(tests[0]));
    
    if (result == 0) {
        printf("\n*** All symbol table tests passed ***\n");
    }
    
    return result;
}
