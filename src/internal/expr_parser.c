#include "expr.h"
#include <stdlib.h>
#include <string.h>

/* AST Allocators */
cco_expr_t* cco_expr_new_literal(cco_object_t* obj) {
    cco_expr_t* e = calloc(1, sizeof(cco_expr_t));
    if (!e) return NULL;
    e->type = CCO_EXPR_LITERAL;
    e->as.literal = obj;
    if (obj) cco_object_retain(obj);
    return e;
}

cco_expr_t* cco_expr_new_identifier(const char* name, size_t len) {
    cco_expr_t* e = calloc(1, sizeof(cco_expr_t));
    if (!e) return NULL;
    e->type = CCO_EXPR_IDENTIFIER;
    e->as.identifier.name = malloc(len + 1);
    if (e->as.identifier.name) {
        memcpy(e->as.identifier.name, name, len);
        e->as.identifier.name[len] = '\0';
    }
    e->as.identifier.length = len;
    return e;
}

cco_expr_t* cco_expr_new_unary(cco_op_t op, cco_expr_t* right) {
    cco_expr_t* e = calloc(1, sizeof(cco_expr_t));
    if (!e) {
        cco_expr_free(right);
        return NULL;
    }
    e->type = CCO_EXPR_UNARY;
    e->as.unary.op = op;
    e->as.unary.right = right;
    return e;
}

cco_expr_t* cco_expr_new_binary(cco_op_t op, cco_expr_t* left, cco_expr_t* right) {
    cco_expr_t* e = calloc(1, sizeof(cco_expr_t));
    if (!e) {
        cco_expr_free(left);
        cco_expr_free(right);
        return NULL;
    }
    e->type = CCO_EXPR_BINARY;
    e->as.binary.op = op;
    e->as.binary.left = left;
    e->as.binary.right = right;
    return e;
}

cco_expr_t* cco_expr_new_ternary(cco_expr_t* cond, cco_expr_t* t, cco_expr_t* f) {
    cco_expr_t* e = calloc(1, sizeof(cco_expr_t));
    if (!e) {
        cco_expr_free(cond); cco_expr_free(t); cco_expr_free(f);
        return NULL;
    }
    e->type = CCO_EXPR_TERNARY;
    e->as.ternary.condition = cond;
    e->as.ternary.true_branch = t;
    e->as.ternary.false_branch = f;
    return e;
}

cco_expr_t* cco_expr_new_range(cco_expr_t* start, cco_expr_t* end, bool inclusive) {
    cco_expr_t* e = calloc(1, sizeof(cco_expr_t));
    if (!e) {
        cco_expr_free(start); cco_expr_free(end);
        return NULL;
    }
    e->type = CCO_EXPR_RANGE;
    e->as.range.start = start;
    e->as.range.end = end;
    e->as.range.inclusive = inclusive;
    return e;
}

void cco_expr_free(cco_expr_t* e) {
    if (!e) return;
    switch (e->type) {
        case CCO_EXPR_LITERAL:
            if (e->as.literal) cco_object_release(e->as.literal);
            break;
        case CCO_EXPR_IDENTIFIER:
            free(e->as.identifier.name);
            break;
        case CCO_EXPR_UNARY:
            cco_expr_free(e->as.unary.right);
            break;
        case CCO_EXPR_BINARY:
            cco_expr_free(e->as.binary.left);
            cco_expr_free(e->as.binary.right);
            break;
        case CCO_EXPR_TERNARY:
            cco_expr_free(e->as.ternary.condition);
            cco_expr_free(e->as.ternary.true_branch);
            cco_expr_free(e->as.ternary.false_branch);
            break;
        case CCO_EXPR_RANGE:
            cco_expr_free(e->as.range.start);
            cco_expr_free(e->as.range.end);
            break;
        case CCO_EXPR_CALL:
            free(e->as.call.name);
            for (size_t i = 0; i < e->as.call.arg_count; i++) {
                cco_expr_free(e->as.call.args[i]);
            }
            free(e->as.call.args);
            break;
    }
    free(e);
}

