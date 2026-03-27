/**
 * parse_utils.c - Parser utility functions
 *
 * This module contains utility functions for the parser.
 * Note: Registry functions (typedef, struct, enum) remain in parser.c as static functions
 * during this refactoring transition.
 */

#include "parser_internal.h"
#include "../common/util.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Parser helper functions
Token advance_p(Parser *p) { 
    p->previous = p->current; 
    p->current = lexer_next_token(p->lexer);
    return p->previous;
}

Token peek_p(Parser *p) { 
    return p->current; 
}

Token previous_p(Parser *p) {
    return p->previous;
}

bool check_p(Parser *p, TokenType t) { 
    return peek_p(p).type == t; 
}

void error_at(Parser *p, Token *token, const char *message) {
    if (p->panic_mode) return;
    p->panic_mode = true;
    fprintf(stderr, "Error at line %d: %s\n", token->line, message);
    p->had_error = true;
}

void parser_error(Parser *p, const char *message) {
    error_at(p, &p->previous, message);
}

void error_at_current(Parser *p, const char *message) {
    error_at(p, &p->current, message);
}

void expect(Parser *p, TokenType t, const char *msg) {
    if (p->current.type == t) {
        advance_p(p);
        return;
    }
    char buf[256];
    snprintf(buf, sizeof(buf), "%s (got '%s')", msg, p->current.lexeme ? p->current.lexeme : "EOF");
    error_at_current(p, buf);
}
