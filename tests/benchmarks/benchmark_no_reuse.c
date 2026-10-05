#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "cnt/cco.h"
#include "cnt/cco_options.h"

double get_time_sec() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

int main() {
    size_t num_parses = 1000000;
    const char* json = "{\"name\":\"benchmark\",\"data\":[1,2,3,4,5],\"nested\":{\"k\":\"v\"}}";
    size_t len = strlen(json);

    cco_parse_options_t opts;
    cco_parse_options_init(&opts);
    opts.fast_mode = true;
    opts.zero_copy_dom = true;
    
    printf("Starting %zu parses WITHOUT memory reuse...\n", num_parses);
    double start = get_time_sec();
    for (size_t i = 0; i < num_parses; i++) {
        cco_object_t* obj = cco_parse_string(json, len, &opts);
        if (!obj) {
            printf("Parse failed at %zu\n", i);
            break;
        }
        cco_object_release(obj);
    }
    double end = get_time_sec();

    printf("Finished %zu parses in %.6f seconds.\n", num_parses, end - start);
    return 0;
}
