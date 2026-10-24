#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "symtab.h"
#include "diag.h"

void cco_symtab_init(cco_symtab_t* symtab, cco_arena_t* arena)
{
    if (symtab)
    {
        memset(symtab, 0, sizeof(*symtab));
        cco_intern_table_init(&symtab->aliases, arena);
        cco_intern_table_init(&symtab->enums, arena);
        symtab->arena = arena;
        symtab->owns_arena = false;
    }
}

cco_symtab_t* cco_symtab_new_with_arena(cco_arena_t* arena)
{
    cco_symtab_t* symtab = (cco_symtab_t*)calloc(1, sizeof(cco_symtab_t));
    if (!symtab) return NULL;
    cco_symtab_init(symtab, arena);
    return symtab;
}

cco_symtab_t* cco_symtab_new(void)
{
    cco_arena_t* arena = (cco_arena_t*)malloc(sizeof(cco_arena_t));
    if (!arena) return NULL;
    cco_arena_init(arena, 0);
    cco_symtab_t* symtab = cco_symtab_new_with_arena(arena);
    if (!symtab) {
        cco_arena_destroy(arena);
        free(arena);
        return NULL;
    }
    symtab->owns_arena = true;
    return symtab;
}

void cco_symtab_free(cco_symtab_t* symtab)
{
    if (!symtab) return;
    cco_symtab_destroy(symtab);
    free(symtab);
}

const char* cco_symtab_lookup_typedef(cco_symtab_t* symtab, const char* name)
{
    if (!symtab || !name) return NULL;
    for (size_t i = 0; i < symtab->typedefs.count; ++i) {
        if (strcmp(symtab->typedefs.entries[i].name, name) == 0) {
            return symtab->typedefs.entries[i].target;
        }
    }
    return NULL;
}

const char* cco_symtab_resolve_alias(cco_symtab_t* symtab, const char* name)
{
    if (!symtab || !name) return name;
    const char* curr = name;
    int depth = 0;
    while (depth < 64) {
        const char* next = cco_symtab_lookup_typedef(symtab, curr);
        if (!next) break;
        curr = next;
        depth++;
    }
    return curr;
}

cco_error_t cco_symtab_add_typedef(cco_symtab_t* symtab, const char* name,
                                   const char* target)
{
    if (!symtab || !name || !target)
        return CCO_ERR_INVALID_ARG;

    /* Check for duplicate symbol definition */
    if (cco_symtab_lookup_typedef(symtab, name) != NULL ||
        cco_symtab_lookup_enum(symtab, name) ||
        cco_symtab_lookup_template(symtab, name) != NULL)
    {
        char msg[512];
        snprintf(msg, sizeof(msg), "Symbol '%s' already defined", name);
        cco_diag_record(CCO_ERR_PARSE, 0, 0, "cco_symtab_add_typedef", msg);
        return CCO_ERR_PARSE;
    }

    /* Cycle detection */
    if (strcmp(name, target) == 0)
    {
        cco_diag_record(CCO_ERR_INVALID_ARG, 0, 0, "cco_symtab_add_typedef",
                        "Direct typedef cycle detected");
        return CCO_ERR_INVALID_ARG;
    }

    const char* curr = target;
    while (curr) {
        if (strcmp(name, curr) == 0) {
            cco_diag_record(CCO_ERR_INVALID_ARG, 0, 0, "cco_symtab_add_typedef",
                            "Typedef cycle detected");
            return CCO_ERR_INVALID_ARG;
        }
        curr = cco_symtab_lookup_typedef(symtab, curr);
    }

    if (symtab->typedefs.count == symtab->typedefs.capacity) {
        size_t new_cap = symtab->typedefs.capacity == 0 ? 8 : symtab->typedefs.capacity * 2;
        cco_symtab_typedef_entry_t* new_entries =
            (cco_symtab_typedef_entry_t*)realloc(symtab->typedefs.entries,
                                                new_cap * sizeof(cco_symtab_typedef_entry_t));
        if (!new_entries) return CCO_ERR_NO_MEMORY;
        symtab->typedefs.entries = new_entries;
        symtab->typedefs.capacity = new_cap;
    }

    symtab->typedefs.entries[symtab->typedefs.count].name = strdup(name);
    symtab->typedefs.entries[symtab->typedefs.count].target = strdup(target);
    symtab->typedefs.count++;

    cco_intern_string(&symtab->aliases, name, strlen(name));
    return CCO_OK;
}

