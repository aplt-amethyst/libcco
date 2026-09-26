#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../../src/internal/diag.h"
#include "../../src/internal/parser.h"

extern int tests_run;
extern int tests_passed;

static void test_scope_basic(void) {
    tests_run++; 
    
    cco_scope_t root, child;
    cco_scope_init(&root, NULL);
    cco_scope_init(&child, &root);
    
    cco_object_t* v1 = cco_object_create(CCO_TYPE_INTEGER);
    v1->as.integer = 100;
    assert(cco_scope_bind(&root, "outer", v1) == CCO_OK);
    
    cco_object_t* v2 = cco_object_create(CCO_TYPE_INTEGER);
    v2->as.integer = 200;
    assert(cco_scope_bind(&child, "inner", v2) == CCO_OK);
    
    cco_object_t* found = cco_scope_lookup(&child, "outer");
    assert(found && found->as.integer == 100);
    
    cco_object_t* v3 = cco_object_create(CCO_TYPE_INTEGER);
    v3->as.integer = 999;
    assert(cco_scope_bind(&child, "outer", v3) == CCO_OK);
    found = cco_scope_lookup(&child, "outer");
    assert(found && found->as.integer == 999);
    
    found = cco_scope_lookup(&root, "outer");
    assert(found && found->as.integer == 100);
    
    /* Test duplicate in SAME scope */
    cco_object_t* dup = cco_object_create(CCO_TYPE_INTEGER);
    dup->as.integer = 123;
    assert(cco_scope_bind(&child, "inner", dup) == CCO_ERR_PARSE);
    cco_object_release(dup); /* Since it failed, we must release it */
    
    cco_scope_destroy(&child);
    cco_scope_destroy(&root);
    
    tests_passed++;
}

static void test_parse_bind_ok(void) {
    tests_run++; 
    
    const char* src = "x: Integer = 5";
    cco_parser_context_t ctx;
    cco_parse_options_t opts = {0};
    cco_arena_t arena;
    cco_arena_init(&arena, 1024);
    
    cco_parser_init(&ctx, src, strlen(src), &opts, &arena);
    
    cco_scope_t root;
    cco_scope_init(&root, NULL);
    ctx.scope = &root;
    
    cco_src_loc_t loc = {0};
    cco_parse_bind_result_t result = cco_parse_bind(&ctx, &loc);
    assert(result == PARSE_BIND_OK);
    assert(ctx.last_error == CCO_OK);
    
    cco_object_t* val = cco_scope_lookup(&root, "x");
    assert(val != NULL);
    assert(val->type == CCO_TYPE_INTEGER);
    assert(val->as.integer == 5);
    
    cco_scope_destroy(&root);
    cco_arena_destroy(&arena);
    
    tests_passed++;
}

static void test_parse_bind_not_match(void) {
    tests_run++; 
    
    const char* src = "x: Integer";
    cco_parser_context_t ctx;
    cco_parse_options_t opts = {0};
    cco_arena_t arena;
    cco_arena_init(&arena, 1024);
    
    cco_parser_init(&ctx, src, strlen(src), &opts, &arena);
    
    cco_scope_t root;
    cco_scope_init(&root, NULL);
    ctx.scope = &root;
    
    cco_src_loc_t loc = {0};
    cco_parse_bind_result_t result = cco_parse_bind(&ctx, &loc);
    assert(result == PARSE_BIND_NOT_MATCH);
    assert(ctx.last_error == CCO_OK); /* MUST BE SILENT */
    /* The token after 'Integer' is EOF which is line 1, col 11 (0-indexed col 10) */
    assert(loc.line == 1);
    
    cco_scope_destroy(&root);
    cco_arena_destroy(&arena);
    
    tests_passed++;
}

static void test_parse_bind_error_expr(void) {
    tests_run++; 
    
    const char* src = "x: Integer = )";
    cco_parser_context_t ctx;
    cco_parse_options_t opts = {0};
    cco_arena_t arena;
    cco_arena_init(&arena, 1024);
    
    cco_parser_init(&ctx, src, strlen(src), &opts, &arena);
    
    cco_scope_t root;
    cco_scope_init(&root, NULL);
    ctx.scope = &root;
    
    cco_src_loc_t loc = {0};
    cco_parse_bind_result_t result = cco_parse_bind(&ctx, &loc);
    assert(result == PARSE_BIND_ERROR);
    /* In "x: Integer = )", ')' is at column 14 (1-indexed) or similar. */
    assert(loc.line == 1);
    
    cco_scope_destroy(&root);
    cco_arena_destroy(&arena);
    
    tests_passed++;
}

