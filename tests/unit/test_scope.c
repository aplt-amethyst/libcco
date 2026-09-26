#include <stdio.h>
#include <string.h>
#include <assert.h>
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
    test_scope_memory_leak();
}