bool cco_symtab_lookup_enum(cco_symtab_t* symtab, const char* name)
{
    if (!symtab || !name) return false;
    for (size_t i = 0; i < symtab->enum_defs.count; ++i) {
        if (strcmp(symtab->enum_defs.entries[i].name, name) == 0) {
            return true;
        }
    }
    return false;
}

cco_error_t cco_symtab_add_enum(cco_symtab_t* symtab, const char* name,
                                const cco_enum_def_t* def)
{
    if (!symtab || !name)
        return CCO_ERR_INVALID_ARG;
    (void)def;

    if (cco_symtab_lookup_enum(symtab, name) ||
        cco_symtab_lookup_typedef(symtab, name) != NULL ||
        cco_symtab_lookup_template(symtab, name) != NULL)
    {
        char msg[512];
        snprintf(msg, sizeof(msg), "Enum '%s' already defined", name);
        cco_diag_record(CCO_ERR_PARSE, 0, 0, "cco_symtab_add_enum", msg);
        return CCO_ERR_PARSE;
    }

    if (symtab->enum_defs.count == symtab->enum_defs.capacity) {
        size_t new_cap = symtab->enum_defs.capacity == 0 ? 8 : symtab->enum_defs.capacity * 2;
        cco_symtab_enum_entry_t* new_entries =
            (cco_symtab_enum_entry_t*)realloc(symtab->enum_defs.entries,
                                             new_cap * sizeof(cco_symtab_enum_entry_t));
        if (!new_entries) return CCO_ERR_NO_MEMORY;
        symtab->enum_defs.entries = new_entries;
        symtab->enum_defs.capacity = new_cap;
    }

    symtab->enum_defs.entries[symtab->enum_defs.count].name = strdup(name);
    symtab->enum_defs.entries[symtab->enum_defs.count].members = NULL;
    symtab->enum_defs.entries[symtab->enum_defs.count].member_count = 0;
    symtab->enum_defs.entries[symtab->enum_defs.count].member_capacity = 0;
    symtab->enum_defs.count++;

    cco_intern_string(&symtab->enums, name, strlen(name));
    return CCO_OK;
}

cco_template_def_t* cco_symtab_lookup_template(cco_symtab_t* symtab, const char* name)
{
    if (!symtab || !name) return NULL;
    for (size_t i = 0; i < symtab->templates.count; ++i) {
        if (strcmp(symtab->templates.entries[i].name, name) == 0) {
            return symtab->templates.entries[i].def;
        }
    }
    return NULL;
}