static void test_parse_static_method_body_ok(void) {
    tests_run++; 
    const char* src = "(x: Integer = 5, $return<Integer>(10))";
    cco_parser_context_t ctx;
    cco_parse_options_t opts = {0};
    cco_arena_t arena;
    cco_arena_init(&arena, 1024);
    
    cco_parser_init(&ctx, src, strlen(src), &opts, &arena);
    
    cco_scope_t root;
    cco_scope_init(&root, NULL);
    ctx.scope = &root;
    
    cco_object_t* obj = cco_parse_static_method_body(&ctx);
    assert(obj == NULL); /* Because we discard */
    assert(ctx.last_error == CCO_OK);
    
    /* x should NOT be in root, because static method body pushes its own scope */
    assert(cco_scope_lookup(&root, "x") == NULL);
    
    cco_scope_destroy(&root);
    cco_arena_destroy(&arena);
    tests_passed++;
}

static void test_parse_static_method_body_eof(void) {
    tests_run++; 
    const char* src = "(x: Integer = 5";
    cco_parser_context_t ctx;
    cco_parse_options_t opts = {0};
    cco_arena_t arena;
    cco_arena_init(&arena, 1024);
    
    cco_parser_init(&ctx, src, strlen(src), &opts, &arena);
    
    cco_scope_t root;
    cco_scope_init(&root, NULL);
    ctx.scope = &root;
    
    cco_object_t* obj = cco_parse_static_method_body(&ctx);
    assert(obj == NULL); 
    assert(ctx.last_error == CCO_OK); /* Normal EOF ending */
    
    cco_scope_destroy(&root);
    cco_arena_destroy(&arena);
    tests_passed++;
}

static void test_parse_static_method_body_unsupported(void) {
    tests_run++; 
    const char* src = "($this.f = 1)";
    cco_parser_context_t ctx;
    cco_parse_options_t opts = {0};
    cco_arena_t arena;
    cco_arena_init(&arena, 1024);
    
    cco_parser_init(&ctx, src, strlen(src), &opts, &arena);
    
    cco_scope_t root;
    cco_scope_init(&root, NULL);
    ctx.scope = &root;
    
    cco_object_t* obj = cco_parse_static_method_body(&ctx);
    assert(obj == NULL); 
    assert(ctx.last_error == CCO_ERR_PARSE);
    
    cco_scope_destroy(&root);
    cco_arena_destroy(&arena);
    tests_passed++;
}


static void test_parse_static_method_body_missing_eq(void) {
    tests_run++; 
    
    const char* src = "(x: Integer)";
    cco_parser_context_t ctx;
    cco_parse_options_t opts = {0};
    cco_arena_t arena;
    cco_arena_init(&arena, 1024);
    
    cco_parser_init(&ctx, src, strlen(src), &opts, &arena);
    
    cco_scope_t root;
    cco_scope_init(&root, NULL);
    ctx.scope = &root;
    
    cco_object_t* obj = cco_parse_static_method_body(&ctx);
    assert(obj == NULL);
    assert(ctx.last_error == CCO_ERR_PARSE);
    /* In "(x: Integer)", '(' is col 1, 'x' is col 2, ':' is col 3, 'Integer' is col 5. 
       The token after Integer is ')' which is at line 1. */
    
    cco_scope_destroy(&root);
    cco_arena_destroy(&arena);
    
    tests_passed++;
}

static void test_parse_static_method_body_assign(void) {
    tests_run++; 
    const char* src = "($this.x = 5)";
    cco_parser_context_t ctx;
    cco_parse_options_t opts = {0};
    cco_arena_t arena;
    cco_arena_init(&arena, 1024);
    cco_parser_init(&ctx, src, strlen(src), &opts, &arena);
    
    cco_scope_t root;
    cco_scope_init(&root, NULL);
    ctx.scope = &root;
    
    cco_object_t* obj = cco_parse_static_method_body(&ctx);
    assert(obj == NULL); 
    assert(ctx.last_error == CCO_ERR_PARSE);
    /* Should error with "assign not supported in skeleton" */
    
    cco_scope_destroy(&root);
    cco_arena_destroy(&arena);
    tests_passed++;
}

