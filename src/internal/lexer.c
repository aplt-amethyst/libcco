#include "lexer.h"

#include <ctype.h>
#include <stdint.h>
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

static inline int peek(cco_lexer_t* lexer)
{
    if (lexer->pos >= lexer->src_len)
        return -1;
    return lexer->src[lexer->pos];
}

static inline int advance(cco_lexer_t* lexer)
{
    int c = peek(lexer);
    if (c != -1)
    {
        lexer->pos++;
#ifndef CCO_ENABLE_FAST
        if (c == '\n')
        {
            lexer->line++;
            lexer->col = 1;
        }
        else
        {
            lexer->col++;
        }
#endif
    }
    return c;
}

static bool is_template_or_method_hash(const cco_lexer_t* lexer)
{
    size_t pos = lexer->pos;
    const char* src = lexer->src;
    size_t len = lexer->src_len;

    if (pos >= len || src[pos] != '#')
        return false;

    /* Check backward for "$function." */
    if (pos > 0)
    {
        size_t b = pos;
        while (b > 0 && (src[b - 1] == ' ' || src[b - 1] == '\t'))
            b--;
        if (b > 0 && src[b - 1] == '!')
            b--;
        while (b > 0 && (src[b - 1] == ' ' || src[b - 1] == '\t'))
            b--;
        if (b >= 10 && strncmp(src + b - 10, "$function.", 10) == 0)
            return true;
    }

    size_t p = pos + 1;
    if (p >= len)
        return false;

    /* A template or method name must begin with an alphabetic character or underscore */
    if (!isalpha((unsigned char)src[p]) && src[p] != '_')
        return false;

    /* Scan identifier (allowing dotted names like Provider.OpenAI) */
    while (p < len && (isalnum((unsigned char)src[p]) || src[p] == '_' || src[p] == '.'))
    {
        p++;
    }

    /* Skip horizontal whitespace between identifier and delimiter */
    while (p < len && (src[p] == ' ' || src[p] == '\t'))
    {
        p++;
    }

    if (p >= len)
        return false;

    /* Form 1: #Identifier( ... ) */
    if (src[p] == '(')
        return true;

    /* Form 2: #Identifier: ( ... ) or #Identifier:method( ... ) */
    if (src[p] == ':')
    {
        p++;
        while (p < len && (src[p] == ' ' || src[p] == '\t'))
        {
            p++;
        }
        if (p < len && src[p] == '(')
            return true;

        /* Check for static method: #Name:method( */
        if (p < len && (isalpha((unsigned char)src[p]) || src[p] == '_'))
        {
            while (p < len && (isalnum((unsigned char)src[p]) || src[p] == '_'))
                p++;
            while (p < len && (src[p] == ' ' || src[p] == '\t'))
                p++;
            if (p < len && src[p] == '(')
                return true;
        }
    }

    /* Form 3: #Identifier<Generic>( ... ) */
    if (src[p] == '<')
    {
        int depth = 1;
        p++;
        while (p < len && depth > 0)
        {
            if (src[p] == '<') depth++;
            else if (src[p] == '>') depth--;
            p++;
        }
        while (p < len && (src[p] == ' ' || src[p] == '\t'))
            p++;
        if (p < len && (src[p] == '(' || src[p] == ':'))
            return true;
    }

    return false;
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
            size_t start_line = lexer->line;
            size_t start_col = lexer->col;
            advance(lexer); /* skip '/' */
            advance(lexer); /* skip '*' */
            bool closed = false;
            while (peek(lexer) != -1)
            {
                if (peek(lexer) == '*' && lexer->pos + 1 < lexer->src_len &&
                    lexer->src[lexer->pos + 1] == '/')
                {
                    advance(lexer); /* '*' */
                    advance(lexer); /* '/' */
                    closed = true;
                    break;
                }
                advance(lexer);
            }
            if (!closed)
            {
                cco_diag_record(CCO_ERR_PARSE, start_line, start_col,
                                "skip_whitespace", "Unterminated block comment");
                return CCO_ERR_PARSE;
            }
        }
        else if (c == '#')
        {
            /* Distinguish template instantiation (#Point(...)) from single-line comment */
            if (is_template_or_method_hash(lexer))
            {
                break; /* Do not consume; let cco_lexer_next emit CCO_TOK_HASH */
            }

            /* Single-line comment: skip until '\n' or EOF */
            while (peek(lexer) != -1 && peek(lexer) != '\n')
            {
                advance(lexer);
            }
            if (peek(lexer) == '\n')
            {
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
    size_t start_col = lexer->col;
    size_t start_line = lexer->line;
    advance(lexer); /* skip " */

    size_t start_pos = lexer->pos;
    bool has_escape = false;

    /* Fast scan */
    while (lexer->pos < lexer->src_len) {
        char c = lexer->src[lexer->pos];
        if (c == '"') break;
        if (c == '\\') {
            has_escape = true;
            break;
        }
        lexer->pos++;
    }

    if (lexer->pos >= lexer->src_len) {
        cco_diag_record(CCO_ERR_PARSE, start_line, start_col, "parse_string",
                        "Unterminated string literal");
        return CCO_ERR_PARSE;
    }

    if (!has_escape) {
#ifndef CCO_ENABLE_FAST
        lexer->col += (lexer->pos - start_pos);
#endif
        out_token->type = CCO_TOK_STRING;
        out_token->text_ptr = lexer->src + start_pos;
        out_token->text_len = lexer->pos - start_pos;
        lexer->pos++; /* skip " */
#ifndef CCO_ENABLE_FAST
        lexer->col++;
#endif
        return CCO_OK;
    }

    /* Rewind and parse slowly with escapes */
    lexer->pos = start_pos;
#ifndef CCO_ENABLE_FAST
    lexer->col = start_col + 1;
#endif

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
            else if (ec == 'x')
            {
                int h1 = advance(lexer);
                int h2 = advance(lexer);
                if (h1 != -1 && h2 != -1 && isxdigit(h1) && isxdigit(h2))
                {
                    int v1 = (h1 >= '0' && h1 <= '9') ? (h1 - '0') :
                             (h1 >= 'a' && h1 <= 'f') ? (h1 - 'a' + 10) : (h1 - 'A' + 10);
                    int v2 = (h2 >= '0' && h2 <= '9') ? (h2 - '0') :
                             (h2 >= 'a' && h2 <= 'f') ? (h2 - 'a' + 10) : (h2 - 'A' + 10);
                    char byte_val = (char)((v1 << 4) | v2);
                    cco_strbuf_append(&lexer->scratch_buf, &byte_val, 1);
                }
                else
                {
                    cco_diag_record(CCO_ERR_INVALID_ARG, lexer->line, lexer->col,
                                    "parse_string", "Invalid hex escape sequence");
                    return CCO_ERR_INVALID_ARG;
                }
            }
            else if (ec == 'u')
            {
                int h[4];
                bool valid = true;
                for (int j = 0; j < 4; j++)
                {
                    h[j] = advance(lexer);
                    if (h[j] == -1 || !isxdigit(h[j])) { valid = false; break; }
                }
                if (valid)
                {
                    uint32_t cp = 0;
                    for (int j = 0; j < 4; j++)
                    {
                        int digit = (h[j] >= '0' && h[j] <= '9') ? (h[j] - '0') :
                                    (h[j] >= 'a' && h[j] <= 'f') ? (h[j] - 'a' + 10) :
                                    (h[j] - 'A' + 10);
                        cp = (cp << 4) | (uint32_t)digit;
                    }
                    if (cp <= 0x7F)
                    {
                        char b = (char)cp;
                        cco_strbuf_append(&lexer->scratch_buf, &b, 1);
                    }
                    else if (cp <= 0x7FF)
                    {
                        char b[2];
                        b[0] = (char)(0xC0 | ((cp >> 6) & 0x1F));
                        b[1] = (char)(0x80 | (cp & 0x3F));
                        cco_strbuf_append(&lexer->scratch_buf, b, 2);
                    }
                    else
                    {
                        char b[3];
                        b[0] = (char)(0xE0 | ((cp >> 12) & 0x0F));
                        b[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
                        b[2] = (char)(0x80 | (cp & 0x3F));
                        cco_strbuf_append(&lexer->scratch_buf, b, 3);
                    }
                }
                else
                {
                    cco_diag_record(CCO_ERR_INVALID_ARG, lexer->line, lexer->col,
                                    "parse_string", "Invalid unicode escape sequence");
                    return CCO_ERR_INVALID_ARG;
                }
            }
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

    cco_error_t ws_err = skip_whitespace(lexer);
    if (ws_err != CCO_OK)
    {
        out_token->type = CCO_TOK_EOF;
        out_token->text_ptr = NULL;
        out_token->text_len = 0;
        return ws_err;
    }

#ifndef CCO_ENABLE_FAST
    out_token->line = lexer->line;
    out_token->col = lexer->col;
#endif

    int c = peek(lexer);
    if (c == -1 || c == '\0')
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

    if (c == '`')
    {
        size_t start_col = lexer->col;
        size_t start_line = lexer->line;
        advance(lexer); /* skip ` */
        size_t start_pos = lexer->pos;
        while (lexer->pos < lexer->src_len && lexer->src[lexer->pos] != '`')
        {
#ifndef CCO_ENABLE_FAST
            if (lexer->src[lexer->pos] == '\n')
            {
                lexer->line++;
                lexer->col = 1;
            }
            else
            {
                lexer->col++;
            }
#endif
            lexer->pos++;
        }
        if (lexer->pos >= lexer->src_len)
        {
            cco_diag_record(CCO_ERR_PARSE, start_line, start_col, "parse_string",
                            "Unterminated raw string literal");
            return CCO_ERR_PARSE;
        }
        out_token->type = CCO_TOK_STRING;
        out_token->text_ptr = lexer->src + start_pos;
        out_token->text_len = lexer->pos - start_pos;
        advance(lexer); /* skip ` */
        return CCO_OK;
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

        if (c == '$' && lexer->pos + 1 < lexer->src_len && lexer->src[lexer->pos + 1] == '(') {
        out_token->type = CCO_TOK_DOLLAR_LPAREN;
        out_token->text_ptr = lexer->src + lexer->pos;
        out_token->text_len = 2;
        advance(lexer);
        advance(lexer);
        return CCO_OK;
    }
    if (isalpha(c) || c == '_' || c == '$')
    {
        size_t start = lexer->pos;
#ifdef CCO_ENABLE_FAST
        const char* s = lexer->src;
        size_t len_src = lexer->src_len;
        size_t p = lexer->pos;
        while (p < len_src) {
            unsigned char ch = s[p];
            if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '_' || ch == '-' || ch == '$')
                p++;
            else
                break;
        }
        lexer->pos = p;
#else
        while (isalnum(peek(lexer)) || peek(lexer) == '_' || peek(lexer) == '-' || peek(lexer) == '$')
        {
            advance(lexer);
        }
#endif
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

        /* Check for base prefixes: hex (0x/0X), binary (0b/0B), octal (0o/0O) */
        if (c == '0')
        {
            int next_c = (lexer->pos + 1 < lexer->src_len) ? lexer->src[lexer->pos + 1] : -1;
            if (next_c == 'x' || next_c == 'X')
            {
                advance(lexer); /* '0' */
                advance(lexer); /* 'x' or 'X' */
                while (isxdigit(peek(lexer)) || peek(lexer) == '_')
                {
                    advance(lexer);
                }
                out_token->type = CCO_TOK_INTEGER;
                out_token->text_ptr = lexer->src + start;
                out_token->text_len = lexer->pos - start;
                return CCO_OK;
            }
            if (next_c == 'b' || next_c == 'B')
            {
                advance(lexer); /* '0' */
                advance(lexer); /* 'b' or 'B' */
                while (peek(lexer) == '0' || peek(lexer) == '1' || peek(lexer) == '_')
                {
                    advance(lexer);
                }
                out_token->type = CCO_TOK_INTEGER;
                out_token->text_ptr = lexer->src + start;
                out_token->text_len = lexer->pos - start;
                return CCO_OK;
            }
            if (next_c == 'o' || next_c == 'O')
            {
                advance(lexer); /* '0' */
                advance(lexer); /* 'o' or 'O' */
                while ((peek(lexer) >= '0' && peek(lexer) <= '7') || peek(lexer) == '_')
                {
                    advance(lexer);
                }
                out_token->type = CCO_TOK_INTEGER;
                out_token->text_ptr = lexer->src + start;
                out_token->text_len = lexer->pos - start;
                return CCO_OK;
            }
        }

        /* Decimal digits before optional decimal point or exponent */
        while (isdigit(peek(lexer)) || peek(lexer) == '_')
        {
            advance(lexer);
        }

        /* Decimal point (distinguish from range operator '..' and '..=') */
        if (peek(lexer) == '.')
        {
            int next_char = (lexer->pos + 1 < lexer->src_len) ? lexer->src[lexer->pos + 1] : -1;
            if (next_char != '.')
            {
                is_float = true;
                advance(lexer); /* consume '.' */
                while (isdigit(peek(lexer)) || peek(lexer) == '_')
                {
                    advance(lexer);
                }
            }
        }

        /* Scientific notation exponent (e/E optionally followed by +/- and digits) */
        if (peek(lexer) == 'e' || peek(lexer) == 'E')
        {
            int after_e = (lexer->pos + 1 < lexer->src_len) ? lexer->src[lexer->pos + 1] : -1;
            if (after_e == '+' || after_e == '-')
            {
                int after_sign = (lexer->pos + 2 < lexer->src_len) ? lexer->src[lexer->pos + 2] : -1;
                if (isdigit(after_sign))
                {
                    is_float = true;
                    advance(lexer); /* consume 'e' or 'E' */
                    advance(lexer); /* consume '+' or '-' */
                    while (isdigit(peek(lexer)) || peek(lexer) == '_')
                    {
                        advance(lexer);
                    }
                }
            }
            else if (isdigit(after_e))
            {
                is_float = true;
                advance(lexer); /* consume 'e' or 'E' */
                while (isdigit(peek(lexer)) || peek(lexer) == '_')
                {
                    advance(lexer);
                }
            }
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
    case '@':
        out_token->type = CCO_TOK_AT;
        break;
    case ':':
        out_token->type = CCO_TOK_COLON;
        break;
    case '#':
        out_token->type = CCO_TOK_HASH;
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
        else if (peek(lexer) == '<')
        {
            advance(lexer);
            out_token->type = CCO_TOK_LSHIFT;
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
        else if (peek(lexer) == '>')
        {
            advance(lexer);
            out_token->type = CCO_TOK_RSHIFT;
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
    default:
        cco_diag_record(CCO_ERR_INVALID_ARG, lexer->line, lexer->col,
                        "cco_lexer_next", "Invalid token");
        return CCO_ERR_INVALID_ARG;
    }

    return CCO_OK;
}
