#define _POSIX_C_SOURCE 200809L
#include "parser.h"
#include "expr.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "diag.h"
#include "symtab.h"
#include "inst.h"

static inline void advance_token(cco_parser_context_t* ctx)
{
    ctx->current_token = ctx->next_token;
    cco_error_t lex_err = cco_lexer_next(&ctx->lexer, &ctx->next_token);
    if (lex_err != CCO_OK && ctx->last_error == CCO_OK)
    {
        ctx->last_error = lex_err;
    }
#ifndef CCO_ENABLE_FAST
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
#endif
}

static inline void* fast_arena_alloc(cco_arena_t* restrict a, size_t n) {
    size_t aligned_n = (n + 7) & ~((size_t)7);
    if (a->curr && a->curr->used + aligned_n <= a->curr->capacity) {
        void* ptr = a->curr->data + a->curr->used;
        a->curr->used += aligned_n;
        a->total_allocated += aligned_n;
        return ptr;
    }
    return cco_arena_alloc(a, n);
}

#ifdef CCO_ENABLE_ZERO_COPY
static inline void* fast_arena_alloc_obj(cco_arena_t* restrict a) {
    return fast_arena_alloc(a, sizeof(cco_object_t));
}
#endif

#ifdef CCO_ENABLE_FAST
static inline cco_error_t fast_map_append(cco_arena_t* arena, cco_object_t* map, const char* key, size_t klen, cco_object_t* value) {
    if (arena && (map->flags & 1)) {
        if (map->as.map.capacity == 0) {
            map->as.map.capacity = 2;
            map->as.map.keys = fast_arena_alloc(arena, 2 * sizeof(char*));
            map->as.map.values = fast_arena_alloc(arena, 2 * sizeof(cco_object_t*));
        } else if (map->as.map.count >= map->as.map.capacity) {
            size_t new_cap = map->as.map.capacity * 2;
            char** new_keys = fast_arena_alloc(arena, new_cap * sizeof(char*));
            cco_object_t** new_vals = fast_arena_alloc(arena, new_cap * sizeof(cco_object_t*));
            memcpy(new_keys, map->as.map.keys, map->as.map.count * sizeof(char*));
            memcpy(new_vals, map->as.map.values, map->as.map.count * sizeof(cco_object_t*));
            map->as.map.keys = new_keys;
            map->as.map.values = new_vals;
            map->as.map.capacity = new_cap;
        }
        
        char* dup_key = fast_arena_alloc(arena, klen + 1);
        memcpy(dup_key, key, klen);
        dup_key[klen] = '\0';
        map->as.map.keys[map->as.map.count] = dup_key;
        map->as.map.values[map->as.map.count] = value;
        map->as.map.count++;
        return CCO_OK;
    }
    return cco_map_append_fast_len(map, key, klen, value);
}
#endif

static inline cco_error_t fast_array_append(cco_arena_t* arena, cco_object_t* arr, cco_object_t* item) {
    if (arena && (arr->flags & 1)) {
        if (arr->as.array.capacity == 0) {
            arr->as.array.capacity = 2;
            arr->as.array.items = fast_arena_alloc(arena, 2 * sizeof(cco_object_t*));
        } else if (arr->as.array.count >= arr->as.array.capacity) {
            size_t new_cap = arr->as.array.capacity * 2;
            cco_object_t** new_items = fast_arena_alloc(arena, new_cap * sizeof(cco_object_t*));
            memcpy(new_items, arr->as.array.items, arr->as.array.count * sizeof(cco_object_t*));
            arr->as.array.items = new_items;
            arr->as.array.capacity = new_cap;
        }
        arr->as.array.items[arr->as.array.count++] = item;
        return CCO_OK;
    }
    return cco_array_append(arr, item);
}

static inline cco_object_t* fast_create_object(cco_parser_context_t* ctx, cco_type_t type)
{
    (void)ctx;
#ifdef CCO_ENABLE_ZERO_COPY
    if (ctx->arena) {
        cco_object_t* obj = fast_arena_alloc_obj(ctx->arena);
        if (obj) {
            obj->type = type;
            obj->flags = 1;
            obj->refcount = 1;
            obj->dom_arena = NULL;
            if (type == CCO_TYPE_MAP) {
                obj->as.map.count = 0;
                obj->as.map.capacity = 0;
                obj->as.map.keys = NULL;
                obj->as.map.values = NULL;
                obj->as.map.hashes = NULL;
            } else if (type == CCO_TYPE_ARRAY) {
                obj->as.array.count = 0;
                obj->as.array.capacity = 0;
                obj->as.array.items = NULL;
            }
        }
        return obj;
    }
#endif
    return cco_object_create(type);
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
        ctx->symtab = NULL;
        ctx->owns_symtab = false;
        ctx->inst_count = 0;
        ctx->in_expr = false;
        ctx->in_return = false;
        ctx->in_generic_params = false;

        /* Prime the tokens */
        cco_error_t prime_err = cco_lexer_next(&ctx->lexer, &ctx->next_token);
        if (prime_err != CCO_OK && ctx->last_error == CCO_OK)
        {
            ctx->last_error = prime_err;
        }
        advance_token(ctx);
    }
}

void cco_parser_advance(cco_parser_context_t* ctx)
{
    advance_token(ctx);
}

/* Forward declarations */
static cco_object_t* parse_value(cco_parser_context_t* ctx);
cco_object_t* parse_expr(cco_parser_context_t* ctx);

/* State capture for speculative bind backtracking */
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
 * Bind parsing rules:
 * - Identifier ":" TypeExpr "=" Expr       -> OK
 * - Identifier ":" ... without "="         -> NOT_MATCH, silently backtrack
 * - Identifier ":" TypeExpr "=" parse error -> ERROR
 */