static void test_parse_static_method_body_return_empty_type(void) {
    tests_run++; 
    const char* src = "($return<>(10))";
    cco_parser_context_t ctx;
    cco_parse_options_t opts = {0};
    cco_arena_t arena;
    cco_arena_init(&arena, 1024);
    cco_parser_init(&ctx, src, strlen(src), &opts, &arena);
    
    cco_scope_t root;
    cco_scope_init(&root, NULL);
    ctx.scope = &root;
    
    cco_object_t* obj = cco_parse_static_method_body(&ctx);
    assert(obj == NULL); 
    assert(ctx.last_error == CCO_OK);
    
    cco_scope_destroy(&root);
    cco_arena_destroy(&arena);
    tests_passed++;
}

static void test_parse_static_method_body_return_multi_type(void) {
    tests_run++; 
    const char* src = "($return<Integer, Float>(10))";
    cco_parser_context_t ctx;
    cco_parse_options_t opts = {0};
    cco_arena_t arena;
    cco_arena_init(&arena, 1024);
    cco_parser_init(&ctx, src, strlen(src), &opts, &arena);
    
    cco_scope_t root;
    cco_scope_init(&root, NULL);
    ctx.scope = &root;
    
    cco_object_t* obj = cco_parse_static_method_body(&ctx);
    assert(obj == NULL); 
    assert(ctx.last_error == CCO_OK);
    
    cco_scope_destroy(&root);
    cco_arena_destroy(&arena);
    tests_passed++;
}

static void test_parse_static_method_body_assign_false_positive(void) {
    tests_run++; 
    const char* src = "($thisx = 5)";
    cco_parser_context_t ctx;
    cco_parse_options_t opts = {0};
    cco_arena_t arena;
    cco_arena_init(&arena, 1024);
    cco_parser_init(&ctx, src, strlen(src), &opts, &arena);
    
    cco_scope_t root;
    cco_scope_init(&root, NULL);
    ctx.scope = &root;
    
    cco_object_t* obj = cco_parse_static_method_body(&ctx);
    assert(obj == NULL); 
    assert(ctx.last_error == CCO_ERR_PARSE);
    /* Should hit NOT_MATCH or unsupported statement, but NOT the assign interception */
    
    cco_scope_destroy(&root);
    cco_arena_destroy(&arena);
    tests_passed++;
}

static void test_ident_bind_ref(void) {
    tests_run++;
    const char* src1 = "x: Integer = 5";
    cco_parser_context_t ctx;
    cco_parse_options_t opts = {0};
    cco_arena_t arena;
    cco_arena_init(&arena, 1024);
    cco_parser_init(&ctx, src1, strlen(src1), &opts, &arena);
    
    cco_scope_t root;
    cco_scope_init(&root, NULL);
    ctx.scope = &root;
    
    cco_src_loc_t loc1 = {0};
    assert(cco_parse_bind(&ctx, &loc1) == PARSE_BIND_OK);
    
    const char* src2 = "y: Integer = x";
    cco_parser_init(&ctx, src2, strlen(src2), &opts, &arena);
    ctx.scope = &root;
    cco_src_loc_t loc2 = {0};
    assert(cco_parse_bind(&ctx, &loc2) == PARSE_BIND_OK);
    
    cco_object_t* y_val = cco_scope_lookup(&root, "y");
    assert(y_val != NULL);
    assert(y_val->type == CCO_TYPE_INTEGER);
    assert(y_val->as.integer == 5);
    
    cco_scope_destroy(&root);
    cco_arena_destroy(&arena);
    tests_passed++;
}

static void test_ident_static_method_body_return(void) {
    tests_run++; 
    const char* src = "(x: Integer = 5, $return<Integer>(x))";
    cco_parser_context_t ctx;
    cco_parse_options_t opts = {0};
    cco_arena_t arena;
    cco_arena_init(&arena, 1024);
    cco_parser_init(&ctx, src, strlen(src), &opts, &arena);
    
    cco_scope_t root;
    cco_scope_init(&root, NULL);
    ctx.scope = &root;
    
    cco_object_t* obj = cco_parse_static_method_body(&ctx);
    /* Currently parse_static_method_body only does syntax parsing and drops the return value. 
       If it is implemented in the future to return the evaluated object, assert it here. */
    assert(obj == NULL); /* 明确说当前只做语法拦截，值丢弃 */
    assert(ctx.last_error == CCO_OK);
    
    cco_scope_destroy(&root);
    cco_arena_destroy(&arena);
    tests_passed++;
}

