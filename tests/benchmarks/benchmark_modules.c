#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "cnt/cco.h"
#include "lexer.h"
#include "object.h"

const char* PAYLOAD = 
"project: {\n"
"    name: \"libcco_benchmark\",\n"
"    version: \"1.0.0\",\n"
"    description: \"A benchmark test for libcco parser\",\n"
"    features: [\"fast\", \"safe\", \"flexible\"],\n"
"    dependencies: {\n"
"        libc: \">=2.28\",\n"
"        cnt: \"latest\"\n"
"    },\n"
"    flags: $( $(1 << 3) | $(1 << 5) | $(1 << 1) ),\n"
"    range_test: $( 1..100 ),\n"
"    boolean: true,\n"
"    nested: {\n"
"        a: {\n"
"            b: {\n"
"                c: $( 5 * 10 + 20 / 4 - 3 )\n"
"            }\n"
"        }\n"
"    },\n"
"    interpolation: \"Result is $(5 * 5)\"\n"
"}\n";

double get_time_sec() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

void bench_lexer(int iterations) {
    size_t len = strlen(PAYLOAD);
    cco_parse_options_t opts;
    cco_parse_options_init(&opts);
    
    cco_arena_t arena;
    
    double start = get_time_sec();
    size_t tokens_count = 0;
    
    for (int i = 0; i < iterations; i++) {
        cco_arena_init(&arena, 0);
        cco_lexer_t lex;
        cco_lexer_init(&lex, PAYLOAD, len, &opts, &arena);
        cco_token_t tok;
        do {
            cco_lexer_next(&lex, &tok);
            tokens_count++;
        } while (tok.type != CCO_TOK_EOF);
        cco_arena_destroy(&arena);
    }
    
    double end = get_time_sec();
    double t = end - start;
    printf("  Lexer only:         %.4f s (%.2f MB/s, %.2fM tokens/s)\n", t, (len * iterations)/(1024.0*1024.0*t), (tokens_count/1e6)/t);
}

void bench_parser_full(int iterations) {
    size_t len = strlen(PAYLOAD);
    cco_parse_options_t opts;
    cco_parse_options_init(&opts);
    
    double start = get_time_sec();
    
    for (int i = 0; i < iterations; i++) {
        cco_object_t* obj = cco_parse_string(PAYLOAD, len, &opts);
        if (obj) cco_object_release(obj);
    }
    
    double end = get_time_sec();
    double t = end - start;
    printf("  Full parse (AST):   %.4f s (%.2f MB/s, %.2f us/op)\n", t, (len * iterations)/(1024.0*1024.0*t), (t/iterations)*1e6);
}

void bench_arena(int iterations) {
    double start = get_time_sec();
    for (int i = 0; i < iterations; i++) {
        cco_arena_t arena;
        cco_arena_init(&arena, 0);
        for (int j = 0; j < 100; j++) {
            void* p = cco_arena_alloc(&arena, 64);
            (void)p;
        }
        cco_arena_destroy(&arena);
    }
    double end = get_time_sec();
    double t = end - start;
    printf("  Arena Alloc (x100): %.4f s (%.2f M allocs/s)\n", t, (iterations * 100 / 1e6) / t);
}

void bench_object_map(int iterations) {
    double start = get_time_sec();
    for (int i = 0; i < iterations; i++) {
        cco_object_t* map = cco_object_create(CCO_TYPE_MAP);
        for (int j = 0; j < 50; j++) {
            char key[32];
            snprintf(key, sizeof(key), "key_%d", j);
            cco_object_t* val = cco_object_create(CCO_TYPE_INTEGER);
            val->as.integer = j;
            cco_map_insert(map, key, val);
            cco_object_release(val);
        }
        cco_object_release(map);
    }
    double end = get_time_sec();
    double t = end - start;
    printf("  Map Insert (x50):   %.4f s (%.2f M inserts/s)\n", t, (iterations * 50 / 1e6) / t);
}

int main() {
    int iters_parse = 100000;
    int iters_ops = 500000;
    
    printf("== libcco Modules Performance Benchmark ==\n\n");
    
    printf("[1] Core Parsing (Lexer vs Parser/AST)\n");
    bench_lexer(iters_parse);
    bench_parser_full(iters_parse);
    
    printf("\n[2] Object & Memory Management\n");
    bench_arena(iters_ops);
    bench_object_map(iters_ops);
    
    return 0;
}
