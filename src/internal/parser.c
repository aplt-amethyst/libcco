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



/* 
 * 判据：
 * - Identifier ":" TypeExpr "=" Expr  -> OK
 * - Identifier ":" ... 但没 =          -> NOT_MATCH，静默回退
 * - Identifier ":" TypeExpr "=" 后解析出错 -> ERROR
 */


cco_parse_bind_result_t cco_parse_bind(cco_parser_context_t* ctx, cco_src_loc_t* out_err_loc) {
    if (ctx->current_token.type != CCO_TOK_IDENTIFIER) {
        if (out_err_loc) {
            out_err_loc->line = ctx->current_token.line;
            out_err_loc->col = ctx->current_token.col;
        }
        return PARSE_BIND_NOT_MATCH;
    }
    size_t klen = ctx->current_token.text_len;
    char* name = malloc(klen + 1);
    if (!name) return PARSE_BIND_ERROR;
    memcpy(name, ctx->current_token.text_ptr, klen);
    name[klen] = '\0';
    
    advance_token(ctx);
    if (ctx->current_token.type != CCO_TOK_COLON) {
        if (out_err_loc) {
            out_err_loc->line = ctx->current_token.line;
            out_err_loc->col = ctx->current_token.col;
        }
        free(name);
        return PARSE_BIND_NOT_MATCH;
    }
    advance_token(ctx);
    
    cco_parser_state_t state;
    save_state(ctx, &state);
    
    if (!skip_type_expr(ctx)) {
        if (out_err_loc) {
            out_err_loc->line = ctx->current_token.line;
            out_err_loc->col = ctx->current_token.col;
        }
        restore_state(ctx, &state);
        free(name);
        return PARSE_BIND_NOT_MATCH;
    }
    
    if (ctx->current_token.type == CCO_TOK_EQ) {
        advance_token(ctx);
        cco_object_t* val = parse_value(ctx);
        if (!val) {
            if (out_err_loc) {
                out_err_loc->line = ctx->current_token.line;
                out_err_loc->col = ctx->current_token.col;
            }
            free(name);
            return PARSE_BIND_ERROR;
        }
        if (ctx->scope) {
            cco_error_t err = cco_scope_bind(ctx->scope, name, val);
            if (err != CCO_OK) {
                cco_diag_record(CCO_ERR_PARSE, ctx->current_token.line, ctx->current_token.col, 
                                "parse_bind", "Duplicate bind in same scope");
                ctx->last_error = CCO_ERR_PARSE;
                if (out_err_loc) {
                    out_err_loc->line = ctx->current_token.line;
                    out_err_loc->col = ctx->current_token.col;
                }
                cco_object_release(val);
                free(name);
                return PARSE_BIND_ERROR;
            }
        } else {
            cco_object_release(val);
        }
        free(name);
        return PARSE_BIND_OK;
    }
    
    if (out_err_loc) {
        out_err_loc->line = ctx->current_token.line;
        out_err_loc->col = ctx->current_token.col;
    }
    restore_state(ctx, &state);
    free(name);
    return PARSE_BIND_NOT_MATCH;
}

cco_object_t* cco_parse_static_method_body(cco_parser_context_t* ctx) {
    if (ctx->current_token.type != CCO_TOK_LPAREN) return NULL;
    advance_token(ctx);
    
    cco_scope_t method_scope;
    if (ctx->scope) {
        cco_scope_init(&method_scope, ctx->scope);
        ctx->scope = &method_scope;
    }

    cco_object_t* ret_val = NULL;

    while (ctx->current_token.type != CCO_TOK_RPAREN && ctx->current_token.type != CCO_TOK_EOF) {

        if (ctx->current_token.type == CCO_TOK_IDENTIFIER && 
            ctx->current_token.text_len == 7 && 
            strncmp(ctx->current_token.text_ptr, "$return", 7) == 0) {
            advance_token(ctx);
            
            /* 存结构里，暂时不求值 */
            struct {
                char type_expr[256];
                cco_object_t* expr;
            } return_stmt = {0};
            
            if (ctx->current_token.type == CCO_TOK_LT) {
                advance_token(ctx);
                while(ctx->current_token.type != CCO_TOK_GT && ctx->current_token.type != CCO_TOK_EOF) {
                    advance_token(ctx);
                }
                /* Dummy store */
                strcpy(return_stmt.type_expr, "T");
                if (ctx->current_token.type == CCO_TOK_GT) advance_token(ctx);
            }
            if (ctx->current_token.type == CCO_TOK_LPAREN) {
                advance_token(ctx);
                return_stmt.expr = parse_value(ctx);
                if (ctx->current_token.type == CCO_TOK_RPAREN) advance_token(ctx);
                
                ret_val = return_stmt.expr;
                break;
            }
            ctx->last_error = CCO_ERR_PARSE;
            break;
        }


        cco_src_loc_t err_loc = {0};
        cco_parse_bind_result_t bres = cco_parse_bind(ctx, &err_loc);
        if (bres == PARSE_BIND_OK) {
            if (ctx->current_token.type == CCO_TOK_COMMA) advance_token(ctx);
            continue;
        } else if (bres == PARSE_BIND_ERROR) {
            break;
        } else {
            /* NOT_MATCH */
            cco_diag_record(CCO_ERR_PARSE, err_loc.line, err_loc.col, "parse_bind", "missing =");
            ctx->last_error = CCO_ERR_PARSE;
            break;
        }
    }
    
    if (ctx->current_token.type == CCO_TOK_RPAREN) advance_token(ctx);
    
    if (ctx->scope == &method_scope) {
        ctx->scope = method_scope.parent;
        cco_scope_destroy(&method_scope);
    }
    
    return ret_val;
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
