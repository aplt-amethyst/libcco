#define _POSIX_C_SOURCE 200809L
#include "inst.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "diag.h"

static cco_error_t check_inst_limit(cco_parser_context_t* ctx,
                                    cco_inst_context_t* inst_ctx)
{
    if (ctx && ctx->options && ctx->options->max_instantiations > 0)
    {
        size_t current = inst_ctx ? inst_ctx->inst_count : ctx->inst_count;
        if (current >= ctx->options->max_instantiations)
        {
            size_t line = ctx ? ctx->current_token.line : 0;
            size_t col = ctx ? ctx->current_token.col : 0;
            cco_diag_record(CCO_ERR_OUT_OF_RANGE, line, col, "check_inst_limit",
                            "Max instantiations exceeded");
            return CCO_ERR_OUT_OF_RANGE;
        }
    }
    if (inst_ctx)
        inst_ctx->inst_count++;
    if (ctx)
        ctx->inst_count++;
    return CCO_OK;
}

cco_error_t cco_instantiate_positional(cco_parser_context_t* ctx,
                                       cco_symtab_t* symtab,
                                       cco_inst_context_t* inst_ctx,
                                       const char* tmpl_name, cco_array_t* args,
                                       cco_object_t** out_obj)
{
    if (!out_obj || !tmpl_name) return CCO_ERR_INVALID_ARG;
    *out_obj = NULL;

    cco_error_t err = check_inst_limit(ctx, inst_ctx);
    if (err != CCO_OK) return err;

    if (!symtab) {
        cco_diag_record(CCO_ERR_PARSE, ctx ? ctx->current_token.line : 0,
                        ctx ? ctx->current_token.col : 0, "cco_instantiate_positional",
                        "no symbol table available for template resolution");
        return CCO_ERR_PARSE;
    }

    cco_template_def_t* def = cco_symtab_lookup_template(symtab, tmpl_name);
    if (!def) {
        char msg[128];
        snprintf(msg, sizeof(msg), "unknown template '%s'", tmpl_name);
        cco_diag_record(CCO_ERR_PARSE, ctx ? ctx->current_token.line : 0,
                        ctx ? ctx->current_token.col : 0, "cco_instantiate_positional", msg);
        return CCO_ERR_PARSE;
    }

    size_t arg_count = args ? args->count : 0;
    if (arg_count > def->field_count) {
        char msg[128];
        snprintf(msg, sizeof(msg), "too many arguments for template '%s'", tmpl_name);
        cco_diag_record(CCO_ERR_PARSE, ctx ? ctx->current_token.line : 0,
                        ctx ? ctx->current_token.col : 0, "cco_instantiate_positional", msg);
        return CCO_ERR_PARSE;
    }

    cco_object_t* obj = cco_object_create(CCO_TYPE_TEMPLATE_INSTANCE);
    if (!obj) return CCO_ERR_NO_MEMORY;

    obj->as.tmpl_inst.template_name = strdup(tmpl_name);
    if (!obj->as.tmpl_inst.template_name) {
        cco_object_release(obj);
        return CCO_ERR_NO_MEMORY;
    }

    /* Positional arguments */
    for (size_t i = 0; i < arg_count; i++) {
        cco_object_t* val = args->items[i];
        if (!check_field_type(ctx, val, def->fields[i].type_name, def->fields[i].name)) {
            cco_object_release(obj);
            return CCO_ERR_PARSE;
        }
        cco_tmpl_inst_insert(obj, def->fields[i].name, val);
    }

    /* Fill missing fields from defaults */
    for (size_t i = arg_count; i < def->field_count; i++) {
        if (def->fields[i].default_value) {
            cco_tmpl_inst_insert(obj, def->fields[i].name, def->fields[i].default_value);
        } else {
            char msg[128];
            snprintf(msg, sizeof(msg), "missing field '%s' for template '%s'", def->fields[i].name, tmpl_name);
            cco_diag_record(CCO_ERR_PARSE, ctx ? ctx->current_token.line : 0,
                            ctx ? ctx->current_token.col : 0, "cco_instantiate_positional", msg);
            cco_object_release(obj);
            return CCO_ERR_PARSE;
        }
    }

    *out_obj = obj;
    return CCO_OK;
}

