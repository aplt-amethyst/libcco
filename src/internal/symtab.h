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

typedef struct cco_symtab_s
{
    cco_intern_table_t aliases; /* map of interned name -> cco_typedef_t* */
    cco_intern_table_t enums;   /* map of interned name -> cco_enum_def_t* */
    cco_template_map_t templates;
    cco_arena_t* arena;
} cco_symtab_t;

void cco_symtab_destroy(cco_symtab_t* symtab);
void cco_symtab_init(cco_symtab_t* symtab, cco_arena_t* arena);

/* Type alias registration with cycle detection */
cco_error_t cco_symtab_add_typedef(cco_symtab_t* symtab, const char* name,
                                   const char* target);

/* Enum registration */
cco_error_t cco_symtab_add_enum(cco_symtab_t* symtab, const char* name,
                                const cco_enum_def_t* def);

/* Template registration with inheritance cycle detection */
cco_error_t cco_symtab_add_template(cco_symtab_t* symtab, const char* name,
                                    cco_template_def_t* def);

/* Parse top-level declarations (simplified entry point for the parser) */
cco_template_def_t* cco_symtab_lookup_template(cco_symtab_t* symtab, const char* name);

cco_template_def_t* cco_symtab_lookup_template(cco_symtab_t* symtab, const char* name);

cco_error_t cco_parse_declarations(cco_parser_context_t* ctx,
                                   cco_symtab_t* symtab);

#endif /* CNT_CCO_INTERNAL_SYMTAB_H */
