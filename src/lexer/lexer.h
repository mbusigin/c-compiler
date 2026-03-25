#ifndef LEXER_H
#define LEXER_H

#include "tokens.h"
#include <stdio.h>
#include <stdbool.h>

typedef struct Lexer {
    const char *source;
    size_t length;
    size_t position;     // Current position in source
    int line;            // Current line number
    int column;          // Current column number
    Token current_token; // Current token
    bool had_error;      // Error flag
} Lexer;

// Lexer functions
Lexer *lexer_create(const char *source);
void lexer_destroy(Lexer *lexer);

Token lexer_next_token(Lexer *lexer);
Token lexer_peek_token(Lexer *lexer);
void lexer_advance(Lexer *lexer);

const char *token_type_name(TokenType type);
void token_print(const Token *token, FILE *out);

#endif // LEXER_H
