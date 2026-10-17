#include "eval.h"
#include "expr.h"
#include "object.h"
#include <math.h>

static cco_object_t* eval_unary(cco_op_t op, cco_object_t* right) {
    if (!right) return NULL;
    cco_object_t* res = cco_object_create(CCO_TYPE_NONE);
    if (op == CCO_OP_NOT) {
        bool val = false;
        if (right->type == CCO_TYPE_BOOLEAN) val = right->as.boolean;
        res->type = CCO_TYPE_BOOLEAN;
        res->as.boolean = !val;
    } else if (op == CCO_OP_NEG) {
        if (right->type == CCO_TYPE_INTEGER) {
            res->type = CCO_TYPE_INTEGER;
            res->as.integer = -right->as.integer;
        } else if (right->type == CCO_TYPE_FLOAT) {
            res->type = CCO_TYPE_FLOAT;
            res->as.floating = -right->as.floating;
        }
    } else if (op == CCO_OP_POS) {
        if (right->type == CCO_TYPE_INTEGER || right->type == CCO_TYPE_FLOAT) {
            res = cco_object_clone(right);
        }
    } else if (op == CCO_OP_BIT_NOT) {
        if (right->type == CCO_TYPE_INTEGER) {
            res->type = CCO_TYPE_INTEGER;
            res->as.integer = ~right->as.integer;
        }
    }
    return res;
}

static cco_object_t* eval_binary(cco_op_t op, cco_object_t* left, cco_object_t* right) {
    if (!left || !right) return NULL;
    cco_object_t* res = cco_object_create(CCO_TYPE_NONE);

    if (op == CCO_OP_ADD || op == CCO_OP_SUB || op == CCO_OP_MUL || op == CCO_OP_DIV || op == CCO_OP_POW) {
        bool is_float = (left->type == CCO_TYPE_FLOAT || right->type == CCO_TYPE_FLOAT);
        double f_l = left->type == CCO_TYPE_FLOAT ? left->as.floating : (double)left->as.integer;
        double f_r = right->type == CCO_TYPE_FLOAT ? right->as.floating : (double)right->as.integer;
        int64_t i_l = left->type == CCO_TYPE_INTEGER ? left->as.integer : (int64_t)left->as.floating;
        int64_t i_r = right->type == CCO_TYPE_INTEGER ? right->as.integer : (int64_t)right->as.floating;

        if (is_float) {
            res->type = CCO_TYPE_FLOAT;
            if (op == CCO_OP_ADD) res->as.floating = f_l + f_r;
            else if (op == CCO_OP_SUB) res->as.floating = f_l - f_r;
            else if (op == CCO_OP_MUL) res->as.floating = f_l * f_r;
            else if (op == CCO_OP_DIV) res->as.floating = f_r != 0 ? f_l / f_r : 0;
            else if (op == CCO_OP_POW) res->as.floating = pow(f_l, f_r);
        } else {
            res->type = CCO_TYPE_INTEGER;
            if (op == CCO_OP_ADD) res->as.integer = i_l + i_r;
            else if (op == CCO_OP_SUB) res->as.integer = i_l - i_r;
            else if (op == CCO_OP_MUL) res->as.integer = i_l * i_r;
            else if (op == CCO_OP_DIV) res->as.integer = i_r != 0 ? i_l / i_r : 0;
            else if (op == CCO_OP_POW) res->as.integer = (int64_t)pow((double)i_l, (double)i_r);
        }
    } else if (op == CCO_OP_BIT_OR || op == CCO_OP_BIT_AND || op == CCO_OP_BIT_XOR) {
        if (left->type == CCO_TYPE_INTEGER && right->type == CCO_TYPE_INTEGER) {
            res->type = CCO_TYPE_INTEGER;
            if (op == CCO_OP_BIT_OR) res->as.integer = left->as.integer | right->as.integer;
            else if (op == CCO_OP_BIT_AND) res->as.integer = left->as.integer & right->as.integer;
            else if (op == CCO_OP_BIT_XOR) res->as.integer = left->as.integer ^ right->as.integer;
        }
    } else if (op == CCO_OP_EQ || op == CCO_OP_NEQ) {
        res->type = CCO_TYPE_BOOLEAN;
        bool eq = cco_object_equals(left, right);
        res->as.boolean = (op == CCO_OP_EQ) ? eq : !eq;
    } else if (op >= CCO_OP_LT && op <= CCO_OP_GTE) {
        res->type = CCO_TYPE_BOOLEAN;
        if ((left->type == CCO_TYPE_INTEGER || left->type == CCO_TYPE_FLOAT) &&
            (right->type == CCO_TYPE_INTEGER || right->type == CCO_TYPE_FLOAT)) {
            double f_l = left->type == CCO_TYPE_FLOAT ? left->as.floating : (double)left->as.integer;
            double f_r = right->type == CCO_TYPE_FLOAT ? right->as.floating : (double)right->as.integer;
            if (op == CCO_OP_LT) res->as.boolean = f_l < f_r;
            else if (op == CCO_OP_LTE) res->as.boolean = f_l <= f_r;
            else if (op == CCO_OP_GT) res->as.boolean = f_l > f_r;
            else if (op == CCO_OP_GTE) res->as.boolean = f_l >= f_r;
        }
    }
    return res;
}

