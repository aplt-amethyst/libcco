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

/* cco_scope_bind 接管 val 所有权，调用方释放完不能再用。 */
cco_error_t cco_scope_bind(cco_scope_t* scope, const char* name, cco_object_t* value);

/* Lookup */
cco_object_t* cco_scope_lookup(cco_scope_t* scope, const char* name);

#endif /* CNT_CCO_INTERNAL_SCOPE_H */
