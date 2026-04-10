#ifndef PARSER_INTERNAL_H
#define PARSER_INTERNAL_H

// Internal parser interface - shared between parser modules
#include "parser.h"
#include "../lexer/lexer.h"

// Forward declarations for parsing functions
ASTNode *parse_declaration(Parser *p);
ASTNode *parse_statement(Parser *p);
ASTNode *parse_expression(Parser *p);
ASTNode *parse_assignment_expr(Parser *p);
ASTNode *parse_conditional_expr(Parser *p);
ASTNode *parse_binary_expr(Parser *p, int min_prec);
ASTNode *parse_cast_expr(Parser *p);
ASTNode *parse_unary_expr(Parser *p);
ASTNode *parse_postfix_expr(Parser *p);
ASTNode *parse_postfix_expr_with_expr(Parser *p, ASTNode *expr);
ASTNode *parse_primary_expr(Parser *p);
Type *parse_type(Parser *p);
ASTNode *parse_parameter(Parser *p);
ASTNode *parse_function_body(Parser *p);
ASTNode *parse_function_definition(Parser *p, Type *return_type, const char *name);

// Utility functions
void expect(Parser *p, TokenType t, const char *msg);
int check_p(Parser *p, TokenType t);
void advance_p(Parser *p, Token *out);
void peek_p(Parser *p, Token *out);
void previous_p(Parser *p, Token *out);
void error_at(Parser *p, Token *token, const char *message);
void parser_error(Parser *p, const char *message);
void error_at_current(Parser *p, const char *message);

// Parser helper macros
#define PARSER_Synchronized(p) for (int _sync = (p)->panic_mode ? (advance_p(p, ((void*)0)), (p)->panic_mode = 0, 1) : 1; _sync; _sync = 0)

#endif // PARSER_INTERNAL_H