cco_parse_bind_result_t cco_parse_bind(cco_parser_context_t* ctx, cco_src_loc_t* out_err_loc) {
    if (ctx->current_token.type != CCO_TOK_IDENTIFIER) {
        if (out_err_loc) {
            out_err_loc->line = ctx->current_token.line;
            out_err_loc->col = ctx->current_token.col;
        }
        return PARSE_BIND_NOT_MATCH;
    }
    
    cco_parser_state_t initial_state;
    save_state(ctx, &initial_state);

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
        restore_state(ctx, &initial_state);
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
        restore_state(ctx, &initial_state);
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
    restore_state(ctx, &initial_state);
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
    
    bool is_ctor = false;
    if (ctx->current_token.type == CCO_TOK_AT) {
        is_ctor = true;
        advance_token(ctx); /* skip @ */
    } else if (ctx->current_token.type == CCO_TOK_HASH) {
        advance_token(ctx); /* skip # */
        if (ctx->current_token.type == CCO_TOK_IDENTIFIER) advance_token(ctx);
    } else if (ctx->current_token.type == CCO_TOK_IDENTIFIER) {
        advance_token(ctx); /* skip function name */
    } else {
        cco_diag_record(CCO_ERR_PARSE, ctx->current_token.line, ctx->current_token.col, "parse_function", "expected '@', '#', or function name");
        ctx->last_error = CCO_ERR_PARSE; return NULL;
    }
    
    cco_scope_t* method_scope = malloc(sizeof(cco_scope_t));
    if (!method_scope) {
        ctx->last_error = CCO_ERR_NO_MEMORY;
        return NULL;
    }
    if (ctx->scope) {
        cco_scope_init(method_scope, ctx->scope);
    } else {
        cco_scope_init(method_scope, NULL);
    }
    cco_scope_t* old_scope = ctx->scope;
    ctx->scope = method_scope;

    if (ctx->current_token.type == CCO_TOK_LT) {
        advance_token(ctx); /* skip < */
        ctx->in_generic_params = true;
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
                
                /* Parse parameter type annotation */
                if (ctx->current_token.type == CCO_TOK_IDENTIFIER) {
                    advance_token(ctx);
                } else {
                    cco_diag_record(CCO_ERR_PARSE, ctx->current_token.line, ctx->current_token.col, "parse_function", "expected type after ':'");
                    ctx->last_error = CCO_ERR_PARSE; free(pname); break;
                }

                /* Optional default parameter value: param: Type = default */
                if (ctx->current_token.type == CCO_TOK_EQ) {
                    advance_token(ctx);
                    cco_object_t* def_param = parse_expr(ctx);
                    if (def_param) cco_object_release(def_param);
                }
                
                bool found = false;
                for (size_t i = 0; i < method_scope->count; ++i) {
                    if (strcmp(method_scope->entries[i].name, pname) == 0) { found = true; break; }
                }
                
                if (found) {
                    char msg[128];
                    snprintf(msg, sizeof(msg), "duplicate parameter '%s'", pname);
                    cco_diag_record(CCO_ERR_PARSE, ctx->current_token.line, ctx->current_token.col, "parse_function", msg);
                    ctx->last_error = CCO_ERR_PARSE; free(pname); break;
                }
                /* create PARAM value for parse time lookup */
                cco_object_t* arg_val = fast_create_object(ctx, CCO_TYPE_PARAM);
                if (arg_val) arg_val->as.param.name = strdup(pname);
                cco_scope_bind(method_scope, pname, arg_val);
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
        ctx->in_generic_params = false;
    }
    
    if (ctx->last_error != CCO_OK) {
        ctx->in_generic_params = false;
        cco_scope_destroy(method_scope);
        free(method_scope);
        ctx->scope = old_scope;
        return NULL;
    }
    
    if (ctx->current_token.type == CCO_TOK_COLON) advance_token(ctx);
    if (ctx->current_token.type == CCO_TOK_IDENTIFIER) advance_token(ctx); /* RetType */
    
    if (ctx->current_token.type != CCO_TOK_LPAREN) {
        cco_scope_destroy(method_scope);
        free(method_scope);
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
                ctx->in_return = true;
                cco_object_t* expr_val = parse_expr(ctx);
                ctx->in_return = false;
                if (expr_val) cco_object_release(expr_val);
                if (ctx->current_token.type == CCO_TOK_RPAREN) advance_token(ctx);
            } else {
                cco_diag_record(CCO_ERR_PARSE, ctx->current_token.line, ctx->current_token.col, 
                                "parse_function", "missing ( after $return");
                ctx->last_error = CCO_ERR_PARSE;
                break;
            }
        } else if (ctx->current_token.type == CCO_TOK_KW_THIS) {
            if (!is_ctor) {
                cco_diag_record(CCO_ERR_PARSE, ctx->current_token.line, ctx->current_token.col, 
                                "parse_function", "cannot use '$this' in static method");
                ctx->last_error = CCO_ERR_PARSE;
                break;
            }
            advance_token(ctx); /* skip $this */
            if (ctx->current_token.type != CCO_TOK_DOT) {
                cco_diag_record(CCO_ERR_PARSE, ctx->current_token.line, ctx->current_token.col, 
                                "parse_function", "expected '.' after '$this'");
                ctx->last_error = CCO_ERR_PARSE;
                break;
            }
            advance_token(ctx); /* skip . */

            if (ctx->current_token.type == CCO_TOK_LPAREN) {
                /* Bulk field assignment: $this.(field: expr, ...) */
                advance_token(ctx); /* skip ( */
                while (ctx->current_token.type != CCO_TOK_RPAREN && ctx->current_token.type != CCO_TOK_EOF) {
                    if (ctx->current_token.type != CCO_TOK_IDENTIFIER) {
                        cco_diag_record(CCO_ERR_PARSE, ctx->current_token.line, ctx->current_token.col, 
                                        "parse_function", "expected field name in $this assignment");
                        ctx->last_error = CCO_ERR_PARSE;
                        break;
                    }
                    advance_token(ctx); /* skip field name */
                    if (ctx->current_token.type != CCO_TOK_COLON) {
                        cco_diag_record(CCO_ERR_PARSE, ctx->current_token.line, ctx->current_token.col, 
                                        "parse_function", "expected ':' after field name in $this assignment");
                        ctx->last_error = CCO_ERR_PARSE;
                        break;
                    }
                    advance_token(ctx); /* skip : */
                    cco_object_t* expr_val = parse_expr(ctx);
                    if (expr_val) cco_object_release(expr_val);
                    if (ctx->current_token.type == CCO_TOK_COMMA) {
                        advance_token(ctx);
                    }
                }
                if (ctx->last_error != CCO_OK) break;
                if (ctx->current_token.type == CCO_TOK_RPAREN) {
                    advance_token(ctx); /* skip ) */
                }
            } else if (ctx->current_token.type == CCO_TOK_IDENTIFIER) {
                /* Single assignment: $this.field = expr */
                advance_token(ctx); /* skip field name */
                if (ctx->current_token.type != CCO_TOK_EQ) {
                    cco_diag_record(CCO_ERR_PARSE, ctx->current_token.line, ctx->current_token.col, 
                                    "parse_function", "expected '=' in $this assignment");
                    ctx->last_error = CCO_ERR_PARSE;
                    break;
                }
                advance_token(ctx); /* skip = */
                cco_object_t* expr_val = parse_expr(ctx);
                if (expr_val) cco_object_release(expr_val);
            } else {
                cco_diag_record(CCO_ERR_PARSE, ctx->current_token.line, ctx->current_token.col, 
                                "parse_function", "expected '(' or field identifier after '$this.'");
                ctx->last_error = CCO_ERR_PARSE;
                break;
            }
        } else if (ctx->current_token.type == CCO_TOK_IDENTIFIER) {
            cco_src_loc_t err_loc = {0};
            cco_parse_bind_result_t bres = cco_parse_bind(ctx, &err_loc);
            if (bres == PARSE_BIND_NOT_MATCH) {
                cco_object_t* expr_val = parse_expr(ctx);
                if (expr_val) {
                    cco_object_release(expr_val);
                } else {
                    cco_diag_record(CCO_ERR_PARSE, err_loc.line, err_loc.col, "parse_bind", "missing =");
                    ctx->last_error = CCO_ERR_PARSE;
                    break;
                }
            } else if (bres == PARSE_BIND_ERROR) {
                ctx->last_error = CCO_ERR_PARSE;
                break;
            }
        } else {
            cco_object_t* expr_val = parse_expr(ctx);
            if (expr_val) {
                cco_object_release(expr_val);
            } else {
                cco_diag_record(CCO_ERR_PARSE, ctx->current_token.line, ctx->current_token.col, 
                                "parse_function", "invalid expression in function body");
                ctx->last_error = CCO_ERR_PARSE;
                break;
            }
        }
        
        if (ctx->current_token.type == CCO_TOK_COMMA) {
            advance_token(ctx);
        } else if (ctx->current_token.type != CCO_TOK_RPAREN && ctx->current_token.type != CCO_TOK_EOF) {
            cco_diag_record(CCO_ERR_PARSE, ctx->current_token.line, ctx->current_token.col, 
                            "parse_function", "expected comma or closing delimiter in function body");
            ctx->last_error = CCO_ERR_PARSE;
            break;
        }
    }
    
    cco_scope_destroy(method_scope);
    free(method_scope);
    ctx->scope = old_scope;
    
    return NULL;
}