/* Pratt Parser */
static void advance(cco_parser_context_t* ctx) {
    ctx->current_token = ctx->next_token;
    cco_lexer_next(&ctx->lexer, &ctx->next_token);
}

static int get_infix_precedence(cco_token_type_t t) {
    switch (t) {
        case CCO_TOK_OR: return 10;
        case CCO_TOK_QMARK: return 15; /* Ternary ? */
        case CCO_TOK_AND: return 20;
        case CCO_TOK_PIPE: return 30; /* Bitwise OR */
        case CCO_TOK_CARET: return 40; /* Bitwise XOR */
        case CCO_TOK_AMPERSAND: return 50; /* Bitwise AND */
        case CCO_TOK_EQEQ:
        case CCO_TOK_NEQ: return 60;
        case CCO_TOK_LT:
        case CCO_TOK_LTE:
        case CCO_TOK_GT:
        case CCO_TOK_GTE: return 70;
        case CCO_TOK_QQ: return 80; /* ?? Coalesce */
        case CCO_TOK_DOTDOT:
        case CCO_TOK_DOTDOTEQ: return 85; /* Ranges */
        case CCO_TOK_PLUS:
        case CCO_TOK_MINUS: return 90;
        case CCO_TOK_STAR:
        case CCO_TOK_SLASH: return 100;
        case CCO_TOK_STARSTAR: return 110; /* Power */
        default: return 0;
    }
}

static cco_expr_t* parse_prefix(cco_parser_context_t* ctx);
static cco_expr_t* parse_infix(cco_parser_context_t* ctx, cco_expr_t* left, int prec);

cco_expr_t* cco_parse_expr(cco_parser_context_t* ctx, int min_prec) {
    cco_expr_t* left = parse_prefix(ctx);
    if (!left) return NULL;
    
    while (1) {
        int prec = get_infix_precedence(ctx->current_token.type);
        if (prec == 0 || prec < min_prec) break;
        left = parse_infix(ctx, left, prec);
        if (!left) return NULL;
    }
    return left;
}

static cco_expr_t* parse_prefix(cco_parser_context_t* ctx) {
    cco_expr_t* expr = NULL;
    cco_token_t t = ctx->current_token;
    
    if (t.type == CCO_TOK_INTEGER) {
        cco_object_t* obj = cco_object_create(CCO_TYPE_INTEGER);
        if (obj) {
            obj->as.integer = cco_parse_integer_literal(t.text_ptr, t.text_len);
        }
        expr = cco_expr_new_literal(obj);
        cco_object_release(obj);
        advance(ctx);
    } else if (t.type == CCO_TOK_FLOAT) {
        cco_object_t* obj = cco_object_create(CCO_TYPE_FLOAT);
        if (obj) {
            obj->as.floating = cco_parse_float_literal(t.text_ptr, t.text_len);
        }
        expr = cco_expr_new_literal(obj);
        cco_object_release(obj);
        advance(ctx);
    } else if (t.type == CCO_TOK_TRUE) {
        cco_object_t* obj = cco_object_create(CCO_TYPE_BOOLEAN);
        obj->as.boolean = true;
        expr = cco_expr_new_literal(obj);
        cco_object_release(obj);
        advance(ctx);
    } else if (t.type == CCO_TOK_FALSE) {
        cco_object_t* obj = cco_object_create(CCO_TYPE_BOOLEAN);
        obj->as.boolean = false;
        expr = cco_expr_new_literal(obj);
        cco_object_release(obj);
        advance(ctx);
    } else if (t.type == CCO_TOK_NONE) {
        cco_object_t* obj = cco_object_create(CCO_TYPE_NONE);
        expr = cco_expr_new_literal(obj);
        cco_object_release(obj);
        advance(ctx);
    } else if (t.type == CCO_TOK_STRING) {
        cco_object_t* obj = cco_object_create(CCO_TYPE_STRING);
        obj->as.string.data = malloc(t.text_len + 1);
        if (obj->as.string.data) {
            memcpy(obj->as.string.data, t.text_ptr, t.text_len);
            obj->as.string.data[t.text_len] = '\0';
        }
        obj->as.string.length = t.text_len;
        expr = cco_expr_new_literal(obj);
        cco_object_release(obj);
        advance(ctx);
    } else if (t.type == CCO_TOK_IDENTIFIER) {
        expr = cco_expr_new_identifier(t.text_ptr, t.text_len);
        advance(ctx);
    } else if (t.type == CCO_TOK_LPAREN) {
        advance(ctx); /* skip ( */
        expr = cco_parse_expr(ctx, 0);
        if (ctx->current_token.type == CCO_TOK_RPAREN) {
            advance(ctx); /* skip ) */
        }
    } else if (t.type == CCO_TOK_MINUS) {
        advance(ctx);
        expr = cco_expr_new_unary(CCO_OP_NEG, cco_parse_expr(ctx, 95));
    } else if (t.type == CCO_TOK_PLUS) {
        advance(ctx);
        expr = cco_expr_new_unary(CCO_OP_POS, cco_parse_expr(ctx, 95));
    } else if (t.type == CCO_TOK_NOT) {
        advance(ctx);
        expr = cco_expr_new_unary(CCO_OP_NOT, cco_parse_expr(ctx, 95));
    } else if (t.type == CCO_TOK_TILDE) {
        advance(ctx);
        expr = cco_expr_new_unary(CCO_OP_BIT_NOT, cco_parse_expr(ctx, 95));
    } else {
        /* Error */
    }
    return expr;
}

