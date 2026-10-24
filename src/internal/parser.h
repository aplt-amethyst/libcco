#ifndef CNT_CCO_INTERNAL_PARSER_H
#define CNT_CCO_INTERNAL_PARSER_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"
#include "scope.h"
#include "object.h"

static inline int64_t cco_parse_integer_literal(const char* ptr, size_t len)
{
    char tmp[128];
    size_t cpy_len = 0;
    for (size_t i = 0; i < len && cpy_len < sizeof(tmp) - 1; i++)
    {
        if (ptr[i] != '_')
        {
            tmp[cpy_len++] = ptr[i];
        }
    }
    tmp[cpy_len] = '\0';

    const char* p = tmp;
    bool neg = false;
    if (*p == '-')
    {
        neg = true;
        p++;
    }
    else if (*p == '+')
    {
        p++;
    }

    int64_t val = 0;
    if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X'))
    {
        val = (int64_t)strtoull(p + 2, NULL, 16);
        return neg ? -val : val;
    }
    else if (p[0] == '0' && (p[1] == 'b' || p[1] == 'B'))
    {
        val = (int64_t)strtoull(p + 2, NULL, 2);
        return neg ? -val : val;
    }
    else if (p[0] == '0' && (p[1] == 'o' || p[1] == 'O'))
    {
        val = (int64_t)strtoull(p + 2, NULL, 8);
        return neg ? -val : val;
    }
    else
    {
        return strtoll(tmp, NULL, 10);
    }
}

static inline double cco_parse_float_literal(const char* ptr, size_t len)
{
    char tmp[128];
    size_t cpy_len = 0;
    for (size_t i = 0; i < len && cpy_len < sizeof(tmp) - 1; i++)
    {
        if (ptr[i] != '_')
        {
            tmp[cpy_len++] = ptr[i];
        }
    }
    tmp[cpy_len] = '\0';
    return strtod(tmp, NULL);
}

typedef struct cco_parser_context_s
{
    cco_lexer_t lexer;
    cco_token_t current_token;
    cco_token_t next_token;

    size_t depth;
    size_t step_count;

    const cco_parse_options_t* options;
    cco_arena_t* arena;

    cco_error_t last_error;
    bool recovery_mode;
    bool in_expr;
    bool in_return;
    bool in_generic_params;
    cco_scope_t* scope;
    struct cco_symtab_s* symtab;
    bool owns_symtab;
    size_t inst_count;

} cco_parser_context_t;

/* Initializes the parser context */
void cco_parser_init(cco_parser_context_t* ctx, const char* src, size_t len,
                     const cco_parse_options_t* opts, cco_arena_t* arena);

void cco_parser_advance(cco_parser_context_t* ctx);
cco_object_t* parse_expr(cco_parser_context_t* ctx);
bool check_field_type(cco_parser_context_t* ctx, cco_object_t* val,
                      const char* expected_type, const char* field_name);

typedef struct {
    size_t line;
    size_t col;
} cco_src_loc_t;

typedef enum {
    PARSE_BIND_OK,
    PARSE_BIND_NOT_MATCH,
    PARSE_BIND_ERROR
} cco_parse_bind_result_t;

cco_parse_bind_result_t cco_parse_bind(cco_parser_context_t* ctx, cco_src_loc_t* out_err_loc);

cco_object_t* cco_parse_function(cco_parser_context_t* ctx);

/* Parses the document. Returns the root object or NULL on error */
cco_object_t* cco_parse_document(cco_parser_context_t* ctx);

typedef struct cco_template_def_s cco_template_def_t;
cco_template_def_t* cco_parse_template_decl(cco_parser_context_t* ctx);

#endif /* CNT_CCO_INTERNAL_PARSER_H */
