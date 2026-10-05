#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "cnt/cco.h"

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

int main(int argc, char** argv) {
    (void)argc; (void)argv;
    
    printf("== libcco Performance Benchmark ==\n");
    
    size_t payload_len = strlen(PAYLOAD);
    printf("Payload size: %zu bytes\n", payload_len);
    
    cco_parse_options_t opts;
    cco_parse_options_init(&opts);
    
    int iterations = 100000;
    
    cco_object_t* obj = cco_parse_string(PAYLOAD, payload_len, &opts);
    if (!obj) {
        fprintf(stderr, "Parsing failed initially\n");
        return 1;
    }
    cco_object_release(obj);
    
    printf("Running %d iterations...\n", iterations);
    double start_time = get_time_sec();
    
    for (int i = 0; i < iterations; i++) {
        cco_object_t* obj = cco_parse_string(PAYLOAD, payload_len, &opts);
        if (!obj) return 1;
        cco_object_release(obj);
    }
    
    double end_time = get_time_sec();
    double total_time = end_time - start_time;
    
    double ops_per_sec = iterations / total_time;
    double mb_per_sec = (payload_len * iterations) / (1024.0 * 1024.0 * total_time);
    double latency_us = (total_time / iterations) * 1e6;
    
    printf("\n== Results ==\n");
    printf("Total time:       %.4f s\n", total_time);
    printf("Throughput:       %.2f ops/sec\n", ops_per_sec);
    printf("Bandwidth:        %.2f MB/sec\n", mb_per_sec);
    printf("Average latency:  %.4f us/op\n", latency_us);
    
    return 0;
}