static cco_expr_t* parse_infix(cco_parser_context_t* ctx, cco_expr_t* left, int prec) {
    cco_token_t op_tok = ctx->current_token;
    advance(ctx); /* Consume operator */
    
    if (op_tok.type == CCO_TOK_QMARK) {
        /* Ternary condition */
        cco_expr_t* true_branch = cco_parse_expr(ctx, 0);
        if (ctx->current_token.type == CCO_TOK_COLON) {
            advance(ctx);
        }
        /* Right associative: pass the same prec */
        cco_expr_t* false_branch = cco_parse_expr(ctx, prec - 1); 
        return cco_expr_new_ternary(left, true_branch, false_branch);
    } else if (op_tok.type == CCO_TOK_DOTDOT || op_tok.type == CCO_TOK_DOTDOTEQ) {
        cco_expr_t* right = cco_parse_expr(ctx, prec + 1);
        return cco_expr_new_range(left, right, op_tok.type == CCO_TOK_DOTDOTEQ);
    }
    
    cco_op_t op = CCO_OP_NONE;
    int next_prec = prec + 1; /* Left associative by default */
    
    switch (op_tok.type) {
        case CCO_TOK_OR: op = CCO_OP_OR; break;
        case CCO_TOK_AND: op = CCO_OP_AND; break;
        case CCO_TOK_PIPE: op = CCO_OP_BIT_OR; break;
        case CCO_TOK_CARET: op = CCO_OP_BIT_XOR; break;
        case CCO_TOK_AMPERSAND: op = CCO_OP_BIT_AND; break;
        case CCO_TOK_EQEQ: op = CCO_OP_EQ; break;
        case CCO_TOK_NEQ: op = CCO_OP_NEQ; break;
        case CCO_TOK_LT: op = CCO_OP_LT; break;
        case CCO_TOK_LTE: op = CCO_OP_LTE; break;
        case CCO_TOK_GT: op = CCO_OP_GT; break;
        case CCO_TOK_GTE: op = CCO_OP_GTE; break;
        case CCO_TOK_QQ: op = CCO_OP_COALESCE; break;
        case CCO_TOK_PLUS: op = CCO_OP_ADD; break;
        case CCO_TOK_MINUS: op = CCO_OP_SUB; break;
        case CCO_TOK_STAR: op = CCO_OP_MUL; break;
        case CCO_TOK_SLASH: op = CCO_OP_DIV; break;
        case CCO_TOK_STARSTAR: 
            op = CCO_OP_POW; 
            next_prec = prec; /* Right associative */
            break;
        default: break;
    }
    
    cco_expr_t* right = cco_parse_expr(ctx, next_prec);
    return cco_expr_new_binary(op, left, right);
}
