#include "lexer.h"

#include <ctype.h>
#include <string.h>

#include "diag.h"

void cco_lexer_init(cco_lexer_t* lexer, const char* src, size_t src_len,
                    const cco_parse_options_t* opts, cco_arena_t* arena)
{
    if (lexer)
    {
        lexer->src = src;
        lexer->src_len = src_len;
        lexer->pos = 0;
        lexer->line = 1;
        lexer->col = 1;
        lexer->options = opts;
        cco_strbuf_init(&lexer->scratch_buf, arena);
    }
}

static int peek(cco_lexer_t* lexer)
{
    if (lexer->pos >= lexer->src_len)
        return -1;
    return lexer->src[lexer->pos];
}

static int advance(cco_lexer_t* lexer)
{
    int c = peek(lexer);
    if (c != -1)
    {
        lexer->pos++;
        if (c == '\n')
        {
            lexer->line++;
            lexer->col = 1;
        }
        else
        {
            lexer->col++;
        }
    }
    return c;
}

static cco_error_t skip_whitespace(cco_lexer_t* lexer)
{
    while (1)
    {
        int c = peek(lexer);
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
        {
            advance(lexer);
        }
        else if (c == '/' && lexer->pos + 1 < lexer->src_len &&
                 lexer->src[lexer->pos + 1] == '*')
        {
            /* Block comment */
#ifdef CCO_ENABLE_COMMENT_PRESERVE
            if (lexer->options->preserve_comments)
            {
                return CCO_OK; /* Let the main loop handle it as a token */
            }
#endif
            advance(lexer); /* skip '/' */
            advance(lexer); /* skip '*' */
            while (peek(lexer) != -1)
            {
                if (peek(lexer) == '*' && lexer->pos + 1 < lexer->src_len &&
                    lexer->src[lexer->pos + 1] == '/')
                {
                    advance(lexer); /* '*' */
                    advance(lexer); /* '/' */
                    break;
                }
                advance(lexer);
            }
        }
        else
        {
            break;
        }
    }
    return CCO_OK;
}

static cco_error_t parse_string(cco_lexer_t* lexer, cco_token_t* out_token)
{
    /* simplified string parser */
    size_t start_col = lexer->col;
    size_t start_line = lexer->line;
    advance(lexer); /* skip " */

    lexer->scratch_buf.length = 0; /* reset */

    while (peek(lexer) != -1 && peek(lexer) != '"')
    {
        int c = advance(lexer);
        if (c == '\\')
        {
            int ec = advance(lexer);
            if (ec == 'n')
                cco_strbuf_append(&lexer->scratch_buf, "\n", 1);
            else if (ec == 't')
                cco_strbuf_append(&lexer->scratch_buf, "\t", 1);
            else if (ec == 'r')
                cco_strbuf_append(&lexer->scratch_buf, "\r", 1);
            else if (ec == '"')
                cco_strbuf_append(&lexer->scratch_buf, "\"", 1);
            else if (ec == '\\')
                cco_strbuf_append(&lexer->scratch_buf, "\\", 1);
            else
            {
                cco_diag_record(CCO_ERR_INVALID_ARG, lexer->line, lexer->col,
                                "parse_string", "Invalid escape sequence");
                return CCO_ERR_INVALID_ARG;
            }
        }
        else
        {
            char ch = (char)c;
            cco_strbuf_append(&lexer->scratch_buf, &ch, 1);
        }
    }

    if (peek(lexer) == -1)
    {
        cco_diag_record(CCO_ERR_PARSE, start_line, start_col, "parse_string",
                        "Unterminated string literal");
        return CCO_ERR_PARSE;
    }
    advance(lexer); /* skip " */

    out_token->type = CCO_TOK_STRING;
    out_token->text_ptr = lexer->scratch_buf.data;
    out_token->text_len = lexer->scratch_buf.length;
    return CCO_OK;
}

