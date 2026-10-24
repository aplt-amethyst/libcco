#ifndef CNT_CCO_INTERNAL_SCOPE_H
#define CNT_CCO_INTERNAL_SCOPE_H

#include "object.h"
#include <stdbool.h>

typedef struct cco_scope_entry_s {
    char* name;
    cco_object_t* value;
} cco_scope_entry_t;

typedef struct cco_scope_s cco_scope_t;

struct cco_scope_s {
    cco_scope_t* parent;
    cco_scope_entry_t* entries;
    size_t count;
    size_t capacity;
};

/* Init and destroy */
void cco_scope_init(cco_scope_t* scope, cco_scope_t* parent);
void cco_scope_destroy(cco_scope_t* scope);

/* cco_scope_bind takes ownership of val; caller must not release or reuse it. */
cco_error_t cco_scope_bind(cco_scope_t* scope, const char* name, cco_object_t* value);

/* Lookup an object by name.
 * Returns a borrowed pointer to the object. The object is owned by the scope.
 * The caller MUST NOT release the returned object.
 * If the caller needs to retain the object beyond the scope's lifetime,
 * they must call cco_object_retain() on it. */
cco_object_t* cco_scope_lookup(cco_scope_t* scope, const char* name);

#endif /* CNT_CCO_INTERNAL_SCOPE_H */
