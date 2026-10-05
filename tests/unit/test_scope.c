#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../../src/internal/diag.h"
#include "../../src/internal/parser.h"
#include "../../src/internal/symtab.h"
#include <stdlib.h>

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
    const char* src = "$function.#f<>: (x: Integer = 5, $return<Integer>($(10)))";
    cco_parser_context_t ctx;
    cco_parse_options_t opts = {0};
    cco_arena_t arena;
    cco_arena_init(&arena, 1024);
    
    cco_parser_init(&ctx, src, strlen(src), &opts, &arena);
    
    cco_scope_t root;
    cco_scope_init(&root, NULL);
    ctx.scope = &root;
    
    cco_object_t* obj = cco_parse_function(&ctx);
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
    const char* src = "$function.#f<>: (x: Integer = 5";
    cco_parser_context_t ctx;
    cco_parse_options_t opts = {0};
    cco_arena_t arena;
    cco_arena_init(&arena, 1024);
    
    cco_parser_init(&ctx, src, strlen(src), &opts, &arena);
    
    cco_scope_t root;
    cco_scope_init(&root, NULL);
    ctx.scope = &root;
    
    cco_object_t* obj = cco_parse_function(&ctx);
    assert(obj == NULL); 
    assert(ctx.last_error == CCO_OK); /* Normal EOF ending */
    
    cco_scope_destroy(&root);
    cco_arena_destroy(&arena);
    tests_passed++;
}

static void test_parse_static_method_body_unsupported(void) {
    tests_run++; 
    const char* src = "$function.#f<>: ($this.f = 1)";
    cco_parser_context_t ctx;
    cco_parse_options_t opts = {0};
    cco_arena_t arena;
    cco_arena_init(&arena, 1024);
    
    cco_parser_init(&ctx, src, strlen(src), &opts, &arena);
    
    cco_scope_t root;
    cco_scope_init(&root, NULL);
    ctx.scope = &root;
    
    cco_object_t* obj = cco_parse_function(&ctx);
    assert(obj == NULL); 
    assert(ctx.last_error == CCO_ERR_PARSE);
    
    cco_scope_destroy(&root);
    cco_arena_destroy(&arena);
    tests_passed++;
}


static void test_parse_static_method_body_missing_eq(void) {
    tests_run++; 
    
    const char* src = "$function.#f<>: (x: Integer)";
    cco_parser_context_t ctx;
    cco_parse_options_t opts = {0};
    cco_arena_t arena;
    cco_arena_init(&arena, 1024);
    
    cco_parser_init(&ctx, src, strlen(src), &opts, &arena);
    
    cco_scope_t root;
    cco_scope_init(&root, NULL);
    ctx.scope = &root;
    
    cco_object_t* obj = cco_parse_function(&ctx);
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
    const char* src = "$function.#f<>: ($this.x = 5)";
    cco_parser_context_t ctx;
    cco_parse_options_t opts = {0};
    cco_arena_t arena;
    cco_arena_init(&arena, 1024);
    cco_parser_init(&ctx, src, strlen(src), &opts, &arena);
    
    cco_scope_t root;
    cco_scope_init(&root, NULL);
    ctx.scope = &root;
    
    cco_object_t* obj = cco_parse_function(&ctx);
    assert(obj == NULL); 
    assert(ctx.last_error == CCO_ERR_PARSE);
    /* Should error with "assign not supported in skeleton" */
    
    cco_scope_destroy(&root);
    cco_arena_destroy(&arena);
    tests_passed++;
}

static void test_parse_static_method_body_return_empty_type(void) {
    tests_run++; 
    const char* src = "$function.#f<>: ($return<>(10))";
    cco_parser_context_t ctx;
    cco_parse_options_t opts = {0};
    cco_arena_t arena;
    cco_arena_init(&arena, 1024);
    cco_parser_init(&ctx, src, strlen(src), &opts, &arena);
    
    cco_scope_t root;
    cco_scope_init(&root, NULL);
    ctx.scope = &root;
    
    cco_object_t* obj = cco_parse_function(&ctx);
    assert(obj == NULL); 
    assert(ctx.last_error == CCO_OK);
    
    cco_scope_destroy(&root);
    cco_arena_destroy(&arena);
    tests_passed++;
}

static void test_parse_static_method_body_return_multi_type(void) {
    tests_run++; 
    const char* src = "$function.#f<>: ($return<Integer, Float>(10))";
    cco_parser_context_t ctx;
    cco_parse_options_t opts = {0};
    cco_arena_t arena;
    cco_arena_init(&arena, 1024);
    cco_parser_init(&ctx, src, strlen(src), &opts, &arena);
    
    cco_scope_t root;
    cco_scope_init(&root, NULL);
    ctx.scope = &root;
    
    cco_object_t* obj = cco_parse_function(&ctx);
    assert(obj == NULL); 
    assert(ctx.last_error == CCO_OK);
    
    cco_scope_destroy(&root);
    cco_arena_destroy(&arena);
    tests_passed++;
}

