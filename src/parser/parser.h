#ifndef PARSER_H
#define PARSER_H

#include "../lexer/lexer.h"
#include "ast.h"

typedef struct Parser {
    Lexer *lexer;
    Token current;
    Token previous;
    bool had_error;
    bool panic_mode;
    ASTNode *translation_unit;
} Parser;

Parser *parser_create(Lexer *lexer);
void parser_destroy(Parser *parser);
ASTNode *parse(Parser *parser);

#endif // PARSER_H