cco_object_t* cco_eval_expr(cco_parser_context_t* ctx, cco_expr_t* expr) {
    if (!expr) return NULL;
    
    switch (expr->type) {
        case CCO_EXPR_LITERAL: {
            if (expr->as.literal) return cco_object_clone(expr->as.literal);
            return cco_object_create(CCO_TYPE_NONE);
        }
        case CCO_EXPR_IDENTIFIER: {
            /* Not fully implemented scoping yet. Just return None for now */
            return cco_object_create(CCO_TYPE_NONE);
        }
        case CCO_EXPR_UNARY: {
            cco_object_t* r = cco_eval_expr(ctx, expr->as.unary.right);
            cco_object_t* res = eval_unary(expr->as.unary.op, r);
            if (r) cco_object_release(r);
            return res;
        }
        case CCO_EXPR_BINARY: {
            if (expr->as.binary.op == CCO_OP_AND) {
                cco_object_t* l = cco_eval_expr(ctx, expr->as.binary.left);
                bool l_val = (l && l->type == CCO_TYPE_BOOLEAN && l->as.boolean);
                if (!l_val) return l; /* short circuit */
                if (l) cco_object_release(l);
                return cco_eval_expr(ctx, expr->as.binary.right);
            }
            if (expr->as.binary.op == CCO_OP_OR) {
                cco_object_t* l = cco_eval_expr(ctx, expr->as.binary.left);
                bool l_val = (l && l->type == CCO_TYPE_BOOLEAN && l->as.boolean);
                if (l_val) return l; /* short circuit */
                if (l) cco_object_release(l);
                return cco_eval_expr(ctx, expr->as.binary.right);
            }
            if (expr->as.binary.op == CCO_OP_COALESCE) {
                cco_object_t* l = cco_eval_expr(ctx, expr->as.binary.left);
                if (l && l->type != CCO_TYPE_NONE) return l;
                if (l) cco_object_release(l);
                return cco_eval_expr(ctx, expr->as.binary.right);
            }
            
            cco_object_t* l = cco_eval_expr(ctx, expr->as.binary.left);
            cco_object_t* r = cco_eval_expr(ctx, expr->as.binary.right);
            cco_object_t* res = eval_binary(expr->as.binary.op, l, r);
            if (l) cco_object_release(l);
            if (r) cco_object_release(r);
            return res;
        }
        case CCO_EXPR_TERNARY: {
            cco_object_t* cond = cco_eval_expr(ctx, expr->as.ternary.condition);
            bool is_true = (cond && cond->type == CCO_TYPE_BOOLEAN && cond->as.boolean);
            if (cond) cco_object_release(cond);
            if (is_true) return cco_eval_expr(ctx, expr->as.ternary.true_branch);
            else return cco_eval_expr(ctx, expr->as.ternary.false_branch);
        }
        case CCO_EXPR_RANGE: {
            cco_object_t* l = cco_eval_expr(ctx, expr->as.range.start);
            cco_object_t* r = cco_eval_expr(ctx, expr->as.range.end);
            cco_object_t* arr = cco_object_create(CCO_TYPE_ARRAY);
            if (l && r && l->type == CCO_TYPE_INTEGER && r->type == CCO_TYPE_INTEGER) {
                int64_t start = l->as.integer;
                int64_t end = r->as.integer;
                if (expr->as.range.inclusive) {
                    if (start <= end) end++;
                    else end--;
                }
                size_t count = (start <= end) ? (size_t)(end - start) : (size_t)(start - end);
                if (count > 10000) {
                    /* OOM Threshold! Return empty or error. */
                } else {
                    if (start <= end) {
                        for (int64_t i = start; i < end; i++) {
                            cco_object_t* item = cco_object_create(CCO_TYPE_INTEGER);
                            if (item) {
                                item->as.integer = i;
                                cco_array_append(arr, item);
                                cco_object_release(item);
                            }
                        }
                    } else {
                        for (int64_t i = start; i > end; i--) {
                            cco_object_t* item = cco_object_create(CCO_TYPE_INTEGER);
                            if (item) {
                                item->as.integer = i;
                                cco_array_append(arr, item);
                                cco_object_release(item);
                            }
                        }
                    }
                }
            }
            if (l) cco_object_release(l);
            if (r) cco_object_release(r);
            return arr;
        }
        case CCO_EXPR_CALL:
            return cco_object_create(CCO_TYPE_NONE);
    }
    return NULL;
}
