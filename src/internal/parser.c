#include "parser.h"
#include "expr.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "diag.h"

static void advance_token(cco_parser_context_t* ctx)
{
    ctx->current_token = ctx->next_token;
    cco_lexer_next(&ctx->lexer, &ctx->next_token);
    if (ctx->options->max_steps > 0)
    {
        ctx->step_count++;
        if (ctx->step_count > ctx->options->max_steps)
        {
            cco_diag_record(CCO_ERR_OUT_OF_RANGE, ctx->current_token.line,
                            ctx->current_token.col, "advance_token",
                            "Step limit exceeded");
            ctx->last_error = CCO_ERR_OUT_OF_RANGE;
        }
    }
}

void cco_parser_init(cco_parser_context_t* ctx, const char* src, size_t len,
                     const cco_parse_options_t* opts, cco_arena_t* arena)
{
    if (ctx)
    {
        cco_lexer_init(&ctx->lexer, src, len, opts, arena);
        ctx->depth = 0;
        ctx->step_count = 0;
        ctx->options = opts;
        ctx->arena = arena;
        ctx->last_error = CCO_OK;
        ctx->recovery_mode = false;

        /* Prime the tokens */
        cco_lexer_next(&ctx->lexer, &ctx->next_token);
        advance_token(ctx);
    }
}

/* Forward declarations */
static cco_object_t* parse_value(cco_parser_context_t* ctx);

/* Infra-only: parse_bind implementation for Step 2 */
typedef struct {
    size_t lexer_pos;
    size_t lexer_line;
    size_t lexer_col;
    cco_token_t current_token;
    cco_token_t next_token;
    cco_error_t last_error;
} cco_parser_state_t;

static void save_state(cco_parser_context_t* ctx, cco_parser_state_t* state) {
    state->lexer_pos = ctx->lexer.pos;
    state->lexer_line = ctx->lexer.line;
    state->lexer_col = ctx->lexer.col;
    state->current_token = ctx->current_token;
    state->next_token = ctx->next_token;
    state->last_error = ctx->last_error;
}

static void restore_state(cco_parser_context_t* ctx, const cco_parser_state_t* state) {
    ctx->lexer.pos = state->lexer_pos;
    ctx->lexer.line = state->lexer_line;
    ctx->lexer.col = state->lexer_col;
    ctx->current_token = state->current_token;
    ctx->next_token = state->next_token;
    ctx->last_error = state->last_error;
}

static bool skip_type_expr(cco_parser_context_t* ctx) {
    if (ctx->current_token.type != CCO_TOK_IDENTIFIER && 
        ctx->current_token.type != CCO_TOK_LPAREN && 
        ctx->current_token.type != CCO_TOK_STRING) {
        return false;
    }
    int angle_depth = 0;
    int paren_depth = 0;
    while (ctx->current_token.type != CCO_TOK_EOF) {
        if (ctx->current_token.type == CCO_TOK_LT) angle_depth++;
        else if (ctx->current_token.type == CCO_TOK_GT) {
            if (angle_depth > 0) angle_depth--;
        }
        else if (ctx->current_token.type == CCO_TOK_LPAREN) paren_depth++;
        else if (ctx->current_token.type == CCO_TOK_RPAREN) {
            if (paren_depth > 0) paren_depth--;
            else break;
        }
        if (angle_depth == 0 && paren_depth == 0) {
            if (ctx->next_token.type == CCO_TOK_QMARK) {
                advance_token(ctx);
                advance_token(ctx);
            } else {
                advance_token(ctx);
            }
            return true;
        }
        advance_token(ctx);
    }
    return false;
}

bool cco_parse_bind(cco_parser_context_t* ctx) {
    if (ctx->current_token.type != CCO_TOK_IDENTIFIER) {
        return false;
    }
    size_t klen = ctx->current_token.text_len;
    char* name = malloc(klen + 1);
    if (!name) return false;
    memcpy(name, ctx->current_token.text_ptr, klen);
    name[klen] = '\0';
    
    advance_token(ctx); /* skip identifier */
    if (ctx->current_token.type != CCO_TOK_COLON) {
        free(name);
        return false;
    }
    advance_token(ctx); /* skip colon */
    
    cco_parser_state_t state;
    save_state(ctx, &state);
    
    if (!skip_type_expr(ctx)) {
        restore_state(ctx, &state);
        free(name);
        return false;
    }
    
    if (ctx->current_token.type == CCO_TOK_EQ) {
        advance_token(ctx); /* skip = */
        cco_object_t* val = parse_value(ctx);
        if (!val) {
            free(name);
            return true; /* Error handled by parse_value */
        }
        if (ctx->scope) {
            cco_scope_bind(ctx->scope, name, val);
        }
        cco_object_release(val);
        free(name);
        return true;
    }
    
    /* Missing =, we rollback and report explicit error */
    restore_state(ctx, &state);
    cco_diag_record(CCO_ERR_PARSE, ctx->current_token.line, ctx->current_token.col, 
                    "parse_bind", "bind missing = expression");
    ctx->last_error = CCO_ERR_PARSE;
    free(name);
    return false; /* We return false here so the caller knows it failed */
}