cco_error_t cco_symtab_add_template(cco_symtab_t* symtab, const char* name, cco_template_def_t* def)
{
    if (!symtab || !name || !def) return CCO_ERR_INVALID_ARG;

    /* Check for duplicate symbol / template definition */
    if (cco_symtab_lookup_template(symtab, name) != NULL ||
        cco_symtab_lookup_typedef(symtab, name) != NULL ||
        cco_symtab_lookup_enum(symtab, name)) {
        char msg[512];
        snprintf(msg, sizeof(msg), "Template '%s' already defined", name);
        cco_diag_record(CCO_ERR_PARSE, 0, 0, "cco_symtab_add_template", msg);
        return CCO_ERR_PARSE;
    }

    if (def->parent_name && strcmp(name, def->parent_name) == 0) {
        cco_diag_record(CCO_ERR_INVALID_ARG, 0, 0, "cco_symtab_add_template", "Self inheritance detected");
        return CCO_ERR_INVALID_ARG;
    }

    if (!def->name) {
        def->name = strdup(name);
    }
    
    if (def->parent_name) {
        cco_template_def_t* parent_def = cco_symtab_lookup_template(symtab, def->parent_name);
        if (!parent_def) {
            char msg[512];
            snprintf(msg, sizeof(msg), "unknown parent '%s'", def->parent_name);
            cco_diag_record(CCO_ERR_PARSE, 0, 0, "cco_symtab_add_template", msg);
            return CCO_ERR_PARSE;
        }
        
        /* Merge fields: parent fields first, then child fields */
        size_t merged_capacity = parent_def->field_count + def->field_count;
        cco_field_def_t* merged_fields = malloc(merged_capacity * sizeof(cco_field_def_t));
        size_t merged_count = 0;
        
        /* 1. Add all parent fields, but skip if overridden by child */
        for (size_t i = 0; i < parent_def->field_count; ++i) {
            bool overridden = false;
            for (size_t j = 0; j < def->field_count; ++j) {
                if (strcmp(parent_def->fields[i].name, def->fields[j].name) == 0) {
                    overridden = true;
                    break;
                }
            }
            if (!overridden) {
                merged_fields[merged_count].name = strdup(parent_def->fields[i].name);
                merged_fields[merged_count].type_name = parent_def->fields[i].type_name ? strdup(parent_def->fields[i].type_name) : NULL;
                merged_fields[merged_count].default_value = parent_def->fields[i].default_value;
                if (merged_fields[merged_count].default_value) {
                    cco_object_retain(merged_fields[merged_count].default_value);
                }
                merged_count++;
            }
        }
        
        /* 2. Add all child fields */
        for (size_t i = 0; i < def->field_count; ++i) {
            merged_fields[merged_count++] = def->fields[i];
        }
        
        free(def->fields);
        def->fields = merged_fields;
        def->field_count = merged_count;
        def->field_capacity = merged_capacity;
    }

    const char* interned_name = cco_intern_string(&symtab->aliases, name, strlen(name));
    if (!interned_name) return CCO_ERR_NO_MEMORY;
    
    if (symtab->templates.count == symtab->templates.capacity) {
        size_t new_cap = symtab->templates.capacity == 0 ? 16 : symtab->templates.capacity * 2;
        cco_symtab_template_entry_t* new_entries =
            (cco_symtab_template_entry_t*)realloc(symtab->templates.entries,
                                                  new_cap * sizeof(cco_symtab_template_entry_t));
        if (!new_entries) return CCO_ERR_NO_MEMORY;
        symtab->templates.entries = new_entries;
        symtab->templates.capacity = new_cap;
    }
    symtab->templates.entries[symtab->templates.count].name = interned_name;
    symtab->templates.entries[symtab->templates.count].def = def;
    symtab->templates.count++;
    return CCO_OK;
}

cco_error_t cco_symtab_add_macro(cco_symtab_t* symtab, const char* name, cco_object_t* ast)
{
    if (!symtab || !name) return CCO_ERR_INVALID_ARG;
    if (symtab->macros.count == symtab->macros.capacity) {
        size_t new_cap = symtab->macros.capacity == 0 ? 4 : symtab->macros.capacity * 2;
        cco_symtab_macro_entry_t* new_entries =
            (cco_symtab_macro_entry_t*)realloc(symtab->macros.entries,
                                               new_cap * sizeof(cco_symtab_macro_entry_t));
        if (!new_entries) return CCO_ERR_NO_MEMORY;
        symtab->macros.entries = new_entries;
        symtab->macros.capacity = new_cap;
    }
    symtab->macros.entries[symtab->macros.count].name = strdup(name);
    symtab->macros.entries[symtab->macros.count].ast = ast;
    if (ast) cco_object_retain(ast);
    symtab->macros.count++;
    return CCO_OK;
}