static void test_parse_static_method_body_assign_false_positive(void) {
    tests_run++; 
    const char* src = "$function.#f<>: ($thisx = 5)";
    cco_parser_context_t ctx;
    cco_parse_options_t opts = {0};
    cco_arena_t arena;
    cco_arena_init(&arena, 1024);
    cco_parser_init(&ctx, src, strlen(src), &opts, &arena);
    
    cco_scope_t root;
    cco_scope_init(&root, NULL);
    ctx.scope = &root;
    
    cco_object_t* obj = cco_parse_function(&ctx);
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
    const char* src = "$function.#f<>: (x: Integer = 5, $return<Integer>(x))";
    cco_parser_context_t ctx;
    cco_parse_options_t opts = {0};
    cco_arena_t arena;
    cco_arena_init(&arena, 1024);
    cco_parser_init(&ctx, src, strlen(src), &opts, &arena);
    
    cco_scope_t root;
    cco_scope_init(&root, NULL);
    ctx.scope = &root;
    
    cco_object_t* obj = cco_parse_function(&ctx);
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
    
    cco_parser_init(&ctx, "x: Integer = $(10 + 20)", strlen("x: Integer = $(10 + 20)"), &opts, &arena);
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
    
    cco_parser_init(&ctx, "y: Integer = $(10 + 20 * 3)", strlen("y: Integer = $(10 + 20 * 3)"), &opts, &arena);
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
    
    cco_parser_init(&ctx, "z: Integer = $(10 * 20 + 3)", strlen("z: Integer = $(10 * 20 + 3)"), &opts, &arena);
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
    
    cco_parser_init(&ctx, "w: Integer = $(10 - 5 - 3)", strlen("w: Integer = $(10 - 5 - 3)"), &opts, &arena);
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
    
    cco_parser_init(&ctx, "f1: Float = $(10.5 + 20)", strlen("f1: Float = $(10.5 + 20)"), &opts, &arena);
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
    
    cco_parser_init(&ctx, "f2: Float = $(1.5 * 2)", strlen("f2: Float = $(1.5 * 2)"), &opts, &arena);
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
    
    cco_parser_init(&ctx, "div: Integer = $(10 / 3)", strlen("div: Integer = $(10 / 3)"), &opts, &arena);
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
    cco_parser_init(&ctx, "x: Float = $(10.5 / 0.0)", strlen("x: Float = $(10.5 / 0.0)"), &opts, &arena);
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
    const char* src = "x: Integer = $(1 + \"a\")";
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
    assert(diags[diag_count - 1].col == 18);
    
    cco_scope_destroy(&root);
    cco_arena_destroy(&arena);
    tests_passed++;
}

static void test_expr_arith_nested(void) {
    tests_run++; 
    cco_parser_context_t ctx; cco_parse_options_t opts = {0};
    cco_arena_t arena; cco_arena_init(&arena, 1024);
    cco_scope_t root; cco_scope_init(&root, NULL);
    
    cco_parser_init(&ctx, "x: Integer = $($(10 + 20) * 3)", strlen("x: Integer = $($(10 + 20) * 3)"), &opts, &arena);
    ctx.scope = &root; cco_src_loc_t loc = {0};
    assert(cco_parse_bind(&ctx, &loc) == PARSE_BIND_OK);
    cco_object_t* val = cco_scope_lookup(&root, "x");
    assert(val != NULL); assert(val->as.integer == 90);
    
    cco_scope_destroy(&root); cco_arena_destroy(&arena); tests_passed++;
}

static void test_expr_bare_ok(void) {
    tests_run++; 
    cco_parser_context_t ctx; cco_parse_options_t opts = {0};
    cco_arena_t arena; cco_arena_init(&arena, 1024);
    cco_scope_t root; cco_scope_init(&root, NULL);
    
    cco_diag_clear();
    cco_parser_init(&ctx, "x: Integer = 10 + 20", strlen("x: Integer = 10 + 20"), &opts, &arena);
    ctx.scope = &root; cco_src_loc_t loc = {0};
    
    assert(cco_parse_bind(&ctx, &loc) == PARSE_BIND_OK);
    cco_object_t* val = cco_scope_lookup(&root, "x");
    assert(val != NULL); assert(val->as.integer == 30);
    
    cco_scope_destroy(&root); cco_arena_destroy(&arena); tests_passed++;
}

static void test_unary_minus(void) {
    tests_run++; 
    cco_parser_context_t ctx; cco_parse_options_t opts = {0};
    cco_arena_t arena; cco_arena_init(&arena, 1024);
    cco_scope_t root; cco_scope_init(&root, NULL);
    
    cco_parser_init(&ctx, "x: Integer = -10", strlen("x: Integer = -10"), &opts, &arena);
    ctx.scope = &root; cco_src_loc_t loc = {0};
    assert(cco_parse_bind(&ctx, &loc) == PARSE_BIND_OK);
    cco_object_t* val = cco_scope_lookup(&root, "x");
    assert(val != NULL); assert(val->as.integer == -10);
    
    cco_parser_init(&ctx, "f: Float = -3.14", strlen("f: Float = -3.14"), &opts, &arena);
    ctx.scope = &root;
    assert(cco_parse_bind(&ctx, &loc) == PARSE_BIND_OK);
    val = cco_scope_lookup(&root, "f");
    assert(val != NULL); assert(val->as.floating == -3.14);
    
    /* lookup variable */
    cco_parser_init(&ctx, "y: Integer = -x", strlen("y: Integer = -x"), &opts, &arena);
    ctx.scope = &root; /* re-attach */
    assert(cco_parse_bind(&ctx, &loc) == PARSE_BIND_OK);
    val = cco_scope_lookup(&root, "y");
    assert(val != NULL); assert(val->as.integer == 10);
    
    /* error case */
    cco_diag_clear();
    cco_parser_init(&ctx, "e: Integer = -\"str\"", strlen("e: Integer = -\"str\""), &opts, &arena);
    ctx.scope = &root;
    assert(cco_parse_bind(&ctx, &loc) == PARSE_BIND_ERROR);
    size_t diag_count = 0;
    const cco_diag_entry_t* diags = cco_diag_get_all(&diag_count);
    assert(diag_count > 0);
    assert(strstr(diags[diag_count - 1].message, "unary minus not defined for String") != NULL);
    
    cco_scope_destroy(&root); cco_arena_destroy(&arena); tests_passed++;
}

