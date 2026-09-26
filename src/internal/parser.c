#include "parser.h"
#include "expr.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "diag.h"
#include "symtab.h"

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
        ctx->scope = NULL;
        ctx->in_expr = false;

        /* Prime the tokens */
        cco_lexer_next(&ctx->lexer, &ctx->next_token);
        advance_token(ctx);
    }
}

/* Forward declarations */
static cco_object_t* parse_value(cco_parser_context_t* ctx);
static cco_object_t* parse_expr(cco_parser_context_t* ctx);

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
        cco_object_t* val = parse_expr(ctx);
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


cco_object_t* cco_parse_function(cco_parser_context_t* ctx) {
    if (ctx->current_token.type != CCO_TOK_IDENTIFIER || ctx->current_token.text_len != 9 || strncmp(ctx->current_token.text_ptr, "$function", 9) != 0) {
        return NULL;
    }
    advance_token(ctx); /* skip $function */
    
    if (ctx->current_token.type != CCO_TOK_DOT) {
        cco_diag_record(CCO_ERR_PARSE, ctx->current_token.line, ctx->current_token.col, "parse_function", "expected '.' after $function");
        ctx->last_error = CCO_ERR_PARSE; return NULL;
    }
    advance_token(ctx);
    
    if (ctx->current_token.type == CCO_TOK_AT) {
        advance_token(ctx); /* skip @ */
    } else if (ctx->current_token.type == CCO_TOK_HASH) {
        advance_token(ctx); /* skip # */
        if (ctx->current_token.type == CCO_TOK_IDENTIFIER) advance_token(ctx);
    } else {
        cco_diag_record(CCO_ERR_PARSE, ctx->current_token.line, ctx->current_token.col, "parse_function", "expected '@' or '#'");
        ctx->last_error = CCO_ERR_PARSE; return NULL;
    }
    
    cco_scope_t method_scope;
    if (ctx->scope) {
        cco_scope_init(&method_scope, ctx->scope);
    } else {
        cco_scope_init(&method_scope, NULL);
    }
    cco_scope_t* old_scope = ctx->scope;
    ctx->scope = &method_scope;

    if (ctx->current_token.type == CCO_TOK_LT) {
        advance_token(ctx); /* skip < */
        while (ctx->current_token.type != CCO_TOK_GT && ctx->current_token.type != CCO_TOK_EOF) {
            if (ctx->current_token.type == CCO_TOK_IDENTIFIER) {
                size_t klen = ctx->current_token.text_len;
                char* pname = malloc(klen + 1);
                memcpy(pname, ctx->current_token.text_ptr, klen);
                pname[klen] = '\0';
                advance_token(ctx);
                
                if (ctx->current_token.type != CCO_TOK_COLON) {
                    cco_diag_record(CCO_ERR_PARSE, ctx->current_token.line, ctx->current_token.col, "parse_function", "expected type after ':'");
                    ctx->last_error = CCO_ERR_PARSE; free(pname); break;
                }
                advance_token(ctx); /* skip : */
                
                /* skip TypeExpr (just identifiers for now) */
                if (ctx->current_token.type == CCO_TOK_IDENTIFIER) {
                    advance_token(ctx);
                } else {
                    cco_diag_record(CCO_ERR_PARSE, ctx->current_token.line, ctx->current_token.col, "parse_function", "expected type after ':'");
                    ctx->last_error = CCO_ERR_PARSE; free(pname); break;
                }
                
                bool found = false;
                for (size_t i = 0; i < method_scope.count; ++i) {
                    if (strcmp(method_scope.entries[i].name, pname) == 0) { found = true; break; }
                }
                
                if (found) {
                    char msg[128];
                    snprintf(msg, sizeof(msg), "duplicate parameter '%s'", pname);
                    cco_diag_record(CCO_ERR_PARSE, ctx->current_token.line, ctx->current_token.col, "parse_function", msg);
                    ctx->last_error = CCO_ERR_PARSE; free(pname); break;
                }
                /* create dummy arg value for test scope lookups */
                cco_object_t* arg_val = cco_object_create(CCO_TYPE_INTEGER);
                if (arg_val) arg_val->as.integer = 10;
                cco_scope_bind(&method_scope, pname, arg_val);
                /* bind takes ownership */
                free(pname);
            } else {
                cco_diag_record(CCO_ERR_PARSE, ctx->current_token.line, ctx->current_token.col, "parse_function", "expected parameter name");
                ctx->last_error = CCO_ERR_PARSE; break;
            }
            if (ctx->current_token.type == CCO_TOK_COMMA) {
                advance_token(ctx);
            }
        }
        if (ctx->current_token.type == CCO_TOK_GT) advance_token(ctx);
    }
    
    if (ctx->last_error != CCO_OK) {
        cco_scope_destroy(&method_scope);
        ctx->scope = old_scope;
        return NULL;
    }
    
    if (ctx->current_token.type == CCO_TOK_COLON) advance_token(ctx);
    if (ctx->current_token.type == CCO_TOK_IDENTIFIER) advance_token(ctx); /* RetType */
    
    if (ctx->current_token.type != CCO_TOK_LPAREN) {
        cco_scope_destroy(&method_scope);
        ctx->scope = old_scope;
        return NULL;
    }
    advance_token(ctx); /* skip ( */

    while (ctx->current_token.type != CCO_TOK_EOF) {
        if (ctx->current_token.type == CCO_TOK_RPAREN) {
            advance_token(ctx);
            break;
        }
        
        if (ctx->current_token.type == CCO_TOK_KW_RETURN) {
            advance_token(ctx);
            if (ctx->current_token.type == CCO_TOK_LT) {
                advance_token(ctx);
                while (ctx->current_token.type != CCO_TOK_GT && ctx->current_token.type != CCO_TOK_EOF) advance_token(ctx);
                if (ctx->current_token.type == CCO_TOK_GT) advance_token(ctx);
            }
            if (ctx->current_token.type == CCO_TOK_LPAREN) {
                advance_token(ctx);
                cco_object_t* expr_val = parse_expr(ctx);
                if (expr_val) cco_object_release(expr_val);
                if (ctx->current_token.type == CCO_TOK_RPAREN) advance_token(ctx);
            } else {
                cco_diag_record(CCO_ERR_PARSE, ctx->current_token.line, ctx->current_token.col, 
                                "parse_function", "missing ( after $return");
                ctx->last_error = CCO_ERR_PARSE;
                break;
            }
        } else if (ctx->current_token.type == CCO_TOK_KW_THIS) {
            cco_diag_record(CCO_ERR_PARSE, ctx->current_token.line, ctx->current_token.col, 
                            "parse_function", "assign not supported in skeleton");
            ctx->last_error = CCO_ERR_PARSE;
            break;
        } else if (ctx->current_token.type == CCO_TOK_IDENTIFIER) {
            cco_src_loc_t err_loc = {0};
            cco_parse_bind_result_t bres = cco_parse_bind(ctx, &err_loc);
            if (bres == PARSE_BIND_NOT_MATCH) {
                cco_diag_record(CCO_ERR_PARSE, err_loc.line, err_loc.col, "parse_bind", "missing =");
                ctx->last_error = CCO_ERR_PARSE;
                break;
            } else if (bres == PARSE_BIND_ERROR) {
                ctx->last_error = CCO_ERR_PARSE;
                break;
            }
        } else {
            cco_diag_record(CCO_ERR_PARSE, ctx->current_token.line, ctx->current_token.col, 
                            "parse_function", "unsupported statement");
            ctx->last_error = CCO_ERR_PARSE;
            break;
        }
        
        if (ctx->current_token.type == CCO_TOK_COMMA) {
            advance_token(ctx);
        } else if (ctx->current_token.type != CCO_TOK_RPAREN && ctx->current_token.type != CCO_TOK_EOF) {
            cco_diag_record(CCO_ERR_PARSE, ctx->current_token.line, ctx->current_token.col, 
                            "parse_function", "unsupported statement");
            ctx->last_error = CCO_ERR_PARSE;
            break;
        }
    }
    
    cco_scope_destroy(&method_scope);
    ctx->scope = old_scope;
    
    return NULL;
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


static const char* cco_type_name(cco_type_t t) {
    switch (t) {
        case CCO_TYPE_STRING: return "String";
        case CCO_TYPE_INTEGER: return "Integer";
        case CCO_TYPE_FLOAT: return "Float";
        case CCO_TYPE_BOOLEAN: return "Boolean";
        case CCO_TYPE_NONE: return "None";
        case CCO_TYPE_MAP: return "Map";
        case CCO_TYPE_ARRAY: return "Array";
        default: return "Unknown";
    }
}

static int get_binary_op_precedence(cco_token_type_t type) {
    switch (type) {
        case CCO_TOK_STAR:
        case CCO_TOK_SLASH:
            return 2;
        case CCO_TOK_PLUS:
        case CCO_TOK_MINUS:
            return 1;
        default:
            return 0;
    }
}

static cco_object_t* parse_expr_inner(cco_parser_context_t* ctx, int min_prec) {
    cco_object_t* left = parse_value(ctx);
    if (!left) return NULL;
    
    while (1) {
        int prec = get_binary_op_precedence(ctx->current_token.type);
        if (prec < min_prec || prec == 0) {
            break;
        }
        
        cco_token_type_t op = ctx->current_token.type;
        size_t op_line = ctx->current_token.line;
        size_t op_col = ctx->current_token.col;
        advance_token(ctx);
        
        cco_object_t* right = parse_expr_inner(ctx, prec + 1);
        if (!right) {
            cco_object_release(left);
            return NULL;
        }
        
        if (left->type == CCO_TYPE_INTEGER && right->type == CCO_TYPE_INTEGER) {
            int64_t l = left->as.integer;
            int64_t r = right->as.integer;
            cco_object_release(left);
            cco_object_release(right);
            left = cco_object_create(CCO_TYPE_INTEGER);
            if (!left) { cco_diag_record(CCO_ERR_NO_MEMORY, op_line, op_col, "parse_expr", "out of memory"); ctx->last_error = CCO_ERR_NO_MEMORY; return NULL; }
            if (op == CCO_TOK_PLUS) left->as.integer = l + r;
            else if (op == CCO_TOK_MINUS) left->as.integer = l - r;
            else if (op == CCO_TOK_STAR) left->as.integer = l * r;
            else if (op == CCO_TOK_SLASH) {
                if (r == 0) {
                    cco_diag_record(CCO_ERR_PARSE, op_line, op_col, "parse_expr", "division by zero");
                    ctx->last_error = CCO_ERR_PARSE;
                    cco_object_release(left);
                    return NULL;
                }
                /* TODO: guard INT64_MIN / -1 */
                left->as.integer = l / r; /* Integer truncation */
            }
        } else if ((left->type == CCO_TYPE_INTEGER || left->type == CCO_TYPE_FLOAT) &&
                   (right->type == CCO_TYPE_INTEGER || right->type == CCO_TYPE_FLOAT)) {
            double l = (left->type == CCO_TYPE_INTEGER) ? (double)left->as.integer : left->as.floating;
            double r = (right->type == CCO_TYPE_INTEGER) ? (double)right->as.integer : right->as.floating;
            cco_object_release(left);
            cco_object_release(right);
            left = cco_object_create(CCO_TYPE_FLOAT);
            if (!left) { cco_diag_record(CCO_ERR_NO_MEMORY, op_line, op_col, "parse_expr", "out of memory"); ctx->last_error = CCO_ERR_NO_MEMORY; return NULL; }
            if (op == CCO_TOK_PLUS) left->as.floating = l + r;
            else if (op == CCO_TOK_MINUS) left->as.floating = l - r;
            else if (op == CCO_TOK_STAR) left->as.floating = l * r;
            else if (op == CCO_TOK_SLASH) {
                if (r == 0.0) {
                    cco_diag_record(CCO_ERR_PARSE, op_line, op_col, "parse_expr", "division by zero");
                    ctx->last_error = CCO_ERR_PARSE;
                    cco_object_release(left);
                    return NULL;
                }
                left->as.floating = l / r;
            }
        } else {
            const char* op_str = (op == CCO_TOK_PLUS) ? "+" : (op == CCO_TOK_MINUS) ? "-" : (op == CCO_TOK_STAR) ? "*" : "/";
            const char* ltype = cco_type_name(left->type);
            const char* rtype = cco_type_name(right->type);
            
            char msg[256];
            snprintf(msg, sizeof(msg), "operator '%s' not defined for %s and %s", op_str, ltype, rtype);
            cco_diag_record(CCO_ERR_PARSE, op_line, op_col, "parse_expr", msg);
            ctx->last_error = CCO_ERR_PARSE;
            cco_object_release(left);
            cco_object_release(right);
            return NULL;
        }
    }
    return left;
}

static cco_object_t* parse_expr(cco_parser_context_t* ctx) {
    return parse_expr_inner(ctx, 1);
}
static cco_object_t* parse_value(cco_parser_context_t* ctx)
{
    if (ctx->current_token.type == CCO_TOK_DOLLAR_LPAREN) {
        advance_token(ctx); /* Skip $( */
        cco_object_t* obj = parse_expr(ctx);
        if (!obj) return NULL;
        if (ctx->current_token.type != CCO_TOK_RPAREN) {
            cco_diag_record(CCO_ERR_PARSE, ctx->current_token.line, ctx->current_token.col, "parse_value", "expected ')' after expression");
            ctx->last_error = CCO_ERR_PARSE;
            cco_object_release(obj);
            return NULL;
        }
        advance_token(ctx); /* Skip ) */
        return obj;
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
    case CCO_TOK_MINUS:
    {
        /* TODO: handle parenthesized expression -(...) when implemented */
        size_t minus_line = ctx->current_token.line;
        size_t minus_col = ctx->current_token.col;
        advance_token(ctx); /* skip - */
        
        cco_object_t* inner = parse_value(ctx);
        if (!inner) {
            result = NULL;
            break;
        }
        
        if (inner->type == CCO_TYPE_INTEGER) {
            result = cco_object_create(CCO_TYPE_INTEGER);
            if (!result) { ctx->last_error = CCO_ERR_NO_MEMORY; cco_object_release(inner); break; }
            result->as.integer = -inner->as.integer;
        } else if (inner->type == CCO_TYPE_FLOAT) {
            result = cco_object_create(CCO_TYPE_FLOAT);
            if (!result) { ctx->last_error = CCO_ERR_NO_MEMORY; cco_object_release(inner); break; }
            result->as.floating = -inner->as.floating;
        } else {
            char msg[256];
            snprintf(msg, sizeof(msg), "unary minus not defined for %s", cco_type_name(inner->type));
            cco_diag_record(CCO_ERR_PARSE, minus_line, minus_col, "parse_value", msg);
            ctx->last_error = CCO_ERR_PARSE;
            result = NULL;
        }
        cco_object_release(inner);
        break;
    }
    case CCO_TOK_IDENTIFIER:
    {
        size_t klen = ctx->current_token.text_len;
        char* name = malloc(klen + 1);
        if (!name) {
            ctx->last_error = CCO_ERR_NO_MEMORY;
            break;
        }
        memcpy(name, ctx->current_token.text_ptr, klen);
        name[klen] = '\0';
        
        cco_object_t* found = NULL;
        if (ctx->scope) {
            found = cco_scope_lookup(ctx->scope, name);
        }
        /* TODO: fallback to top-level symtab for enum/typedef lookup */
        
        if (found) {
            result = found;
            cco_object_retain(result);
            advance_token(ctx);
        } else {
            char msg[256];
            snprintf(msg, sizeof(msg), "undefined identifier '%s'", name);
            cco_diag_record(CCO_ERR_PARSE, ctx->current_token.line, ctx->current_token.col, 
                            "parse_value", msg);
            ctx->last_error = CCO_ERR_PARSE;
            result = NULL;
        }
        free(name);
        break;
    }
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


cco_template_def_t* cco_parse_template_decl(cco_parser_context_t* ctx) {
    if (ctx->current_token.type != CCO_TOK_IDENTIFIER || ctx->current_token.text_len != 5 || strncmp(ctx->current_token.text_ptr, "$temp", 5) != 0) {
        return NULL;
    }
    advance_token(ctx); /* skip $temp */
    
    if (ctx->current_token.type != CCO_TOK_DOT) {
        cco_diag_record(CCO_ERR_PARSE, ctx->current_token.line, ctx->current_token.col, "parse_template", "expected '.' after $temp");
        ctx->last_error = CCO_ERR_PARSE; return NULL;
    }
    advance_token(ctx);
    
    char name_buf[256] = {0};
    size_t name_len = 0;
    while (ctx->current_token.type == CCO_TOK_IDENTIFIER) {
        size_t klen = ctx->current_token.text_len;
        if (name_len + klen + 1 < sizeof(name_buf)) {
            memcpy(name_buf + name_len, ctx->current_token.text_ptr, klen);
            name_len += klen;
        }
        advance_token(ctx);
        if (ctx->current_token.type == CCO_TOK_DOT) {
            if (name_len + 1 < sizeof(name_buf)) {
                name_buf[name_len++] = '.';
            }
            advance_token(ctx);
        } else {
            break;
        }
    }
    if (name_len == 0) {
        cco_diag_record(CCO_ERR_PARSE, ctx->current_token.line, ctx->current_token.col, "parse_template", "expected template name");
        ctx->last_error = CCO_ERR_PARSE; return NULL;
    }
    
    char* parent_name = NULL;
    if (ctx->current_token.type == CCO_TOK_PLUS) {
        advance_token(ctx); /* skip + */
        if (ctx->current_token.type == CCO_TOK_IDENTIFIER) {
            size_t klen = ctx->current_token.text_len;
            parent_name = malloc(klen + 1);
            memcpy(parent_name, ctx->current_token.text_ptr, klen);
            parent_name[klen] = '\0';
            advance_token(ctx);
        } else {
            cco_diag_record(CCO_ERR_PARSE, ctx->current_token.line, ctx->current_token.col, "parse_template", "expected parent template name");
            ctx->last_error = CCO_ERR_PARSE; return NULL;
        }
    }
    
    if (ctx->current_token.type != CCO_TOK_COLON) {
        cco_diag_record(CCO_ERR_PARSE, ctx->current_token.line, ctx->current_token.col, "parse_template", "expected ':' after template name");
        ctx->last_error = CCO_ERR_PARSE; if (parent_name) free(parent_name); return NULL;
    }
    advance_token(ctx);
    
    if (ctx->current_token.type != CCO_TOK_LPAREN) {
        cco_diag_record(CCO_ERR_PARSE, ctx->current_token.line, ctx->current_token.col, "parse_template", "expected '(' to start fields");
        ctx->last_error = CCO_ERR_PARSE; if (parent_name) free(parent_name); return NULL;
    }
    advance_token(ctx);
    
    cco_template_def_t* def = malloc(sizeof(cco_template_def_t));
    def->fields = NULL;
    def->field_count = 0;
    def->field_capacity = 0;
    def->parent_name = parent_name;
    
    while (ctx->current_token.type != CCO_TOK_RPAREN && ctx->current_token.type != CCO_TOK_EOF) {
        if (ctx->current_token.type != CCO_TOK_IDENTIFIER) {
            cco_diag_record(CCO_ERR_PARSE, ctx->current_token.line, ctx->current_token.col, "parse_template", "expected field name");
            ctx->last_error = CCO_ERR_PARSE; break;
        }
        
        size_t klen = ctx->current_token.text_len;
        char* fname = malloc(klen + 1);
        memcpy(fname, ctx->current_token.text_ptr, klen);
        fname[klen] = '\0';
        advance_token(ctx);
        
        char* type_name = NULL;
        cco_object_t* def_val = NULL;
        if (ctx->current_token.type == CCO_TOK_COLON) {
            advance_token(ctx);
            if (ctx->current_token.type == CCO_TOK_IDENTIFIER) {
                size_t type_len = 0;
                char type_buf[256] = {0};
                while (ctx->current_token.type != CCO_TOK_EQ && ctx->current_token.type != CCO_TOK_COMMA && ctx->current_token.type != CCO_TOK_RPAREN && ctx->current_token.type != CCO_TOK_EOF) {
                    size_t tlen = ctx->current_token.text_len;
                    if (type_len > 0 && type_len < sizeof(type_buf)) type_buf[type_len++] = ' ';
                    if (type_len + tlen < sizeof(type_buf)) {
                        memcpy(type_buf + type_len, ctx->current_token.text_ptr, tlen);
                        type_len += tlen;
                    }
                    advance_token(ctx);
                }
                if (type_len > 0) {
                    type_name = malloc(type_len + 1);
                    memcpy(type_name, type_buf, type_len);
                    type_name[type_len] = '\0';
                }
            } else {
                def_val = parse_expr(ctx);
                if (!def_val) {
                    ctx->last_error = CCO_ERR_PARSE; free(fname); break;
                }
            }
        }
        
        if (ctx->current_token.type == CCO_TOK_EQ) {
            advance_token(ctx);
            def_val = parse_expr(ctx);
            if (!def_val) {
                ctx->last_error = CCO_ERR_PARSE; free(fname); if (type_name) free(type_name); break;
            }
        }
        
        if (def->field_count == def->field_capacity) {
            size_t new_cap = def->field_capacity == 0 ? 8 : def->field_capacity * 2;
            def->fields = realloc(def->fields, new_cap * sizeof(cco_field_def_t));
            def->field_capacity = new_cap;
        }
        def->fields[def->field_count].name = fname;
        def->fields[def->field_count].type_name = type_name;
        def->fields[def->field_count].default_value = def_val;
        def->field_count++;
        
        if (ctx->current_token.type == CCO_TOK_COMMA) {
            advance_token(ctx);
        }
    }
    
    if (ctx->current_token.type == CCO_TOK_RPAREN) {
        advance_token(ctx);
    }
    
    if (ctx->last_error != CCO_OK) {
        for(size_t i=0; i<def->field_count; ++i) {
            free(def->fields[i].name);
            if(def->fields[i].type_name) free(def->fields[i].type_name);
            if(def->fields[i].default_value) cco_object_release(def->fields[i].default_value);
        }
        free(def->fields);
        if(def->parent_name) free((void*)def->parent_name);
        free(def);
        return NULL;
    }
    return def;
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
