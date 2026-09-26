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
    assert(cco_scope_bind(&child, "inner", dup) == CCO_ERR_INVALID_ARG);
    cco_object_release(dup); /* Since it failed, we must release it */
    
    cco_scope_destroy(&child);
    cco_scope_destroy(&root);
    
    tests_passed++;
}

static void test_parse_bind(void) {
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
    
    cco_parse_bind_result_t result = cco_parse_bind(&ctx);
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

static void test_parse_static_method_body(void) {
    tests_run++;
    /* Test calling the method body with a valid bind and return */
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
    assert(obj != NULL);
    assert(obj->type == CCO_TYPE_INTEGER);
    assert(obj->as.integer == 10);
    cco_object_release(obj);
    
    /* x should NOT be in root, because static method body pushes its own scope */
    assert(cco_scope_lookup(&root, "x") == NULL);
    
    cco_scope_destroy(&root);
    cco_arena_destroy(&arena);
    tests_passed++;
}

static void test_parse_bind_error(void) {
    tests_run++;
    
    /* Using the method body to trigger the exact error reporting logic */
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
    /* The error location should be at ')' which is where '=' should have been. 
       In "(x: Integer)", '(' is at col 1, 'x' at col 2, ':' at col 3, 'Integer' at col 5, ')' at col 12. */
    
    cco_scope_destroy(&root);
    cco_arena_destroy(&arena);
    
    tests_passed++;
}

void test_scope_all(void) {
    test_scope_basic();
    test_parse_bind();
    test_parse_static_method_body();
    test_parse_bind_error();
}
