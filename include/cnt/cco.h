#ifndef CNT_CCO_H
#define CNT_CCO_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* Include export macros, error codes and options */
#include "cnt/cco_export.h"
#include <cnt/cco_error.h>
#include <cnt/cco_options.h>

    /* Opaque handles for internal types */
    typedef struct cco_object_s cco_object_t;
    typedef struct cco_parser_context_s cco_parser_context_t;
    typedef struct cco_arena_s cco_arena_t;

    /* --- Lifecycle APIs --- */

    /* Parses a CCO document from a string.
       Caller owns the returned object and MUST call cco_object_release() on it
       when done. If an error occurs, returns NULL and the specific error code
       can be fetched via diagnostic APIs. */
    CCO_API cco_object_t* cco_parse_string(const char* src, size_t len,
                                           const cco_parse_options_t* opts);

    /* Serializes a CCO object into a newly allocated C-string.
       Caller MUST free() the returned string. Returns NULL on failure. */
    CCO_API char* cco_serialize_to_string(const cco_object_t* obj, bool pretty);

    /* --- Memory Pool (Arena) API for Extreme Performance --- */
    
    /* Creates a new reusable memory pool. max_limit is max bytes, or SIZE_MAX for unlimited. */
    CCO_API cco_arena_t* cco_arena_new(size_t max_limit);
    
    /* Resets the memory pool pointers for reuse, achieving Zero Heap Allocation across parses. */
    CCO_API void cco_arena_reset(cco_arena_t* arena);
    
    /* Frees the memory pool and all associated memory. */
    CCO_API void cco_arena_free(cco_arena_t* arena);


/* Object Types */
#define CCO_TYPE_NONE 0
#define CCO_TYPE_BOOLEAN 1
#define CCO_TYPE_INTEGER 2
#define CCO_TYPE_FLOAT 3
#define CCO_TYPE_STRING 4
#define CCO_TYPE_ARRAY 5
#define CCO_TYPE_MAP 6
#define CCO_TYPE_TEMPLATE_INSTANCE 7
#define CCO_TYPE_PARAM 8

    /* --- Object Model APIs --- */

    /* Retrieves the type of the given object. */
    CCO_API int cco_object_get_type(const cco_object_t* obj);

    /* Increments the reference count of the object. */
    CCO_API void cco_object_retain(cco_object_t* obj);

    /* Decrements the reference count of the object. Frees memory if count
     * reaches zero. */
    CCO_API void cco_object_release(cco_object_t* obj);

    /* Returns true if object is a boolean, and sets *out_val. */
    CCO_API bool cco_object_get_boolean(const cco_object_t* obj, bool* out_val);

    /* Returns true if object is an integer, and sets *out_val. */
    CCO_API bool cco_object_get_integer(const cco_object_t* obj, int64_t* out_val);

    /* Returns true if object is a float, and sets *out_val. */
    CCO_API bool cco_object_get_float(const cco_object_t* obj, double* out_val);

    /* Returns true if object is a string. Returns the string length and sets
       *out_val to a pointer to the string data. The returned pointer is valid
       as long as the object is alive. */
    CCO_API bool cco_object_get_string(const cco_object_t* obj, const char** out_val,
                                       size_t* out_len);

    /* Array getters */
    CCO_API size_t cco_array_get_count(const cco_object_t* obj);
    CCO_API cco_object_t* cco_array_get_item(const cco_object_t* obj, size_t index);

    /* Map getters */
    CCO_API size_t cco_map_get_count(const cco_object_t* obj);
    CCO_API const char* cco_map_get_key(const cco_object_t* obj, size_t index);
    CCO_API cco_object_t* cco_map_get_value(const cco_object_t* obj, size_t index);
    CCO_API cco_object_t* cco_map_get_by_key(const cco_object_t* obj, const char* key);

    /* --- Diagnostic APIs --- */

    /* Retrieves the first error encountered during the last parse operation on
     * this thread. */
    CCO_API cco_error_t cco_get_last_error(void);

    /* Clears thread-local diagnostics. */
    CCO_API void cco_clear_diagnostics(void);

#ifdef __cplusplus
}
#endif

#endif /* CNT_CCO_H */
