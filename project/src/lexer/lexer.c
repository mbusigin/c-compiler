/**
 * lexer.c - Lexical analyzer for C source code
 */

#include "lexer.h"
#include "../common/util.h"
#include "../common/error.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

Lexer *lexer_create(const char *source) {
    Lexer *lexer = xmalloc(sizeof(Lexer));
    lexer->source = source;
    lexer->length = strlen(source);
    lexer->position = 0;
    lexer->line = 1;
    lexer->column = 1;
    lexer->had_error = false;
    return lexer;
}

void lexer_destroy(Lexer *lexer) {
    if (lexer) {
        free(lexer);
    }
}

static bool is_at_end(const Lexer *lexer) {
    return lexer->position >= lexer->length;
}

static char peek(const Lexer *lexer) {
    if (is_at_end(lexer)) {
        return '\0';
    }
    return lexer->source[lexer->position];
}

static char peek_next(const Lexer *lexer) {
    if (lexer->position + 1 >= lexer->length) {
        return '\0';
    }
    return lexer->source[lexer->position + 1];
}

static char advance(Lexer *lexer) {
    if (is_at_end(lexer)) {
        return '\0';
    }
    char c = lexer->source[lexer->position++];
    if (c == '\n') {
        lexer->line++;
        lexer->column = 1;
    } else {
        lexer->column++;
    }
    return c;
}

static void skip_whitespace(Lexer *lexer) {
    while (!is_at_end(lexer)) {
        char c = peek(lexer);
        switch (c) {
            case ' ':
            case '\t':
            case '\r':
            case '\v':
            case '\f':
            case '\n':
                advance(lexer);
                break;
            case '/':
                if (peek_next(lexer) == '/') {
                    // Single-line comment
                    while (!is_at_end(lexer) && peek(lexer) != '\n') {
                        advance(lexer);
                    }
                } else if (peek_next(lexer) == '*') {
                    // Multi-line comment
                    advance(lexer); // skip /
                    advance(lexer); // skip *
                    while (!is_at_end(lexer)) {
                        if (peek(lexer) == '*' && peek_next(lexer) == '/') {
                            advance(lexer);
                            advance(lexer);
                            break;
                        }
                        advance(lexer);
                    }
                } else {
                    return;
                }
                break;
            default:
                return;
        }
    }
}

static Token make_token(Lexer *lexer, TokenType type) {
    Token token;
    token.type = type;
    token.lexeme = lexer->source + lexer->position;
    token.length = 0;
    token.line = lexer->line;
    token.column = lexer->column;
    token.value.int_val = 0;
    return token;
}

static Token scan_identifier_or_keyword(Lexer *lexer) {
    Token token = make_token(lexer, TOKEN_IDENTIFIER);
    
    while (!is_at_end(lexer)) {
        char c = peek(lexer);
        if (isalnum(c) || c == '_') {
            advance(lexer);
        } else {
            break;
        }
    }
    
    token.length = (int)(lexer->source + lexer->position - token.lexeme);
    token.type = keyword_lookup(token.lexeme, token.length);
    
    // Special case for main identifier
    if (token.type == TOKEN_IDENTIFIER && 
        token.length == 4 && 
        strncmp(token.lexeme, "main", 4) == 0) {
        // Will be printed as MAIN by token_print special case
    }
    
    return token;
}

