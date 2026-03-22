/**
 * test_lexer.c - Lexer unit tests
 */

#include "../../common/test_framework.h"
#include "../../lexer/lexer.h"
#include <stdio.h>
#include <string.h>

static bool test_keywords(void) {
    TEST_START("Test Keywords");
    
    const char *keywords[] = {
        "auto", "break", "case", "char", "const", "continue",
        "default", "do", "double", "else", "enum", "extern",
        "float", "for", "goto", "if", "inline", "int",
        "long", "register", "restrict", "return", "short",
        "signed", "sizeof", "static", "struct", "switch",
        "typedef", "union", "unsigned", "void", "volatile", "while"
    };
    
    for (size_t i = 0; i < sizeof(keywords) / sizeof(keywords[0]); i++) {
        Lexer *lexer = lexer_create(keywords[i]);
        Token token = lexer_next_token(lexer);
        
        if (token.type == TOKEN_IDENTIFIER) {
            printf("\n  FAILED: '%s' recognized as identifier, not keyword\n", keywords[i]);
            lexer_destroy(lexer);
            return false;
        }
        
        lexer_destroy(lexer);
    }
    
    TEST_PASS();
    return true;
}

static bool test_integers(void) {
    TEST_START("Test Integer Constants");
    
    const char *tests[] = {
        "42", "0", "0xFF", "0x1A2B3C", "0b1010", "0755"
    };
    
    for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
        Lexer *lexer = lexer_create(tests[i]);
        Token token = lexer_next_token(lexer);
        
        if (token.type != TOKEN_INT_CONSTANT) {
            printf("\n  FAILED: '%s' not recognized as integer\n", tests[i]);
            lexer_destroy(lexer);
            return false;
        }
        
        lexer_destroy(lexer);
    }
    
    TEST_PASS();
    return true;
}

static bool test_floats(void) {
    TEST_START("Test Float Constants");
    
    const char *tests[] = {
        "3.14", "0.5", ".25", "1e10", "2.5e-3", "1.0f"
    };
    
    for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
        Lexer *lexer = lexer_create(tests[i]);
        Token token = lexer_next_token(lexer);
        
        if (token.type != TOKEN_FLOAT_CONSTANT && token.type != TOKEN_INT_CONSTANT) {
            printf("\n  FAILED: '%s' not recognized as number\n", tests[i]);
            lexer_destroy(lexer);
            return false;
        }
        
        lexer_destroy(lexer);
    }
    
    TEST_PASS();
    return true;
}

static bool test_operators(void) {
    TEST_START("Test Operators");
    
    struct { const char *input; TokenType expected; } tests[] = {
        {"+", TOKEN_PLUS},
        {"-", TOKEN_MINUS},
        {"*", TOKEN_STAR},
        {"/", TOKEN_SLASH},
        {"%", TOKEN_PERCENT},
        {"&&", TOKEN_AMP_AMP},
        {"||", TOKEN_PIPE_PIPE},
        {"==", TOKEN_EQ_EQ},
        {"!=", TOKEN_EXCLAIM_EQ},
        {"<=", TOKEN_LESS_EQ},
        {">=", TOKEN_GREATER_EQ},
        {"++", TOKEN_PLUS_PLUS},
        {"--", TOKEN_MINUS_MINUS},
        {"->", TOKEN_ARROW},
    };
    
    for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
        Lexer *lexer = lexer_create(tests[i].input);
        Token token = lexer_next_token(lexer);
        
        if (token.type != tests[i].expected) {
            printf("\n  FAILED: '%s' expected %d, got %d\n", 
                   tests[i].input, tests[i].expected, token.type);
            lexer_destroy(lexer);
            return false;
        }
        
        lexer_destroy(lexer);
    }
    
    TEST_PASS();
    return true;
}

static bool test_comments(void) {
    TEST_START("Test Comments");
    
    // Single line comment
    Lexer *lexer1 = lexer_create("// comment\nint");
    lexer_next_token(lexer1); // skip comment
    Token t2 = lexer_next_token(lexer1);
    
    if (t2.type != TOKEN_INT) {
        printf("\n  FAILED: Single line comment not stripped\n");
        lexer_destroy(lexer1);
        return false;
    }
    lexer_destroy(lexer1);
    
    // Multi-line comment
    Lexer *lexer2 = lexer_create("/* comment */int");
    Token t3 = lexer_next_token(lexer2);
    
    if (t3.type != TOKEN_INT) {
        printf("\n  FAILED: Multi-line comment not stripped\n");
        lexer_destroy(lexer2);
        return false;
    }
    lexer_destroy(lexer2);
    
    TEST_PASS();
    return true;
}

static bool test_simple_program(void) {
    TEST_START("Test Simple Program Tokenization");
    
    const char *source = "int main() { return 0; }";
    Lexer *lexer = lexer_create(source);
    
    TokenType expected[] = {
        TOKEN_INT, TOKEN_IDENTIFIER, TOKEN_LPAREN, TOKEN_RPAREN,
        TOKEN_LBRACE, TOKEN_RETURN, TOKEN_INT_CONSTANT, TOKEN_SEMICOLON,
        TOKEN_RBRACE, TOKEN_EOF
    };
    
    for (size_t i = 0; i < sizeof(expected) / sizeof(expected[0]); i++) {
        Token token = lexer_next_token(lexer);
        if (token.type != expected[i]) {
            printf("\n  FAILED at token %zu: expected %d, got %d\n", 
                   i, expected[i], token.type);
            lexer_destroy(lexer);
            return false;
        }
    }
    
    lexer_destroy(lexer);
    TEST_PASS();
    return true;
}

int main(void) {
    TestCase tests[] = {
        {"Keywords", test_keywords},
        {"Integer Constants", test_integers},
        {"Float Constants", test_floats},
        {"Operators", test_operators},
        {"Comments", test_comments},
        {"Simple Program", test_simple_program}
    };
    
    int result = run_tests(tests, sizeof(tests) / sizeof(tests[0]));
    
    if (result == 0) {
        printf("\n*** All lexer tests passed ***\n");
    }
    
    return result;
}
