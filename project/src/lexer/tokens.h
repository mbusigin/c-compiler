#ifndef TOKEN_H
#define TOKEN_H

// Token types for the C lexer
typedef enum {
    // End of file
    TOKEN_EOF,
    
    // Identifiers and literals
    TOKEN_IDENTIFIER,
    TOKEN_INT_CONSTANT,
    TOKEN_FLOAT_CONSTANT,
    TOKEN_CHAR_CONSTANT,
    TOKEN_STRING_LITERAL,
    TOKEN_NUMBER,
    
    // Keywords
    TOKEN_AUTO,
    TOKEN_BREAK,
    TOKEN_CASE,
    TOKEN_CHAR,
    TOKEN_CONST,
    TOKEN_CONTINUE,
    TOKEN_DEFAULT,
    TOKEN_DO,
    TOKEN_DOUBLE,
    TOKEN_ELSE,
    TOKEN_ENUM,
    TOKEN_EXTERN,
    TOKEN_FLOAT,
    TOKEN_FOR,
    TOKEN_GOTO,
    TOKEN_IF,
    TOKEN_INLINE,
    TOKEN_INT,
    TOKEN_LONG,
    TOKEN_REGISTER,
    TOKEN_RESTRICT,
    TOKEN_RETURN,
    TOKEN_SHORT,
    TOKEN_SIGNED,
    TOKEN_SIZEOF,
    TOKEN_STATIC,
    TOKEN_STRUCT,
    TOKEN_SWITCH,
    TOKEN_TYPEDEF,
    TOKEN_UNION,
    TOKEN_UNSIGNED,
    TOKEN_VOID,
    TOKEN_VOLATILE,
    TOKEN_WHILE,
    TOKEN_ALIGNAS,
    TOKEN_ALIGNOF,
    TOKEN_ATOMIC,
    TOKEN_BOOL,
    TOKEN_COMPLEX,
    TOKEN_IMAGINARY,
    TOKEN_NORETURN,
    TOKEN_STATIC_ASSERT,
    TOKEN_THREAD_LOCAL,
    
    // Operators
    TOKEN_ASSIGN,         // =
    TOKEN_PLUS,           // +
    TOKEN_MINUS,          // -
    TOKEN_STAR,           // *
    TOKEN_SLASH,          // /
    TOKEN_PERCENT,        // %
    TOKEN_AMPERSAND,      // &
    TOKEN_PIPE,           // |
    TOKEN_CARET,          // ^
    TOKEN_TILDE,          // ~
    TOKEN_EXCLAIM,        // !
    TOKEN_QUESTION,       // ?
    TOKEN_COLON,          // :
    TOKEN_DOT,            // .
    TOKEN_ARROW,          // ->
    
    // Compound assignment
    TOKEN_PLUS_EQ,        // +=
    TOKEN_MINUS_EQ,      // -=
    TOKEN_STAR_EQ,       // *=
    TOKEN_SLASH_EQ,      // /=
    TOKEN_PERCENT_EQ,    // %=
    TOKEN_AMP_EQ,        // &=
    TOKEN_PIPE_EQ,       // |=
    TOKEN_CARET_EQ,      // ^=
    TOKEN_LSHIFT_EQ,     // <<=
    TOKEN_RSHIFT_EQ,     // >>=
    
    // Increment/Decrement
    TOKEN_PLUS_PLUS,     // ++
    TOKEN_MINUS_MINUS,   // --
    
    // Relational
    TOKEN_LESS,          // <
    TOKEN_GREATER,       // >
    TOKEN_LESS_EQ,       // <=
    TOKEN_GREATER_EQ,    // >=
    TOKEN_EQ_EQ,         // ==
    TOKEN_EXCLAIM_EQ,    // !=
    
    // Logical
    TOKEN_AMP_AMP,       // &&
    TOKEN_PIPE_PIPE,     // ||
    
    // Shift
    TOKEN_LSHIFT,        // <<
    TOKEN_RSHIFT,        // >>
    
    // Punctuation
    TOKEN_LPAREN,        // (
    TOKEN_RPAREN,        // )
    TOKEN_LBRACKET,      // [
    TOKEN_RBRACKET,      // ]
    TOKEN_LBRACE,        // {
    TOKEN_RBRACE,        // }
    TOKEN_COMMA,         // ,
    TOKEN_SEMICOLON,     // ;
    TOKEN_ELLIPSIS,      // ...
    
    // Preprocessor
    TOKEN_HASH,          // #
    TOKEN_HASH_HASH,     // ##

    // Special
    TOKEN_ERROR          // Lexical error
} TokenType;

// Token structure with source location
typedef struct {
    TokenType type;
    const char *lexeme;      // Original text
    int length;              // Length of lexeme
    int line;                // Line number
    int column;              // Column number
    union {
        long long int_val;   // Integer constant value
        double float_val;    // Float constant value
    } value;
} Token;

// Keyword lookup
TokenType keyword_lookup(const char *str, int length);

#endif // TOKEN_H