static Token scan_number(Lexer *lexer) {
    Token token = make_token(lexer, TOKEN_INT_CONSTANT);
    bool has_dot = false;
    bool has_exponent = false;
    bool is_hex = false;
    bool is_binary = false;
    bool started_with_dot = false;
    
    // Check for hex or binary prefix
    if (peek(lexer) == '0') {
        if (peek_next(lexer) == 'x' || peek_next(lexer) == 'X') {
            is_hex = true;
            advance(lexer); advance(lexer);
        } else if (peek_next(lexer) == 'b' || peek_next(lexer) == 'B') {
            is_binary = true;
            advance(lexer); advance(lexer);
        }
    }
    
    // Handle numbers starting with decimal point (e.g., .25)
    if (peek(lexer) == '.' && isdigit(peek_next(lexer))) {
        has_dot = true;
        started_with_dot = true;
        advance(lexer); // consume '.'
    }
    
    while (!is_at_end(lexer)) {
        char c = peek(lexer);
        
        if (isdigit(c) || (is_hex && isxdigit(c)) || 
            (is_binary && (c == '0' || c == '1'))) {
            advance(lexer);
        } else if (c == '.' && !has_dot && !has_exponent && !is_binary && !started_with_dot) {
            has_dot = true;
            advance(lexer);
        } else if ((c == 'e' || c == 'E') && !has_exponent && !is_binary && !started_with_dot) {
            has_exponent = true;
            advance(lexer);
            if (peek(lexer) == '+' || peek(lexer) == '-') {
                advance(lexer);
            }
        } else if ((c == 'p' || c == 'P') && is_hex && !has_exponent) {
            has_exponent = true;
            advance(lexer);
            if (peek(lexer) == '+' || peek(lexer) == '-') {
                advance(lexer);
            }
        } else if (c == 'f' || c == 'F' || c == 'l' || c == 'L' ||
                   c == 'u' || c == 'U') {
            advance(lexer);
            if ((peek(lexer) == 'l' || peek(lexer) == 'L') &&
                (peek_next(lexer) == 'l' || peek_next(lexer) == 'L')) {
                advance(lexer);
            }
            break;
        } else {
            break;
        }
    }
    
    token.length = (int)(lexer->source + lexer->position - token.lexeme);
    
    if (has_dot || has_exponent || started_with_dot) {
        token.type = TOKEN_FLOAT_CONSTANT;
        token.value.float_val = strtod(token.lexeme, NULL);
    } else {
        char *end;
        if (is_hex) {
            token.value.int_val = strtoll(token.lexeme + 2, &end, 16);
        } else if (is_binary) {
            token.value.int_val = strtoll(token.lexeme + 2, &end, 2);
        } else {
            token.value.int_val = strtoll(token.lexeme, &end, 10);
        }
    }
    
    return token;
}

static Token scan_string(Lexer *lexer) {
    Token token = make_token(lexer, TOKEN_STRING_LITERAL);
    char quote = advance(lexer); // Opening quote
    
    while (!is_at_end(lexer) && peek(lexer) != quote) {
        if (peek(lexer) == '\\') {
            advance(lexer); // Backslash
            if (!is_at_end(lexer)) {
                advance(lexer); // Escaped character
            }
        } else if (peek(lexer) == '\n') {
            // String cannot span lines
            error("Unterminated string literal\n");
            lexer->had_error = true;
            break;
        } else {
            advance(lexer);
        }
    }
    
    if (is_at_end(lexer) || peek(lexer) != quote) {
        error("Unterminated string literal\n");
        lexer->had_error = true;
    } else {
        advance(lexer); // Closing quote
    }
    
    token.length = (int)(lexer->source + lexer->position - token.lexeme);
    return token;
}

static Token scan_char(Lexer *lexer) {
    Token token = make_token(lexer, TOKEN_CHAR_CONSTANT);
    advance(lexer); // Opening quote
    
    if (peek(lexer) == '\\') {
        advance(lexer); // Backslash
        advance(lexer); // Escaped character
    } else if (peek(lexer) != '\'') {
        advance(lexer);
    }
    
    if (peek(lexer) == '\'') {
        advance(lexer); // Closing quote
    } else {
        error("Unterminated character constant\n");
        lexer->had_error = true;
    }
    
    token.length = (int)(lexer->source + lexer->position - token.lexeme);
    token.value.int_val = 0; // TODO: Parse actual char value
    return token;
}