cco_error_t cco_lexer_next(cco_lexer_t* lexer, cco_token_t* out_token)
{
    if (!lexer || !out_token)
        return CCO_ERR_INVALID_ARG;

    skip_whitespace(lexer);

    out_token->line = lexer->line;
    out_token->col = lexer->col;

    int c = peek(lexer);
    if (c == -1)
    {
        out_token->type = CCO_TOK_EOF;
        out_token->text_ptr = NULL;
        out_token->text_len = 0;
        return CCO_OK;
    }

    if (c == '"')
    {
        return parse_string(lexer, out_token);
    }


    if (c == 'r' && peek(lexer) == '"')
    {
        advance(lexer); /* skip r */
        advance(lexer); /* skip " */
        if (peek(lexer) == '(') {
            advance(lexer); /* skip ( */
            lexer->scratch_buf.length = 0;
            size_t start_line = lexer->line;
            size_t start_col = lexer->col;
            while (peek(lexer) != -1) {
                if (peek(lexer) == ')') {
                    int next1 = lexer->pos + 1 < lexer->src_len ? lexer->src[lexer->pos + 1] : -1;
                    if (next1 == '"') {
                        advance(lexer); /* skip ) */
                        advance(lexer); /* skip " */
                        break;
                    }
                }
                char ch = (char)advance(lexer);
                cco_strbuf_append(&lexer->scratch_buf, &ch, 1);
            }
            if (peek(lexer) == -1) {
                cco_diag_record(CCO_ERR_PARSE, start_line, start_col, "parse_raw_string",
                                "Unterminated raw string");
                return CCO_ERR_PARSE;
            }
            out_token->type = CCO_TOK_STRING;
            out_token->text_ptr = lexer->scratch_buf.data;
            out_token->text_len = lexer->scratch_buf.length;
            return CCO_OK;
        } else {
            /* Fallback or error? For now just let it fail or handle normally */
            cco_diag_record(CCO_ERR_PARSE, lexer->line, lexer->col, "parse_raw_string",
                            "Expected ( after r\"");
            return CCO_ERR_PARSE;
        }
    }

    if (isalpha(c) || c == '_' || c == '$')
    {
        size_t start = lexer->pos;
        while (isalnum(peek(lexer)) || peek(lexer) == '_' || peek(lexer) == '-' || peek(lexer) == '$')
        {
            advance(lexer);
        }
        size_t len = lexer->pos - start;
        out_token->text_ptr = lexer->src + start;
        out_token->text_len = len;

        if (len == 4 && strncmp(out_token->text_ptr, "None", 4) == 0)
        {
            out_token->type = CCO_TOK_NONE;
        }
        else if (len == 4 && strncmp(out_token->text_ptr, "null", 4) == 0)
        {
            out_token->type = CCO_TOK_NONE;
        }
        else if (len == 4 && strncmp(out_token->text_ptr, "true", 4) == 0)
        {
            out_token->type = CCO_TOK_TRUE;
        }
        else if (len == 5 && strncmp(out_token->text_ptr, "false", 5) == 0)
        {
            out_token->type = CCO_TOK_FALSE;
        }
        else if (len == 7 && strncmp(out_token->text_ptr, "$return", 7) == 0)
        {
            out_token->type = CCO_TOK_KW_RETURN;
        }
        else if (len == 5 && strncmp(out_token->text_ptr, "$this", 5) == 0)
        {
            out_token->type = CCO_TOK_KW_THIS;
        }
        else
        {
            out_token->type = CCO_TOK_IDENTIFIER;
        }
        return CCO_OK;
    }

    if (isdigit(c))
    {
        size_t start = lexer->pos;
        bool is_float = false;
        while (isalnum(peek(lexer)) || peek(lexer) == '_' || peek(lexer) == '.')
        {
            if (peek(lexer) == '.') is_float = true;
            advance(lexer);
        }
        out_token->type = is_float ? CCO_TOK_FLOAT : CCO_TOK_INTEGER;
        out_token->text_ptr = lexer->src + start;
        out_token->text_len = lexer->pos - start;
        return CCO_OK;
    }

    advance(lexer);
    out_token->text_ptr = lexer->src + lexer->pos - 1;
    out_token->text_len = 1;

    switch (c)
    {
    case '(':
        out_token->type = CCO_TOK_LPAREN;
        break;
    case ')':
        out_token->type = CCO_TOK_RPAREN;
        break;
    case '{':
        out_token->type = CCO_TOK_LBRACE;
        break;
    case '}':
        out_token->type = CCO_TOK_RBRACE;
        break;
    case '[':
        out_token->type = CCO_TOK_LBRACKET;
        break;
    case ']':
        out_token->type = CCO_TOK_RBRACKET;
        break;
    case ':':
        out_token->type = CCO_TOK_COLON;
        break;
    case ',':
        out_token->type = CCO_TOK_COMMA;
        break;

    case '?':
        if (peek(lexer) == '?') { advance(lexer); out_token->type = CCO_TOK_QQ; out_token->text_len = 2; }
        else out_token->type = CCO_TOK_QMARK;
        break;
    case '~': out_token->type = CCO_TOK_TILDE; break;
    case '^': out_token->type = CCO_TOK_CARET; break;
    case '+':
        out_token->type = CCO_TOK_PLUS;
        break;
    case '-':
        out_token->type = CCO_TOK_MINUS;
        break;
    case '*':
        if (peek(lexer) == '*') { advance(lexer); out_token->type = CCO_TOK_STARSTAR; out_token->text_len = 2; }
        else out_token->type = CCO_TOK_STAR;
        break;
    case '/':
        out_token->type = CCO_TOK_SLASH;
        break;
    case '=':
        if (peek(lexer) == '=')
        {
            advance(lexer);
            out_token->type = CCO_TOK_EQEQ;
            out_token->text_len = 2;
        }
        else
            out_token->type = CCO_TOK_EQ;
        break;
    case '!':
        if (peek(lexer) == '=')
        {
            advance(lexer);
            out_token->type = CCO_TOK_NEQ;
            out_token->text_len = 2;
        }
        else
            out_token->type = CCO_TOK_NOT;
        break;
    case '<':
        if (peek(lexer) == '=')
        {
            advance(lexer);
            out_token->type = CCO_TOK_LTE;
            out_token->text_len = 2;
        }
        else
            out_token->type = CCO_TOK_LT;
        break;
    case '>':
        if (peek(lexer) == '=')
        {
            advance(lexer);
            out_token->type = CCO_TOK_GTE;
            out_token->text_len = 2;
        }
        else
            out_token->type = CCO_TOK_GT;
        break;
    case '&':
        if (peek(lexer) == '&')
        {
            advance(lexer);
            out_token->type = CCO_TOK_AND;
            out_token->text_len = 2;
        }
        else
        {
            out_token->type = CCO_TOK_AMPERSAND;
        }
        break;
    case '|':
        if (peek(lexer) == '|')
        {
            advance(lexer);
            out_token->type = CCO_TOK_OR;
            out_token->text_len = 2;
        }
        else
            out_token->type = CCO_TOK_PIPE;
        break;
    case '.':
        if (peek(lexer) == '.') {
            advance(lexer);
            if (peek(lexer) == '=') {
                advance(lexer);
                out_token->type = CCO_TOK_DOTDOTEQ;
                out_token->text_len = 3;
            } else {
                out_token->type = CCO_TOK_DOTDOT;
                out_token->text_len = 2;
            }
        } else {
            out_token->type = CCO_TOK_DOT;
        }
        break;
    case '#':
        out_token->type = CCO_TOK_HASH;
        break;
    default:
        cco_diag_record(CCO_ERR_INVALID_ARG, lexer->line, lexer->col,
                        "cco_lexer_next", "Invalid token");
        return CCO_ERR_INVALID_ARG;
    }

    return CCO_OK;
}