static bool is_dotted_map_key(const cco_parser_context_t* ctx)
{
    if (ctx->current_token.type != CCO_TOK_IDENTIFIER ||
        ctx->next_token.type != CCO_TOK_DOT)
    {
        return false;
    }
    const char* src = ctx->lexer.src;
    size_t len = ctx->lexer.src_len;
    if (!src || !ctx->next_token.text_ptr)
        return false;
    size_t pos = (size_t)(ctx->next_token.text_ptr - src);
    while (pos < len)
    {
        char c = src[pos];
        if (c == '/' && pos + 1 < len && src[pos + 1] == '*')
        {
            pos += 2;
            while (pos + 1 < len && !(src[pos] == '*' && src[pos + 1] == '/'))
                pos++;
            if (pos + 1 < len)
                pos += 2;
            continue;
        }

        if (c == '#')
        {
            pos++;
            while (pos < len && src[pos] != '\n')
                pos++;
            if (pos < len && src[pos] == '\n')
                pos++;
            continue;
        }

        if (c == '.' || c == '_' || c == '-' || c == '$' ||
            (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == ' ' || c == '\t' ||
            c == '\r' || c == '\n')
        {
            pos++;
        }
        else if (c == ':')
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    return false;
}

static inline bool is_map_entry_start(const cco_parser_context_t* ctx)
{
    if (ctx->next_token.type == CCO_TOK_COLON)
    {
        return (ctx->current_token.type == CCO_TOK_IDENTIFIER ||
                ctx->current_token.type == CCO_TOK_STRING);
    }
    if (ctx->current_token.type == CCO_TOK_IDENTIFIER &&
        ctx->next_token.type == CCO_TOK_DOT)
    {
        return is_dotted_map_key(ctx);
    }
    return false;
}

static cco_object_t* parse_map(cco_parser_context_t* ctx)
{
    cco_object_t* obj = fast_create_object(ctx, CCO_TYPE_MAP);
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

        size_t key_line = ctx->current_token.line;
        size_t key_col = ctx->current_token.col;
        const char* token_key_ptr = ctx->current_token.text_ptr;
        size_t token_key_len = ctx->current_token.text_len;

        const char* src = ctx->lexer.src;
        size_t src_len = ctx->lexer.src_len;
        bool is_in_src = (src && token_key_ptr >= src && token_key_ptr < src + src_len);

        char stack_key_buf[256];
        char* heap_key_buf = NULL;
        char* key_buf = stack_key_buf;
        size_t key_cap = sizeof(stack_key_buf) - 1;
        size_t key_len = 0;
        bool has_copied_key = false;

        if (!is_in_src)
        {
            /* Key came from scratch_buf (escaped string). Copy immediately. */
            if (token_key_len > key_cap)
            {
                heap_key_buf = (char*)malloc(token_key_len + 1);
                if (!heap_key_buf)
                {
                    ctx->last_error = CCO_ERR_NO_MEMORY;
                    break;
                }
                key_buf = heap_key_buf;
                key_cap = token_key_len;
            }
            if (token_key_ptr && token_key_len > 0)
            {
                memcpy(key_buf, token_key_ptr, token_key_len);
            }
            key_len = token_key_len;
            has_copied_key = true;
        }

        advance_token(ctx);
        if (ctx->current_token.type == CCO_TOK_DOT)
        {
            if (!has_copied_key)
            {
                if (token_key_len > key_cap)
                {
                    heap_key_buf = (char*)malloc(token_key_len * 2 + 1);
                    if (!heap_key_buf)
                    {
                        ctx->last_error = CCO_ERR_NO_MEMORY;
                        break;
                    }
                    key_buf = heap_key_buf;
                    key_cap = token_key_len * 2;
                }
                if (token_key_ptr && token_key_len > 0)
                {
                    memcpy(key_buf, token_key_ptr, token_key_len);
                }
                key_len = token_key_len;
                has_copied_key = true;
            }
            while (ctx->current_token.type == CCO_TOK_DOT)
            {
                advance_token(ctx);
                if (ctx->current_token.type != CCO_TOK_IDENTIFIER)
                {
                    ctx->last_error = CCO_ERR_PARSE;
                    break;
                }
                size_t seg_len = ctx->current_token.text_len;
                size_t new_len = key_len + 1 + seg_len;
                if (new_len > key_cap)
                {
                    size_t new_cap = new_len * 2;
                    char* new_heap = (char*)realloc(heap_key_buf, new_cap + 1);
                    if (!new_heap)
                    {
                        ctx->last_error = CCO_ERR_NO_MEMORY;
                        break;
                    }
                    if (!heap_key_buf)
                    {
                        memcpy(new_heap, stack_key_buf, key_len);
                    }
                    heap_key_buf = new_heap;
                    key_buf = heap_key_buf;
                    key_cap = new_cap;
                }
                key_buf[key_len++] = '.';
                if (ctx->current_token.text_ptr && seg_len > 0)
                {
                    memcpy(key_buf + key_len, ctx->current_token.text_ptr, seg_len);
                    key_len += seg_len;
                }
                advance_token(ctx);
            }
        }

        if (ctx->last_error != CCO_OK)
        {
            if (heap_key_buf)
                free(heap_key_buf);
            break;
        }

        if (ctx->current_token.type != CCO_TOK_COLON && !ctx->in_expr)
        {
            ctx->last_error = CCO_ERR_PARSE;
            if (heap_key_buf)
                free(heap_key_buf);
            break;
        }
        advance_token(ctx);

        cco_object_t* val = parse_value(ctx);
        if (!val)
        {
            if (heap_key_buf)
                free(heap_key_buf);
            break;
        }

        if (!has_copied_key)
        {
            if (token_key_len > key_cap)
            {
                heap_key_buf = (char*)malloc(token_key_len + 1);
                if (!heap_key_buf)
                {
                    ctx->last_error = CCO_ERR_NO_MEMORY;
                    cco_object_release(val);
                    break;
                }
                key_buf = heap_key_buf;
                key_cap = token_key_len;
            }
            if (token_key_ptr && token_key_len > 0)
            {
                memcpy(key_buf, token_key_ptr, token_key_len);
            }
            key_len = token_key_len;
            has_copied_key = true;
        }
        key_buf[key_len] = '\0';

#ifdef CCO_ENABLE_FAST
        if (!ctx->options || !ctx->options->fast_mode)
#endif
        {
            if (cco_map_has_key(obj, key_buf))
            {
                char msg[320];
                snprintf(msg, sizeof(msg), "duplicate key '%s' in map", key_buf);
                cco_diag_record(CCO_ERR_PARSE, key_line, key_col, "parse_map", msg);
                ctx->last_error = CCO_ERR_PARSE;
                cco_object_release(val);
                if (heap_key_buf)
                {
                    free(heap_key_buf);
                    heap_key_buf = NULL;
                }
                break;
            }
        }

        cco_error_t err = CCO_OK;
#ifdef CCO_ENABLE_FAST
        err = fast_map_append(ctx->arena, obj, key_buf, key_len, val);
#else
        if (strncmp(key_buf, "$define.", 8) == 0)
        {
            if (ctx->symtab)
                cco_symtab_add_macro(ctx->symtab, key_buf + 8, val);
        }
        else
        {
            err = cco_map_insert(obj, key_buf, val);
        }
#endif
        if (heap_key_buf)
        {
            free(heap_key_buf);
            heap_key_buf = NULL;
        }

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
    cco_object_t* obj = fast_create_object(ctx, CCO_TYPE_ARRAY);
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
        cco_object_t* val = parse_expr(ctx);
        if (!val)
            break;
        cco_error_t err = fast_array_append(ctx->arena, obj, val);
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
        case CCO_TYPE_PARAM: return "Parameter";
        case CCO_TYPE_TEMPLATE_INSTANCE: return "TemplateInstance";
        default: return "Unknown";
    }
}

static int get_binary_op_precedence(cco_token_type_t type) {
    switch (type) {
        case CCO_TOK_STAR:
        case CCO_TOK_SLASH:
            return 12;
        case CCO_TOK_PLUS:
        case CCO_TOK_MINUS:
            return 11;
        case CCO_TOK_LSHIFT:
        case CCO_TOK_RSHIFT:
            return 10;
        case CCO_TOK_LT:
        case CCO_TOK_LTE:
        case CCO_TOK_GT:
        case CCO_TOK_GTE:
            return 9;
        case CCO_TOK_EQEQ:
        case CCO_TOK_NEQ:
            return 8;
        case CCO_TOK_AMPERSAND:
            return 8;
        case CCO_TOK_CARET:
            return 7;
        case CCO_TOK_PIPE:
            return 6;
        case CCO_TOK_DOTDOT:
        case CCO_TOK_DOTDOTEQ:
            return 5;
        case CCO_TOK_AND:
            return 4;
        case CCO_TOK_OR:
            return 3;
        case CCO_TOK_QQ:
            return 2;
        case CCO_TOK_QMARK:
            return 1;
        default:
            return 0;
    }
}


bool check_field_type(cco_parser_context_t* ctx, cco_object_t* val, const char* expected_type, const char* field_name) {
    if (!expected_type || expected_type[0] == '\0' || !val) return true;
    if (val->type == CCO_TYPE_PARAM) return true;

    /* Handle nullable types (e.g. "String?", "Url?", "Any?") */
    size_t elen = strlen(expected_type);
    while (elen > 0 && expected_type[elen - 1] == ' ') elen--;
    if (elen > 1 && expected_type[elen - 1] == '?') {
        if (val->type == CCO_TYPE_NONE)
            return true;
        char base_type[128] = {0};
        size_t blen = elen - 1;
        while (blen > 0 && expected_type[blen - 1] == ' ') blen--;
        if (blen < sizeof(base_type)) {
            memcpy(base_type, expected_type, blen);
            base_type[blen] = '\0';
            return check_field_type(ctx, val, base_type, field_name);
        }
    }

    /* 1. Resolve typedef alias chain (e.g. Url -> String, MyAny -> Any, TaskLevel -> Level) */
    const char* resolved_type = expected_type;
    if (ctx && ctx->symtab) {
        resolved_type = cco_symtab_resolve_alias(ctx->symtab, expected_type);
    }

    if (strcmp(resolved_type, "Any") == 0) {
        return true;
    }

    /* 2. Check enum constraint: an enum field accepts CCO_TYPE_INTEGER */
    if (ctx && ctx->symtab && cco_symtab_lookup_enum(ctx->symtab, resolved_type)) {
        if (val->type == CCO_TYPE_INTEGER) {
            return true;
        }
    }

    /* 3. Direct comparison */
    const char* actual_type = cco_type_name(val->type);
    if (strcmp(actual_type, resolved_type) == 0) {
        return true;
    }

    /* 4. Numeric compatibility */
    if (strcmp(resolved_type, "Float") == 0 && val->type == CCO_TYPE_INTEGER) {
        return true;
    }

    /* 5. Container types */
    if (strcmp(resolved_type, "Array") == 0 && val->type == CCO_TYPE_ARRAY) {
        return true;
    }
    if (strcmp(resolved_type, "Map") == 0 && val->type == CCO_TYPE_MAP) {
        return true;
    }

    /* 6. Template instance hierarchy */
    if (val->type == CCO_TYPE_TEMPLATE_INSTANCE && val->as.tmpl_inst.template_name) {
        if (strcmp(val->as.tmpl_inst.template_name, resolved_type) == 0) {
            return true;
        }
        if (ctx && ctx->symtab) {
            cco_template_def_t* tdef = cco_symtab_lookup_template(ctx->symtab, val->as.tmpl_inst.template_name);
            while (tdef && tdef->parent_name) {
                if (strcmp(tdef->parent_name, resolved_type) == 0) {
                    return true;
                }
                tdef = cco_symtab_lookup_template(ctx->symtab, tdef->parent_name);
            }
        }
    }

    char msg[256];
    snprintf(msg, sizeof(msg), "type mismatch for field '%s': expected '%s', got '%s'", field_name ? field_name : "unknown", expected_type, actual_type);
    size_t line = ctx ? ctx->current_token.line : 0;
    size_t col = ctx ? ctx->current_token.col : 0;
    cco_diag_record(CCO_ERR_PARSE, line, col, "check_field_type", msg);
    if (ctx) ctx->last_error = CCO_ERR_PARSE;
    return false;
}

static cco_object_t* parse_expr_inner(cco_parser_context_t* ctx, int min_prec) {
    cco_object_t* left = parse_value(ctx);
    if (!left) return NULL;
    
    while (1) {
        int prec = get_binary_op_precedence(ctx->current_token.type);
        if (ctx->in_generic_params && (ctx->current_token.type == CCO_TOK_GT ||
                                       ctx->current_token.type == CCO_TOK_GTE ||
                                       ctx->current_token.type == CCO_TOK_RSHIFT)) {
            prec = 0;
        }
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
        
                if (left->type == CCO_TYPE_PARAM || right->type == CCO_TYPE_PARAM) {
            if (ctx->in_return) {
                const char* pname = (left->type == CCO_TYPE_PARAM && left->as.param.name) ? left->as.param.name :
                                    (right->type == CCO_TYPE_PARAM && right->as.param.name) ? right->as.param.name : "x";
                char msg[128];
                snprintf(msg, sizeof(msg), "parameter '%s' cannot be used in arithmetic at parse time", pname);
                cco_diag_record(CCO_ERR_PARSE, op_line, op_col, "parse_expr", msg);
                ctx->last_error = CCO_ERR_PARSE;
                cco_object_release(left); cco_object_release(right);
                return NULL;
            }
            cco_object_t* res = fast_create_object(ctx, CCO_TYPE_PARAM);
            if (res) {
                const char* pname = (left->type == CCO_TYPE_PARAM && left->as.param.name) ? left->as.param.name :
                                    (right->type == CCO_TYPE_PARAM && right->as.param.name) ? right->as.param.name : "param";
                res->as.param.name = strdup(pname);
            }
            cco_object_release(left);
            cco_object_release(right);
            left = res;
            continue;
        }
        
                if (op == CCO_TOK_QMARK) {
            cco_object_t* mid = right;
            if (ctx->current_token.type != CCO_TOK_COLON) {
                cco_diag_record(CCO_ERR_PARSE, op_line, op_col, "parse_expr", "expected ':' for ternary operator");
                ctx->last_error = CCO_ERR_PARSE;
                cco_object_release(left); cco_object_release(mid);
                return NULL;
            }
            advance_token(ctx);
            right = parse_expr_inner(ctx, 1);
            if (!right) {
                cco_object_release(left); cco_object_release(mid);
                return NULL;
            }
            bool l_truthy = false;
            if (left->type == CCO_TYPE_BOOLEAN) l_truthy = left->as.boolean;
            else if (left->type == CCO_TYPE_INTEGER) l_truthy = (left->as.integer != 0);
            else if (left->type == CCO_TYPE_FLOAT) l_truthy = (left->as.floating != 0.0);
            else if (left->type == CCO_TYPE_STRING) l_truthy = (left->as.string.length > 0);
            
            cco_object_release(left);
            if (l_truthy) {
                cco_object_release(right);
                left = mid;
            } else {
                cco_object_release(mid);
                left = right;
            }
            continue;
        } else if (op == CCO_TOK_DOTDOT || op == CCO_TOK_DOTDOTEQ) {
            if (left->type == CCO_TYPE_INTEGER && right->type == CCO_TYPE_INTEGER) {
                long long start = left->as.integer;
                long long end = right->as.integer;
                if (op == CCO_TOK_DOTDOTEQ) end++;
                
                cco_object_t* arr = fast_create_object(ctx, CCO_TYPE_ARRAY);
                if (arr) {
                    for (long long i = start; i < end; i++) {
                        cco_object_t* num = fast_create_object(ctx, CCO_TYPE_INTEGER);
                        if (num) {
                            num->as.integer = i;
                            fast_array_append(ctx->arena, arr, num);
                            cco_object_release(num);
                        }
                    }
                }
                cco_object_release(left);
                cco_object_release(right);
                left = arr;
                continue;
            } else {
                cco_diag_record(CCO_ERR_PARSE, op_line, op_col, "parse_expr", "range limits must be integers");
                ctx->last_error = CCO_ERR_PARSE;
                cco_object_release(left); cco_object_release(right);
                return NULL;
            }
        }
        else if (op == CCO_TOK_QQ) {
            if (left->type == CCO_TYPE_NONE) {
                cco_object_release(left);
                left = right;
            } else {
                cco_object_release(right);
            }
            continue;
        } else if (op == CCO_TOK_AND || op == CCO_TOK_OR) {
            bool l_truthy = false;
            bool r_truthy = false;
            
            if (left->type == CCO_TYPE_BOOLEAN) l_truthy = left->as.boolean;
            else if (left->type == CCO_TYPE_INTEGER) l_truthy = (left->as.integer != 0);
            else if (left->type == CCO_TYPE_FLOAT) l_truthy = (left->as.floating != 0.0);
            else if (left->type == CCO_TYPE_STRING) l_truthy = (left->as.string.length > 0);
            
            if (right->type == CCO_TYPE_BOOLEAN) r_truthy = right->as.boolean;
            else if (right->type == CCO_TYPE_INTEGER) r_truthy = (right->as.integer != 0);
            else if (right->type == CCO_TYPE_FLOAT) r_truthy = (right->as.floating != 0.0);
            else if (right->type == CCO_TYPE_STRING) r_truthy = (right->as.string.length > 0);
            
            cco_object_release(left);
            cco_object_release(right);
            left = fast_create_object(ctx, CCO_TYPE_BOOLEAN);
            if (!left) { cco_diag_record(CCO_ERR_NO_MEMORY, op_line, op_col, "parse_expr", "out of memory"); ctx->last_error = CCO_ERR_NO_MEMORY; return NULL; }
            if (op == CCO_TOK_AND) left->as.boolean = (l_truthy && r_truthy);
            else left->as.boolean = (l_truthy || r_truthy);
            continue;
        }
        
        if (left->type == CCO_TYPE_INTEGER && right->type == CCO_TYPE_INTEGER) {
            int64_t l = left->as.integer;
            int64_t r = right->as.integer;
            cco_object_release(left);
            cco_object_release(right);
            
            if (op == CCO_TOK_PLUS || op == CCO_TOK_MINUS || op == CCO_TOK_STAR || op == CCO_TOK_SLASH ||
                op == CCO_TOK_LSHIFT || op == CCO_TOK_RSHIFT || op == CCO_TOK_AMPERSAND || op == CCO_TOK_CARET || op == CCO_TOK_PIPE) {
                left = fast_create_object(ctx, CCO_TYPE_INTEGER);
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
                    left->as.integer = l / r;
                }
                else if (op == CCO_TOK_LSHIFT) left->as.integer = l << r;
                else if (op == CCO_TOK_RSHIFT) left->as.integer = l >> r;
                else if (op == CCO_TOK_AMPERSAND) left->as.integer = l & r;
                else if (op == CCO_TOK_CARET) left->as.integer = l ^ r;
                else if (op == CCO_TOK_PIPE) left->as.integer = l | r;
            } else if (op == CCO_TOK_EQEQ || op == CCO_TOK_NEQ || op == CCO_TOK_LT || op == CCO_TOK_LTE || op == CCO_TOK_GT || op == CCO_TOK_GTE) {
                left = fast_create_object(ctx, CCO_TYPE_BOOLEAN);
                if (!left) { cco_diag_record(CCO_ERR_NO_MEMORY, op_line, op_col, "parse_expr", "out of memory"); ctx->last_error = CCO_ERR_NO_MEMORY; return NULL; }
                if (op == CCO_TOK_EQEQ) left->as.boolean = (l == r);
                else if (op == CCO_TOK_NEQ) left->as.boolean = (l != r);
                else if (op == CCO_TOK_LT) left->as.boolean = (l < r);
                else if (op == CCO_TOK_LTE) left->as.boolean = (l <= r);
                else if (op == CCO_TOK_GT) left->as.boolean = (l > r);
                else if (op == CCO_TOK_GTE) left->as.boolean = (l >= r);
            } else {
                goto op_error;
            }
        } else if ((left->type == CCO_TYPE_INTEGER || left->type == CCO_TYPE_FLOAT) &&
                   (right->type == CCO_TYPE_INTEGER || right->type == CCO_TYPE_FLOAT)) {
            double l = (left->type == CCO_TYPE_INTEGER) ? (double)left->as.integer : left->as.floating;
            double r = (right->type == CCO_TYPE_INTEGER) ? (double)right->as.integer : right->as.floating;
            cco_object_release(left);
            cco_object_release(right);
            
            if (op == CCO_TOK_PLUS || op == CCO_TOK_MINUS || op == CCO_TOK_STAR || op == CCO_TOK_SLASH) {
                left = fast_create_object(ctx, CCO_TYPE_FLOAT);
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
            } else if (op == CCO_TOK_EQEQ || op == CCO_TOK_NEQ || op == CCO_TOK_LT || op == CCO_TOK_LTE || op == CCO_TOK_GT || op == CCO_TOK_GTE) {
                left = fast_create_object(ctx, CCO_TYPE_BOOLEAN);
                if (!left) { cco_diag_record(CCO_ERR_NO_MEMORY, op_line, op_col, "parse_expr", "out of memory"); ctx->last_error = CCO_ERR_NO_MEMORY; return NULL; }
                if (op == CCO_TOK_EQEQ) left->as.boolean = (l == r);
                else if (op == CCO_TOK_NEQ) left->as.boolean = (l != r);
                else if (op == CCO_TOK_LT) left->as.boolean = (l < r);
                else if (op == CCO_TOK_LTE) left->as.boolean = (l <= r);
                else if (op == CCO_TOK_GT) left->as.boolean = (l > r);
                else if (op == CCO_TOK_GTE) left->as.boolean = (l >= r);
            } else {
                goto op_error;
            }
        
        } else if (left->type == CCO_TYPE_STRING && right->type == CCO_TYPE_STRING && op == CCO_TOK_PLUS) {
            // String concatenation
            size_t new_len = left->as.string.length + right->as.string.length;
            char* new_str = malloc(new_len + 1);
            if (!new_str) {
                cco_diag_record(CCO_ERR_NO_MEMORY, op_line, op_col, "parse_expr", "out of memory");
                ctx->last_error = CCO_ERR_NO_MEMORY;
                cco_object_release(left); cco_object_release(right);
                return NULL;
            }
            memcpy(new_str, left->as.string.data, left->as.string.length);
            memcpy(new_str + left->as.string.length, right->as.string.data, right->as.string.length);
            new_str[new_len] = '\0';
            
            cco_object_release(left);
            cco_object_release(right);
            left = fast_create_object(ctx, CCO_TYPE_STRING);
            if (!left) { free(new_str); cco_diag_record(CCO_ERR_NO_MEMORY, op_line, op_col, "parse_expr", "out of memory"); ctx->last_error = CCO_ERR_NO_MEMORY; return NULL; }
            left->as.string.data = new_str;
            left->as.string.length = new_len;
        } else if ((op == CCO_TOK_EQEQ || op == CCO_TOK_NEQ) && left->type == right->type) {
            bool eq = false;
            if (left->type == CCO_TYPE_STRING) {
                eq = (left->as.string.length == right->as.string.length) &&
                     (memcmp(left->as.string.data, right->as.string.data, left->as.string.length) == 0);
            } else if (left->type == CCO_TYPE_BOOLEAN) {
                eq = (left->as.boolean == right->as.boolean);
            } else if (left->type == CCO_TYPE_NONE) {
                eq = true;
            }
            cco_object_release(left);
            cco_object_release(right);
            left = fast_create_object(ctx, CCO_TYPE_BOOLEAN);
            if (!left) { cco_diag_record(CCO_ERR_NO_MEMORY, op_line, op_col, "parse_expr", "out of memory"); ctx->last_error = CCO_ERR_NO_MEMORY; return NULL; }
            left->as.boolean = (op == CCO_TOK_EQEQ) ? eq : !eq;
        } else {
        op_error:
            ; // empty statement for label
            const char* op_str = "";
            if (op == CCO_TOK_PLUS) op_str = "+";
            else if (op == CCO_TOK_MINUS) op_str = "-";
            else if (op == CCO_TOK_STAR) op_str = "*";
            else if (op == CCO_TOK_SLASH) op_str = "/";
            else if (op == CCO_TOK_EQEQ) op_str = "==";
            else if (op == CCO_TOK_NEQ) op_str = "!=";
            else if (op == CCO_TOK_LT) op_str = "<";
            else if (op == CCO_TOK_LTE) op_str = "<=";
            else if (op == CCO_TOK_GT) op_str = ">";
            else if (op == CCO_TOK_GTE) op_str = ">=";
            else if (op == CCO_TOK_AND) op_str = "&&";
            else if (op == CCO_TOK_OR) op_str = "||";
            
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

cco_object_t* parse_expr(cco_parser_context_t* ctx) {
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
    
    if (ctx->current_token.type == CCO_TOK_IDENTIFIER && ctx->current_token.text_len == 4 && strncmp(ctx->current_token.text_ptr, "$env", 4) == 0) {
        if (ctx->next_token.type == CCO_TOK_LPAREN) {
            advance_token(ctx);
            advance_token(ctx);
            if (ctx->current_token.type == CCO_TOK_STRING) {
                char env_name[256];
                size_t cpy_len = ctx->current_token.text_len < 255 ? ctx->current_token.text_len : 255;
                memcpy(env_name, ctx->current_token.text_ptr, cpy_len);
                env_name[cpy_len] = '\0';
                advance_token(ctx);
                if (ctx->current_token.type == CCO_TOK_RPAREN) advance_token(ctx);
                
#ifdef CCO_ENABLE_ENV
                if (ctx->options->allow_env_builtin) {
                    const char* env_val = getenv(env_name);
                    if (env_val) {
                        cco_object_t* str_obj = fast_create_object(ctx, CCO_TYPE_STRING);
                        if (str_obj) {
                            size_t elen = strlen(env_val);
                            char* dup = cco_arena_alloc(ctx->arena, elen + 1);
                            if (dup) {
                                memcpy(dup, env_val, elen);
                                dup[elen] = '\0';
                                str_obj->as.string.data = dup;
                                str_obj->as.string.length = elen;
                                return str_obj;
                            }
                            cco_object_release(str_obj);
                        }
                    }
                }
#endif
                return fast_create_object(ctx, CCO_TYPE_NONE);
            }
        }
    }
    if (ctx->current_token.type == CCO_TOK_IDENTIFIER && ctx->current_token.text_len == 8 && strncmp(ctx->current_token.text_ptr, "$include", 8) == 0) {
        if (ctx->next_token.type == CCO_TOK_LPAREN) {
            advance_token(ctx); /* Skip $include */
            advance_token(ctx); /* Skip ( */
            if (ctx->current_token.type == CCO_TOK_STRING) {
                char filename[256];
                size_t cpy_len = ctx->current_token.text_len < 255 ? ctx->current_token.text_len : 255;
                memcpy(filename, ctx->current_token.text_ptr, cpy_len);
                filename[cpy_len] = '\0';
                advance_token(ctx); /* skip string */
                if (ctx->current_token.type == CCO_TOK_RPAREN) advance_token(ctx);
                
                if (ctx->options && ctx->options->restrict_filesystem) {
                    if (strstr(filename, "..") || filename[0] == '/') {
                        cco_diag_record(CCO_ERR_PARSE, ctx->current_token.line, ctx->current_token.col, "parse_value", "directory traversal not allowed");
                        ctx->last_error = CCO_ERR_PARSE;
                        return NULL;
                    }
                }
                
                char filepath[1024];
                if (ctx->options && ctx->options->base_dir) {
                    snprintf(filepath, sizeof(filepath), "%s/%s", ctx->options->base_dir, filename);
                } else {
                    snprintf(filepath, sizeof(filepath), "%s", filename);
                }
                
                FILE* f = fopen(filepath, "rb");
                if (!f) {
                    cco_diag_record(CCO_ERR_PARSE, ctx->current_token.line, ctx->current_token.col, "parse_value", "failed to open included file");
                    ctx->last_error = CCO_ERR_PARSE;
                    return NULL;
                }
                fseek(f, 0, SEEK_END);
                long fsize_long = ftell(f);
                fseek(f, 0, SEEK_SET);
                if (fsize_long < 0) {
                    fclose(f);
                    ctx->last_error = CCO_ERR_PARSE;
                    return NULL;
                }
                size_t fsize = (size_t)fsize_long;
                
                char* fcontent = malloc(fsize + 1);
                if (!fcontent) {
                    fclose(f);
                    ctx->last_error = CCO_ERR_NO_MEMORY;
                    return NULL;
                }
                fread(fcontent, 1, fsize, f);
                fcontent[fsize] = '\0';
                fclose(f);
                
                cco_object_t* file_obj = cco_parse_string(fcontent, fsize, ctx->options);
                free(fcontent);
                
                if (!file_obj) {
                    ctx->last_error = CCO_ERR_PARSE;
                    return NULL;
                }
                return file_obj;
            }
        }
    }

    if (ctx->last_error != CCO_OK)
        return NULL;

#ifndef CCO_ENABLE_FAST
    if (ctx->options->max_depth > 0 && ctx->depth >= ctx->options->max_depth)
    {
        cco_diag_record(CCO_ERR_OUT_OF_RANGE, ctx->current_token.line,
                        ctx->current_token.col, "parse_value",
                        "Max depth exceeded");
        ctx->last_error = CCO_ERR_OUT_OF_RANGE;
        return NULL;
    }
#endif

#ifndef CCO_ENABLE_FAST
    ctx->depth++;
#endif
    cco_object_t* result = NULL;

    switch (ctx->current_token.type)
    {
    case CCO_TOK_PLUS:
    case CCO_TOK_MINUS:
    case CCO_TOK_TILDE:
    case CCO_TOK_NOT:
    {
        cco_token_type_t un_op = ctx->current_token.type;
        size_t op_line = ctx->current_token.line;
        size_t op_col = ctx->current_token.col;
        advance_token(ctx);

        if (un_op == CCO_TOK_MINUS && ctx->current_token.type == CCO_TOK_INTEGER) {
            result = fast_create_object(ctx, CCO_TYPE_INTEGER);
            if (result) {
                char buf[128];
                buf[0] = '-';
                size_t clen = ctx->current_token.text_len;
                if (clen > sizeof(buf) - 2) clen = sizeof(buf) - 2;
                memcpy(buf + 1, ctx->current_token.text_ptr, clen);
                buf[1 + clen] = '\0';
                result->as.integer = cco_parse_integer_literal(buf, 1 + clen);
            }
            advance_token(ctx);
            break;
        }
        
        cco_object_t* inner = parse_value(ctx);
        if (!inner) {
            result = NULL;
            break;
        }
        
        if (inner->type == CCO_TYPE_PARAM) {
            char msg[128];
            snprintf(msg, sizeof(msg), "parameter '%s' cannot be used in arithmetic at parse time", inner->as.param.name);
            cco_diag_record(CCO_ERR_PARSE, op_line, op_col, "parse_value", msg);
            ctx->last_error = CCO_ERR_PARSE;
            result = NULL;
        } else if (un_op == CCO_TOK_MINUS) {
            if (inner->type == CCO_TYPE_INTEGER) {
                result = fast_create_object(ctx, CCO_TYPE_INTEGER);
                if (result) result->as.integer = -inner->as.integer;
            } else if (inner->type == CCO_TYPE_FLOAT) {
                result = fast_create_object(ctx, CCO_TYPE_FLOAT);
                if (result) result->as.floating = -inner->as.floating;
            } else {
                char msg[256];
                snprintf(msg, sizeof(msg), "unary minus not defined for %s", cco_type_name(inner->type));
                cco_diag_record(CCO_ERR_PARSE, op_line, op_col, "parse_value", msg);
                ctx->last_error = CCO_ERR_PARSE;
                result = NULL;
            }
        } else if (un_op == CCO_TOK_PLUS) {
            if (inner->type == CCO_TYPE_INTEGER) {
                result = fast_create_object(ctx, CCO_TYPE_INTEGER);
                if (result) result->as.integer = inner->as.integer;
            } else if (inner->type == CCO_TYPE_FLOAT) {
                result = fast_create_object(ctx, CCO_TYPE_FLOAT);
                if (result) result->as.floating = inner->as.floating;
            } else {
                char msg[256];
                snprintf(msg, sizeof(msg), "unary plus not defined for %s", cco_type_name(inner->type));
                cco_diag_record(CCO_ERR_PARSE, op_line, op_col, "parse_value", msg);
                ctx->last_error = CCO_ERR_PARSE;
                result = NULL;
            }
        } else if (un_op == CCO_TOK_TILDE) {
            if (inner->type == CCO_TYPE_INTEGER) {
                result = fast_create_object(ctx, CCO_TYPE_INTEGER);
                if (result) result->as.integer = ~inner->as.integer;
            } else {
                ctx->last_error = CCO_ERR_PARSE;
                result = NULL;
            }
        } else if (un_op == CCO_TOK_NOT) {
            bool truthy = false;
            if (inner->type == CCO_TYPE_BOOLEAN) truthy = inner->as.boolean;
            else if (inner->type == CCO_TYPE_INTEGER) truthy = (inner->as.integer != 0);
            else if (inner->type == CCO_TYPE_FLOAT) truthy = (inner->as.floating != 0.0);
            else if (inner->type == CCO_TYPE_STRING) truthy = (inner->as.string.length > 0);
            result = fast_create_object(ctx, CCO_TYPE_BOOLEAN);
            if (result) result->as.boolean = !truthy;
        }
        if (!result && ctx->last_error != CCO_ERR_PARSE) ctx->last_error = CCO_ERR_NO_MEMORY;
        cco_object_release(inner);
        break;
    }
    case CCO_TOK_HASH:
    {
        advance_token(ctx); /* skip # */
        if (ctx->current_token.type != CCO_TOK_IDENTIFIER) {
            cco_diag_record(CCO_ERR_PARSE, ctx->current_token.line, ctx->current_token.col, "parse_value", "expected template name after #");
            ctx->last_error = CCO_ERR_PARSE; break;
        }
        size_t klen = ctx->current_token.text_len;
        size_t cap = klen + 64;
        if (cap < 128) cap = 128;
        char* tname = malloc(cap);
        if (!tname) { ctx->last_error = CCO_ERR_NO_MEMORY; break; }
        memcpy(tname, ctx->current_token.text_ptr, klen);
        tname[klen] = '\0';
        size_t tname_len = klen;
        advance_token(ctx);

        while (ctx->current_token.type == CCO_TOK_DOT) {
            advance_token(ctx); /* skip . */
            if (ctx->current_token.type != CCO_TOK_IDENTIFIER) {
                free(tname);
                cco_diag_record(CCO_ERR_PARSE, ctx->current_token.line, ctx->current_token.col, "parse_value", "expected identifier after '.' in template name");
                ctx->last_error = CCO_ERR_PARSE;
                break;
            }
            size_t seg_len = ctx->current_token.text_len;
            if (tname_len + 1 + seg_len + 1 > cap) {
                cap = (tname_len + 1 + seg_len + 1) * 2;
                char* new_t = realloc(tname, cap);
                if (!new_t) { free(tname); ctx->last_error = CCO_ERR_NO_MEMORY; break; }
                tname = new_t;
            }
            tname[tname_len++] = '.';
            memcpy(tname + tname_len, ctx->current_token.text_ptr, seg_len);
            tname_len += seg_len;
            tname[tname_len] = '\0';
            advance_token(ctx);
        }

        if (ctx->last_error != CCO_OK) {
            break;
        }

        if (ctx->current_token.type == CCO_TOK_COLON) {
            advance_token(ctx); /* skip optional : */
        }
        
        if (ctx->current_token.type != CCO_TOK_LPAREN) {
            free(tname);
            cco_diag_record(CCO_ERR_PARSE, ctx->current_token.line, ctx->current_token.col, "parse_value", "expected '(' for template instantiation");
            ctx->last_error = CCO_ERR_PARSE; break;
        }
        advance_token(ctx);
        
        bool is_named = false;
        bool is_positional = false;
        cco_object_t* map_args = fast_create_object(ctx, CCO_TYPE_MAP);
        cco_object_t* arr_args = fast_create_object(ctx, CCO_TYPE_ARRAY);
        
        while (ctx->current_token.type != CCO_TOK_RPAREN && ctx->current_token.type != CCO_TOK_EOF) {
            bool current_is_named = false;
            if (ctx->current_token.type == CCO_TOK_DOT) {
                current_is_named = true;
                advance_token(ctx); /* skip . */
            }
            
            if (current_is_named) {
                if (is_positional) {
                    cco_diag_record(CCO_ERR_PARSE, ctx->current_token.line, ctx->current_token.col, "parse_value", "cannot mix positional and named arguments");
                    ctx->last_error = CCO_ERR_PARSE; break;
                }
                is_named = true;
                
                if (ctx->current_token.type != CCO_TOK_IDENTIFIER) {
                    cco_diag_record(CCO_ERR_PARSE, ctx->current_token.line, ctx->current_token.col, "parse_value", "expected field name after '.'");
                    ctx->last_error = CCO_ERR_PARSE; break;
                }
                size_t flen = ctx->current_token.text_len;
                char* fname = malloc(flen + 1);
                if (!fname) { ctx->last_error = CCO_ERR_NO_MEMORY; break; }
                memcpy(fname, ctx->current_token.text_ptr, flen);
                fname[flen] = '\0';
                advance_token(ctx);
                
                if (ctx->current_token.type != CCO_TOK_COLON) {
                    cco_diag_record(CCO_ERR_PARSE, ctx->current_token.line, ctx->current_token.col, "parse_value", "expected ':' after field name");
                    free(fname); ctx->last_error = CCO_ERR_PARSE; break;
                }
                advance_token(ctx);
                
                cco_object_t* val = parse_expr(ctx);
                if (!val) { free(fname); ctx->last_error = CCO_ERR_PARSE; break; }
                
                if (map_args) {
                    cco_map_set(map_args, fname, val);
                }
                cco_object_release(val);
                free(fname);
            } else {
                if (is_named) {
                    cco_diag_record(CCO_ERR_PARSE, ctx->current_token.line, ctx->current_token.col, "parse_value", "cannot mix positional and named arguments");
                    ctx->last_error = CCO_ERR_PARSE; break;
                }
                is_positional = true;
                
                cco_object_t* val = parse_expr(ctx);
                if (!val) { ctx->last_error = CCO_ERR_PARSE; break; }
                
                if (arr_args) {
                    cco_array_append(arr_args, val);
                }
                cco_object_release(val);
            }
            
            if (ctx->current_token.type == CCO_TOK_COMMA) {
                advance_token(ctx);
            }
        }
        
        if (ctx->last_error != CCO_OK) {
            if (map_args) cco_object_release(map_args);
            if (arr_args) cco_object_release(arr_args);
            free(tname);
            result = NULL;
            break;
        }
        
        if (ctx->current_token.type == CCO_TOK_RPAREN) {
            advance_token(ctx);
        }
        
        cco_inst_context_t inst_ctx = { .inst_count = ctx->inst_count };
        cco_error_t err = CCO_OK;
        if (is_named) {
            err = cco_instantiate_named(ctx, ctx->symtab, &inst_ctx, tname, map_args ? &map_args->as.map : NULL, &result);
        } else {
            err = cco_instantiate_positional(ctx, ctx->symtab, &inst_ctx, tname, arr_args ? &arr_args->as.array : NULL, &result);
        }
        ctx->inst_count = inst_ctx.inst_count;
        if (err != CCO_OK) {
            ctx->last_error = err;
            result = NULL;
        }
        
        if (map_args) cco_object_release(map_args);
        if (arr_args) cco_object_release(arr_args);
        free(tname);
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
        if (!found && ctx->symtab) {
            found = cco_symtab_lookup_macro(ctx->symtab, name);
        }
        
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
        result = fast_create_object(ctx, CCO_TYPE_NONE);
        advance_token(ctx);
        break;
    case CCO_TOK_TRUE:
        result = fast_create_object(ctx, CCO_TYPE_BOOLEAN);
        if (result)
            result->as.boolean = true;
        advance_token(ctx);
        break;
    case CCO_TOK_FALSE:
        result = fast_create_object(ctx, CCO_TYPE_BOOLEAN);
        if (result)
            result->as.boolean = false;
        advance_token(ctx);
        break;
    case CCO_TOK_INTEGER:
        result = fast_create_object(ctx, CCO_TYPE_INTEGER);
        if (result)
        {
            result->as.integer = cco_parse_integer_literal(ctx->current_token.text_ptr, ctx->current_token.text_len);
        }
        advance_token(ctx);
        break;
    case CCO_TOK_FLOAT:
        result = fast_create_object(ctx, CCO_TYPE_FLOAT);
        if (result)
        {
            result->as.floating = cco_parse_float_literal(ctx->current_token.text_ptr, ctx->current_token.text_len);
        }
        advance_token(ctx);
        break;
    case CCO_TOK_STRING: {
        const char* s_data = ctx->current_token.text_ptr;
        size_t s_len = ctx->current_token.text_len;

#ifdef CCO_ENABLE_FAST
        /* In fast mode, bypass interpolation entirely or use fast memchr */
        if (memchr(s_data, '$', s_len) == NULL) {
            cco_object_t* obj = fast_create_object(ctx, CCO_TYPE_STRING);
            if (obj) {
                obj->as.string.data = (char*)s_data; // zero-copy pointer directly into src
                obj->as.string.length = s_len;
            }
            advance_token(ctx);
            result = obj;
            break;
        }
#else
        if (memchr(s_data, '$', s_len) == NULL) {
            cco_object_t* obj = fast_create_object(ctx, CCO_TYPE_STRING);
            if (obj) {
                char* dup = malloc(s_len + 1);
                if (dup) {
                    memcpy(dup, s_data, s_len);
                    dup[s_len] = '\0';
                    obj->as.string.data = dup;
                    obj->as.string.length = s_len;
                }
            }
            advance_token(ctx);
            result = obj;
            break;
        }
#endif

        cco_strbuf_t sb;
        cco_strbuf_init(&sb, ctx->arena);
        
        size_t i = 0;
        bool has_error = false;
        while (i < s_len) {
            if (s_data[i] == '$' && i + 1 < s_len && (s_data[i+1] == '(' || s_data[i+1] == '{')) {
                
                cco_token_type_t close_tok = (s_data[i+1] == '(') ? CCO_TOK_RPAREN : CCO_TOK_RBRACE;
                i += 2;
                cco_parser_context_t sub_ctx;
                cco_parser_init(&sub_ctx, s_data + i, s_len - i, ctx->options, ctx->arena);
                sub_ctx.scope = ctx->scope;
                
                cco_object_t* expr_val = parse_expr(&sub_ctx);
                if (!expr_val) {
                    ctx->last_error = (sub_ctx.last_error != CCO_OK) ? sub_ctx.last_error : CCO_ERR_PARSE;
                    has_error = true;
                    break;
                }
                if (sub_ctx.current_token.type != close_tok) {
                    cco_diag_record(CCO_ERR_PARSE, ctx->current_token.line, ctx->current_token.col, "parse_value", "expected closing token after interpolation");
                    ctx->last_error = CCO_ERR_PARSE;
                    cco_object_release(expr_val);
                    has_error = true;
                    break;
                }
                
                size_t consumed = 0;
                if (sub_ctx.current_token.text_ptr >= s_data + i &&
                    sub_ctx.current_token.text_ptr <= s_data + s_len) {
                    consumed = (size_t)(sub_ctx.current_token.text_ptr - (s_data + i)) + sub_ctx.current_token.text_len;
                } else if (sub_ctx.lexer.pos > 0) {
                    consumed = sub_ctx.lexer.pos;
                } else {
                    consumed = 1;
                }
                i += (consumed > 0) ? consumed : 1;
                
                if (expr_val->type == CCO_TYPE_STRING) {
                    cco_strbuf_append(&sb, expr_val->as.string.data, expr_val->as.string.length);
                } else if (expr_val->type == CCO_TYPE_INTEGER) {
                    char buf[64];
                    int n = snprintf(buf, sizeof(buf), "%lld", (long long)expr_val->as.integer);
                    if (n > 0) cco_strbuf_append(&sb, buf, (size_t)n);
                } else if (expr_val->type == CCO_TYPE_FLOAT) {
                    char buf[64];
                    int n = snprintf(buf, sizeof(buf), "%g", expr_val->as.floating);
                    if (n > 0) cco_strbuf_append(&sb, buf, (size_t)n);
                } else if (expr_val->type == CCO_TYPE_BOOLEAN) {
                    if (expr_val->as.boolean) cco_strbuf_append(&sb, "true", 4);
                    else cco_strbuf_append(&sb, "false", 5);
                } else if (expr_val->type == CCO_TYPE_NONE) {
                    cco_strbuf_append(&sb, "none", 4);
                } else if (expr_val->type == CCO_TYPE_PARAM) {
                    cco_strbuf_append(&sb, "$", 1);
                    if (expr_val->as.param.name) {
                        cco_strbuf_append(&sb, expr_val->as.param.name, strlen(expr_val->as.param.name));
                    }
                }
                cco_object_release(expr_val);
            } else {
                size_t start = i;
                if (s_data[i] == '$') {
                    cco_strbuf_append(&sb, "$", 1);
                    i++;
                } else {
                    while (i < s_len && s_data[i] != '$') {
                        i++;
                    }
                    cco_strbuf_append(&sb, &s_data[start], i - start);
                }
            }
        }
        
        if (!has_error) {
            result = fast_create_object(ctx, CCO_TYPE_STRING);
            if (result) {
                char* sdup = malloc(sb.length + 1);
                if (sdup) {
                    memcpy(sdup, sb.data, sb.length);
                    sdup[sb.length] = '\0';
                    result->as.string.data = sdup;
                    result->as.string.length = sb.length;
                } else {
                    ctx->last_error = CCO_ERR_NO_MEMORY;
                    cco_object_release(result);
                    result = NULL;
                }
            } else {
                ctx->last_error = CCO_ERR_NO_MEMORY;
            }
        }
        advance_token(ctx);
        break;
    }
    case CCO_TOK_LPAREN:
        advance_token(ctx);
        if (is_map_entry_start(ctx))
        {
            result = parse_map(ctx);
        }
        else if (ctx->current_token.type == CCO_TOK_RPAREN)
        {
            result = parse_map(ctx);
        }
        else if (ctx->current_token.type == CCO_TOK_COMMA && ctx->next_token.type == CCO_TOK_RPAREN)
        {
            advance_token(ctx);
            result = fast_create_object(ctx, CCO_TYPE_ARRAY);
        }
        else
        {
            result = parse_array(ctx);
        }
        if (ctx->current_token.type == CCO_TOK_RPAREN)
        {
            advance_token(ctx);
        }
        else
        {
            cco_diag_record(CCO_ERR_PARSE, ctx->current_token.line,
                            ctx->current_token.col, "parse_value",
                            "Expected closing ')'");
            ctx->last_error = CCO_ERR_PARSE;
            cco_object_release(result);
            result = NULL;
        }
        break;
    case CCO_TOK_LBRACE:
        if (ctx->options && !ctx->options->lenient_brackets)
        {
            cco_diag_record(CCO_ERR_PARSE, ctx->current_token.line,
                            ctx->current_token.col, "parse_value",
                            "Unexpected token '{' (lenient_brackets disabled)");
            ctx->last_error = CCO_ERR_PARSE;
            break;
        }
        advance_token(ctx);
        result = parse_map(ctx);
        if (ctx->current_token.type == CCO_TOK_RBRACE)
        {
            advance_token(ctx);
        }
        else
        {
            cco_diag_record(CCO_ERR_PARSE, ctx->current_token.line,
                            ctx->current_token.col, "parse_value",
                            "Expected closing '}'");
            ctx->last_error = CCO_ERR_PARSE;
            cco_object_release(result);
            result = NULL;
        }
        break;
    case CCO_TOK_LBRACKET:
        if (ctx->options && !ctx->options->lenient_brackets)
        {
            cco_diag_record(CCO_ERR_PARSE, ctx->current_token.line,
                            ctx->current_token.col, "parse_value",
                            "Unexpected token '[' (lenient_brackets disabled)");
            ctx->last_error = CCO_ERR_PARSE;
            break;
        }
        advance_token(ctx);
        result = parse_array(ctx);
        if (ctx->current_token.type == CCO_TOK_RBRACKET)
        {
            advance_token(ctx);
        }
        else
        {
            cco_diag_record(CCO_ERR_PARSE, ctx->current_token.line,
                            ctx->current_token.col, "parse_value",
                            "Expected closing ']'");
            ctx->last_error = CCO_ERR_PARSE;
            cco_object_release(result);
            result = NULL;
        }
        break;
    default:
        cco_diag_record(CCO_ERR_PARSE, ctx->current_token.line,
                        ctx->current_token.col, "parse_value",
                        "Unexpected token");
        ctx->last_error = CCO_ERR_PARSE;
        break;
    }

#ifndef CCO_ENABLE_FAST
    ctx->depth--;
#endif
    return result;
}


/* Note: template dotted names (like A.B) are independent templates, unrelated to A */
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
        char pbuf[256] = {0};
        size_t plen = 0;
        while (ctx->current_token.type == CCO_TOK_IDENTIFIER) {
            size_t klen = ctx->current_token.text_len;
            if (plen + klen + 1 < sizeof(pbuf)) {
                memcpy(pbuf + plen, ctx->current_token.text_ptr, klen);
                plen += klen;
            }
            advance_token(ctx);
            if (ctx->current_token.type == CCO_TOK_DOT) {
                if (plen + 1 < sizeof(pbuf)) {
                    pbuf[plen++] = '.';
                }
                advance_token(ctx);
            } else {
                break;
            }
        }
        if (plen == 0) {
            cco_diag_record(CCO_ERR_PARSE, ctx->current_token.line, ctx->current_token.col, "parse_template", "expected parent template name");
            ctx->last_error = CCO_ERR_PARSE; return NULL;
        }
        parent_name = strdup(pbuf);
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
    def->name = strdup(name_buf);
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
        cco_template_def_destroy(def);
        return NULL;
    }

    if (ctx->symtab) {
        cco_error_t terr = cco_symtab_add_template(ctx->symtab, def->name, def);
        if (terr != CCO_OK) {
            ctx->last_error = terr;
            cco_template_def_destroy(def);
            return NULL;
        }
    }
    return def;
}

cco_object_t* cco_parse_document(cco_parser_context_t* ctx)
{
    if (!ctx)
        return NULL;

    cco_object_t* root = NULL;

    if (ctx->options && ctx->options->max_document_size > 0 &&
        ctx->lexer.src_len > ctx->options->max_document_size)
    {
        cco_diag_record(CCO_ERR_OUT_OF_RANGE, 1, 1, "cco_parse_document",
                        "Document size exceeds limit");
        ctx->last_error = CCO_ERR_OUT_OF_RANGE;
        goto cleanup;
    }

    if (!ctx->symtab) {
        ctx->symtab = cco_symtab_new();
        ctx->owns_symtab = true;
    }

    cco_error_t decl_err = cco_parse_declarations(ctx, ctx->symtab);
    if (decl_err != CCO_OK) {
        ctx->last_error = decl_err;
        goto cleanup;
    }

    while (ctx->current_token.type == CCO_TOK_COMMA) {
        advance_token(ctx);
    }

    if (ctx->current_token.type == CCO_TOK_EOF) {
        root = NULL;
        ctx->last_error = CCO_OK;
        goto cleanup;
    }

    if (is_map_entry_start(ctx))
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
    if (ctx->owns_symtab && ctx->symtab) {
        cco_symtab_free(ctx->symtab);
        ctx->symtab = NULL;
        ctx->owns_symtab = false;
    }
    return root;
}
