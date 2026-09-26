#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../../src/internal/parser.h"

extern int tests_run;
extern int tests_passed;

extern bool cco_parse_bind(cco_parser_context_t* ctx);

static void test_scope_basic(void) {
    tests_run++;
    
    cco_scope_t root, child;
    cco_scope_init(&root, NULL);
    cco_scope_init(&child, &root);
    
    cco_object_t* v1 = cco_object_create(CCO_TYPE_INTEGER);
    v1->as.integer = 100;
    cco_scope_bind(&root, "outer", v1);
    
    cco_object_t* v2 = cco_object_create(CCO_TYPE_INTEGER);
    v2->as.integer = 200;
    cco_scope_bind(&child, "inner", v2);
    
    cco_object_t* found = cco_scope_lookup(&child, "outer");
    assert(found && found->as.integer == 100);
    
    cco_object_t* v3 = cco_object_create(CCO_TYPE_INTEGER);
    v3->as.integer = 999;
    cco_scope_bind(&child, "outer", v3);
    found = cco_scope_lookup(&child, "outer");
    assert(found && found->as.integer == 999);
    
    found = cco_scope_lookup(&root, "outer");
    assert(found && found->as.integer == 100);
    
    cco_object_release(v1);
    cco_object_release(v2);
    cco_object_release(v3);
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
    
    bool result = cco_parse_bind(&ctx);
    assert(result == true);
    assert(ctx.last_error == CCO_OK);
    
    cco_object_t* val = cco_scope_lookup(&root, "x");
    assert(val != NULL);
    assert(val->type == CCO_TYPE_INTEGER);
    assert(val->as.integer == 5);
    
    cco_scope_destroy(&root);
    cco_arena_destroy(&arena);
    
    tests_passed++;
}

static void test_parse_bind_error(void) {
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
    
    bool result = cco_parse_bind(&ctx);
    /* Should return false and record error */
    assert(result == false);
    assert(ctx.last_error == CCO_ERR_PARSE);
    
    cco_scope_destroy(&root);
    cco_arena_destroy(&arena);
    
    tests_passed++;
}

void test_scope_all(void) {
    test_scope_basic();
    test_parse_bind();
    test_parse_bind_error();
}
