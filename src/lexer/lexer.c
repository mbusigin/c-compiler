/**
 * lexer.c - Lexical analyzer for C source code
 * NOTE: Uses if-else chains instead of switch statements to avoid
 * codegen bugs in the self-compiled compiler's switch lowering.
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
    lexer->had_error = 0;
    return lexer;
}

void lexer_destroy(Lexer *lexer) {
    if (lexer) {
        free(lexer);
    }
}

static int is_at_end(const Lexer *lexer) {
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
    char c = lexer->source[lexer->position];
    lexer->position = lexer->position + 1;
    if (c == '\n') {
        lexer->line = lexer->line + 1;
        lexer->column = 1;
    } else {
        lexer->column = lexer->column + 1;
    }
    return c;
}

static void skip_whitespace(Lexer *lexer) {
    while (!is_at_end(lexer)) {
        char c = peek(lexer);
        if (c == ' ' || c == '\t' || c == '\r' || c == '\v' || c == '\f' || c == '\n') {
            advance(lexer);
            continue;
        }
        if (c == '/') {
            if (peek_next(lexer) == '/') {
                /* Single-line comment */
                while (!is_at_end(lexer) && peek(lexer) != '\n') {
                    advance(lexer);
                }
                continue;
            } else if (peek_next(lexer) == '*') {
                /* Multi-line comment */
                advance(lexer); /* skip / */
                advance(lexer); /* skip * */
                while (!is_at_end(lexer)) {
                    if (peek(lexer) == '*' && peek_next(lexer) == '/') {
                        advance(lexer);
                        advance(lexer);
                        break;
                    }
                    advance(lexer);
                }
                continue;
            } else {
                return;
            }
        }
        return;
    }
}

/* Write initial token fields into *out */
static void make_token(Token *out, Lexer *lexer, TokenType type) {
    out->type = type;
    out->lexeme = lexer->source + lexer->position;
    out->length = 0;
    out->line = lexer->line;
    out->column = lexer->column;
    out->value.int_val = 0;
}

static void scan_identifier_or_keyword(Token *out, Lexer *lexer) {
    make_token(out, lexer, TOKEN_IDENTIFIER);
    
    while (!is_at_end(lexer)) {
        char c = peek(lexer);
        if (isalnum(c) || c == '_') {
            advance(lexer);
        } else {
            break;
        }
    }
    
    out->length = (int)(lexer->source + lexer->position - out->lexeme);
    out->type = keyword_lookup(out->lexeme, out->length);
}