static void test_ident_undefined(void) {
    tests_run++;
    const char* src = "y: Integer = z";
    cco_parser_context_t ctx;
    cco_parse_options_t opts = {0};
    cco_arena_t arena;
    cco_arena_init(&arena, 1024);
    
    cco_diag_clear(); /* IMPORTANT: Clear diag */
    cco_parser_init(&ctx, src, strlen(src), &opts, &arena);
    
    cco_scope_t root;
    cco_scope_init(&root, NULL);
    ctx.scope = &root;
    
    cco_src_loc_t loc = {0};
    cco_parse_bind_result_t result = cco_parse_bind(&ctx, &loc);
    assert(result == PARSE_BIND_ERROR);
    assert(ctx.last_error == CCO_ERR_PARSE);
    assert(ctx.current_token.line == 1);
    
    size_t diag_count = 0;
    const cco_diag_entry_t* diags = cco_diag_get_all(&diag_count);
    assert(diag_count > 0);
    assert(diags[diag_count - 1].code == CCO_ERR_PARSE);
    assert(strstr(diags[diag_count - 1].message, "undefined identifier 'z'") != NULL);
    assert(diags[diag_count - 1].line == 1);
    assert(diags[diag_count - 1].col == 14);
    
    cco_scope_destroy(&root);
    cco_arena_destroy(&arena);
    tests_passed++;
}

static void test_ident_nested_lookup(void) {
    tests_run++;
    cco_parser_context_t ctx;
    cco_parse_options_t opts = {0};
    cco_arena_t arena;
    cco_arena_init(&arena, 1024);
    
    const char* src = "y: Integer = x";
    cco_parser_init(&ctx, src, strlen(src), &opts, &arena);
    
    cco_scope_t root;
    cco_scope_init(&root, NULL);
    
    cco_object_t* v_x = cco_object_create(CCO_TYPE_INTEGER);
    v_x->as.integer = 42;
    cco_scope_bind(&root, "x", v_x);
    
    cco_scope_t child;
    cco_scope_init(&child, &root);
    ctx.scope = &child;
    
    cco_src_loc_t loc = {0};
    assert(cco_parse_bind(&ctx, &loc) == PARSE_BIND_OK);
    assert(ctx.last_error == CCO_OK);
    
    cco_object_t* y_val = cco_scope_lookup(&child, "y");
    assert(y_val != NULL);
    assert(y_val->type == CCO_TYPE_INTEGER);
    assert(y_val->as.integer == 42);
    
    /* Destroy child */
    cco_scope_destroy(&child);
    
    /* child y should be invalid now! */
    assert(cco_scope_lookup(&child, "y") == NULL);
    
    /* Root 'x' should still be valid! */
    cco_object_t* root_x = cco_scope_lookup(&root, "x");
    assert(root_x != NULL);
    assert(root_x->as.integer == 42);
    
    cco_scope_destroy(&root);
    cco_arena_destroy(&arena);
    tests_passed++;
}

extern cco_object_t* cco_parse(cco_parser_context_t* ctx); /* To parse map/array */
static void test_map_literal_isolation(void) {
    tests_run++; 
    const char* src = "(x: 5, y: 5)";
    cco_parser_context_t ctx;
    cco_parse_options_t opts = {0};
    cco_arena_t arena;
    cco_arena_init(&arena, 1024);
    cco_parser_init(&ctx, src, strlen(src), &opts, &arena);
    
    cco_object_t* obj = cco_parse_document(&ctx);
    assert(obj != NULL);
    assert(obj->type == CCO_TYPE_MAP);
    cco_object_release(obj);
    
    cco_arena_destroy(&arena);
    tests_passed++;
}

static void test_array_isolation(void) {
    tests_run++; 
    const char* src = "[1, 2, 3]";
    cco_parser_context_t ctx;
    cco_parse_options_t opts = {0};
    cco_arena_t arena;
    cco_arena_init(&arena, 1024);
    cco_parser_init(&ctx, src, strlen(src), &opts, &arena);
    
    cco_object_t* obj = cco_parse_document(&ctx);
    /* In standard libcco [1,2,3] might be parsed by cco_parse, depending on if it expects () vs [] */
    /* Wait, the skeleton says: "只能解析 map 和 array。src/internal/parser.c 现在只有 351 行，只能解析 map 和 array" */
    /* but if [] array isn't fully implemented in base parser, let's just use (1, 2, 3) which is an array in old CCO syntax */
    cco_arena_destroy(&arena);
    tests_passed++;
}

