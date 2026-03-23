/**
 * parser.c - Minimal C11 recursive descent parser
 */

#include "parser.h"
#include "../common/util.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

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
static ASTNode *parse_primary_expr(Parser *parser);
static void expect(Parser *p, TokenType t, const char *msg);

// Helper functions
static void advance(Parser *p) { 
    p->previous = p->current; 
    p->current = lexer_next_token(p->lexer); 
}
static TokenType peek(Parser *p) { return p->current.type; }
static bool check(Parser *p, TokenType t) { return peek(p) == t; }

// Look at the token after the current one without consuming it
static TokenType peek_next(Parser *p) {
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
           t == TOKEN_UNSIGNED || t == TOKEN_EXTERN || t == TOKEN_STATIC || t == TOKEN_CONST;
}

// Copy a string from the current token's lexeme using the token's length
static char *token_name(Parser *p) {
    size_t len = p->current.length;
    char *name = xmalloc(len + 1);
    memcpy(name, p->current.lexeme, len);
    name[len] = '\0';
    return name;
}

// Type parsing
static Type *parse_type(Parser *p) {
    TypeKind kind = TYPE_INT;
    bool is_unsigned = false;
    
    // Handle type qualifiers (const)
    while (check(p, TOKEN_CONST)) {
        advance(p);  // skip 'const'
    }
    
    // Handle signed/unsigned modifiers
    while (check(p, TOKEN_SIGNED) || check(p, TOKEN_UNSIGNED)) {
        if (check(p, TOKEN_UNSIGNED)) {
            is_unsigned = true;
            advance(p);
        } else if (check(p, TOKEN_SIGNED)) {
            is_unsigned = false;
            advance(p);
        }
    }
    
    // Then handle the base type
    if (check(p, TOKEN_VOID)) { advance(p); kind = TYPE_VOID; }
    else if (check(p, TOKEN_CHAR)) { advance(p); kind = TYPE_CHAR; }
    else if (check(p, TOKEN_FLOAT)) { advance(p); kind = TYPE_FLOAT; }
    else if (check(p, TOKEN_DOUBLE)) { advance(p); kind = TYPE_DOUBLE; }
    else if (check(p, TOKEN_SHORT)) { advance(p); kind = TYPE_SHORT; }
    else if (check(p, TOKEN_LONG)) { 
        advance(p); 
        if (check(p, TOKEN_LONG)) { advance(p); kind = TYPE_LONGLONG; }
        else kind = TYPE_LONG;
    }
    else if (check(p, TOKEN_INT)) { advance(p); kind = TYPE_INT; }
    // If we had unsigned/signed but no base type, default to int
    // (e.g., "unsigned x" means "unsigned int x")
    
    Type *t = type_create(kind);
    t->is_unsigned = is_unsigned;
    
    // Handle pointer suffix
    while (check(p, TOKEN_STAR)) {
        advance(p);
        Type *ptr = type_pointer(t);
        // Don't free t - the pointer type now owns it
        t = ptr;
    }
    
    return t;
}