static void scan_number(Token *out, Lexer *lexer) {
    make_token(out, lexer, TOKEN_INT_CONSTANT);
    int has_dot = 0;
    int has_exponent = 0;
    int is_hex = 0;
    int is_binary = 0;
    int started_with_dot = 0;
    
    /* Check for hex or binary prefix */
    if (peek(lexer) == '0') {
        if (peek_next(lexer) == 'x' || peek_next(lexer) == 'X') {
            is_hex = 1;
            advance(lexer); advance(lexer);
        } else if (peek_next(lexer) == 'b' || peek_next(lexer) == 'B') {
            is_binary = 1;
            advance(lexer); advance(lexer);
        }
    }
    
    /* Handle numbers starting with decimal point (e.g., .25) */
    if (peek(lexer) == '.' && isdigit(peek_next(lexer))) {
        has_dot = 1;
        started_with_dot = 1;
        advance(lexer); /* consume '.' */
    }
    
    while (!is_at_end(lexer)) {
        char c = peek(lexer);
        
        if (isdigit(c) || (is_hex && isxdigit(c)) || 
            (is_binary && (c == '0' || c == '1'))) {
            advance(lexer);
        } else if (c == '.' && !has_dot && !has_exponent && !is_binary && !started_with_dot) {
            has_dot = 1;
            advance(lexer);
        } else if ((c == 'e' || c == 'E') && !has_exponent && !is_binary && !started_with_dot) {
            has_exponent = 1;
            advance(lexer);
            if (peek(lexer) == '+' || peek(lexer) == '-') {
                advance(lexer);
            }
        } else if ((c == 'p' || c == 'P') && is_hex && !has_exponent) {
            has_exponent = 1;
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
    
    out->length = (int)(lexer->source + lexer->position - out->lexeme);
    
    if (has_dot || has_exponent || started_with_dot) {
        out->type = TOKEN_FLOAT_CONSTANT;
        out->value.float_val = strtod(out->lexeme, NULL);
    } else {
        char *end;
        if (is_hex) {
            out->value.int_val = strtoll(out->lexeme + 2, &end, 16);
        } else if (is_binary) {
            out->value.int_val = strtoll(out->lexeme + 2, &end, 2);
        } else {
            out->value.int_val = strtoll(out->lexeme, &end, 10);
        }
    }
}

static void scan_string(Token *out, Lexer *lexer) {
    make_token(out, lexer, TOKEN_STRING_LITERAL);
    char quote = advance(lexer); /* Opening quote */
    (void)quote;
    
    while (!is_at_end(lexer) && peek(lexer) != quote) {
        if (peek(lexer) == '\\') {
            advance(lexer); /* Backslash */
            if (!is_at_end(lexer)) {
                advance(lexer); /* Escaped character */
            }
        } else if (peek(lexer) == '\n') {
            /* String cannot span lines */
            error("Unterminated string literal\n");
            lexer->had_error = 1;
            break;
        } else {
            advance(lexer);
        }
    }
    
    if (is_at_end(lexer) || peek(lexer) != quote) {
        error("Unterminated string literal\n");
        lexer->had_error = 1;
    } else {
        advance(lexer); /* Closing quote */
    }
    
    out->length = (int)(lexer->source + lexer->position - out->lexeme);
}

static void scan_char(Token *out, Lexer *lexer) {
    make_token(out, lexer, TOKEN_CHAR_CONSTANT);
    advance(lexer); /* Opening quote */
    
    char char_value = 0;
    if (peek(lexer) == '\\') {
        advance(lexer); /* Backslash */
        char escape_char = peek(lexer);
        if (escape_char == 'n') char_value = '\n';
        else if (escape_char == 't') char_value = '\t';
        else if (escape_char == 'r') char_value = '\r';
        else if (escape_char == '0') char_value = '\0';
        else if (escape_char == '\\') char_value = '\\';
        else if (escape_char == '\'') char_value = '\'';
        else if (escape_char == '"') char_value = '"';
        else char_value = escape_char;
        advance(lexer); /* Escaped character */
    } else if (peek(lexer) != '\'') {
        char_value = peek(lexer);
        advance(lexer);
    }
    
    if (peek(lexer) == '\'') {
        advance(lexer); /* Closing quote */
    } else {
        error("Unterminated character constant\n");
        lexer->had_error = 1;
    }
    
    out->length = (int)(lexer->source + lexer->position - out->lexeme);
    out->value.int_val = char_value;
}

static void scan_operator(Token *out, Lexer *lexer) {
    make_token(out, lexer, TOKEN_ERROR);
    char c = advance(lexer);
    
    if (c == '+') {
        if (peek(lexer) == '+') { advance(lexer); out->type = TOKEN_PLUS_PLUS; }
        else if (peek(lexer) == '=') { advance(lexer); out->type = TOKEN_PLUS_EQ; }
        else { out->type = TOKEN_PLUS; }
    } else if (c == '-') {
        if (peek(lexer) == '-') { advance(lexer); out->type = TOKEN_MINUS_MINUS; }
        else if (peek(lexer) == '>') { advance(lexer); out->type = TOKEN_ARROW; }
        else if (peek(lexer) == '=') { advance(lexer); out->type = TOKEN_MINUS_EQ; }
        else { out->type = TOKEN_MINUS; }
    } else if (c == '*') {
        if (peek(lexer) == '=') { advance(lexer); out->type = TOKEN_STAR_EQ; }
        else { out->type = TOKEN_STAR; }
    } else if (c == '/') {
        if (peek(lexer) == '=') { advance(lexer); out->type = TOKEN_SLASH_EQ; }
        else { out->type = TOKEN_SLASH; }
    } else if (c == '%') {
        if (peek(lexer) == '=') { advance(lexer); out->type = TOKEN_PERCENT_EQ; }
        else { out->type = TOKEN_PERCENT; }
    } else if (c == '=') {
        if (peek(lexer) == '=') { advance(lexer); out->type = TOKEN_EQ_EQ; }
        else { out->type = TOKEN_ASSIGN; }
    } else if (c == '!') {
        if (peek(lexer) == '=') { advance(lexer); out->type = TOKEN_EXCLAIM_EQ; }
        else { out->type = TOKEN_EXCLAIM; }
    } else if (c == '<') {
        if (peek(lexer) == '=') { advance(lexer); out->type = TOKEN_LESS_EQ; }
        else if (peek(lexer) == '<') {
            advance(lexer);
            if (peek(lexer) == '=') { advance(lexer); out->type = TOKEN_LSHIFT_EQ; }
            else { out->type = TOKEN_LSHIFT; }
        } else { out->type = TOKEN_LESS; }
    } else if (c == '>') {
        if (peek(lexer) == '=') { advance(lexer); out->type = TOKEN_GREATER_EQ; }
        else if (peek(lexer) == '>') {
            advance(lexer);
            if (peek(lexer) == '=') { advance(lexer); out->type = TOKEN_RSHIFT_EQ; }
            else { out->type = TOKEN_RSHIFT; }
        } else { out->type = TOKEN_GREATER; }
    } else if (c == '&') {
        if (peek(lexer) == '&') { advance(lexer); out->type = TOKEN_AMP_AMP; }
        else if (peek(lexer) == '=') { advance(lexer); out->type = TOKEN_AMP_EQ; }
        else { out->type = TOKEN_AMPERSAND; }
    } else if (c == '|') {
        if (peek(lexer) == '|') { advance(lexer); out->type = TOKEN_PIPE_PIPE; }
        else if (peek(lexer) == '=') { advance(lexer); out->type = TOKEN_PIPE_EQ; }
        else { out->type = TOKEN_PIPE; }
    } else if (c == '^') {
        if (peek(lexer) == '=') { advance(lexer); out->type = TOKEN_CARET_EQ; }
        else { out->type = TOKEN_CARET; }
    } else if (c == '#') {
        if (peek(lexer) == '#') { advance(lexer); out->type = TOKEN_HASH_HASH; }
        else { out->type = TOKEN_HASH; }
    } else {
        out->type = TOKEN_ERROR;
    }
    
    out->length = (int)(lexer->source + lexer->position - out->lexeme);
}

void lexer_next_token(Lexer *lexer, Token *out) {
    skip_whitespace(lexer);
    
    if (is_at_end(lexer)) {
        make_token(out, lexer, TOKEN_EOF);
        out->lexeme = "";
        out->length = 0;
        return;
    }
    
    char c = peek(lexer);
    
    if (isalpha(c) || c == '_') {
        scan_identifier_or_keyword(out, lexer);
        return;
    }
    
    if (isdigit(c)) {
        scan_number(out, lexer);
        return;
    }
    
    /* Simple single-char tokens */
    if (c == '(') { make_token(out, lexer, TOKEN_LPAREN); advance(lexer); out->length = 1; return; }
    if (c == ')') { make_token(out, lexer, TOKEN_RPAREN); advance(lexer); out->length = 1; return; }
    if (c == '[') { make_token(out, lexer, TOKEN_LBRACKET); advance(lexer); out->length = 1; return; }
    if (c == ']') { make_token(out, lexer, TOKEN_RBRACKET); advance(lexer); out->length = 1; return; }
    if (c == '{') { make_token(out, lexer, TOKEN_LBRACE); advance(lexer); out->length = 1; return; }
    if (c == '}') { make_token(out, lexer, TOKEN_RBRACE); advance(lexer); out->length = 1; return; }
    if (c == ',') { make_token(out, lexer, TOKEN_COMMA); advance(lexer); out->length = 1; return; }
    if (c == ';') { make_token(out, lexer, TOKEN_SEMICOLON); advance(lexer); out->length = 1; return; }
    if (c == '~') { make_token(out, lexer, TOKEN_TILDE); advance(lexer); out->length = 1; return; }
    if (c == '?') { make_token(out, lexer, TOKEN_QUESTION); advance(lexer); out->length = 1; return; }
    if (c == ':') { make_token(out, lexer, TOKEN_COLON); advance(lexer); out->length = 1; return; }
    if (c == '.') {
        /* Check for ellipsis (...) */
        if (peek(lexer) == '.' && peek_next(lexer) == '.') {
            advance(lexer); advance(lexer); advance(lexer);
            make_token(out, lexer, TOKEN_ELLIPSIS);
            out->length = 3;
            return;
        }
        /* Check if it's a float literal (e.g., .25) */
        if (isdigit(peek(lexer))) {
            scan_number(out, lexer);
            return;
        }
        make_token(out, lexer, TOKEN_DOT); advance(lexer); out->length = 1; return;
    }
    if (c == '"') { scan_string(out, lexer); return; }
    if (c == '\'') { scan_char(out, lexer); return; }
    scan_operator(out, lexer);
}

void lexer_peek_token(Lexer *lexer, Token *out) {
    out->type = lexer->current_token.type;
    out->lexeme = lexer->current_token.lexeme;
    out->length = lexer->current_token.length;
    out->line = lexer->current_token.line;
    out->column = lexer->current_token.column;
    out->value.int_val = lexer->current_token.value.int_val;
}

void lexer_advance(Lexer *lexer) {
    lexer_next_token(lexer, &lexer->current_token);
}

void token_copy(Token *dst, const Token *src) {
    dst->type = src->type;
    dst->lexeme = src->lexeme;
    dst->length = src->length;
    dst->line = src->line;
    dst->column = src->column;
    dst->value.int_val = src->value.int_val;
}

const char *token_type_name(TokenType type) {
    if (type == TOKEN_EOF) return "EOF";
    if (type == TOKEN_IDENTIFIER) return "IDENTIFIER";
    if (type == TOKEN_INT_CONSTANT || type == TOKEN_NUMBER) return "NUMBER";
    if (type == TOKEN_FLOAT_CONSTANT) return "FLOAT_CONSTANT";
    if (type == TOKEN_CHAR_CONSTANT) return "CHAR_CONSTANT";
    if (type == TOKEN_STRING_LITERAL) return "STRING_LITERAL";
    if (type == TOKEN_AUTO) return "AUTO";
    if (type == TOKEN_BREAK) return "BREAK";
    if (type == TOKEN_CASE) return "CASE";
    if (type == TOKEN_CHAR) return "CHAR";
    if (type == TOKEN_CONST) return "CONST";
    if (type == TOKEN_CONTINUE) return "CONTINUE";
    if (type == TOKEN_DEFAULT) return "DEFAULT";
    if (type == TOKEN_DO) return "DO";
    if (type == TOKEN_DOUBLE) return "DOUBLE";
    if (type == TOKEN_ELSE) return "ELSE";
    if (type == TOKEN_ENUM) return "ENUM";
    if (type == TOKEN_EXTERN) return "EXTERN";
    if (type == TOKEN_FLOAT) return "FLOAT";
    if (type == TOKEN_FOR) return "FOR";
    if (type == TOKEN_GOTO) return "GOTO";
    if (type == TOKEN_IF) return "IF";
    if (type == TOKEN_INLINE) return "INLINE";
    if (type == TOKEN_INT) return "INT";
    if (type == TOKEN_LONG) return "LONG";
    if (type == TOKEN_REGISTER) return "REGISTER";
    if (type == TOKEN_RESTRICT) return "RESTRICT";
    if (type == TOKEN_RETURN) return "RETURN";
    if (type == TOKEN_SHORT) return "SHORT";
    if (type == TOKEN_SIGNED) return "SIGNED";
    if (type == TOKEN_SIZEOF) return "SIZEOF";
    if (type == TOKEN_STATIC) return "STATIC";
    if (type == TOKEN_STRUCT) return "STRUCT";
    if (type == TOKEN_SWITCH) return "SWITCH";
    if (type == TOKEN_TYPEDEF) return "TYPEDEF";
    if (type == TOKEN_UNION) return "UNION";
    if (type == TOKEN_UNSIGNED) return "UNSIGNED";
    if (type == TOKEN_VOID) return "VOID";
    if (type == TOKEN_VOLATILE) return "VOLATILE";
    if (type == TOKEN_WHILE) return "WHILE";
    if (type == TOKEN_LPAREN) return "LPAREN";
    if (type == TOKEN_RPAREN) return "RPAREN";
    if (type == TOKEN_LBRACKET) return "LBRACKET";
    if (type == TOKEN_RBRACKET) return "RBRACKET";
    if (type == TOKEN_LBRACE) return "LBRACE";
    if (type == TOKEN_RBRACE) return "RBRACE";
    if (type == TOKEN_COMMA) return "COMMA";
    if (type == TOKEN_SEMICOLON) return "SEMICOLON";
    if (type == TOKEN_COLON) return "COLON";
    if (type == TOKEN_DOT) return "DOT";
    if (type == TOKEN_ELLIPSIS) return "ELLIPSIS";
    if (type == TOKEN_ASSIGN) return "ASSIGN";
    if (type == TOKEN_EQ_EQ) return "EQ_EQ";
    if (type == TOKEN_EXCLAIM_EQ) return "EXCLAIM_EQ";
    if (type == TOKEN_LESS) return "LESS";
    if (type == TOKEN_GREATER) return "GREATER";
    if (type == TOKEN_LESS_EQ) return "LESS_EQ";
    if (type == TOKEN_GREATER_EQ) return "GREATER_EQ";
    if (type == TOKEN_PLUS) return "PLUS";
    if (type == TOKEN_MINUS) return "MINUS";
    if (type == TOKEN_STAR) return "STAR";
    if (type == TOKEN_SLASH) return "SLASH";
    if (type == TOKEN_PERCENT) return "PERCENT";
    if (type == TOKEN_PLUS_PLUS) return "PLUS_PLUS";
    if (type == TOKEN_MINUS_MINUS) return "MINUS_MINUS";
    if (type == TOKEN_PLUS_EQ) return "PLUS_EQ";
    if (type == TOKEN_MINUS_EQ) return "MINUS_EQ";
    if (type == TOKEN_STAR_EQ) return "STAR_EQ";
    if (type == TOKEN_SLASH_EQ) return "SLASH_EQ";
    if (type == TOKEN_PERCENT_EQ) return "PERCENT_EQ";
    if (type == TOKEN_AMPERSAND) return "AMPERSAND";
    if (type == TOKEN_PIPE) return "PIPE";
    if (type == TOKEN_CARET) return "CARET";
    if (type == TOKEN_TILDE) return "TILDE";
    if (type == TOKEN_EXCLAIM) return "EXCLAIM";
    if (type == TOKEN_QUESTION) return "QUESTION";
    if (type == TOKEN_ARROW) return "ARROW";
    if (type == TOKEN_AMP_AMP) return "AMP_AMP";
    if (type == TOKEN_PIPE_PIPE) return "PIPE_PIPE";
    if (type == TOKEN_AMP_EQ) return "AMP_EQ";
    if (type == TOKEN_PIPE_EQ) return "PIPE_EQ";
    if (type == TOKEN_CARET_EQ) return "CARET_EQ";
    if (type == TOKEN_LSHIFT) return "LSHIFT";
    if (type == TOKEN_RSHIFT) return "RSHIFT";
    if (type == TOKEN_LSHIFT_EQ) return "LSHIFT_EQ";
    if (type == TOKEN_RSHIFT_EQ) return "RSHIFT_EQ";
    if (type == TOKEN_HASH) return "HASH";
    if (type == TOKEN_HASH_HASH) return "HASH_HASH";
    if (type == TOKEN_ERROR) return "ERROR";
    return "UNKNOWN";
}

void token_print(const Token *token, FILE *out) {
    fprintf(out, "[%d:%d] %s", token->line, token->column, token_type_name(token->type));
    
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

/* Keyword lookup - uses direct comparisons to avoid static array init issues */
TokenType keyword_lookup(const char *str, int length) {
    if (length == 3 && str[0] == 'i' && str[1] == 'n' && str[2] == 't') return TOKEN_INT;
    if (length == 4 && str[0] == 'v' && str[1] == 'o' && str[2] == 'i' && str[3] == 'd') return TOKEN_VOID;
    if (length == 4 && str[0] == 'c' && str[1] == 'h' && str[2] == 'a' && str[3] == 'r') return TOKEN_CHAR;
    if (length == 2 && str[0] == 'i' && str[1] == 'f') return TOKEN_IF;
    if (length == 4 && str[0] == 'e' && str[1] == 'l' && str[2] == 's' && str[3] == 'e') return TOKEN_ELSE;
    if (length == 5 && str[0] == 'w' && str[1] == 'h' && str[2] == 'i' && str[3] == 'l' && str[4] == 'e') return TOKEN_WHILE;
    if (length == 3 && str[0] == 'f' && str[1] == 'o' && str[2] == 'r') return TOKEN_FOR;
    if (length == 6 && str[0] == 'r' && str[1] == 'e' && str[2] == 't' && str[3] == 'u' && str[4] == 'r' && str[5] == 'n') return TOKEN_RETURN;
    if (length == 6 && str[0] == 's' && str[1] == 't' && str[2] == 'r' && str[3] == 'u' && str[4] == 'c' && str[5] == 't') return TOKEN_STRUCT;
    if (length == 5 && str[0] == 'b' && str[1] == 'r' && str[2] == 'e' && str[3] == 'a' && str[4] == 'k') return TOKEN_BREAK;
    if (length == 8 && str[0] == 'c' && str[1] == 'o' && str[2] == 'n' && str[3] == 't' && str[4] == 'i' && str[5] == 'n' && str[6] == 'u' && str[7] == 'e') return TOKEN_CONTINUE;
    if (length == 6 && str[0] == 's' && str[1] == 'w' && str[2] == 'i' && str[3] == 't' && str[4] == 'c' && str[5] == 'h') return TOKEN_SWITCH;
    if (length == 4 && str[0] == 'c' && str[1] == 'a' && str[2] == 's' && str[3] == 'e') return TOKEN_CASE;
    if (length == 7 && str[0] == 'd' && str[1] == 'e' && str[2] == 'f' && str[3] == 'a' && str[4] == 'u' && str[5] == 'l' && str[6] == 't') return TOKEN_DEFAULT;
    if (length == 4 && str[0] == 'l' && str[1] == 'o' && str[2] == 'n' && str[3] == 'g') return TOKEN_LONG;
    if (length == 5 && str[0] == 's' && str[1] == 'h' && str[2] == 'o' && str[3] == 'r' && str[4] == 't') return TOKEN_SHORT;
    if (length == 6 && str[0] == 's' && str[1] == 'i' && str[2] == 'g' && str[3] == 'n' && str[4] == 'e' && str[5] == 'd') return TOKEN_SIGNED;
    if (length == 8 && str[0] == 'u' && str[1] == 'n' && str[2] == 's' && str[3] == 'i' && str[4] == 'g' && str[5] == 'n' && str[6] == 'e' && str[7] == 'd') return TOKEN_UNSIGNED;
    if (length == 6 && str[0] == 'd' && str[1] == 'o' && str[2] == 'u' && str[3] == 'b' && str[4] == 'l' && str[5] == 'e') return TOKEN_DOUBLE;
    if (length == 5 && str[0] == 'f' && str[1] == 'l' && str[2] == 'o' && str[3] == 'a' && str[4] == 't') return TOKEN_FLOAT;
    if (length == 4 && str[0] == 'a' && str[1] == 'u' && str[2] == 't' && str[3] == 'o') return TOKEN_AUTO;
    if (length == 5 && str[0] == 'c' && str[1] == 'o' && str[2] == 'n' && str[3] == 's' && str[4] == 't') return TOKEN_CONST;
    if (length == 6 && str[0] == 's' && str[1] == 't' && str[2] == 'a' && str[3] == 't' && str[4] == 'i' && str[5] == 'c') return TOKEN_STATIC;
    if (length == 6 && str[0] == 'e' && str[1] == 'x' && str[2] == 't' && str[3] == 'e' && str[4] == 'r' && str[5] == 'n') return TOKEN_EXTERN;
    if (length == 4 && str[0] == 'e' && str[1] == 'n' && str[2] == 'u' && str[3] == 'm') return TOKEN_ENUM;
    if (length == 5 && str[0] == 'u' && str[1] == 'n' && str[2] == 'i' && str[3] == 'o' && str[4] == 'n') return TOKEN_UNION;
    if (length == 8 && str[0] == 'v' && str[1] == 'o' && str[2] == 'l' && str[3] == 'a' && str[4] == 't' && str[5] == 'i' && str[6] == 'l' && str[7] == 'e') return TOKEN_VOLATILE;
    if (length == 7 && str[0] == 't' && str[1] == 'y' && str[2] == 'p' && str[3] == 'e' && str[4] == 'd' && str[5] == 'e' && str[6] == 'f') return TOKEN_TYPEDEF;
    if (length == 6 && str[0] == 's' && str[1] == 'i' && str[2] == 'z' && str[3] == 'e' && str[4] == 'o' && str[5] == 'f') return TOKEN_SIZEOF;
    if (length == 4 && str[0] == 'g' && str[1] == 'o' && str[2] == 't' && str[3] == 'o') return TOKEN_GOTO;
    if (length == 6 && str[0] == 'i' && str[1] == 'n' && str[2] == 'l' && str[3] == 'i' && str[4] == 'n' && str[5] == 'e') return TOKEN_INLINE;
    if (length == 2 && str[0] == 'd' && str[1] == 'o') return TOKEN_DO;
    if (length == 5 && str[0] == '_' && str[1] == 'B' && str[2] == 'o' && str[3] == 'o' && str[4] == 'l') return TOKEN_BOOL;
    return TOKEN_IDENTIFIER;
}