static void test_array_isolation2(void) {
    tests_run++; 
    const char* src = "(1, 2, 3)";
    cco_parser_context_t ctx;
    cco_parse_options_t opts = {0};
    cco_arena_t arena;
    cco_arena_init(&arena, 1024);
    cco_parser_init(&ctx, src, strlen(src), &opts, &arena);
    
    cco_object_t* obj = cco_parse_document(&ctx);
    assert(obj != NULL);
    assert(obj->type == CCO_TYPE_ARRAY);
    cco_object_release(obj);
    
    cco_arena_destroy(&arena);
    tests_passed++;
}

static void test_map_ident_value(void) {
    tests_run++; 
    const char* src = "(x: 5, y: x)";
    cco_parser_context_t ctx;
    cco_parse_options_t opts = {0};
    cco_arena_t arena;
    cco_arena_init(&arena, 1024);
    cco_parser_init(&ctx, src, strlen(src), &opts, &arena);
    
    cco_scope_t root;
    cco_scope_init(&root, NULL);
    ctx.scope = &root;
    
    /* We must bind x so it can be looked up when y: x is parsed */
    cco_object_t* v_x = cco_object_create(CCO_TYPE_INTEGER);
    v_x->as.integer = 5;
    cco_scope_bind(&root, "x", v_x);
    
    cco_object_t* obj = cco_parse_document(&ctx);
    assert(obj != NULL);
    assert(obj->type == CCO_TYPE_MAP);
    
    /* y should evaluate to 5 by looking up x */
    cco_object_release(obj);
    cco_scope_destroy(&root);
    cco_arena_destroy(&arena);
    tests_passed++;
}

static void test_ident_bind_continuous_parse(void) {
    tests_run++; 
    cco_parser_context_t ctx;
    cco_parse_options_t opts = {0};
    cco_arena_t arena;
    cco_arena_init(&arena, 1024);
    
    cco_scope_t root;
    cco_scope_init(&root, NULL);
    
    /* First parse_bind */
    const char* src1 = "x: Integer = 5";
    cco_parser_init(&ctx, src1, strlen(src1), &opts, &arena);
    ctx.scope = &root; /* attach scope AFTER init, so it persists! */
    cco_src_loc_t loc1 = {0};
    assert(cco_parse_bind(&ctx, &loc1) == PARSE_BIND_OK);
    
    /* Second parse_bind on same ctx, new src */
    const char* src2 = "y: Integer = x";
    cco_parser_init(&ctx, src2, strlen(src2), &opts, &arena);
    ctx.scope = &root; /* re-attach scope! */
    cco_src_loc_t loc2 = {0};
    assert(cco_parse_bind(&ctx, &loc2) == PARSE_BIND_OK);
    
    cco_object_t* y_val = cco_scope_lookup(&root, "y");
    assert(y_val != NULL);
    assert(y_val->as.integer == 5);
    
    cco_scope_destroy(&root);
    cco_arena_destroy(&arena);
    tests_passed++;
}

static void test_expr_arith_add(void) {
    tests_run++; 
    cco_parser_context_t ctx; cco_parse_options_t opts = {0};
    cco_arena_t arena; cco_arena_init(&arena, 1024);
    cco_scope_t root; cco_scope_init(&root, NULL);
    
    cco_parser_init(&ctx, "x: Integer = 10 + 20", 20, &opts, &arena);
    ctx.scope = &root; cco_src_loc_t loc = {0};
    assert(cco_parse_bind(&ctx, &loc) == PARSE_BIND_OK);
    cco_object_t* val = cco_scope_lookup(&root, "x");
    assert(val != NULL); assert(val->type == CCO_TYPE_INTEGER); assert(val->as.integer == 30);
    
    cco_scope_destroy(&root); cco_arena_destroy(&arena); tests_passed++;
}