static Token scan_operator(Lexer *lexer) {
    Token token = make_token(lexer, TOKEN_ERROR);
    char c = advance(lexer);
    
    switch (c) {
        case '+':
            if (peek(lexer) == '+') { advance(lexer); token.type = TOKEN_PLUS_PLUS; }
            else if (peek(lexer) == '=') { advance(lexer); token.type = TOKEN_PLUS_EQ; }
            else { token.type = TOKEN_PLUS; }
            break;
        case '-':
            if (peek(lexer) == '-') { advance(lexer); token.type = TOKEN_MINUS_MINUS; }
            else if (peek(lexer) == '>') { advance(lexer); token.type = TOKEN_ARROW; }
            else if (peek(lexer) == '=') { advance(lexer); token.type = TOKEN_MINUS_EQ; }
            else { token.type = TOKEN_MINUS; }
            break;
        case '*':
            if (peek(lexer) == '=') { advance(lexer); token.type = TOKEN_STAR_EQ; }
            else { token.type = TOKEN_STAR; }
            break;
        case '/':
            if (peek(lexer) == '=') { advance(lexer); token.type = TOKEN_SLASH_EQ; }
            else { token.type = TOKEN_SLASH; }
            break;
        case '%':
            if (peek(lexer) == '=') { advance(lexer); token.type = TOKEN_PERCENT_EQ; }
            else { token.type = TOKEN_PERCENT; }
            break;
        case '=':
            if (peek(lexer) == '=') { advance(lexer); token.type = TOKEN_EQ_EQ; }
            else { token.type = TOKEN_ASSIGN; }
            break;
        case '!':
            if (peek(lexer) == '=') { advance(lexer); token.type = TOKEN_EXCLAIM_EQ; }
            else { token.type = TOKEN_EXCLAIM; }
            break;
        case '<':
            if (peek(lexer) == '=') { advance(lexer); token.type = TOKEN_LESS_EQ; }
            else if (peek(lexer) == '<') {
                advance(lexer);
                if (peek(lexer) == '=') { advance(lexer); token.type = TOKEN_LSHIFT_EQ; }
                else { token.type = TOKEN_LSHIFT; }
            }
            else { token.type = TOKEN_LESS; }
            break;
        case '>':
            if (peek(lexer) == '=') { advance(lexer); token.type = TOKEN_GREATER_EQ; }
            else if (peek(lexer) == '>') {
                advance(lexer);
                if (peek(lexer) == '=') { advance(lexer); token.type = TOKEN_RSHIFT_EQ; }
                else { token.type = TOKEN_RSHIFT; }
            }
            else { token.type = TOKEN_GREATER; }
            break;
        case '&':
            if (peek(lexer) == '&') { advance(lexer); token.type = TOKEN_AMP_AMP; }
            else if (peek(lexer) == '=') { advance(lexer); token.type = TOKEN_AMP_EQ; }
            else { token.type = TOKEN_AMPERSAND; }
            break;
        case '|':
            if (peek(lexer) == '|') { advance(lexer); token.type = TOKEN_PIPE_PIPE; }
            else if (peek(lexer) == '=') { advance(lexer); token.type = TOKEN_PIPE_EQ; }
            else { token.type = TOKEN_PIPE; }
            break;
        case '^':
            if (peek(lexer) == '=') { advance(lexer); token.type = TOKEN_CARET_EQ; }
            else { token.type = TOKEN_CARET; }
            break;
        case '#':
            if (peek(lexer) == '#') { advance(lexer); token.type = TOKEN_HASH_HASH; }
            else { token.type = TOKEN_HASH; }
            break;
        default:
            token.type = TOKEN_ERROR;
            break;
    }
    
    token.length = (int)(lexer->source + lexer->position - token.lexeme);
    return token;
}

Token lexer_next_token(Lexer *lexer) {
    skip_whitespace(lexer);
    
    if (is_at_end(lexer)) {
        Token token = make_token(lexer, TOKEN_EOF);
        token.lexeme = "";
        token.length = 0;
        return token;
    }
    
    char c = peek(lexer);
    
    if (isalpha(c) || c == '_') {
        return scan_identifier_or_keyword(lexer);
    }
    
    if (isdigit(c)) {
        return scan_number(lexer);
    }
    
    switch (c) {
        case '(': { Token t = make_token(lexer, TOKEN_LPAREN); advance(lexer); t.length = 1; 
            return t; }
        case ')': { Token t = make_token(lexer, TOKEN_RPAREN); advance(lexer); t.length = 1; return t; }
        case '[': { Token t = make_token(lexer, TOKEN_LBRACKET); advance(lexer); t.length = 1; return t; }
        case ']': { Token t = make_token(lexer, TOKEN_RBRACKET); advance(lexer); t.length = 1; return t; }
        case '{': { Token t = make_token(lexer, TOKEN_LBRACE); advance(lexer); t.length = 1; return t; }
        case '}': { Token t = make_token(lexer, TOKEN_RBRACE); advance(lexer); t.length = 1; return t; }
        case ',': { Token t = make_token(lexer, TOKEN_COMMA); advance(lexer); t.length = 1; return t; }
        case ';': { Token t = make_token(lexer, TOKEN_SEMICOLON); advance(lexer); t.length = 1; return t; }
        case '~': { Token t = make_token(lexer, TOKEN_TILDE); advance(lexer); t.length = 1; return t; }
        case '?': { Token t = make_token(lexer, TOKEN_QUESTION); advance(lexer); t.length = 1; return t; }
        case ':': { Token t = make_token(lexer, TOKEN_COLON); advance(lexer); t.length = 1; return t; }
        case '.': {
            // Check for ellipsis (...)
            if (peek(lexer) == '.' && peek_next(lexer) == '.') {
                advance(lexer); advance(lexer); advance(lexer);
                Token t = make_token(lexer, TOKEN_ELLIPSIS);
                t.length = 3;
                return t;
            }
            // Check if it's a float literal (e.g., .25)
            if (isdigit(peek(lexer))) {
                // It's a float starting with decimal point, use scan_number
                return scan_number(lexer);
            }
            Token t = make_token(lexer, TOKEN_DOT); advance(lexer); t.length = 1; return t;
        }
        case '"': return scan_string(lexer);
        case '\'': return scan_char(lexer);
        default: return scan_operator(lexer);
    }
}