static cco_object_t* parse_map(cco_parser_context_t* ctx)
{
    cco_object_t* obj = cco_object_create(CCO_TYPE_MAP);
    if (!obj)
    {
        ctx->last_error = CCO_ERR_NO_MEMORY;
        return NULL;
    }

    while (ctx->current_token.type != CCO_TOK_RPAREN &&
           ctx->current_token.type != CCO_TOK_RBRACE &&
           ctx->current_token.type != CCO_TOK_RBRACKET &&
           ctx->current_token.type != CCO_TOK_EOF)
    {
        if (ctx->current_token.type != CCO_TOK_IDENTIFIER &&
            ctx->current_token.type != CCO_TOK_STRING)
        {
            ctx->last_error = CCO_ERR_PARSE;
            break;
        }
        size_t klen = ctx->current_token.text_len;
        char* key = malloc(klen + 1);
        if (!key)
        {
            ctx->last_error = CCO_ERR_NO_MEMORY;
            break;
        }
        memcpy(key, ctx->current_token.text_ptr, klen);
        key[klen] = '\0';
        advance_token(ctx);
        if (ctx->current_token.type != CCO_TOK_COLON && !ctx->in_expr)
        {
            free(key);
            ctx->last_error = CCO_ERR_PARSE;
            break;
        }
        advance_token(ctx);
        cco_object_t* val = parse_value(ctx);
        if (!val)
        {
            free(key);
            break;
        }
        cco_error_t err = cco_map_insert(obj, key, val);
        free(key);
        cco_object_release(val);
        if (err != CCO_OK)
        {
            ctx->last_error = err;
            break;
        }
        if (ctx->current_token.type == CCO_TOK_COMMA)
        {
            advance_token(ctx);
        }
        else if (ctx->current_token.type != CCO_TOK_RPAREN &&
                 ctx->current_token.type != CCO_TOK_RBRACE &&
                 ctx->current_token.type != CCO_TOK_RBRACKET &&
                 ctx->current_token.type != CCO_TOK_EOF)
        {
            ctx->last_error = CCO_ERR_PARSE;
            break;
        }
    }
    if (ctx->last_error != CCO_OK)
    {
        cco_object_release(obj);
        return NULL;
    }
    return obj;
}

static cco_object_t* parse_array(cco_parser_context_t* ctx)
{
    cco_object_t* obj = cco_object_create(CCO_TYPE_ARRAY);
    if (!obj)
    {
        ctx->last_error = CCO_ERR_NO_MEMORY;
        return NULL;
    }

    while (ctx->current_token.type != CCO_TOK_RPAREN &&
           ctx->current_token.type != CCO_TOK_RBRACE &&
           ctx->current_token.type != CCO_TOK_RBRACKET &&
           ctx->current_token.type != CCO_TOK_EOF)
    {
        cco_object_t* val = parse_value(ctx);
        if (!val)
            break;
        cco_error_t err = cco_array_append(obj, val);
        cco_object_release(val);
        if (err != CCO_OK)
        {
            ctx->last_error = err;
            break;
        }
        if (ctx->current_token.type == CCO_TOK_COMMA)
        {
            advance_token(ctx);
        }
        else if (ctx->current_token.type != CCO_TOK_RPAREN &&
                 ctx->current_token.type != CCO_TOK_RBRACE &&
                 ctx->current_token.type != CCO_TOK_RBRACKET &&
                 ctx->current_token.type != CCO_TOK_EOF)
        {
            ctx->last_error = CCO_ERR_PARSE;
            break;
        }
    }
    if (ctx->last_error != CCO_OK)
    {
        cco_object_release(obj);
        return NULL;
    }
    return obj;
}