static void test_expr_missing_rparen(void) {
    tests_run++; 
    cco_parser_context_t ctx; cco_parse_options_t opts = {0};
    cco_arena_t arena; cco_arena_init(&arena, 1024);
    cco_scope_t root; cco_scope_init(&root, NULL);
    
    cco_diag_clear();
    const char* src = "x: Integer = $(10 + 20";
    cco_parser_init(&ctx, src, strlen(src), &opts, &arena);
    ctx.scope = &root; cco_src_loc_t loc = {0};
    
    assert(cco_parse_bind(&ctx, &loc) == PARSE_BIND_ERROR);
    
    size_t diag_count = 0;
    const cco_diag_entry_t* diags = cco_diag_get_all(&diag_count);
    assert(diag_count > 0);
    assert(strstr(diags[diag_count - 1].message, "expected ')'") != NULL);
    /* Should point to EOF, which is line 1, col 23 */
    assert(diags[diag_count - 1].line == 1);
    assert(diags[diag_count - 1].col == 23);
    
    cco_scope_destroy(&root); cco_arena_destroy(&arena); tests_passed++;
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


static void test_param_empty_ctor(void) {
    tests_run++; cco_parser_context_t ctx; cco_parse_options_t opts = {0};
    cco_arena_t arena; cco_arena_init(&arena, 1024);
    cco_parser_init(&ctx, "$function.@<>: ()", strlen("$function.@<>: ()"), &opts, &arena);
    cco_object_t* obj = cco_parse_function(&ctx);
    assert(ctx.last_error == CCO_OK);
    cco_arena_destroy(&arena); tests_passed++;
}

static void test_param_single_ctor(void) {
    tests_run++; cco_parser_context_t ctx; cco_parse_options_t opts = {0};
    cco_arena_t arena; cco_arena_init(&arena, 1024);
    cco_parser_init(&ctx, "$function.@<host: String>: (y: Integer = host)", strlen("$function.@<host: String>: (y: Integer = host)"), &opts, &arena);
    cco_object_t* obj = cco_parse_function(&ctx);
    assert(ctx.last_error == CCO_OK);
    cco_arena_destroy(&arena); tests_passed++;
}

static void test_param_multi_ctor(void) {
    tests_run++; cco_parser_context_t ctx; cco_parse_options_t opts = {0};
    cco_arena_t arena; cco_arena_init(&arena, 1024);
    cco_parser_init(&ctx, "$function.@<host: String, port: Integer>: ()", strlen("$function.@<host: String, port: Integer>: ()"), &opts, &arena);
    cco_object_t* obj = cco_parse_function(&ctx);
    assert(ctx.last_error == CCO_OK);
    cco_arena_destroy(&arena); tests_passed++;
}

static void test_param_static_method(void) {
    tests_run++; cco_parser_context_t ctx; cco_parse_options_t opts = {0};
    cco_arena_t arena; cco_arena_init(&arena, 1024);
    cco_parser_init(&ctx, "$function.#f<x: Integer>: Integer ()", strlen("$function.#f<x: Integer>: Integer ()"), &opts, &arena);
    cco_object_t* obj = cco_parse_function(&ctx);
    assert(ctx.last_error == CCO_OK);
    cco_arena_destroy(&arena); tests_passed++;
}

static void test_param_anon(void) {
    tests_run++; cco_parser_context_t ctx; cco_parse_options_t opts = {0};
    cco_arena_t arena; cco_arena_init(&arena, 1024);
    cco_parser_init(&ctx, "$function.@<_$0: String, _$1: Integer>: (a: String = _$0, b: Integer = _$1)", strlen("$function.@<_$0: String, _$1: Integer>: (a: String = _$0, b: Integer = _$1)"), &opts, &arena);
    cco_object_t* obj = cco_parse_function(&ctx);
    assert(ctx.last_error == CCO_OK);
    cco_arena_destroy(&arena); tests_passed++;
}

static void test_param_missing_type(void) {
    tests_run++; cco_parser_context_t ctx; cco_parse_options_t opts = {0};
    cco_arena_t arena; cco_arena_init(&arena, 1024); cco_diag_clear();
    cco_parser_init(&ctx, "$function.@<host: >: ()", strlen("$function.@<host: >: ()"), &opts, &arena);
    cco_object_t* obj = cco_parse_function(&ctx);
    assert(ctx.last_error == CCO_ERR_PARSE);
    size_t diag_count = 0;
    const cco_diag_entry_t* diags = cco_diag_get_all(&diag_count);
    assert(strstr(diags[diag_count - 1].message, "expected type after ':'") != NULL);
    cco_arena_destroy(&arena); tests_passed++;
}

static void test_param_duplicate(void) {
    tests_run++; cco_parser_context_t ctx; cco_parse_options_t opts = {0};
    cco_arena_t arena; cco_arena_init(&arena, 1024); cco_diag_clear();
    cco_parser_init(&ctx, "$function.@<a: Integer, a: String>: ()", strlen("$function.@<a: Integer, a: String>: ()"), &opts, &arena);
    cco_object_t* obj = cco_parse_function(&ctx);
    assert(ctx.last_error == CCO_ERR_PARSE);
    size_t diag_count = 0;
    const cco_diag_entry_t* diags = cco_diag_get_all(&diag_count);
    assert(strstr(diags[diag_count - 1].message, "duplicate parameter 'a'") != NULL);
    cco_arena_destroy(&arena); tests_passed++;
}


static void test_param_arith_fail(void) {
    tests_run++; cco_parser_context_t ctx; cco_parse_options_t opts = {0};
    cco_arena_t arena; cco_arena_init(&arena, 1024); cco_diag_clear();
    const char* src = "$function.#f<x: Integer>: Integer ($return<Integer>(x * x))";
    cco_parser_init(&ctx, src, strlen(src), &opts, &arena);
    cco_object_t* obj = cco_parse_function(&ctx);
    assert(ctx.last_error == CCO_ERR_PARSE);
    size_t diag_count = 0;
    const cco_diag_entry_t* diags = cco_diag_get_all(&diag_count);
    assert(strstr(diags[diag_count - 1].message, "parameter 'x' cannot be used in arithmetic at parse time") != NULL);
    cco_arena_destroy(&arena); tests_passed++;
}

static void test_param_return_pass(void) {
    tests_run++; cco_parser_context_t ctx; cco_parse_options_t opts = {0};
    cco_arena_t arena; cco_arena_init(&arena, 1024); cco_diag_clear();
    const char* src = "$function.#f<x: Integer>: Integer ($return<Integer>(x))";
    cco_parser_init(&ctx, src, strlen(src), &opts, &arena);
    cco_object_t* obj = cco_parse_function(&ctx);
    assert(ctx.last_error == CCO_OK);
    if(obj) cco_object_release(obj);
    cco_arena_destroy(&arena); tests_passed++;
}



static void test_tmpl_inst_pos_all(void) {
    tests_run++; cco_parser_context_t ctx; cco_parse_options_t opts = {0};
    cco_arena_t arena; cco_arena_init(&arena, 1024); cco_symtab_t symtab; cco_symtab_init(&symtab, &arena); cco_diag_clear();
    const char* src1 = "$temp.Point: (x: Integer, y: Integer)";
    cco_parser_init(&ctx, src1, strlen(src1), &opts, &arena);
    cco_symtab_add_template(&symtab, "Point", cco_parse_template_decl(&ctx));
    
    const char* src2 = "#Point(1, 2)";
    cco_parser_init(&ctx, src2, strlen(src2), &opts, &arena);
    ctx.symtab = &symtab;
    cco_object_t* p = parse_expr(&ctx);
    assert(p != NULL && p->type == CCO_TYPE_TEMPLATE_INSTANCE);
    assert(ctx.last_error == CCO_OK);
    cco_object_t* x = NULL; cco_object_t* y = NULL;
    for(size_t i=0; i<p->as.tmpl_inst.fields.count; ++i) {
        if(strcmp(p->as.tmpl_inst.fields.keys[i], "x") == 0) x = p->as.tmpl_inst.fields.values[i];
        if(strcmp(p->as.tmpl_inst.fields.keys[i], "y") == 0) y = p->as.tmpl_inst.fields.values[i];
    }
    assert(x && x->type == CCO_TYPE_INTEGER && x->as.integer == 1);
    assert(y && y->type == CCO_TYPE_INTEGER && y->as.integer == 2);
    
    cco_object_release(p); cco_symtab_destroy(&symtab); cco_arena_destroy(&arena); tests_passed++;
}

static void test_tmpl_inst_pos_default(void) {
    tests_run++; cco_parser_context_t ctx; cco_parse_options_t opts = {0};
    cco_arena_t arena; cco_arena_init(&arena, 1024); cco_symtab_t symtab; cco_symtab_init(&symtab, &arena); cco_diag_clear();
    const char* src1 = "$temp.Server: (host: String, port: Integer = 8080)";
    cco_parser_init(&ctx, src1, strlen(src1), &opts, &arena);
    cco_symtab_add_template(&symtab, "Server", cco_parse_template_decl(&ctx));
    
    const char* src2 = "#Server(\"localhost\")";
    cco_parser_init(&ctx, src2, strlen(src2), &opts, &arena);
    ctx.symtab = &symtab;
    cco_object_t* s = parse_expr(&ctx);
    assert(s && s->type == CCO_TYPE_TEMPLATE_INSTANCE);
    assert(ctx.last_error == CCO_OK);
    cco_object_t* port = NULL;
    for(size_t i=0; i<s->as.tmpl_inst.fields.count; ++i) {
        if(strcmp(s->as.tmpl_inst.fields.keys[i], "port") == 0) port = s->as.tmpl_inst.fields.values[i];
    }
    assert(port && port->type == CCO_TYPE_INTEGER && port->as.integer == 8080);
    
    cco_object_release(s); cco_symtab_destroy(&symtab); cco_arena_destroy(&arena); tests_passed++;
}

static void test_tmpl_inst_pos_missing(void) {
    tests_run++; cco_parser_context_t ctx; cco_parse_options_t opts = {0};
    cco_arena_t arena; cco_arena_init(&arena, 1024); cco_symtab_t symtab; cco_symtab_init(&symtab, &arena); cco_diag_clear();
    const char* src1 = "$temp.Server: (host: String, port: Integer, timeout: Integer = 30)";
    cco_parser_init(&ctx, src1, strlen(src1), &opts, &arena);
    cco_symtab_add_template(&symtab, "Server", cco_parse_template_decl(&ctx));
    
    const char* src2 = "#Server(\"localhost\")";
    cco_parser_init(&ctx, src2, strlen(src2), &opts, &arena);
    ctx.symtab = &symtab;
    cco_object_t* s = parse_expr(&ctx);
    assert(s == NULL);
    assert(ctx.last_error == CCO_ERR_PARSE);
    size_t diag_count = 0;
    const cco_diag_entry_t* diags = cco_diag_get_all(&diag_count);
    assert(strstr(diags[diag_count - 1].message, "missing field 'port'") != NULL);
    
    cco_symtab_destroy(&symtab); cco_arena_destroy(&arena); tests_passed++;
}

static void test_tmpl_inst_named_all(void) {
    tests_run++; cco_parser_context_t ctx; cco_parse_options_t opts = {0};
    cco_arena_t arena; cco_arena_init(&arena, 1024); cco_symtab_t symtab; cco_symtab_init(&symtab, &arena); cco_diag_clear();
    const char* src1 = "$temp.Server: (host: String, port: Integer = 8080)";
    cco_parser_init(&ctx, src1, strlen(src1), &opts, &arena);
    cco_symtab_add_template(&symtab, "Server", cco_parse_template_decl(&ctx));
    
    const char* src2 = "#Server(.host: \"localhost\", .port: 80)";
    cco_parser_init(&ctx, src2, strlen(src2), &opts, &arena);
    ctx.symtab = &symtab;
    cco_object_t* s = parse_expr(&ctx);
    assert(s && s->type == CCO_TYPE_TEMPLATE_INSTANCE);
    assert(ctx.last_error == CCO_OK);
    
    cco_object_release(s); cco_symtab_destroy(&symtab); cco_arena_destroy(&arena); tests_passed++;
}

static void test_tmpl_inst_named_order(void) {
    tests_run++; cco_parser_context_t ctx; cco_parse_options_t opts = {0};
    cco_arena_t arena; cco_arena_init(&arena, 1024); cco_symtab_t symtab; cco_symtab_init(&symtab, &arena); cco_diag_clear();
    const char* src1 = "$temp.Server: (host: String, port: Integer = 8080)";
    cco_parser_init(&ctx, src1, strlen(src1), &opts, &arena);
    cco_symtab_add_template(&symtab, "Server", cco_parse_template_decl(&ctx));
    
    const char* src2 = "#Server(.port: 80, .host: \"localhost\")";
    cco_parser_init(&ctx, src2, strlen(src2), &opts, &arena);
    ctx.symtab = &symtab;
    cco_object_t* s = parse_expr(&ctx);
    assert(s && s->type == CCO_TYPE_TEMPLATE_INSTANCE);
    assert(ctx.last_error == CCO_OK);
    
    cco_object_release(s); cco_symtab_destroy(&symtab); cco_arena_destroy(&arena); tests_passed++;
}

static void test_tmpl_inst_named_default(void) {
    tests_run++; cco_parser_context_t ctx; cco_parse_options_t opts = {0};
    cco_arena_t arena; cco_arena_init(&arena, 1024); cco_symtab_t symtab; cco_symtab_init(&symtab, &arena); cco_diag_clear();
    const char* src1 = "$temp.Server: (host: String, port: Integer = 8080)";
    cco_parser_init(&ctx, src1, strlen(src1), &opts, &arena);
    cco_symtab_add_template(&symtab, "Server", cco_parse_template_decl(&ctx));
    
    const char* src2 = "#Server(.host: \"localhost\")";
    cco_parser_init(&ctx, src2, strlen(src2), &opts, &arena);
    ctx.symtab = &symtab;
    cco_object_t* s = parse_expr(&ctx);
    assert(s && s->type == CCO_TYPE_TEMPLATE_INSTANCE);
    assert(ctx.last_error == CCO_OK);
    cco_object_t* port = NULL;
    for(size_t i=0; i<s->as.tmpl_inst.fields.count; ++i) {
        if(strcmp(s->as.tmpl_inst.fields.keys[i], "port") == 0) port = s->as.tmpl_inst.fields.values[i];
    }
    assert(port && port->type == CCO_TYPE_INTEGER && port->as.integer == 8080);
    
    cco_object_release(s); cco_symtab_destroy(&symtab); cco_arena_destroy(&arena); tests_passed++;
}

static void test_tmpl_inst_unknown_field(void) {
    tests_run++; cco_parser_context_t ctx; cco_parse_options_t opts = {0};
    cco_arena_t arena; cco_arena_init(&arena, 1024); cco_symtab_t symtab; cco_symtab_init(&symtab, &arena); cco_diag_clear();
    const char* src1 = "$temp.Server: (host: String, port: Integer = 8080)";
    cco_parser_init(&ctx, src1, strlen(src1), &opts, &arena);
    cco_symtab_add_template(&symtab, "Server", cco_parse_template_decl(&ctx));
    
    const char* src2 = "#Server(.host: \"x\", .foo: 1)";
    cco_parser_init(&ctx, src2, strlen(src2), &opts, &arena);
    ctx.symtab = &symtab;
    cco_object_t* s = parse_expr(&ctx);
    assert(s == NULL);
    assert(ctx.last_error == CCO_ERR_PARSE);
    size_t diag_count = 0;
    const cco_diag_entry_t* diags = cco_diag_get_all(&diag_count);
    assert(strstr(diags[diag_count - 1].message, "unknown field 'foo'") != NULL);
    
    cco_symtab_destroy(&symtab); cco_arena_destroy(&arena); tests_passed++;
}

static void test_tmpl_inst_missing_nodef(void) {
    tests_run++; cco_parser_context_t ctx; cco_parse_options_t opts = {0};
    cco_arena_t arena; cco_arena_init(&arena, 1024); cco_symtab_t symtab; cco_symtab_init(&symtab, &arena); cco_diag_clear();
    const char* src1 = "$temp.Server: (host: String, port: Integer = 8080)";
    cco_parser_init(&ctx, src1, strlen(src1), &opts, &arena);
    cco_symtab_add_template(&symtab, "Server", cco_parse_template_decl(&ctx));
    
    const char* src2 = "#Server()";
    cco_parser_init(&ctx, src2, strlen(src2), &opts, &arena);
    ctx.symtab = &symtab;
    cco_object_t* s = parse_expr(&ctx);
    assert(s == NULL);
    assert(ctx.last_error == CCO_ERR_PARSE);
    size_t diag_count = 0;
    const cco_diag_entry_t* diags = cco_diag_get_all(&diag_count);
    assert(strstr(diags[diag_count - 1].message, "missing field 'host'") != NULL);
    
    cco_symtab_destroy(&symtab); cco_arena_destroy(&arena); tests_passed++;
}

static void test_tmpl_inst_mix(void) {
    tests_run++; cco_parser_context_t ctx; cco_parse_options_t opts = {0};
    cco_arena_t arena; cco_arena_init(&arena, 1024); cco_symtab_t symtab; cco_symtab_init(&symtab, &arena); cco_diag_clear();
    const char* src1 = "$temp.Server: (host: String, port: Integer = 8080)";
    cco_parser_init(&ctx, src1, strlen(src1), &opts, &arena);
    cco_symtab_add_template(&symtab, "Server", cco_parse_template_decl(&ctx));
    
    const char* src2 = "#Server(\"localhost\", .port: 80)";
    cco_parser_init(&ctx, src2, strlen(src2), &opts, &arena);
    ctx.symtab = &symtab;
    cco_object_t* s = parse_expr(&ctx);
    assert(s == NULL);
    assert(ctx.last_error == CCO_ERR_PARSE);
    size_t diag_count = 0;
    const cco_diag_entry_t* diags = cco_diag_get_all(&diag_count);
    assert(strstr(diags[diag_count - 1].message, "cannot mix positional and named arguments") != NULL);
    
    cco_symtab_destroy(&symtab); cco_arena_destroy(&arena); tests_passed++;
}

static void test_tmpl_inst_inherit_pos(void) {
    tests_run++; cco_parser_context_t ctx; cco_parse_options_t opts = {0};
    cco_arena_t arena; cco_arena_init(&arena, 1024); cco_symtab_t symtab; cco_symtab_init(&symtab, &arena); cco_diag_clear();
    const char* src1 = "$temp.Server: (host: String, port: Integer = 8080)";
    cco_parser_init(&ctx, src1, strlen(src1), &opts, &arena);
    cco_symtab_add_template(&symtab, "Server", cco_parse_template_decl(&ctx));
    
    const char* src2 = "$temp.SecureServer + Server: (cert: String)";
    cco_parser_init(&ctx, src2, strlen(src2), &opts, &arena);
    cco_symtab_add_template(&symtab, "SecureServer", cco_parse_template_decl(&ctx));
    
    const char* src3 = "#SecureServer(\"localhost\", 8080, \"/etc/cert.pem\")";
    cco_parser_init(&ctx, src3, strlen(src3), &opts, &arena);
    ctx.symtab = &symtab;
    cco_object_t* s = parse_expr(&ctx);
    assert(s && s->type == CCO_TYPE_TEMPLATE_INSTANCE);
    assert(ctx.last_error == CCO_OK);
    
    cco_object_t* cert = NULL;
    for(size_t i=0; i<s->as.tmpl_inst.fields.count; ++i) {
        if(strcmp(s->as.tmpl_inst.fields.keys[i], "cert") == 0) cert = s->as.tmpl_inst.fields.values[i];
    }
    assert(cert != NULL);
    
    cco_object_release(s); cco_symtab_destroy(&symtab); cco_arena_destroy(&arena); tests_passed++;
}


static void test_tmpl_inst_colon(void) {
    tests_run++; cco_parser_context_t ctx; cco_parse_options_t opts = {0};
    cco_arena_t arena; cco_arena_init(&arena, 1024); cco_symtab_t symtab; cco_symtab_init(&symtab, &arena); cco_diag_clear();
    const char* src1 = "$temp.Point: (x: Integer, y: Integer)";
    cco_parser_init(&ctx, src1, strlen(src1), &opts, &arena);
    cco_symtab_add_template(&symtab, "Point", cco_parse_template_decl(&ctx));
    
    const char* src2 = "#Point: (.x: 1, .y: 2)";
    cco_parser_init(&ctx, src2, strlen(src2), &opts, &arena);
    ctx.symtab = &symtab;
    cco_object_t* p = parse_expr(&ctx);
    assert(p != NULL && p->type == CCO_TYPE_TEMPLATE_INSTANCE);
    assert(ctx.last_error == CCO_OK);
    cco_object_t* x = NULL;
    for(size_t i=0; i<p->as.tmpl_inst.fields.count; ++i) {
        if(strcmp(p->as.tmpl_inst.fields.keys[i], "x") == 0) x = p->as.tmpl_inst.fields.values[i];
    }
    assert(x && x->type == CCO_TYPE_INTEGER && x->as.integer == 1);
    
    cco_object_release(p); cco_symtab_destroy(&symtab); cco_arena_destroy(&arena); tests_passed++;
}

static void test_tmpl_inst_type_fail(void) {
    tests_run++; cco_parser_context_t ctx; cco_parse_options_t opts = {0};
    cco_arena_t arena; cco_arena_init(&arena, 1024); cco_symtab_t symtab; cco_symtab_init(&symtab, &arena); cco_diag_clear();
    const char* src1 = "$temp.Server: (host: String, port: Integer = 8080)";
    cco_parser_init(&ctx, src1, strlen(src1), &opts, &arena);
    cco_symtab_add_template(&symtab, "Server", cco_parse_template_decl(&ctx));
    
    const char* src2 = "#Server(1234)";
    cco_parser_init(&ctx, src2, strlen(src2), &opts, &arena);
    ctx.symtab = &symtab;
    cco_object_t* s = parse_expr(&ctx);
    assert(s == NULL);
    assert(ctx.last_error == CCO_ERR_PARSE);
    size_t diag_count = 0;
    const cco_diag_entry_t* diags = cco_diag_get_all(&diag_count);
    assert(strstr(diags[diag_count - 1].message, "type mismatch for field 'host': expected 'String', got 'Integer'") != NULL);
    
    cco_symtab_destroy(&symtab); cco_arena_destroy(&arena); tests_passed++;
}

static void test_symtab_leak(void) {
    tests_run++; cco_parser_context_t ctx; cco_parse_options_t opts = {0};
    cco_arena_t arena; cco_arena_init(&arena, 1024); cco_symtab_t symtab; cco_symtab_init(&symtab, &arena);
    const char* src = "$temp.Point: (x: Integer = 0, y: Integer = 0)";
    cco_parser_init(&ctx, src, strlen(src), &opts, &arena);
    cco_template_def_t* def = cco_parse_template_decl(&ctx);
    cco_symtab_add_template(&symtab, "Point", def);
    cco_symtab_destroy(&symtab); cco_arena_destroy(&arena); tests_passed++;
}

static void test_param_no_leak(void) {
    tests_run++; cco_parser_context_t ctx; cco_parse_options_t opts = {0};
    cco_arena_t arena; cco_arena_init(&arena, 1024);
    cco_scope_t root; cco_scope_init(&root, NULL);
    
    cco_parser_init(&ctx, "$function.@<a: Integer>: ()", strlen("$function.@<a: Integer>: ()"), &opts, &arena);
    ctx.scope = &root;
    cco_parse_function(&ctx);
    
    cco_parser_init(&ctx, "$function.@<>: (x: Integer = a)", strlen("$function.@<>: (x: Integer = a)"), &opts, &arena);
    ctx.scope = &root;
    cco_parse_function(&ctx);
    assert(ctx.last_error == CCO_ERR_PARSE);
    
    cco_scope_destroy(&root);
    cco_arena_destroy(&arena); tests_passed++;
}


static void test_template_simple(void) {
    tests_run++; cco_parser_context_t ctx; cco_parse_options_t opts = {0};
    cco_arena_t arena; cco_arena_init(&arena, 1024);
    const char* src = "$temp.Point: (x: Integer, y: Integer)";
    cco_parser_init(&ctx, src, strlen(src), &opts, &arena);
    cco_template_def_t* def = cco_parse_template_decl(&ctx);
    assert(def != NULL);
    assert(def->field_count == 2);
    assert(strcmp(def->fields[0].name, "x") == 0);
    assert(strcmp(def->fields[0].type_name, "Integer") == 0);
    assert(def->fields[0].default_value == NULL);
    for(size_t i=0; i<def->field_count; ++i) {
        free(def->fields[i].name); if(def->fields[i].type_name) free(def->fields[i].type_name);
        if(def->fields[i].default_value) cco_object_release(def->fields[i].default_value);
    }
    free(def->fields); free(def); cco_arena_destroy(&arena); tests_passed++;
}

static void test_template_default_val(void) {
    tests_run++; cco_parser_context_t ctx; cco_parse_options_t opts = {0};
    cco_arena_t arena; cco_arena_init(&arena, 1024);
    const char* src = "$temp.Point: (x: Integer = 0, y: Integer = 0)";
    cco_parser_init(&ctx, src, strlen(src), &opts, &arena);
    cco_template_def_t* def = cco_parse_template_decl(&ctx);
    assert(def != NULL);
    assert(def->field_count == 2);
    assert(strcmp(def->fields[0].type_name, "Integer") == 0);
    assert(def->fields[0].default_value != NULL);
    assert(def->fields[0].default_value->type == CCO_TYPE_INTEGER);
    assert(def->fields[0].default_value->as.integer == 0);
    for(size_t i=0; i<def->field_count; ++i) {
        free(def->fields[i].name); if(def->fields[i].type_name) free(def->fields[i].type_name);
        if(def->fields[i].default_value) cco_object_release(def->fields[i].default_value);
    }
    free(def->fields); free(def); cco_arena_destroy(&arena); tests_passed++;
}

static void test_template_no_type_default(void) {
    tests_run++; cco_parser_context_t ctx; cco_parse_options_t opts = {0};
    cco_arena_t arena; cco_arena_init(&arena, 1024);
    const char* src = "$temp.Foo: (bar: \"hi\")";
    cco_parser_init(&ctx, src, strlen(src), &opts, &arena);
    cco_template_def_t* def = cco_parse_template_decl(&ctx);
    assert(def != NULL);
    assert(def->field_count == 1);
    assert(strcmp(def->fields[0].name, "bar") == 0);
    assert(def->fields[0].type_name == NULL);
    assert(def->fields[0].default_value != NULL);
    for(size_t i=0; i<def->field_count; ++i) {
        free(def->fields[i].name); if(def->fields[i].type_name) free(def->fields[i].type_name);
        if(def->fields[i].default_value) cco_object_release(def->fields[i].default_value);
    }
    free(def->fields); free(def); cco_arena_destroy(&arena); tests_passed++;
}

static void test_template_empty(void) {
    tests_run++; cco_parser_context_t ctx; cco_parse_options_t opts = {0};
    cco_arena_t arena; cco_arena_init(&arena, 1024);
    const char* src = "$temp.Empty: ()";
    cco_parser_init(&ctx, src, strlen(src), &opts, &arena);
    cco_template_def_t* def = cco_parse_template_decl(&ctx);
    assert(def != NULL);
    assert(def->field_count == 0);
    for(size_t i=0; i<def->field_count; ++i) {
        free(def->fields[i].name); if(def->fields[i].type_name) free(def->fields[i].type_name);
        if(def->fields[i].default_value) cco_object_release(def->fields[i].default_value);
    }
    free(def->fields); free(def); cco_arena_destroy(&arena); tests_passed++;
}

static void test_template_dotted_name(void) {
    tests_run++; cco_parser_context_t ctx; cco_parse_options_t opts = {0};
    cco_arena_t arena; cco_arena_init(&arena, 1024); cco_symtab_t symtab; cco_symtab_init(&symtab, &arena);
    const char* src = "$temp.A.B: (x: Integer)";
    cco_parser_init(&ctx, src, strlen(src), &opts, &arena);
    cco_template_def_t* def = cco_parse_template_decl(&ctx);
    assert(def != NULL);
    cco_symtab_add_template(&symtab, "A.B", def);
    assert(cco_symtab_lookup_template(&symtab, "A.B") != NULL);
    for(size_t i=0; i<def->field_count; ++i) {
        free(def->fields[i].name); if(def->fields[i].type_name) free(def->fields[i].type_name);
        if(def->fields[i].default_value) cco_object_release(def->fields[i].default_value);
    }
    free(def->fields); free(def); free(symtab.templates.entries); cco_intern_table_destroy(&symtab.aliases); cco_arena_destroy(&arena); tests_passed++;
}

static void test_template_inheritance(void) {
    tests_run++; cco_parser_context_t ctx; cco_parse_options_t opts = {0};
    cco_arena_t arena; cco_arena_init(&arena, 1024); cco_symtab_t symtab; cco_symtab_init(&symtab, &arena);
    const char* src1 = "$temp.Base: (a: Integer)";
    cco_parser_init(&ctx, src1, strlen(src1), &opts, &arena);
    cco_template_def_t* def1 = cco_parse_template_decl(&ctx);
    cco_symtab_add_template(&symtab, "Base", def1);
    
    const char* src2 = "$temp.Child + Base: (b: String)";
    cco_parser_init(&ctx, src2, strlen(src2), &opts, &arena);
    cco_template_def_t* def2 = cco_parse_template_decl(&ctx);
    cco_symtab_add_template(&symtab, "Child", def2);
    
    assert(def2->field_count == 2);
    assert(strcmp(def2->fields[0].name, "a") == 0);
    assert(strcmp(def2->fields[1].name, "b") == 0);
    
    for(size_t i=0; i<def1->field_count; ++i) {
        free(def1->fields[i].name); if(def1->fields[i].type_name) free(def1->fields[i].type_name);
        if(def1->fields[i].default_value) cco_object_release(def1->fields[i].default_value);
    }
    free(def1->fields); free(def1);
    for(size_t i=0; i<def2->field_count; ++i) {
        free(def2->fields[i].name); if(def2->fields[i].type_name) free(def2->fields[i].type_name);
        if(def2->fields[i].default_value) cco_object_release(def2->fields[i].default_value);
    }
    free(def2->fields); free((void*)def2->parent_name); free(def2); free(symtab.templates.entries); cco_intern_table_destroy(&symtab.aliases); cco_arena_destroy(&arena); tests_passed++;
}

static void test_template_inheritance_override(void) {
    tests_run++; cco_parser_context_t ctx; cco_parse_options_t opts = {0};
    cco_arena_t arena; cco_arena_init(&arena, 1024); cco_symtab_t symtab; cco_symtab_init(&symtab, &arena);
    const char* src1 = "$temp.Base: (a: Integer)";
    cco_parser_init(&ctx, src1, strlen(src1), &opts, &arena);
    cco_template_def_t* def1 = cco_parse_template_decl(&ctx);
    cco_symtab_add_template(&symtab, "Base", def1);
    
    const char* src2 = "$temp.Child + Base: (a: String)";
    cco_parser_init(&ctx, src2, strlen(src2), &opts, &arena);
    cco_template_def_t* def2 = cco_parse_template_decl(&ctx);
    cco_symtab_add_template(&symtab, "Child", def2);
    
    assert(def2->field_count == 1);
    assert(strcmp(def2->fields[0].name, "a") == 0);
    assert(strcmp(def2->fields[0].type_name, "String") == 0);
    
    for(size_t i=0; i<def1->field_count; ++i) {
        free(def1->fields[i].name); if(def1->fields[i].type_name) free(def1->fields[i].type_name);
        if(def1->fields[i].default_value) cco_object_release(def1->fields[i].default_value);
    }
    free(def1->fields); free(def1);
    for(size_t i=0; i<def2->field_count; ++i) {
        free(def2->fields[i].name); if(def2->fields[i].type_name) free(def2->fields[i].type_name);
        if(def2->fields[i].default_value) cco_object_release(def2->fields[i].default_value);
    }
    free(def2->fields); free((void*)def2->parent_name); free(def2); free(symtab.templates.entries); cco_intern_table_destroy(&symtab.aliases); cco_arena_destroy(&arena); tests_passed++;
}

static void test_template_unknown_parent(void) {
    tests_run++; cco_parser_context_t ctx; cco_parse_options_t opts = {0};
    cco_arena_t arena; cco_arena_init(&arena, 1024); cco_symtab_t symtab; cco_symtab_init(&symtab, &arena); cco_diag_clear();
    const char* src = "$temp.Child + Ghost: (x: Integer)";
    cco_parser_init(&ctx, src, strlen(src), &opts, &arena);
    cco_template_def_t* def = cco_parse_template_decl(&ctx);
    cco_error_t err = cco_symtab_add_template(&symtab, "Child", def);
    assert(err == CCO_ERR_PARSE);
    size_t diag_count = 0;
    const cco_diag_entry_t* diags = cco_diag_get_all(&diag_count);
    assert(strstr(diags[diag_count - 1].message, "unknown parent 'Ghost'") != NULL);
    
    for(size_t i=0; i<def->field_count; ++i) {
        free(def->fields[i].name); if(def->fields[i].type_name) free(def->fields[i].type_name);
        if(def->fields[i].default_value) cco_object_release(def->fields[i].default_value);
    }
    free(def->fields); free((void*)def->parent_name); free(def); free(symtab.templates.entries); cco_intern_table_destroy(&symtab.aliases); cco_arena_destroy(&arena); tests_passed++;
}

static void test_expr_comparison(void) {
    tests_run++;
    cco_parser_context_t ctx; cco_parse_options_t opts = {0};
    cco_arena_t arena; cco_arena_init(&arena, 1024);
    cco_scope_t root; cco_scope_init(&root, NULL);
    
    cco_parser_init(&ctx, "x: Boolean = $(10 < 20)", strlen("x: Boolean = $(10 < 20)"), &opts, &arena);
    ctx.scope = &root; cco_src_loc_t loc = {0};
    assert(cco_parse_bind(&ctx, &loc) == PARSE_BIND_OK);
    cco_object_t* val = cco_scope_lookup(&root, "x");
    assert(val != NULL); assert(val->type == CCO_TYPE_BOOLEAN); assert(val->as.boolean == true);
    
    cco_scope_destroy(&root); cco_arena_destroy(&arena); tests_passed++;
}

static void test_expr_logical(void) {
    tests_run++;
    cco_parser_context_t ctx; cco_parse_options_t opts = {0};
    cco_arena_t arena; cco_arena_init(&arena, 1024);
    cco_scope_t root; cco_scope_init(&root, NULL);
    
    cco_parser_init(&ctx, "x: Boolean = $(true && false)", strlen("x: Boolean = $(true && false)"), &opts, &arena);
    ctx.scope = &root; cco_src_loc_t loc = {0};
    assert(cco_parse_bind(&ctx, &loc) == PARSE_BIND_OK);
    cco_object_t* val = cco_scope_lookup(&root, "x");
    assert(val != NULL); assert(val->type == CCO_TYPE_BOOLEAN); assert(val->as.boolean == false);
    
    cco_scope_destroy(&root); cco_arena_destroy(&arena); tests_passed++;
}

static void test_expr_string_concat(void) {
    tests_run++;
    cco_parser_context_t ctx; cco_parse_options_t opts = {0};
    cco_arena_t arena; cco_arena_init(&arena, 1024);
    cco_scope_t root; cco_scope_init(&root, NULL);
    
    cco_parser_init(&ctx, "x: String = $(\"hello\" + \" world\")", strlen("x: String = $(\"hello\" + \" world\")"), &opts, &arena);
    ctx.scope = &root; cco_src_loc_t loc = {0};
    assert(cco_parse_bind(&ctx, &loc) == PARSE_BIND_OK);
    cco_object_t* val = cco_scope_lookup(&root, "x");
    assert(val != NULL); assert(val->type == CCO_TYPE_STRING); assert(strcmp(val->as.string.data, "hello world") == 0);
    
    cco_scope_destroy(&root); cco_arena_destroy(&arena); tests_passed++;
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
    test_param_empty_ctor();
    test_param_single_ctor();
    test_param_multi_ctor();
    test_param_static_method();
    test_param_anon();
    test_param_missing_type();
    test_param_duplicate();
    test_param_arith_fail();
    test_param_return_pass();
    test_param_no_leak();
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
    test_expr_arith_nested();
    test_expr_bare_ok();
    test_expr_missing_rparen();
    test_unary_minus();
    test_scope_memory_leak();
    test_template_simple();
    test_template_default_val();
    test_template_no_type_default();
    test_template_empty();
    test_template_dotted_name();
    test_template_inheritance();
    test_template_inheritance_override();
    test_template_unknown_parent();
    test_tmpl_inst_pos_all();
    test_tmpl_inst_pos_default();
    test_tmpl_inst_pos_missing();
    test_tmpl_inst_named_all();
    test_tmpl_inst_named_order();
    test_tmpl_inst_named_default();
    test_tmpl_inst_unknown_field();
    test_tmpl_inst_missing_nodef();
    test_tmpl_inst_mix();
    test_tmpl_inst_inherit_pos();
    test_tmpl_inst_colon();
    test_tmpl_inst_type_fail();
    test_expr_comparison();
    test_expr_logical();
    test_expr_string_concat();
}
