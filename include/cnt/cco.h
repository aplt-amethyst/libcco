#ifndef CNT_CCO_H
#define CNT_CCO_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* Include error codes and options */
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
    cco_object_t* cco_parse_string(const char* src, size_t len,
                                   const cco_parse_options_t* opts);

    /* Serializes a CCO object into a newly allocated C-string.
       Caller MUST free() the returned string. Returns NULL on failure. */
    char* cco_serialize_to_string(const cco_object_t* obj, bool pretty);

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
    int cco_object_get_type(const cco_object_t* obj);

    /* Increments the reference count of the object. */
    void cco_object_retain(cco_object_t* obj);

    /* Decrements the reference count of the object. Frees memory if count
     * reaches zero. */
    void cco_object_release(cco_object_t* obj);

    /* Returns true if object is a boolean, and sets *out_val. */
    bool cco_object_get_boolean(const cco_object_t* obj, bool* out_val);

    /* Returns true if object is an integer, and sets *out_val. */
    bool cco_object_get_integer(const cco_object_t* obj, int64_t* out_val);

    /* Returns true if object is a float, and sets *out_val. */
    bool cco_object_get_float(const cco_object_t* obj, double* out_val);

    /* Returns true if object is a string. Returns the string length and sets
       *out_val to a pointer to the string data. The returned pointer is valid
       as long as the object is alive. */
    bool cco_object_get_string(const cco_object_t* obj, const char** out_val,
                               size_t* out_len);

    /* Array getters */
    size_t cco_array_get_count(const cco_object_t* obj);
    cco_object_t* cco_array_get_item(const cco_object_t* obj, size_t index);

    /* Map getters */
    size_t cco_map_get_count(const cco_object_t* obj);
    const char* cco_map_get_key(const cco_object_t* obj, size_t index);
    cco_object_t* cco_map_get_value(const cco_object_t* obj, size_t index);
    cco_object_t* cco_map_get_by_key(const cco_object_t* obj, const char* key);

    /* --- Diagnostic APIs --- */

    /* Retrieves the first error encountered during the last parse operation on
     * this thread. */
    cco_error_t cco_get_last_error(void);

    /* Clears thread-local diagnostics. */
    void cco_clear_diagnostics(void);

#ifdef __cplusplus
}
#endif

#endif /* CNT_CCO_H */