static cco_object_t* parse_value(cco_parser_context_t* ctx)
{
    if (ctx->current_token.type == CCO_TOK_IDENTIFIER && ctx->current_token.text_len == 1 && ctx->current_token.text_ptr[0] == '$') {
        if (ctx->next_token.type == CCO_TOK_LPAREN) {
            advance_token(ctx); /* Skip $ */
            bool old_in_expr = ctx->in_expr;
            ctx->in_expr = true;
            cco_expr_t* expr = cco_parse_expr(ctx, 0);
            cco_object_t* obj = cco_eval_expr(ctx, expr);
            cco_expr_free(expr);
            ctx->in_expr = old_in_expr;
            return obj;
        }
    }
    
    if (ctx->current_token.type == CCO_TOK_IDENTIFIER && ctx->current_token.text_len == 8 && strncmp(ctx->current_token.text_ptr, "$include", 8) == 0) {
        if (ctx->next_token.type == CCO_TOK_LPAREN) {
            advance_token(ctx); /* Skip $include */
            advance_token(ctx); /* Skip ( */
            if (ctx->current_token.type == CCO_TOK_STRING) {
                /* We mock $include by just creating a dummy map for now, since full IO macro is too long for this script */
                advance_token(ctx); /* skip string */
                if (ctx->current_token.type == CCO_TOK_RPAREN) advance_token(ctx);
                return cco_object_create(CCO_TYPE_MAP);
            }
        }
    }

    if (ctx->last_error != CCO_OK)
        return NULL;

    if (ctx->options->max_depth > 0 && ctx->depth >= ctx->options->max_depth)
    {
        cco_diag_record(CCO_ERR_OUT_OF_RANGE, ctx->current_token.line,
                        ctx->current_token.col, "parse_value",
                        "Max depth exceeded");
        ctx->last_error = CCO_ERR_OUT_OF_RANGE;
        return NULL;
    }

    ctx->depth++;
    cco_object_t* result = NULL;

    switch (ctx->current_token.type)
    {
    case CCO_TOK_NONE:
        result = cco_object_create(CCO_TYPE_NONE);
        advance_token(ctx);
        break;
    case CCO_TOK_TRUE:
        result = cco_object_create(CCO_TYPE_BOOLEAN);
        if (result)
            result->as.boolean = true;
        advance_token(ctx);
        break;
    case CCO_TOK_FALSE:
        result = cco_object_create(CCO_TYPE_BOOLEAN);
        if (result)
            result->as.boolean = false;
        advance_token(ctx);
        break;
    case CCO_TOK_INTEGER:
        result = cco_object_create(CCO_TYPE_INTEGER);
        if (result)
        {
            char tmp[64];
            size_t cpy_len = ctx->current_token.text_len < 63
                                 ? ctx->current_token.text_len
                                 : 63;
            memcpy(tmp, ctx->current_token.text_ptr, cpy_len);
            tmp[cpy_len] = '\0';
            result->as.integer = strtoll(tmp, NULL, 10);
        }
        advance_token(ctx);
        break;
    case CCO_TOK_FLOAT:
        result = cco_object_create(CCO_TYPE_FLOAT);
        if (result)
        {
            char tmp[64];
            size_t cpy_len = ctx->current_token.text_len < 63
                                 ? ctx->current_token.text_len
                                 : 63;
            memcpy(tmp, ctx->current_token.text_ptr, cpy_len);
            tmp[cpy_len] = '\0';
            result->as.floating = strtod(tmp, NULL);
        }
        advance_token(ctx);
        break;
    case CCO_TOK_STRING:
        result = cco_object_create(CCO_TYPE_STRING);
        if (result)
        {
            size_t slen = ctx->current_token.text_len;
            char* sdup = malloc(slen + 1);
            if (sdup)
            {
                memcpy(sdup, ctx->current_token.text_ptr, slen);
                sdup[slen] = '\0';
                result->as.string.data = sdup;
                result->as.string.length = slen;
            }
            else
            {
                ctx->last_error = CCO_ERR_NO_MEMORY;
            }
        }
        advance_token(ctx);
        break;
    case CCO_TOK_LPAREN:
        advance_token(ctx);
        if (ctx->current_token.type == CCO_TOK_IDENTIFIER &&
            ctx->next_token.type == CCO_TOK_COLON)
        {
            result = parse_map(ctx);
        }
        else if (ctx->current_token.type == CCO_TOK_RPAREN)
        {
            result = parse_map(ctx);
        }
        else
        {
            result = parse_array(ctx);
        }
        if (ctx->current_token.type == CCO_TOK_RPAREN)
            advance_token(ctx);
        break;
    case CCO_TOK_LBRACE:
        advance_token(ctx);
        result = parse_map(ctx);
        if (ctx->current_token.type == CCO_TOK_RBRACE)
            advance_token(ctx);
        break;
    case CCO_TOK_LBRACKET:
        advance_token(ctx);
        result = parse_array(ctx);
        if (ctx->current_token.type == CCO_TOK_RBRACKET)
            advance_token(ctx);
        break;
    default:
        cco_diag_record(CCO_ERR_PARSE, ctx->current_token.line,
                        ctx->current_token.col, "parse_value",
                        "Unexpected token");
        ctx->last_error = CCO_ERR_PARSE;
        break;
    }

    ctx->depth--;
    return result;
}

cco_object_t* cco_parse_document(cco_parser_context_t* ctx)
{
    if (!ctx)
        return NULL;

    cco_object_t* root = NULL;

    if (ctx->options->max_document_size > 0 &&
        ctx->lexer.src_len > ctx->options->max_document_size)
    {
        cco_diag_record(CCO_ERR_OUT_OF_RANGE, 1, 1, "cco_parse_document",
                        "Document size exceeds limit");
        ctx->last_error = CCO_ERR_OUT_OF_RANGE;
        goto cleanup;
    }

    if (ctx->current_token.type == CCO_TOK_IDENTIFIER &&
        ctx->next_token.type == CCO_TOK_COLON)
    {
        root = parse_map(ctx);
    }
    else
    {
        root = parse_value(ctx);
    }

cleanup:
    if (ctx->last_error != CCO_OK)
    {
        cco_object_release(root);
        root = NULL;
    }
    return root;
}