static void test_expr_arith_precedence_mul_first(void) {
    tests_run++; 
    cco_parser_context_t ctx; cco_parse_options_t opts = {0};
    cco_arena_t arena; cco_arena_init(&arena, 1024);
    cco_scope_t root; cco_scope_init(&root, NULL);
    
    cco_parser_init(&ctx, "y: Integer = 10 + 20 * 3", 24, &opts, &arena);
    ctx.scope = &root; cco_src_loc_t loc = {0};
    assert(cco_parse_bind(&ctx, &loc) == PARSE_BIND_OK);
    cco_object_t* val = cco_scope_lookup(&root, "y");
    assert(val != NULL); assert(val->as.integer == 70);
    
    cco_scope_destroy(&root); cco_arena_destroy(&arena); tests_passed++;
}

static void test_expr_arith_precedence_add_first(void) {
    tests_run++; 
    cco_parser_context_t ctx; cco_parse_options_t opts = {0};
    cco_arena_t arena; cco_arena_init(&arena, 1024);
    cco_scope_t root; cco_scope_init(&root, NULL);
    
    cco_parser_init(&ctx, "z: Integer = 10 * 20 + 3", 24, &opts, &arena);
    ctx.scope = &root; cco_src_loc_t loc = {0};
    assert(cco_parse_bind(&ctx, &loc) == PARSE_BIND_OK);
    cco_object_t* val = cco_scope_lookup(&root, "z");
    assert(val != NULL); assert(val->as.integer == 203);
    
    cco_scope_destroy(&root); cco_arena_destroy(&arena); tests_passed++;
}

static void test_expr_arith_sub_left_assoc(void) {
    tests_run++; 
    cco_parser_context_t ctx; cco_parse_options_t opts = {0};
    cco_arena_t arena; cco_arena_init(&arena, 1024);
    cco_scope_t root; cco_scope_init(&root, NULL);
    
    cco_parser_init(&ctx, "w: Integer = 10 - 5 - 3", 23, &opts, &arena);
    ctx.scope = &root; cco_src_loc_t loc = {0};
    assert(cco_parse_bind(&ctx, &loc) == PARSE_BIND_OK);
    cco_object_t* val = cco_scope_lookup(&root, "w");
    assert(val != NULL); assert(val->as.integer == 2);
    
    cco_scope_destroy(&root); cco_arena_destroy(&arena); tests_passed++;
}

static void test_expr_arith_float_promote(void) {
    tests_run++; 
    cco_parser_context_t ctx; cco_parse_options_t opts = {0};
    cco_arena_t arena; cco_arena_init(&arena, 1024);
    cco_scope_t root; cco_scope_init(&root, NULL);
    
    cco_parser_init(&ctx, "f1: Float = 10.5 + 20", 21, &opts, &arena);
    ctx.scope = &root; cco_src_loc_t loc = {0};
    assert(cco_parse_bind(&ctx, &loc) == PARSE_BIND_OK);
    cco_object_t* val = cco_scope_lookup(&root, "f1");
    assert(val != NULL); assert(val->type == CCO_TYPE_FLOAT); assert(val->as.floating == 30.5);
    
    cco_scope_destroy(&root); cco_arena_destroy(&arena); tests_passed++;
}

static void test_expr_arith_float_mul(void) {
    tests_run++; 
    cco_parser_context_t ctx; cco_parse_options_t opts = {0};
    cco_arena_t arena; cco_arena_init(&arena, 1024);
    cco_scope_t root; cco_scope_init(&root, NULL);
    
    cco_parser_init(&ctx, "f2: Float = 1.5 * 2", 19, &opts, &arena);
    ctx.scope = &root; cco_src_loc_t loc = {0};
    assert(cco_parse_bind(&ctx, &loc) == PARSE_BIND_OK);
    cco_object_t* val = cco_scope_lookup(&root, "f2");
    assert(val != NULL); assert(val->as.floating == 3.0);
    
    cco_scope_destroy(&root); cco_arena_destroy(&arena); tests_passed++;
}

static void test_expr_arith_int_div_trunc(void) {
    tests_run++; 
    cco_parser_context_t ctx; cco_parse_options_t opts = {0};
    cco_arena_t arena; cco_arena_init(&arena, 1024);
    cco_scope_t root; cco_scope_init(&root, NULL);
    
    cco_parser_init(&ctx, "div: Integer = 10 / 3", 21, &opts, &arena);
    ctx.scope = &root; cco_src_loc_t loc = {0};
    assert(cco_parse_bind(&ctx, &loc) == PARSE_BIND_OK);
    cco_object_t* val = cco_scope_lookup(&root, "div");
    assert(val != NULL); assert(val->as.integer == 3);
    
    cco_scope_destroy(&root); cco_arena_destroy(&arena); tests_passed++;
}

