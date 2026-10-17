#include <stdio.h>
#include <stdlib.h>
#include "../test.h"
#include <string.h>
#include <cnt/cco.h>
#include "../../src/internal/parser.h"

void test_expr(void) {
    /* Test basic expressions via cco_parse_string */
    const char* src = "a: $(10 + 20 * 3), b: $(true && false), c: $(10 ?? 20), d: $(10 > 5 ? 100 : 200), e: $include(\"dummy\")";
    cco_parse_options_t opts = {0};
    cco_object_t* obj = cco_parse_string(src, strlen(src), &opts);
    if (!obj) {
        printf("Parse failed\n");
        return;
    }
    
    cco_object_t* a = cco_map_get_by_key(obj, "a");
    if (!a || a->type != CCO_TYPE_INTEGER || a->as.integer != 70) {
        printf("Test a failed: %lld\n", (long long)(a ? a->as.integer : -1));
        return;
    }
    
    cco_object_t* b = cco_map_get_by_key(obj, "b");
    if (!b || b->type != CCO_TYPE_BOOLEAN || b->as.boolean != false) {
        printf("Test b failed\n");
        return;
    }

    cco_object_t* d = cco_map_get_by_key(obj, "d");
    if (!d || d->type != CCO_TYPE_INTEGER || d->as.integer != 100) {
        printf("Test d failed\n");
        return;
    }

    cco_object_release(obj);
    tests_run++; tests_passed++;
    return;
}
