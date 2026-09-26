#ifndef CNT_CCO_INTERNAL_PARSER_H
#define CNT_CCO_INTERNAL_PARSER_H

#include "lexer.h"
#include "scope.h"
#include "object.h"

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
    cco_scope_t* scope;

} cco_parser_context_t;

/* Initializes the parser context */
void cco_parser_init(cco_parser_context_t* ctx, const char* src, size_t len,
                     const cco_parse_options_t* opts, cco_arena_t* arena);

/* Parses the document. Returns the root object or NULL on error */
bool cco_parse_bind(cco_parser_context_t* ctx);

cco_object_t* cco_parse_document(cco_parser_context_t* ctx);

#endif /* CNT_CCO_INTERNAL_PARSER_H */
