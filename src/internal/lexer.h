#ifndef CNT_CCO_INTERNAL_LEXER_H
#define CNT_CCO_INTERNAL_LEXER_H

#include <stddef.h>

#include <cnt/cco_error.h>
#include <cnt/cco_options.h>

#include "strbuf.h"

typedef enum
{
    CCO_TOK_EOF = 0,
    CCO_TOK_LPAREN,
    CCO_TOK_RPAREN,
    CCO_TOK_LBRACE,
    CCO_TOK_RBRACE,
    CCO_TOK_LBRACKET,
    CCO_TOK_RBRACKET,
    CCO_TOK_COLON,
    CCO_TOK_COMMA,
    CCO_TOK_PLUS,
    CCO_TOK_MINUS,
    CCO_TOK_STAR,
    CCO_TOK_SLASH,
    CCO_TOK_EQ,
    CCO_TOK_EQEQ,
    CCO_TOK_NEQ,
    CCO_TOK_LT,
    CCO_TOK_GT,
    CCO_TOK_LTE,
    CCO_TOK_GTE,
    CCO_TOK_AND,
    CCO_TOK_OR,
    CCO_TOK_NOT,
    CCO_TOK_AMPERSAND,
    
    CCO_TOK_QQ,
    CCO_TOK_STARSTAR,
    CCO_TOK_TILDE,
    CCO_TOK_CARET,
    CCO_TOK_QMARK,
    CCO_TOK_DOTDOT,
    CCO_TOK_DOTDOTEQ,    CCO_TOK_PIPE,
    CCO_TOK_DOT,
    CCO_TOK_HASH,
    CCO_TOK_DOLLAR,

    CCO_TOK_NONE,
    CCO_TOK_TRUE,
    CCO_TOK_FALSE,

    CCO_TOK_INTEGER,
    CCO_TOK_FLOAT,
    CCO_TOK_STRING,
    CCO_TOK_IDENTIFIER,
    CCO_TOK_KW_RETURN,
    CCO_TOK_KW_THIS,
    CCO_TOK_COMMENT
} cco_token_type_t;

typedef struct
{
    cco_token_type_t type;
    size_t line;
    size_t col;
    const char* text_ptr; /* Pointer into original buffer or interned string */
    size_t text_len;
} cco_token_t;

typedef struct
{
    const char* src;
    size_t src_len;
    size_t pos;
    size_t line;
    size_t col;
    const cco_parse_options_t* options;
    cco_strbuf_t scratch_buf; /* For building unescaped strings */
} cco_lexer_t;

void cco_lexer_init(cco_lexer_t* lexer, const char* src, size_t src_len,
                    const cco_parse_options_t* opts, cco_arena_t* arena);

/* Scans the next token. If EOF is reached, type is set to CCO_TOK_EOF.
   Returns CCO_OK on success, or an error code on invalid characters. */
cco_error_t cco_lexer_next(cco_lexer_t* lexer, cco_token_t* out_token);

#endif /* CNT_CCO_INTERNAL_LEXER_H */
