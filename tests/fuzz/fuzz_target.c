#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <cnt/cco.h>

/* LLVM libFuzzer entry point */
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    /* Set up strict limits for fuzzing so we don't OOM or timeout intentionally */
    cco_parse_options_t opts;
    cco_parse_options_init(&opts);
    
    opts.max_depth = 128;
    opts.max_document_size = 1024 * 1024;
    opts.max_total_allocation = 10 * 1024 * 1024;
    opts.max_steps = 50000;
    
    /* Ensure the input is null-terminated for any internal functions that might need it (though cco shouldn't) */
    char* buf = (char*)malloc(size + 1);
    if (!buf) return 0;
    
    memcpy(buf, data, size);
    buf[size] = '\0';
    
    /* Parse the fuzzed document */
    cco_object_t* obj = cco_parse_string(buf, size, &opts);
    
    /* If parsing was successful, we should be able to serialize it too */
    if (obj) {
        char* ser = cco_serialize_to_string(obj, false);
        if (ser) free(ser);
        cco_object_release(obj);
    }
    
    free(buf);
    return 0;
}
