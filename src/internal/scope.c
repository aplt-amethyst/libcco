#include "scope.h"
#include <stdlib.h>
#include <string.h>

void cco_scope_init(cco_scope_t* scope, cco_scope_t* parent) {
    scope->parent = parent;
    scope->entries = NULL;
    scope->count = 0;
    scope->capacity = 0;
}

void cco_scope_destroy(cco_scope_t* scope) {
    for (size_t i = 0; i < scope->count; i++) {
        free(scope->entries[i].name);
        cco_object_release(scope->entries[i].value);
    }
    free(scope->entries);
    scope->entries = NULL;
    scope->count = scope->capacity = 0;
}

cco_error_t cco_scope_bind(cco_scope_t* scope, const char* name, cco_object_t* value) {
    /* Simple dynamic array */
    if (scope->count >= scope->capacity) {
        size_t new_cap = scope->capacity == 0 ? 8 : scope->capacity * 2;
        cco_scope_entry_t* new_entries = realloc(scope->entries, new_cap * sizeof(cco_scope_entry_t));
        if (!new_entries) return CCO_ERR_NO_MEMORY;
        scope->entries = new_entries;
        scope->capacity = new_cap;
    }
    scope->entries[scope->count].name = strcpy(malloc(strlen(name) + 1), name);
    if (!scope->entries[scope->count].name) return CCO_ERR_NO_MEMORY;
    
    scope->entries[scope->count].value = value;
    if (value) cco_object_retain(value);
    
    scope->count++;
    return CCO_OK;
}

cco_object_t* cco_scope_lookup(cco_scope_t* scope, const char* name) {
    for (cco_scope_t* curr = scope; curr != NULL; curr = curr->parent) {
        for (size_t i = 0; i < curr->count; i++) {
            if (strcmp(curr->entries[i].name, name) == 0) {
                return curr->entries[i].value;
            }
        }
    }
    return NULL;
}