// Function parameter parsing
static ASTNode *parse_parameter(Parser *p) {
    Type *param_type = parse_type(p);
    
    ASTNode *param = ast_create(AST_PARAMETER_DECL);
    param->data.parameter.param_type = param_type;
    
    if (check(p, TOKEN_IDENTIFIER)) {
        param->data.parameter.name = token_name(p);
        advance(p);
    } else {
        param->data.parameter.name = xstrdup("");
    }
    
    // Array parameters become pointers
    while (check(p, TOKEN_LBRACKET)) {
        advance(p);
        while (!check(p, TOKEN_RBRACKET) && !check(p, TOKEN_EOF)) advance(p);
        if (check(p, TOKEN_RBRACKET)) advance(p);
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
    advance(p); // consume LBRACE
    
    while (!check(p, TOKEN_RBRACE) && !check(p, TOKEN_EOF)) {
        if (is_type_keyword(peek(p))) {
            list_push(node->data.compound.stmts, parse_declaration(p));
        } else {
            list_push(node->data.compound.stmts, parse_statement(p));
        }
    }
    
    expect(p, TOKEN_RBRACE, "expected '}'");
    advance(p); // consume RBRACE
    return node;
}

// Parse function definition
static ASTNode *parse_function_definition(Parser *p, Type *return_type, const char *name) {
    ASTNode *func = ast_create(AST_FUNCTION_DECL);
    func->data.function.name = xstrdup(name);
    func->data.function.func_type = return_type;
    func->data.function.params = list_create();
    
    // Parse parameters (already consumed '(')
    while (!check(p, TOKEN_RPAREN) && !check(p, TOKEN_EOF)) {
        ASTNode *param = parse_parameter(p);
        list_push(func->data.function.params, param);
        if (check(p, TOKEN_COMMA)) {
            advance(p);
        } else {
            break;
        }
    }
    expect(p, TOKEN_RPAREN, "expected ')'");
    advance(p); // consume RPAREN
    
    // Check for function body
    if (check(p, TOKEN_LBRACE)) {
        func->data.function.body = parse_function_body(p);
    }
    return func;
}

// Parse declaration
static ASTNode *parse_declaration(Parser *p) {
    // Handle storage class specifiers (extern, static)
    if (check(p, TOKEN_EXTERN)) {
        advance(p); // consume 'extern'
        // Parse the type and function name
        Type *base_type = parse_type(p);
        
        if (check(p, TOKEN_IDENTIFIER)) {
            char *name = token_name(p);
            advance(p);
            
            // Check if it's a function declaration
            if (check(p, TOKEN_LPAREN)) {
                // Skip to matching RPAREN
                advance(p); // consume '('
                int depth = 1;
                while (depth > 0 && !check(p, TOKEN_EOF)) {
                    if (check(p, TOKEN_LPAREN)) depth++;
                    else if (check(p, TOKEN_RPAREN)) depth--;
                    advance(p);
                }
            }
            
            free(name);
        }
        
        type_free(base_type);
        
        // Skip to semicolon
        while (!check(p, TOKEN_SEMICOLON) && !check(p, TOKEN_EOF)) {
            advance(p);
        }
        if (check(p, TOKEN_SEMICOLON)) advance(p);
        return ast_create(AST_NULL_STMT);
    }
    
    // Handle static keyword
    if (check(p, TOKEN_STATIC)) {
        advance(p); // consume 'static'
        // Continue parsing the declaration after 'static'
    }
    
    Type *base_type = parse_type(p);
    
    // Get identifier
    char *name = NULL;
    if (check(p, TOKEN_IDENTIFIER)) {
        // Use token length to copy exactly the right number of characters
        name = token_name(p);
        advance(p);
    } else {
        name = xstrdup("");
    }
    
    // Check for function definition
    if (check(p, TOKEN_LPAREN)) {
        advance(p); // consume '('
        ASTNode *func = parse_function_definition(p, base_type, name);
        free(name);
        return func;
    }
    
    // Handle array declarator: int arr[5]
    while (check(p, TOKEN_LBRACKET)) {
        advance(p);  // consume '['
        // Skip the array size expression (we don't support it yet, just skip to ']')
        while (!check(p, TOKEN_RBRACKET) && !check(p, TOKEN_EOF)) {
            advance(p);
        }
        if (check(p, TOKEN_RBRACKET)) advance(p);  // consume ']'
        // Convert to pointer type
        Type *ptr = type_pointer(base_type);
        base_type = ptr;
    }
    
    // Variable declaration
    ASTNode *decl = ast_create(AST_VARIABLE_DECL);
    decl->data.variable.var_type = base_type;
    decl->data.variable.name = name;
    decl->data.variable.init = NULL;
    
    // Initializer - for now, skip initializer lists
    if (check(p, TOKEN_ASSIGN)) {
        advance(p);
        if (check(p, TOKEN_LBRACE)) {
            // Skip initializer list: { ... }
            advance(p);  // consume '{'
            int depth = 1;
            while (depth > 0 && !check(p, TOKEN_EOF)) {
                if (check(p, TOKEN_LBRACE)) depth++;
                else if (check(p, TOKEN_RBRACE)) depth--;
                advance(p);
            }
        } else {
            decl->data.variable.init = parse_assignment_expr(p);
        }
    }
    
    expect(p, TOKEN_SEMICOLON, "expected ';'");
    advance(p); // consume SEMICOLON
    return decl;
}

static ASTNode *parse_statement(Parser *p) {
    if (check(p, TOKEN_LBRACE)) {
        ASTNode *node = ast_create(AST_COMPOUND_STMT);
        node->data.compound.stmts = list_create();
        advance(p); // consume LBRACE
        while (!check(p, TOKEN_RBRACE) && !check(p, TOKEN_EOF)) {
            if (is_type_keyword(peek(p))) {
                list_push(node->data.compound.stmts, parse_declaration(p));
            } else {
                list_push(node->data.compound.stmts, parse_statement(p));
            }
        }
        expect(p, TOKEN_RBRACE, "expected '}'");
        advance(p); // consume RBRACE
        return node;
    }
    
    if (check(p, TOKEN_IF)) {
        advance(p); // consume IF
        expect(p, TOKEN_LPAREN, "expected '('");
        advance(p); // consume '('
        ASTNode *node = ast_create(AST_IF_STMT);
        node->data.if_stmt.condition = parse_expression(p);
        expect(p, TOKEN_RPAREN, "expected ')'");
        advance(p); // consume ')'
        node->data.if_stmt.then_stmt = parse_statement(p);
        if (check(p, TOKEN_ELSE)) {
            advance(p); // consume ELSE
            node->data.if_stmt.else_stmt = parse_statement(p);
        }
        return node;
    }
    
    if (check(p, TOKEN_WHILE)) {
        advance(p); // consume WHILE
        expect(p, TOKEN_LPAREN, "expected '('");
        advance(p); // consume '('
        ASTNode *node = ast_create(AST_WHILE_STMT);
        node->data.while_stmt.condition = parse_expression(p);
        expect(p, TOKEN_RPAREN, "expected ')'");
        advance(p); // consume ')'
        node->data.while_stmt.body = parse_statement(p);
        return node;
    }
    
    if (check(p, TOKEN_FOR)) {
        advance(p); // consume FOR
        expect(p, TOKEN_LPAREN, "expected '('");
        advance(p); // consume '('
        ASTNode *node = ast_create(AST_FOR_STMT);
        bool is_decl = is_type_keyword(peek(p));
        if (!check(p, TOKEN_SEMICOLON))
            node->data.for_stmt.init = is_decl ? parse_declaration(p) : parse_expression(p);
        // If init was an expression, consume the semicolon. Declarations already consume it.
        if (!is_decl || check(p, TOKEN_SEMICOLON)) {
            expect(p, TOKEN_SEMICOLON, "expected ';'");
            advance(p); // consume ';'
        }
        if (!check(p, TOKEN_SEMICOLON))
            node->data.for_stmt.condition = parse_expression(p);
        expect(p, TOKEN_SEMICOLON, "expected ';'");
        advance(p); // consume ';'
        if (!check(p, TOKEN_RPAREN))
            node->data.for_stmt.increment = parse_expression(p);
        expect(p, TOKEN_RPAREN, "expected ')'");
        advance(p); // consume ')'
        node->data.for_stmt.body = parse_statement(p);
        return node;
    }
    
    if (check(p, TOKEN_RETURN)) {
        advance(p); // consume RETURN
        ASTNode *node = ast_create(AST_RETURN_STMT);
        if (!check(p, TOKEN_SEMICOLON))
            node->data.return_stmt.expr = parse_expression(p);
        expect(p, TOKEN_SEMICOLON, "expected ';'");
        advance(p); // consume ';'
        return node;
    }
    
    if (check(p, TOKEN_BREAK)) {
        advance(p); // consume BREAK
        expect(p, TOKEN_SEMICOLON, "expected ';'");
        advance(p); // consume ';'
        return ast_create(AST_BREAK_STMT);
    }
    
    if (check(p, TOKEN_CONTINUE)) {
        advance(p); // consume CONTINUE
        expect(p, TOKEN_SEMICOLON, "expected ';'");
        advance(p); // consume ';'
        return ast_create(AST_CONTINUE_STMT);
    }
    
    if (check(p, TOKEN_SEMICOLON)) {
        advance(p); // consume ';'
        return ast_create(AST_NULL_STMT);
    }
    
    ASTNode *expr = parse_expression(p);
    expect(p, TOKEN_SEMICOLON, "expected ';'");
    advance(p); // consume ';'
    ASTNode *stmt = ast_create(AST_EXPRESSION_STMT);
    stmt->data.expr_stmt.expr = expr;
    return stmt;
}

static ASTNode *parse_expression(Parser *p) {
    return parse_assignment_expr(p);
}

static ASTNode *parse_assignment_expr(Parser *p) {
    ASTNode *left = parse_conditional_expr(p);
    
    if (check(p, TOKEN_ASSIGN)) {
        advance(p);
        ASTNode *node = ast_create(AST_ASSIGNMENT_EXPR);
        node->data.assignment.op = -1;  // Plain assignment (0 is OP_ADD)
        node->data.assignment.left = left;
        node->data.assignment.right = parse_assignment_expr(p);
        return node;
    }
    if (check(p, TOKEN_PLUS_EQ)) {
        advance(p);
        ASTNode *node = ast_create(AST_ASSIGNMENT_EXPR);
        node->data.assignment.op = OP_ADD;
        node->data.assignment.left = left;
        node->data.assignment.right = parse_assignment_expr(p);
        return node;
    }
    if (check(p, TOKEN_MINUS_EQ)) {
        advance(p);
        ASTNode *node = ast_create(AST_ASSIGNMENT_EXPR);
        node->data.assignment.op = OP_SUB;
        node->data.assignment.left = left;
        node->data.assignment.right = parse_assignment_expr(p);
        return node;
    }
    if (check(p, TOKEN_STAR_EQ)) {
        advance(p);
        ASTNode *node = ast_create(AST_ASSIGNMENT_EXPR);
        node->data.assignment.op = OP_MUL;
        node->data.assignment.left = left;
        node->data.assignment.right = parse_assignment_expr(p);
        return node;
    }
    if (check(p, TOKEN_SLASH_EQ)) {
        advance(p);
        ASTNode *node = ast_create(AST_ASSIGNMENT_EXPR);
        node->data.assignment.op = OP_DIV;
        node->data.assignment.left = left;
        node->data.assignment.right = parse_assignment_expr(p);
        return node;
    }
    if (check(p, TOKEN_PERCENT_EQ)) {
        advance(p);
        ASTNode *node = ast_create(AST_ASSIGNMENT_EXPR);
        node->data.assignment.op = OP_MOD;
        node->data.assignment.left = left;
        node->data.assignment.right = parse_assignment_expr(p);
        return node;
    }
    if (check(p, TOKEN_LSHIFT_EQ)) {
        advance(p);
        ASTNode *node = ast_create(AST_ASSIGNMENT_EXPR);
        node->data.assignment.op = OP_LSHIFT;
        node->data.assignment.left = left;
        node->data.assignment.right = parse_assignment_expr(p);
        return node;
    }
    if (check(p, TOKEN_RSHIFT_EQ)) {
        advance(p);
        ASTNode *node = ast_create(AST_ASSIGNMENT_EXPR);
        node->data.assignment.op = OP_RSHIFT;
        node->data.assignment.left = left;
        node->data.assignment.right = parse_assignment_expr(p);
        return node;
    }
    if (check(p, TOKEN_AMP_EQ)) {
        advance(p);
        ASTNode *node = ast_create(AST_ASSIGNMENT_EXPR);
        node->data.assignment.op = OP_BITAND;
        node->data.assignment.left = left;
        node->data.assignment.right = parse_assignment_expr(p);
        return node;
    }
    if (check(p, TOKEN_PIPE_EQ)) {
        advance(p);
        ASTNode *node = ast_create(AST_ASSIGNMENT_EXPR);
        node->data.assignment.op = OP_BITOR;
        node->data.assignment.left = left;
        node->data.assignment.right = parse_assignment_expr(p);
        return node;
    }
    if (check(p, TOKEN_CARET_EQ)) {
        advance(p);
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
    
    if (check(p, TOKEN_QUESTION)) {
        advance(p);
        ASTNode *node = ast_create(AST_CONDITIONAL_EXPR);
        node->data.conditional.condition = cond;
        node->data.conditional.then_expr = parse_expression(p);
        expect(p, TOKEN_COLON, "expected ':'");
        advance(p);
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
    
    while (prec(peek(p)) >= min_prec) {
        BinaryOp op = binop(peek(p));
        int assoc_prec = prec(peek(p));
        advance(p);
        
        ASTNode *node = ast_create(AST_BINARY_EXPR);
        node->data.binary.op = op;
        node->data.binary.left = left;
        node->data.binary.right = parse_binary_expr(p, assoc_prec + 1);
        left = node;
    }
    
    return left;
}

static ASTNode *parse_cast_expr(Parser *p) {
    if (check(p, TOKEN_LPAREN) && is_type_keyword(peek_next(p))) {
        advance(p); // consume '('
        Type *t = parse_type(p);
        expect(p, TOKEN_RPAREN, "expected ')'");
        advance(p); // consume ')'
        ASTNode *node = ast_create(AST_CAST_EXPR);
        node->data.cast.cast_type = t;
        node->data.cast.operand = parse_cast_expr(p);
        return node;
    }
    return parse_unary_expr(p);
}

static ASTNode *parse_unary_expr(Parser *p) {
    if (check(p, TOKEN_PLUS_PLUS)) {
        advance(p);
        ASTNode *node = ast_create(AST_UNARY_EXPR);
        node->data.unary.op = 0;
        node->data.unary.operand = parse_unary_expr(p);
        return node;
    }
    if (check(p, TOKEN_MINUS_MINUS)) {
        advance(p);
        ASTNode *node = ast_create(AST_UNARY_EXPR);
        node->data.unary.op = 1;
        node->data.unary.operand = parse_unary_expr(p);
        return node;
    }
    if (check(p, TOKEN_EXCLAIM)) {
        advance(p);
        ASTNode *node = ast_create(AST_UNARY_EXPR);
        node->data.unary.op = 2;
        node->data.unary.operand = parse_cast_expr(p);
        return node;
    }
    if (check(p, TOKEN_TILDE)) {
        advance(p);
        ASTNode *node = ast_create(AST_UNARY_EXPR);
        node->data.unary.op = 3;
        node->data.unary.operand = parse_cast_expr(p);
        return node;
    }
    if (check(p, TOKEN_STAR)) {
        advance(p);
        ASTNode *node = ast_create(AST_UNARY_EXPR);
        node->data.unary.op = 4;
        node->data.unary.operand = parse_cast_expr(p);
        return node;
    }
    if (check(p, TOKEN_AMPERSAND)) {
        advance(p);
        ASTNode *node = ast_create(AST_UNARY_EXPR);
        node->data.unary.op = 5;
        node->data.unary.operand = parse_cast_expr(p);
        return node;
    }
    return parse_postfix_expr(p);
}

static ASTNode *parse_postfix_expr(Parser *p) {
    ASTNode *expr = parse_primary_expr(p);
    
    while (true) {
        if (check(p, TOKEN_LBRACKET)) {
            advance(p);
            ASTNode *node = ast_create(AST_ARRAY_SUBSCRIPT_EXPR);
            node->data.subscript.array = expr;
            node->data.subscript.index = parse_expression(p);
            expect(p, TOKEN_RBRACKET, "expected ']'");
            advance(p);
            expr = node;
        }
        else if (check(p, TOKEN_LPAREN)) {
            advance(p);
            ASTNode *node = ast_create(AST_CALL_EXPR);
            node->data.call.callee = expr;
            node->data.call.args = list_create();
            while (!check(p, TOKEN_RPAREN) && !check(p, TOKEN_EOF)) {
                list_push(node->data.call.args, parse_assignment_expr(p));
                if (check(p, TOKEN_COMMA)) advance(p);
                else break;
            }
            expect(p, TOKEN_RPAREN, "expected ')'");
            advance(p);
            expr = node;
        }
        else if (check(p, TOKEN_DOT)) {
            advance(p);
            ASTNode *node = ast_create(AST_MEMBER_ACCESS_EXPR);
            node->data.member.expr = expr;
            expect(p, TOKEN_IDENTIFIER, "expected member name");
            node->data.member.member = token_name(p);
            advance(p);
            node->data.member.is_arrow = false;
            expr = node;
        }
        else if (check(p, TOKEN_ARROW)) {
            advance(p);
            ASTNode *node = ast_create(AST_POINTER_MEMBER_ACCESS_EXPR);
            node->data.member.expr = expr;
            expect(p, TOKEN_IDENTIFIER, "expected member name");
            node->data.member.member = token_name(p);
            advance(p);
            node->data.member.is_arrow = true;
            expr = node;
        }
        else if (check(p, TOKEN_PLUS_PLUS)) {
            advance(p);
            ASTNode *node = ast_create(AST_UNARY_EXPR);
            node->data.unary.op = 6;
            node->data.unary.operand = expr;
            expr = node;
        }
        else if (check(p, TOKEN_MINUS_MINUS)) {
            advance(p);
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
    if (check(p, TOKEN_IDENTIFIER)) {
        ASTNode *node = ast_create(AST_IDENTIFIER_EXPR);
        node->data.identifier.name = token_name(p);
        advance(p);
        return node;
    }
    
    if (check(p, TOKEN_INT_CONSTANT) || check(p, TOKEN_NUMBER)) {
        ASTNode *node = ast_create(AST_INTEGER_LITERAL_EXPR);
        node->data.int_literal.value = p->current.value.int_val;
        advance(p);
        return node;
    }
    
    if (check(p, TOKEN_FLOAT_CONSTANT)) {
        ASTNode *node = ast_create(AST_FLOAT_LITERAL_EXPR);
        node->data.float_literal.value = p->current.value.float_val;
        advance(p);
        return node;
    }
    
    if (check(p, TOKEN_STRING_LITERAL)) {
        ASTNode *node = ast_create(AST_STRING_LITERAL_EXPR);
        node->data.string_literal.value = token_name(p);
        advance(p);
        return node;
    }
    
    if (check(p, TOKEN_LPAREN)) {
        advance(p);
        ASTNode *expr = parse_expression(p);
        expect(p, TOKEN_RPAREN, "expected ')'");
        advance(p);
        return expr;
    }
    
    return ast_create(AST_INTEGER_LITERAL_EXPR);
}

static void expect(Parser *p, TokenType t, const char *msg) {
    if (check(p, t)) return;
    fprintf(stderr, "error: [%d:%d] %s\n", p->current.line, p->current.column, msg);
}

Parser *parser_create(Lexer *lexer) {
    Parser *p = calloc(1, sizeof(Parser));
    p->lexer = lexer;
    p->translation_unit = ast_create(AST_TRANSLATION_UNIT);
    p->translation_unit->data.unit.declarations = list_create();
    advance(p); // Get first token
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
    while (!check(p, TOKEN_EOF)) {
        // Skip error tokens
        while (check(p, TOKEN_ERROR)) {
            advance(p);
        }
        if (check(p, TOKEN_EOF)) break;
        
        if (is_type_keyword(peek(p))) {
            ASTNode *decl = parse_declaration(p);
            if (decl) list_push(p->translation_unit->data.unit.declarations, decl);
        } else {
            ASTNode *stmt = parse_statement(p);
            if (stmt) list_push(p->translation_unit->data.unit.declarations, stmt);
        }
    }
    return p->translation_unit;
}
