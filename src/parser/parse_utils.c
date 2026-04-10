/**
 * parse_utils.c - Parser utility functions
 *
 * This module contains utility functions for the parser.
 * Uses Token* out params and token_copy to avoid broken struct return/assign.
 */

#include "parser_internal.h"
#include "../common/util.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Parser helper functions - use Token* out params to avoid struct return */
void advance_p(Parser *p, Token *out) { 
    token_copy(&p->previous, &p->current); 
    lexer_next_token(p->lexer, &p->current);
    if (out) {
        token_copy(out, &p->previous);
    }
}

void peek_p(Parser *p, Token *out) { 
    token_copy(out, &p->current);
}

void previous_p(Parser *p, Token *out) {
    token_copy(out, &p->previous);
}

int check_p(Parser *p, TokenType t) { 
    return p->current.type == t; 
}

void error_at(Parser *p, Token *token, const char *message) {
    if (p->panic_mode) return;
    p->panic_mode = 1;
    fprintf(stderr, "Error at line %d: %s\n", token->line, message);
    p->had_error = 1;
}

void parser_error(Parser *p, const char *message) {
    error_at(p, &p->previous, message);
}

void error_at_current(Parser *p, const char *message) {
    error_at(p, &p->current, message);
}

void expect(Parser *p, TokenType t, const char *msg) {
    if (p->current.type == t) {
        advance_p(p, NULL);
        return;
    }
    char buf[256];
    const char *lex = p->current.lexeme ? p->current.lexeme : "EOF";
    snprintf(buf, sizeof(buf), "%s (got '%s')", msg, lex);
    error_at_current(p, buf);
}