cco_object_t* cco_symtab_lookup_macro(cco_symtab_t* symtab, const char* name)
{
    if (!symtab || !name) return NULL;
    for (size_t i = 0; i < symtab->macros.count; ++i) {
        if (strcmp(symtab->macros.entries[i].name, name) == 0) {
            return symtab->macros.entries[i].ast;
        }
    }
    return NULL;
}

void cco_template_def_destroy(cco_template_def_t* def)
{
    if (!def) return;
    if (def->name) free(def->name);
    for (size_t j = 0; j < def->field_count; ++j) {
        if (def->fields[j].name) free(def->fields[j].name);
        if (def->fields[j].type_name) free(def->fields[j].type_name);
        if (def->fields[j].default_value) cco_object_release(def->fields[j].default_value);
    }
    if (def->fields) free(def->fields);
    if (def->parent_name) free((void*)def->parent_name);
    free(def);
}

void cco_template_def_free(cco_template_def_t* def)
{
    cco_template_def_destroy(def);
}

void cco_symtab_destroy(cco_symtab_t* symtab)
{
    if (!symtab) return;
    cco_intern_table_destroy(&symtab->aliases);
    cco_intern_table_destroy(&symtab->enums);

    for (size_t i = 0; i < symtab->templates.count; ++i) {
        cco_template_def_t* def = symtab->templates.entries[i].def;
        cco_template_def_destroy(def);
    }
    if (symtab->templates.entries) free(symtab->templates.entries);

    for (size_t i = 0; i < symtab->macros.count; ++i) {
        if (symtab->macros.entries[i].name) free((void*)symtab->macros.entries[i].name);
        if (symtab->macros.entries[i].ast) cco_object_release(symtab->macros.entries[i].ast);
    }
    if (symtab->macros.entries) free(symtab->macros.entries);

    for (size_t i = 0; i < symtab->typedefs.count; ++i) {
        free(symtab->typedefs.entries[i].name);
        free(symtab->typedefs.entries[i].target);
    }
    if (symtab->typedefs.entries) free(symtab->typedefs.entries);

    for (size_t i = 0; i < symtab->enum_defs.count; ++i) {
        free(symtab->enum_defs.entries[i].name);
        for (size_t j = 0; j < symtab->enum_defs.entries[i].member_count; ++j) {
            free(symtab->enum_defs.entries[i].members[j]);
        }
        if (symtab->enum_defs.entries[i].members) free(symtab->enum_defs.entries[i].members);
    }
    if (symtab->enum_defs.entries) free(symtab->enum_defs.entries);

    if (symtab->owns_arena && symtab->arena) {
        cco_arena_destroy(symtab->arena);
        free(symtab->arena);
        symtab->arena = NULL;
    }
}

