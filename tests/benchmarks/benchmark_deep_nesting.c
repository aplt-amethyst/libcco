#define _POSIX_C_SOURCE 200809L
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
    size_t num_items = 500000;
    size_t buffer_size = 200 * 1024 * 1024;
    char* buf = malloc(buffer_size);
    char* ptr = buf;

    strcpy(buf, "root_array: (\n");
    ptr += strlen(buf);

    for (size_t i = 0; i < num_items; i++) {
        ptr += sprintf(ptr, "(a:(a:(a:(a:(a:(a:(a:(a:(a:(a:(a:(a:(a:(a:(a:(a:(a:(a:(a:(a:1)))))))))))))))))))),\n");
    }
    // Remove the trailing comma
    *(ptr - 2) = '\n';
    *(ptr - 1) = ')';
    *ptr = '\0';
    size_t total_len = ptr - buf;

    cco_parse_options_t opts;
    cco_parse_options_init(&opts);
    opts.max_document_size = SIZE_MAX;
    opts.max_total_allocation = SIZE_MAX;
    opts.max_steps = SIZE_MAX;
    opts.max_instantiations = SIZE_MAX;

    printf("Starting parse of 10 million objects (in array)...\n");
    double start = get_time_sec();
    cco_object_t* obj = cco_parse_string(buf, total_len, &opts);
    double end = get_time_sec();

    if (obj) {
        printf("Successfully parsed 10 million objects in %.4f seconds.\n", end - start);
        printf("Throughput: %.2f MB/s\n", (total_len / (1024.0 * 1024.0)) / (end - start));
        
        printf("Releasing memory...\n");
        double start_free = get_time_sec();
        cco_object_release(obj);
        double end_free = get_time_sec();
        printf("Memory released in %.4f seconds.\n", end_free - start_free);
    } else {
        printf("Failed to parse!\n");
    }

    free(buf);
    return 0;
}
