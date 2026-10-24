#ifndef CNT_CCO_INTERNAL_SYMTAB_H
#define CNT_CCO_INTERNAL_SYMTAB_H

#include "hash.h"
#include "object.h"
#include "parser.h"

typedef struct
{
    const char* target_type_name; /* Interned */
} cco_typedef_t;

typedef struct
{
    cco_map_t values; /* Map of string identifier to integer object */
} cco_enum_def_t;

typedef struct cco_field_def_t {
    char* name;
    char* type_name;      /* nullable */
    cco_object_t* default_value; /* nullable, owned */
} cco_field_def_t;

typedef struct cco_template_def_s
{
    char* name;              /* Template identifier name */
    cco_field_def_t* fields; /* array */
    size_t field_count;
    size_t field_capacity;
    const char* parent_name; /* Interned, NULL if no parent */
} cco_template_def_t;

typedef struct {
    const char* name;
    cco_template_def_t* def;
} cco_symtab_template_entry_t;

typedef struct
{
    cco_symtab_template_entry_t* entries;
    size_t count;
    size_t capacity;
} cco_template_map_t;

typedef struct {
    const char* name;
    cco_object_t* ast;
} cco_symtab_macro_entry_t;

typedef struct {
    cco_symtab_macro_entry_t* entries;
    size_t count;
    size_t capacity;
} cco_macro_map_t;

typedef struct {
    char* name;
    char* target;
} cco_symtab_typedef_entry_t;

typedef struct {
    cco_symtab_typedef_entry_t* entries;
    size_t count;
    size_t capacity;
} cco_typedef_map_t;

typedef struct {
    char* name;
    char** members;
    size_t member_count;
    size_t member_capacity;
} cco_symtab_enum_entry_t;

typedef struct {
    cco_symtab_enum_entry_t* entries;
    size_t count;
    size_t capacity;
} cco_enum_map_t;

typedef struct cco_symtab_s
{
    cco_intern_table_t aliases; /* map of interned name -> cco_typedef_t* */
    cco_intern_table_t enums;   /* map of interned name -> cco_enum_def_t* */
    cco_template_map_t templates;
    cco_macro_map_t macros;
    cco_typedef_map_t typedefs;
    cco_enum_map_t enum_defs;
    cco_arena_t* arena;
    bool owns_arena;
} cco_symtab_t;

void cco_symtab_destroy(cco_symtab_t* symtab);
void cco_symtab_init(cco_symtab_t* symtab, cco_arena_t* arena);
cco_symtab_t* cco_symtab_new(void);
cco_symtab_t* cco_symtab_new_with_arena(cco_arena_t* arena);
void cco_symtab_free(cco_symtab_t* symtab);

/* Type alias registration with cycle detection */
cco_error_t cco_symtab_add_typedef(cco_symtab_t* symtab, const char* name,
                                   const char* target);
const char* cco_symtab_lookup_typedef(cco_symtab_t* symtab, const char* name);

/* Enum registration and lookup */
cco_error_t cco_symtab_add_enum(cco_symtab_t* symtab, const char* name,
                                const cco_enum_def_t* def);
bool cco_symtab_lookup_enum(cco_symtab_t* symtab, const char* name);

/* Template registration with inheritance cycle detection */
cco_error_t cco_symtab_add_template(cco_symtab_t* symtab, const char* name,
                                    cco_template_def_t* def);
cco_template_def_t* cco_symtab_lookup_template(cco_symtab_t* symtab, const char* name);
void cco_template_def_destroy(cco_template_def_t* def);
void cco_template_def_free(cco_template_def_t* def);
const char* cco_symtab_resolve_alias(cco_symtab_t* symtab, const char* name);

/* Parse top-level declarations */
cco_error_t cco_parse_declarations(cco_parser_context_t* ctx,
                                   cco_symtab_t* symtab);

cco_error_t cco_symtab_add_macro(cco_symtab_t* symtab, const char* name, cco_object_t* ast);
cco_object_t* cco_symtab_lookup_macro(cco_symtab_t* symtab, const char* name);
#endif /* CNT_CCO_INTERNAL_SYMTAB_H */