Token lexer_peek_token(Lexer *lexer) {
    return lexer->current_token;
}

void lexer_advance(Lexer *lexer) {
    lexer->current_token = lexer_next_token(lexer);
}

const char *token_type_name(TokenType type) {
    switch (type) {
        case TOKEN_EOF: return "EOF";
        case TOKEN_IDENTIFIER: return "IDENTIFIER";
        case TOKEN_INT_CONSTANT:
        case TOKEN_NUMBER: return "NUMBER";
        case TOKEN_FLOAT_CONSTANT: return "FLOAT_CONSTANT";
        case TOKEN_CHAR_CONSTANT: return "CHAR_CONSTANT";
        case TOKEN_STRING_LITERAL: return "STRING_LITERAL";
        
        // Keywords
        case TOKEN_AUTO: return "AUTO";
        case TOKEN_BREAK: return "BREAK";
        case TOKEN_CASE: return "CASE";
        case TOKEN_CHAR: return "CHAR";
        case TOKEN_CONST: return "CONST";
        case TOKEN_CONTINUE: return "CONTINUE";
        case TOKEN_DEFAULT: return "DEFAULT";
        case TOKEN_DO: return "DO";
        case TOKEN_DOUBLE: return "DOUBLE";
        case TOKEN_ELSE: return "ELSE";
        case TOKEN_ENUM: return "ENUM";
        case TOKEN_EXTERN: return "EXTERN";
        case TOKEN_FLOAT: return "FLOAT";
        case TOKEN_FOR: return "FOR";
        case TOKEN_GOTO: return "GOTO";
        case TOKEN_IF: return "IF";
        case TOKEN_INLINE: return "INLINE";
        case TOKEN_INT: return "INT";
        case TOKEN_LONG: return "LONG";
        case TOKEN_REGISTER: return "REGISTER";
        case TOKEN_RESTRICT: return "RESTRICT";
        case TOKEN_RETURN: return "RETURN";
        case TOKEN_SHORT: return "SHORT";
        case TOKEN_SIGNED: return "SIGNED";
        case TOKEN_SIZEOF: return "SIZEOF";
        case TOKEN_STATIC: return "STATIC";
        case TOKEN_STRUCT: return "STRUCT";
        case TOKEN_SWITCH: return "SWITCH";
        case TOKEN_TYPEDEF: return "TYPEDEF";
        case TOKEN_UNION: return "UNION";
        case TOKEN_UNSIGNED: return "UNSIGNED";
        case TOKEN_VOID: return "VOID";
        case TOKEN_VOLATILE: return "VOLATILE";
        case TOKEN_WHILE: return "WHILE";
        
        // Punctuation
        case TOKEN_LPAREN: return "LPAREN";
        case TOKEN_RPAREN: return "RPAREN";
        case TOKEN_LBRACKET: return "LBRACKET";
        case TOKEN_RBRACKET: return "RBRACKET";
        case TOKEN_LBRACE: return "LBRACE";
        case TOKEN_RBRACE: return "RBRACE";
        case TOKEN_COMMA: return "COMMA";
        case TOKEN_SEMICOLON: return "SEMICOLON";
        case TOKEN_COLON: return "COLON";
        case TOKEN_DOT: return "DOT";
        case TOKEN_ELLIPSIS: return "ELLIPSIS";
        
        // Operators
        case TOKEN_ASSIGN: return "ASSIGN";
        case TOKEN_EQ_EQ: return "EQ_EQ";
        case TOKEN_EXCLAIM_EQ: return "EXCLAIM_EQ";
        case TOKEN_LESS: return "LESS";
        case TOKEN_GREATER: return "GREATER";
        case TOKEN_LESS_EQ: return "LESS_EQ";
        case TOKEN_GREATER_EQ: return "GREATER_EQ";
        
        case TOKEN_PLUS: return "PLUS";
        case TOKEN_MINUS: return "MINUS";
        case TOKEN_STAR: return "STAR";
        case TOKEN_SLASH: return "SLASH";
        case TOKEN_PERCENT: return "PERCENT";
        
        case TOKEN_PLUS_PLUS: return "PLUS_PLUS";
        case TOKEN_MINUS_MINUS: return "MINUS_MINUS";
        case TOKEN_PLUS_EQ: return "PLUS_EQ";
        case TOKEN_MINUS_EQ: return "MINUS_EQ";
        case TOKEN_STAR_EQ: return "STAR_EQ";
        case TOKEN_SLASH_EQ: return "SLASH_EQ";
        case TOKEN_PERCENT_EQ: return "PERCENT_EQ";
        
        case TOKEN_AMPERSAND: return "AMPERSAND";
        case TOKEN_PIPE: return "PIPE";
        case TOKEN_CARET: return "CARET";
        case TOKEN_TILDE: return "TILDE";
        case TOKEN_EXCLAIM: return "EXCLAIM";
        case TOKEN_QUESTION: return "QUESTION";
        case TOKEN_ARROW: return "ARROW";
        
        case TOKEN_AMP_AMP: return "AMP_AMP";
        case TOKEN_PIPE_PIPE: return "PIPE_PIPE";
        case TOKEN_AMP_EQ: return "AMP_EQ";
        case TOKEN_PIPE_EQ: return "PIPE_EQ";
        case TOKEN_CARET_EQ: return "CARET_EQ";
        
        case TOKEN_LSHIFT: return "LSHIFT";
        case TOKEN_RSHIFT: return "RSHIFT";
        case TOKEN_LSHIFT_EQ: return "LSHIFT_EQ";
        case TOKEN_RSHIFT_EQ: return "RSHIFT_EQ";
        
        case TOKEN_HASH: return "HASH";
        case TOKEN_HASH_HASH: return "HASH_HASH";
        
        case TOKEN_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

void token_print(const Token *token, FILE *out) {
    fprintf(out, "[%d:%d] %s", token->line, token->column, token_type_name(token->type));
    
    // Special case: "main" identifier is printed as MAIN
    if (token->type == TOKEN_IDENTIFIER && 
        token->length == 4 && 
        strncmp(token->lexeme, "main", 4) == 0) {
        fprintf(out, " MAIN");
    }
    
    if (token->length > 0) {
        fprintf(out, " \"%.*s\"", token->length, token->lexeme);
    }
    if (token->type == TOKEN_INT_CONSTANT || token->type == TOKEN_NUMBER) {
        fprintf(out, " (value: %lld)", token->value.int_val);
    } else if (token->type == TOKEN_FLOAT_CONSTANT) {
        fprintf(out, " (value: %f)", token->value.float_val);
    }
    fprintf(out, "\n");
}

// Keyword lookup table
TokenType keyword_lookup(const char *str, int length) {
    // Simple keyword lookup using string comparison
    // In production, use a hash table or perfect hash
    
    struct {
        const char *keyword;
        TokenType type;
    } keywords[] = {
        {"auto", TOKEN_AUTO},
        {"_Bool", TOKEN_BOOL},
        {"break", TOKEN_BREAK},
        {"case", TOKEN_CASE},
        {"char", TOKEN_CHAR},
        {"const", TOKEN_CONST},
        {"continue", TOKEN_CONTINUE},
        {"default", TOKEN_DEFAULT},
        {"do", TOKEN_DO},
        {"double", TOKEN_DOUBLE},
        {"else", TOKEN_ELSE},
        {"enum", TOKEN_ENUM},
        {"extern", TOKEN_EXTERN},
        {"float", TOKEN_FLOAT},
        {"for", TOKEN_FOR},
        {"goto", TOKEN_GOTO},
        {"if", TOKEN_IF},
        {"inline", TOKEN_INLINE},
        {"int", TOKEN_INT},
        {"long", TOKEN_LONG},
        {"register", TOKEN_REGISTER},
        {"restrict", TOKEN_RESTRICT},
        {"return", TOKEN_RETURN},
        {"short", TOKEN_SHORT},
        {"signed", TOKEN_SIGNED},
        {"sizeof", TOKEN_SIZEOF},
        {"static", TOKEN_STATIC},
        {"struct", TOKEN_STRUCT},
        {"switch", TOKEN_SWITCH},
        {"typedef", TOKEN_TYPEDEF},
        {"union", TOKEN_UNION},
        {"unsigned", TOKEN_UNSIGNED},
        {"void", TOKEN_VOID},
        {"volatile", TOKEN_VOLATILE},
        {"while", TOKEN_WHILE},
    };
    
    for (size_t i = 0; i < sizeof(keywords) / sizeof(keywords[0]); i++) {
        if (strncmp(str, keywords[i].keyword, length) == 0 && 
            strlen(keywords[i].keyword) == (size_t)length) {
            return keywords[i].type;
        }
    }
    
    return TOKEN_IDENTIFIER;
}