cco_error_t cco_instantiate_named(cco_parser_context_t* ctx,
                                  cco_symtab_t* symtab,
                                  cco_inst_context_t* inst_ctx,
                                  const char* tmpl_name, cco_map_t* args,
                                  cco_object_t** out_obj)
{
    if (!out_obj || !tmpl_name) return CCO_ERR_INVALID_ARG;
    *out_obj = NULL;

    cco_error_t err = check_inst_limit(ctx, inst_ctx);
    if (err != CCO_OK) return err;

    if (!symtab) {
        cco_diag_record(CCO_ERR_PARSE, ctx ? ctx->current_token.line : 0,
                        ctx ? ctx->current_token.col : 0, "cco_instantiate_named",
                        "no symbol table available for template resolution");
        return CCO_ERR_PARSE;
    }

    cco_template_def_t* def = cco_symtab_lookup_template(symtab, tmpl_name);
    if (!def) {
        char msg[128];
        snprintf(msg, sizeof(msg), "unknown template '%s'", tmpl_name);
        cco_diag_record(CCO_ERR_PARSE, ctx ? ctx->current_token.line : 0,
                        ctx ? ctx->current_token.col : 0, "cco_instantiate_named", msg);
        return CCO_ERR_PARSE;
    }

    cco_object_t* obj = cco_object_create(CCO_TYPE_TEMPLATE_INSTANCE);
    if (!obj) return CCO_ERR_NO_MEMORY;

    obj->as.tmpl_inst.template_name = strdup(tmpl_name);
    if (!obj->as.tmpl_inst.template_name) {
        cco_object_release(obj);
        return CCO_ERR_NO_MEMORY;
    }

    bool* provided = (bool*)calloc(def->field_count > 0 ? def->field_count : 1, sizeof(bool));
    if (!provided && def->field_count > 0) {
        cco_object_release(obj);
        return CCO_ERR_NO_MEMORY;
    }

    size_t map_count = args ? args->count : 0;
    for (size_t i = 0; i < map_count; i++) {
        const char* key = args->keys[i];
        cco_object_t* val = args->values[i];

        long fidx = -1;
        for (size_t j = 0; j < def->field_count; j++) {
            if (strcmp(def->fields[j].name, key) == 0) {
                fidx = (long)j;
                break;
            }
        }

        if (fidx == -1) {
            char msg[128];
            snprintf(msg, sizeof(msg), "unknown field '%s' for template '%s'", key, tmpl_name);
            cco_diag_record(CCO_ERR_PARSE, ctx ? ctx->current_token.line : 0,
                            ctx ? ctx->current_token.col : 0, "cco_instantiate_named", msg);
            free(provided);
            cco_object_release(obj);
            return CCO_ERR_PARSE;
        }

        if (provided[fidx]) {
            char msg[128];
            snprintf(msg, sizeof(msg), "duplicate field '%s'", key);
            cco_diag_record(CCO_ERR_PARSE, ctx ? ctx->current_token.line : 0,
                            ctx ? ctx->current_token.col : 0, "cco_instantiate_named", msg);
            free(provided);
            cco_object_release(obj);
            return CCO_ERR_PARSE;
        }

        provided[fidx] = true;

        if (!check_field_type(ctx, val, def->fields[fidx].type_name, key)) {
            free(provided);
            cco_object_release(obj);
            return CCO_ERR_PARSE;
        }

        cco_tmpl_inst_insert(obj, key, val);
    }

    /* Missing fields -> default values */
    for (size_t i = 0; i < def->field_count; i++) {
        if (!provided[i]) {
            if (def->fields[i].default_value) {
                cco_tmpl_inst_insert(obj, def->fields[i].name, def->fields[i].default_value);
            } else {
                char msg[128];
                snprintf(msg, sizeof(msg), "missing field '%s' for template '%s'", def->fields[i].name, tmpl_name);
                cco_diag_record(CCO_ERR_PARSE, ctx ? ctx->current_token.line : 0,
                                ctx ? ctx->current_token.col : 0, "cco_instantiate_named", msg);
                free(provided);
                cco_object_release(obj);
                return CCO_ERR_PARSE;
            }
        }
    }

    free(provided);
    *out_obj = obj;
    return CCO_OK;
}

cco_error_t cco_instantiate_custom(cco_parser_context_t* ctx,
                                   cco_symtab_t* symtab,
                                   cco_inst_context_t* inst_ctx,
                                   const char* tmpl_name, cco_object_t* arg,
                                   cco_object_t** out_obj)
{
#ifdef CCO_ENABLE_CONSTRUCTORS
    if (ctx && ctx->options && !ctx->options->allow_constructors)
    {
        cco_diag_record(CCO_ERR_FORBIDDEN, ctx ? ctx->current_token.line : 0,
                        ctx ? ctx->current_token.col : 0, "cco_instantiate_custom",
                        "Custom constructors are disabled by options");
        return CCO_ERR_FORBIDDEN;
    }
#endif

    if (!arg)
        return cco_instantiate_positional(ctx, symtab, inst_ctx, tmpl_name, NULL, out_obj);

    if (arg->type == CCO_TYPE_ARRAY)
        return cco_instantiate_positional(ctx, symtab, inst_ctx, tmpl_name, &arg->as.array, out_obj);

    if (arg->type == CCO_TYPE_MAP)
        return cco_instantiate_named(ctx, symtab, inst_ctx, tmpl_name, &arg->as.map, out_obj);

    return CCO_ERR_INVALID_ARG;
}