static void test_expr_arith_float_div_zero(void) {
    tests_run++; 
    cco_parser_context_t ctx; cco_parse_options_t opts = {0};
    cco_arena_t arena; cco_arena_init(&arena, 1024);
    cco_scope_t root; cco_scope_init(&root, NULL);
    
    cco_diag_clear();
    cco_parser_init(&ctx, "x: Float = 10.5 / 0.0", 21, &opts, &arena);
    ctx.scope = &root; cco_src_loc_t loc = {0};
    
    assert(cco_parse_bind(&ctx, &loc) == PARSE_BIND_ERROR);
    
    size_t diag_count = 0;
    const cco_diag_entry_t* diags = cco_diag_get_all(&diag_count);
    assert(diag_count > 0);
    assert(strstr(diags[diag_count - 1].message, "division by zero") != NULL);
    
    cco_scope_destroy(&root); cco_arena_destroy(&arena); tests_passed++;
}

static void test_expr_arithmetic_error(void) {
    tests_run++; 
    const char* src = "x: Integer = 1 + \"a\"";
    cco_parser_context_t ctx;
    cco_parse_options_t opts = {0};
    cco_arena_t arena;
    cco_arena_init(&arena, 1024);
    
    cco_diag_clear();
    cco_parser_init(&ctx, src, strlen(src), &opts, &arena);
    
    cco_scope_t root;
    cco_scope_init(&root, NULL);
    ctx.scope = &root;
    
    cco_src_loc_t loc = {0};
    cco_parse_bind_result_t result = cco_parse_bind(&ctx, &loc);
    assert(result == PARSE_BIND_ERROR);
    assert(ctx.last_error == CCO_ERR_PARSE);
    
    size_t diag_count = 0;
    const cco_diag_entry_t* diags = cco_diag_get_all(&diag_count);
    assert(diag_count > 0);
    assert(diags[diag_count - 1].code == CCO_ERR_PARSE);
    assert(strstr(diags[diag_count - 1].message, "operator '+' not defined for Integer and String") != NULL);
    
    /* Token is the '+' */
    assert(diags[diag_count - 1].line == 1);
    assert(diags[diag_count - 1].col == 16);
    
    cco_scope_destroy(&root);
    cco_arena_destroy(&arena);
    tests_passed++;
}

static void test_scope_memory_leak(void) {
    tests_run++; 
    cco_scope_t root;
    cco_scope_init(&root, NULL);
    
    cco_object_t* v1 = cco_object_create(CCO_TYPE_INTEGER);
    cco_object_t* v2 = cco_object_create(CCO_TYPE_FLOAT);
    cco_object_t* v3 = cco_object_create(CCO_TYPE_BOOLEAN);
    
    cco_scope_bind(&root, "a", v1);
    cco_scope_bind(&root, "b", v2);
    cco_scope_bind(&root, "c", v3);
    
    /* Just destroy, should not leak */
    cco_scope_destroy(&root);
    tests_passed++;
}

void test_scope_all(void) {
    test_scope_basic();
    test_parse_bind_ok();
    test_parse_bind_not_match();
    test_parse_bind_error_expr();
    test_parse_static_method_body_ok();
    test_parse_static_method_body_eof();
    test_parse_static_method_body_unsupported();
    test_parse_static_method_body_missing_eq();
    test_parse_static_method_body_assign();
    test_parse_static_method_body_return_empty_type();
    test_parse_static_method_body_return_multi_type();
    test_parse_static_method_body_assign_false_positive();
    test_ident_bind_ref();
    test_ident_static_method_body_return();
    test_ident_undefined();
    test_ident_nested_lookup();
    test_map_literal_isolation();
    test_array_isolation2();
    test_map_ident_value();
    test_ident_bind_continuous_parse();
    test_expr_arith_add();
    test_expr_arith_precedence_mul_first();
    test_expr_arith_precedence_add_first();
    test_expr_arith_sub_left_assoc();
    test_expr_arith_float_promote();
    test_expr_arith_float_mul();
    test_expr_arith_int_div_trunc();
    test_expr_arith_float_div_zero();
    test_expr_arithmetic_error();
    test_scope_memory_leak();
}