cco_error_t cco_parse_declarations(cco_parser_context_t* ctx,
                                   cco_symtab_t* symtab)
{
    if (!ctx || !symtab) return CCO_ERR_INVALID_ARG;
    if (!ctx->symtab) ctx->symtab = symtab;

    while (ctx->current_token.type != CCO_TOK_EOF && ctx->last_error == CCO_OK)
    {
        if (ctx->current_token.type != CCO_TOK_IDENTIFIER) {
            break;
        }

        const char* tptr = ctx->current_token.text_ptr;
        size_t tlen = ctx->current_token.text_len;

        if (tlen == 8 && strncmp(tptr, "$typedef", 8) == 0) {
            cco_parser_advance(ctx); /* skip $typedef */
            if (ctx->current_token.type != CCO_TOK_DOT) {
                cco_diag_record(CCO_ERR_PARSE, ctx->current_token.line, ctx->current_token.col,
                                "parse_typedef", "expected '.' after $typedef");
                ctx->last_error = CCO_ERR_PARSE;
                return CCO_ERR_PARSE;
            }
            cco_parser_advance(ctx); /* skip . */

            if (ctx->current_token.type != CCO_TOK_IDENTIFIER) {
                cco_diag_record(CCO_ERR_PARSE, ctx->current_token.line, ctx->current_token.col,
                                "parse_typedef", "expected type alias name");
                ctx->last_error = CCO_ERR_PARSE;
                return CCO_ERR_PARSE;
            }
            char name_buf[256];
            size_t nlen = ctx->current_token.text_len < 255 ? ctx->current_token.text_len : 255;
            memcpy(name_buf, ctx->current_token.text_ptr, nlen);
            name_buf[nlen] = '\0';
            cco_parser_advance(ctx);

            if (ctx->current_token.type != CCO_TOK_COLON) {
                cco_diag_record(CCO_ERR_PARSE, ctx->current_token.line, ctx->current_token.col,
                                "parse_typedef", "expected ':' after type alias name");
                ctx->last_error = CCO_ERR_PARSE;
                return CCO_ERR_PARSE;
            }
            cco_parser_advance(ctx); /* skip : */

            if (ctx->current_token.type != CCO_TOK_IDENTIFIER) {
                cco_diag_record(CCO_ERR_PARSE, ctx->current_token.line, ctx->current_token.col,
                                "parse_typedef", "expected target type name");
                ctx->last_error = CCO_ERR_PARSE;
                return CCO_ERR_PARSE;
            }
            char target_buf[256];
            size_t tglen = ctx->current_token.text_len < 255 ? ctx->current_token.text_len : 255;
            memcpy(target_buf, ctx->current_token.text_ptr, tglen);
            target_buf[tglen] = '\0';
            cco_parser_advance(ctx);

            /* Optional = Default / Alias */
            if (ctx->current_token.type == CCO_TOK_EQ) {
                cco_parser_advance(ctx);
                if (ctx->current_token.type == CCO_TOK_IDENTIFIER) {
                    cco_parser_advance(ctx);
                } else {
                    cco_object_t* def_val = parse_expr(ctx);
                    if (def_val) cco_object_release(def_val);
                }
            }

            cco_error_t err = cco_symtab_add_typedef(symtab, name_buf, target_buf);
            if (err != CCO_OK) {
                ctx->last_error = err;
                return err;
            }
            if (ctx->current_token.type == CCO_TOK_COMMA) {
                cco_parser_advance(ctx);
            }
        }
        else if (tlen == 5 && strncmp(tptr, "$enum", 5) == 0) {
            cco_parser_advance(ctx); /* skip $enum */
            if (ctx->current_token.type != CCO_TOK_DOT) {
                cco_diag_record(CCO_ERR_PARSE, ctx->current_token.line, ctx->current_token.col,
                                "parse_enum", "expected '.' after $enum");
                ctx->last_error = CCO_ERR_PARSE;
                return CCO_ERR_PARSE;
            }
            cco_parser_advance(ctx); /* skip . */

            if (ctx->current_token.type != CCO_TOK_IDENTIFIER) {
                cco_diag_record(CCO_ERR_PARSE, ctx->current_token.line, ctx->current_token.col,
                                "parse_enum", "expected enum name");
                ctx->last_error = CCO_ERR_PARSE;
                return CCO_ERR_PARSE;
            }
            char enum_name[256];
            size_t enlen = ctx->current_token.text_len < 255 ? ctx->current_token.text_len : 255;
            memcpy(enum_name, ctx->current_token.text_ptr, enlen);
            enum_name[enlen] = '\0';
            cco_parser_advance(ctx);

            if (ctx->current_token.type != CCO_TOK_EQ && ctx->current_token.type != CCO_TOK_COLON) {
                cco_diag_record(CCO_ERR_PARSE, ctx->current_token.line, ctx->current_token.col,
                                "parse_enum", "expected '=' or ':' after enum name");
                ctx->last_error = CCO_ERR_PARSE;
                return CCO_ERR_PARSE;
            }
            cco_parser_advance(ctx); /* skip = or : */

            if (ctx->current_token.type != CCO_TOK_LPAREN) {
                cco_diag_record(CCO_ERR_PARSE, ctx->current_token.line, ctx->current_token.col,
                                "parse_enum", "expected '(' to start enum member list");
                ctx->last_error = CCO_ERR_PARSE;
                return CCO_ERR_PARSE;
            }
            cco_parser_advance(ctx); /* skip ( */

            if (cco_symtab_lookup_enum(symtab, enum_name) ||
                cco_symtab_lookup_typedef(symtab, enum_name) != NULL ||
                cco_symtab_lookup_template(symtab, enum_name) != NULL) {
                char msg[512];
                snprintf(msg, sizeof(msg), "Enum '%s' already defined", enum_name);
                cco_diag_record(CCO_ERR_PARSE, ctx->current_token.line, ctx->current_token.col,
                                "parse_enum", msg);
                ctx->last_error = CCO_ERR_PARSE;
                return CCO_ERR_PARSE;
            }

            int64_t member_idx = 0;
            while (ctx->current_token.type != CCO_TOK_RPAREN && ctx->current_token.type != CCO_TOK_EOF) {
                if (ctx->current_token.type != CCO_TOK_IDENTIFIER) {
                    cco_diag_record(CCO_ERR_PARSE, ctx->current_token.line, ctx->current_token.col,
                                    "parse_enum", "expected identifier in enum list");
                    ctx->last_error = CCO_ERR_PARSE;
                    return CCO_ERR_PARSE;
                }
                char mname[256];
                size_t mlen = ctx->current_token.text_len < 255 ? ctx->current_token.text_len : 255;
                memcpy(mname, ctx->current_token.text_ptr, mlen);
                mname[mlen] = '\0';
                cco_parser_advance(ctx);

                /* Register member in symtab->macros as integer constant */
                cco_object_t* int_obj = cco_object_create(CCO_TYPE_INTEGER);
                if (int_obj) {
                    int_obj->as.integer = member_idx++;
                    cco_symtab_add_macro(symtab, mname, int_obj);
                    cco_object_release(int_obj);
                }

                if (ctx->current_token.type == CCO_TOK_COMMA) {
                    cco_parser_advance(ctx);
                }
            }
            if (ctx->current_token.type == CCO_TOK_RPAREN) {
                cco_parser_advance(ctx);
            }

            cco_error_t aerr = cco_symtab_add_enum(symtab, enum_name, NULL);
            if (aerr != CCO_OK) {
                ctx->last_error = aerr;
                return aerr;
            }

            if (ctx->current_token.type == CCO_TOK_COMMA) {
                cco_parser_advance(ctx);
            }
        }
        else if (tlen == 5 && strncmp(tptr, "$temp", 5) == 0) {
            cco_template_def_t* def = cco_parse_template_decl(ctx);
            if (!def) {
                if (ctx->last_error == CCO_OK) ctx->last_error = CCO_ERR_PARSE;
                return ctx->last_error;
            }
            if (!ctx->symtab && symtab) {
                cco_error_t terr = cco_symtab_add_template(symtab, def->name, def);
                if (terr != CCO_OK) {
                    ctx->last_error = terr;
                    cco_template_def_destroy(def);
                    return terr;
                }
            }
            if (ctx->current_token.type == CCO_TOK_COMMA) {
                cco_parser_advance(ctx);
            }
        }
        else if (tlen == 9 && strncmp(tptr, "$function", 9) == 0) {
            cco_object_t* fn = cco_parse_function(ctx);
            if (ctx->last_error != CCO_OK) {
                return ctx->last_error;
            }
            if (fn) {
                cco_object_release(fn);
            }
            if (ctx->current_token.type == CCO_TOK_COMMA) {
                cco_parser_advance(ctx);
            }
        }
        else if (tlen == 5 && strncmp(tptr, "$bind", 5) == 0) {
            cco_parser_advance(ctx); /* skip $bind */
            if (ctx->current_token.type != CCO_TOK_DOT) {
                cco_diag_record(CCO_ERR_PARSE, ctx->current_token.line, ctx->current_token.col,
                                "parse_bind", "expected '.' after $bind");
                ctx->last_error = CCO_ERR_PARSE;
                return CCO_ERR_PARSE;
            }
            cco_parser_advance(ctx); /* skip . */

            if (ctx->current_token.type != CCO_TOK_IDENTIFIER) {
                cco_diag_record(CCO_ERR_PARSE, ctx->current_token.line, ctx->current_token.col,
                                "parse_bind", "expected identifier name after $bind.");
                ctx->last_error = CCO_ERR_PARSE;
                return CCO_ERR_PARSE;
            }
            char bname[256];
            size_t blen = ctx->current_token.text_len < 255 ? ctx->current_token.text_len : 255;
            memcpy(bname, ctx->current_token.text_ptr, blen);
            bname[blen] = '\0';
            cco_parser_advance(ctx);

            if (ctx->current_token.type != CCO_TOK_COLON) {
                cco_diag_record(CCO_ERR_PARSE, ctx->current_token.line, ctx->current_token.col,
                                "parse_bind", "expected ':' after bind name");
                ctx->last_error = CCO_ERR_PARSE;
                return CCO_ERR_PARSE;
            }
            cco_parser_advance(ctx); /* skip : */

            cco_object_t* bval = parse_expr(ctx);
            if (!bval) {
                if (ctx->last_error == CCO_OK) ctx->last_error = CCO_ERR_PARSE;
                return ctx->last_error;
            }

            if (ctx->scope) {
                cco_scope_bind(ctx->scope, bname, bval);
            }
            cco_symtab_add_macro(symtab, bname, bval);
            cco_object_release(bval);

            if (ctx->current_token.type == CCO_TOK_COMMA) {
                cco_parser_advance(ctx);
            }
        }
        else if (tlen == 8 && strncmp(tptr, "$include", 8) == 0 && ctx->next_token.type == CCO_TOK_LPAREN) {
            cco_parser_advance(ctx); /* skip $include */
            cco_parser_advance(ctx); /* skip ( */
            if (ctx->current_token.type != CCO_TOK_STRING) {
                cco_diag_record(CCO_ERR_PARSE, ctx->current_token.line, ctx->current_token.col,
                                "parse_include", "expected string path in $include");
                ctx->last_error = CCO_ERR_PARSE;
                return CCO_ERR_PARSE;
            }
            char inc_name[256];
            size_t ilen = ctx->current_token.text_len < 255 ? ctx->current_token.text_len : 255;
            memcpy(inc_name, ctx->current_token.text_ptr, ilen);
            inc_name[ilen] = '\0';
            cco_parser_advance(ctx);
            if (ctx->current_token.type == CCO_TOK_RPAREN) cco_parser_advance(ctx);

            char inc_path[1024];
            if (ctx->options && ctx->options->base_dir) {
                snprintf(inc_path, sizeof(inc_path), "%s/%s", ctx->options->base_dir, inc_name);
            } else {
                snprintf(inc_path, sizeof(inc_path), "%s", inc_name);
            }
            FILE* f = fopen(inc_path, "rb");
            if (f) {
                fseek(f, 0, SEEK_END);
                long fsz = ftell(f);
                fseek(f, 0, SEEK_SET);
                if (fsz > 0) {
                    char* inc_buf = (char*)malloc((size_t)fsz + 1);
                    if (inc_buf) {
                        fread(inc_buf, 1, (size_t)fsz, f);
                        inc_buf[fsz] = '\0';
                        cco_parser_context_t sub_ctx;
                        cco_arena_t sub_arena;
                        cco_arena_init(&sub_arena, 0);
                        cco_parser_init(&sub_ctx, inc_buf, (size_t)fsz, ctx->options, &sub_arena);
                        sub_ctx.symtab = symtab;
                        cco_parse_declarations(&sub_ctx, symtab);
                        cco_arena_destroy(&sub_arena);
                        free(inc_buf);
                    }
                }
                fclose(f);
            }
            if (ctx->current_token.type == CCO_TOK_COMMA) {
                cco_parser_advance(ctx);
            }
        }
        else {
            /* Non-declaration identifier, stop declaration loop */
            break;
        }
    }
    return CCO_OK;
}
