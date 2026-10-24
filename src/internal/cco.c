#define _POSIX_C_SOURCE 200809L
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cnt/cco.h>
#include <cnt/cco_error.h>
#include <cnt/cco_options.h>

#include "arena.h"
#include "diag.h"
#include "eval.h"
#include "expr.h"
#include "inst.h"
#include "object.h"
#include "parser.h"
#include "sandbox.h"
#include "serializer.h"
#include "strbuf.h"
#include "symtab.h"

/* Structure definition for parse result */
struct cco_parse_result_s
{
    cco_object_t* root;
    cco_symtab_t* symtab;
    cco_arena_t* arena;
};

/* --- Options API --- */

void cco_parse_options_init(cco_parse_options_t* opts)
{
    if (!opts)
        return;
    memset(opts, 0, sizeof(cco_parse_options_t));
    opts->max_depth = 256;
    opts->max_string_length = 1048576;
    opts->max_document_size = 10485760;
    opts->max_total_allocation = 10485760;
    opts->max_instantiations = 1000;
    opts->max_steps = 10000;
    opts->allow_colon_instantiation = true;
    opts->lenient_brackets = true;
    opts->strict_typing = true;
    opts->restrict_filesystem = true;
    opts->base_dir = ".";
    opts->fast_mode = true;
    opts->zero_copy_dom = true;
    opts->allow_format_builtin = true;
    opts->allow_eval = true;
    opts->allow_env_builtin = true;
    opts->allow_static_calls = true;
    opts->allow_constructors = true;
    opts->strict_safety_parsing = false;
    opts->allow_include = true;
}

cco_parse_options_t cco_parse_options_default(void)
{
    cco_parse_options_t opts;
    cco_parse_options_init(&opts);
    return opts;
}

/* --- Arena Memory Pool API --- */

cco_arena_t* cco_arena_new(size_t max_limit)
{
    cco_arena_t* a = (cco_arena_t*)malloc(sizeof(cco_arena_t));
    if (a)
    {
        cco_arena_init(a, max_limit);
    }
    return a;
}

void cco_arena_free(cco_arena_t* arena)
{
    if (arena)
    {
        cco_arena_destroy(arena);
        free(arena);
    }
}

/* --- Value Construction APIs --- */

cco_object_t* cco_object_new(void)
{
    return cco_object_create(CCO_TYPE_MAP);
}

cco_object_t* cco_none_new(void)
{
    return cco_object_create(CCO_TYPE_NONE);
}

cco_object_t* cco_int_new(int64_t v)
{
    cco_object_t* o = cco_object_create(CCO_TYPE_INTEGER);
    if (o)
        o->as.integer = v;
    return o;
}

cco_object_t* cco_float_new(double v)
{
    cco_object_t* o = cco_object_create(CCO_TYPE_FLOAT);
    if (o)
        o->as.floating = v;
    return o;
}

cco_object_t* cco_bool_new(bool v)
{
    cco_object_t* o = cco_object_create(CCO_TYPE_BOOLEAN);
    if (o)
        o->as.boolean = v;
    return o;
}

cco_object_t* cco_string_new(const char* s)
{
    if (!s)
        return NULL;
    return cco_string_new_len(s, strlen(s));
}

cco_object_t* cco_string_new_len(const char* s, size_t n)
{
    cco_object_t* obj = cco_object_create(CCO_TYPE_STRING);
    if (!obj)
        return NULL;
    obj->as.string.data = (char*)malloc(n + 1);
    if (!obj->as.string.data)
    {
        cco_object_release(obj);
        return NULL;
    }
    if (s)
        memcpy(obj->as.string.data, s, n);
    obj->as.string.data[n] = '\0';
    obj->as.string.length = n;
    return obj;
}

cco_array_t* cco_array_new(void)
{
    cco_array_t* a = (cco_array_t*)calloc(1, sizeof(cco_array_t));
    return a;
}

cco_object_t* cco_array_wrap(cco_array_t* arr)
{
    if (!arr)
        return NULL;
    cco_object_t* obj = cco_object_create(CCO_TYPE_ARRAY);
    if (!obj)
    {
        for (size_t i = 0; i < arr->count; i++)
        {
            cco_object_release(arr->items[i]);
        }
        free(arr->items);
        free(arr);
        return NULL;
    }
    obj->as.array = *arr;
    free(arr);
    return obj;
}

/* --- Refcount & Lifetime --- */

void cco_object_free(cco_object_t* obj)
{
    cco_object_release(obj);
}

/* --- Inspection & Getters --- */

cco_type_t cco_object_type(const cco_object_t* obj)
{
    if (!obj)
        return CCO_TYPE_NONE;
    return (cco_type_t)obj->type;
}

int cco_object_get_type(const cco_object_t* obj)
{
    if (!obj)
        return 0;
    return (int)obj->type;
}

int64_t cco_int_get(const cco_object_t* obj)
{
    if (!obj || obj->type != CCO_TYPE_INTEGER)
    {
        cco_diag_record(CCO_ERR_TYPE_MISMATCH, 0, 0, "cco_int_get", "expected integer");
        return 0;
    }
    return obj->as.integer;
}

double cco_float_get(const cco_object_t* obj)
{
    if (!obj || obj->type != CCO_TYPE_FLOAT)
    {
        cco_diag_record(CCO_ERR_TYPE_MISMATCH, 0, 0, "cco_float_get", "expected float");
        return 0.0;
    }
    return obj->as.floating;
}

bool cco_bool_get(const cco_object_t* obj)
{
    if (!obj || obj->type != CCO_TYPE_BOOLEAN)
    {
        cco_diag_record(CCO_ERR_TYPE_MISMATCH, 0, 0, "cco_bool_get", "expected boolean");
        return false;
    }
    return obj->as.boolean;
}

const char* cco_string_get(const cco_object_t* obj, size_t* len_out)
{
    if (!obj || obj->type != CCO_TYPE_STRING)
    {
        cco_diag_record(CCO_ERR_TYPE_MISMATCH, 0, 0, "cco_string_get", "expected string");
        if (len_out)
            *len_out = 0;
        return NULL;
    }
    if (len_out)
        *len_out = obj->as.string.length;
    return obj->as.string.data ? obj->as.string.data : "";
}

bool cco_object_get_boolean(const cco_object_t* obj, bool* out_val)
{
    if (!obj || obj->type != CCO_TYPE_BOOLEAN)
        return false;
    if (out_val)
        *out_val = obj->as.boolean;
    return true;
}

bool cco_object_get_integer(const cco_object_t* obj, int64_t* out_val)
{
    if (!obj || obj->type != CCO_TYPE_INTEGER)
        return false;
    if (out_val)
        *out_val = obj->as.integer;
    return true;
}

bool cco_object_get_float(const cco_object_t* obj, double* out_val)
{
    if (!obj || obj->type != CCO_TYPE_FLOAT)
        return false;
    if (out_val)
        *out_val = obj->as.floating;
    return true;
}

bool cco_object_get_string(const cco_object_t* obj, const char** out_val, size_t* out_len)
{
    if (!obj || obj->type != CCO_TYPE_STRING)
        return false;
    if (out_val)
        *out_val = obj->as.string.data;
    if (out_len)
        *out_len = obj->as.string.length;
    return true;
}

/* --- Map Operations --- */

int cco_object_set(cco_object_t* obj, const char* key, cco_object_t* value)
{
    if (!obj || obj->type != CCO_TYPE_MAP || !key || !value)
        return CCO_ERR_INVALID_ARG;
    cco_error_t err = cco_map_set(obj, key, value);
    cco_object_release(value); /* ownership transferred */
    return (int)err;
}

cco_object_t* cco_object_get(const cco_object_t* obj, const char* key)
{
    if (!obj || !key)
        return NULL;
    if (obj->type == CCO_TYPE_MAP)
    {
        return cco_map_get_by_key(obj, key);
    }
    if (obj->type == CCO_TYPE_TEMPLATE_INSTANCE)
    {
        for (size_t i = 0; i < obj->as.tmpl_inst.fields.count; i++)
        {
            if (obj->as.tmpl_inst.fields.keys[i] && strcmp(obj->as.tmpl_inst.fields.keys[i], key) == 0)
            {
                return obj->as.tmpl_inst.fields.values[i];
            }
        }
    }
    return NULL;
}

int cco_object_has_key(const cco_object_t* obj, const char* key)
{
    return cco_object_get(obj, key) != NULL ? 1 : 0;
}

int cco_object_del(cco_object_t* obj, const char* key)
{
    if (!obj || obj->type != CCO_TYPE_MAP || !key)
        return CCO_ERR_INVALID_ARG;
    for (size_t i = 0; i < obj->as.map.count; i++)
    {
        if (obj->as.map.keys[i] && strcmp(obj->as.map.keys[i], key) == 0)
        {
            free(obj->as.map.keys[i]);
            cco_object_release(obj->as.map.values[i]);
            for (size_t j = i; j + 1 < obj->as.map.count; j++)
            {
                obj->as.map.keys[j] = obj->as.map.keys[j + 1];
                obj->as.map.values[j] = obj->as.map.values[j + 1];
                if (obj->as.map.hashes)
                    obj->as.map.hashes[j] = obj->as.map.hashes[j + 1];
            }
            obj->as.map.count--;
            return CCO_OK;
        }
    }
    return CCO_ERR_NOT_FOUND;
}

size_t cco_object_size(const cco_object_t* obj)
{
    if (!obj)
        return 0;
    if (obj->type == CCO_TYPE_MAP)
        return obj->as.map.count;
    if (obj->type == CCO_TYPE_ARRAY)
        return obj->as.array.count;
    if (obj->type == CCO_TYPE_TEMPLATE_INSTANCE)
        return obj->as.tmpl_inst.fields.count;
    return 0;
}

size_t cco_map_get_count(const cco_object_t* obj)
{
    if (!obj || obj->type != CCO_TYPE_MAP)
        return 0;
    return obj->as.map.count;
}

const char* cco_map_get_key(const cco_object_t* obj, size_t index)
{
    if (!obj || obj->type != CCO_TYPE_MAP || index >= obj->as.map.count)
        return NULL;
    return obj->as.map.keys[index];
}

cco_object_t* cco_map_get_value(const cco_object_t* obj, size_t index)
{
    if (!obj || obj->type != CCO_TYPE_MAP || index >= obj->as.map.count)
        return NULL;
    return obj->as.map.values[index];
}

cco_object_t* cco_map_get_by_key(const cco_object_t* obj, const char* key)
{
    if (!obj || obj->type != CCO_TYPE_MAP || !key)
        return NULL;
    for (size_t i = 0; i < obj->as.map.count; i++)
    {
        if (obj->as.map.keys[i] && strcmp(obj->as.map.keys[i], key) == 0)
        {
            return obj->as.map.values[i];
        }
    }
    return NULL;
}

/* --- Array Operations --- */

int cco_array_add(cco_array_t* arr, cco_object_t* value)
{
    if (!arr || !value)
        return CCO_ERR_INVALID_ARG;
    if (arr->count >= arr->capacity)
    {
        size_t new_cap = arr->capacity == 0 ? 8 : arr->capacity * 2;
        cco_object_t** new_items = (cco_object_t**)realloc(arr->items, new_cap * sizeof(cco_object_t*));
        if (!new_items)
            return CCO_ERR_NO_MEMORY;
        arr->items = new_items;
        arr->capacity = new_cap;
    }
    arr->items[arr->count++] = value; /* ownership transferred */
    return CCO_OK;
}

cco_object_t* cco_array_get(const cco_array_t* arr, size_t index)
{
    if (!arr || index >= arr->count)
        return NULL;
    return arr->items[index];
}

size_t cco_array_size(const cco_array_t* arr)
{
    if (!arr)
        return 0;
    return arr->count;
}

size_t cco_array_get_count(const cco_object_t* obj)
{
    if (!obj || obj->type != CCO_TYPE_ARRAY)
        return 0;
    return obj->as.array.count;
}

cco_object_t* cco_array_get_item(const cco_object_t* obj, size_t index)
{
    if (!obj || obj->type != CCO_TYPE_ARRAY || index >= obj->as.array.count)
        return NULL;
    return obj->as.array.items[index];
}

/* --- Deep Copy, Equality & Metadata --- */

bool cco_object_equal(const cco_object_t* a, const cco_object_t* b)
{
    return cco_object_equals(a, b);
}

const char* cco_object_template_name(const cco_object_t* obj)
{
    if (!obj || obj->type != CCO_TYPE_TEMPLATE_INSTANCE)
        return NULL;
    return obj->as.tmpl_inst.template_name;
}

/* --- Parsing APIs --- */

cco_object_t* cco_parse_string(const char* src, size_t len,
                               const cco_parse_options_t* opts)
{
    if (!src)
        return NULL;

    cco_parse_options_t default_opts;
    if (!opts)
    {
        cco_parse_options_init(&default_opts);
        opts = &default_opts;
    }

    cco_diag_clear();

    cco_object_t* root = NULL;
#ifdef CCO_ENABLE_ZERO_COPY
    if (opts->zero_copy_dom)
    {
        cco_parser_context_t ctx;
        cco_arena_t* dom = opts->custom_arena;
        bool own_dom = false;
        if (!dom)
        {
            dom = (cco_arena_t*)malloc(sizeof(cco_arena_t));
            cco_arena_init(dom, opts->max_total_allocation);
            own_dom = true;
        }
        cco_set_active_dom_arena(dom);
        cco_parser_init(&ctx, src, len, opts, dom);
        root = cco_parse_document(&ctx);
        if (root)
        {
            root->dom_arena = dom;
            root->flags |= 1; /* arena node */
            if (!own_dom)
                root->flags |= 2; /* borrowed arena */
        }
        else
        {
            if (own_dom)
            {
                cco_arena_destroy(dom);
                free(dom);
            }
        }
        cco_set_active_dom_arena(NULL);
        return root;
    }
#endif

    cco_arena_t arena;
    cco_arena_init(&arena, opts->max_total_allocation);
    cco_parser_context_t ctx;
    cco_parser_init(&ctx, src, len, opts, &arena);
    root = cco_parse_document(&ctx);
    cco_arena_destroy(&arena);
    return root;
}

cco_object_t* cco_parse(const char* text)
{
    if (!text)
        return NULL;
    return cco_parse_string(text, strlen(text), NULL);
}

static char* read_file_to_string(const char* filename, size_t* len_out)
{
    FILE* f = fopen(filename, "rb");
    if (!f)
        return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz < 0)
    {
        fclose(f);
        return NULL;
    }
    char* buf = (char*)malloc(sz + 1);
    if (!buf)
    {
        fclose(f);
        return NULL;
    }
    size_t rd = fread(buf, 1, sz, f);
    buf[rd] = '\0';
    fclose(f);
    if (len_out)
        *len_out = rd;
    return buf;
}

cco_object_t* cco_parse_from_file(const char* filename)
{
    if (!filename)
        return NULL;
    cco_parse_options_t opts = cco_parse_options_default();
    int chk = cco_sandbox_check_path(&opts, filename);
    if (chk != CCO_OK)
        return NULL;
    size_t len = 0;
    char* text = read_file_to_string(filename, &len);
    if (!text)
        return NULL;
    cco_object_t* obj = cco_parse_string(text, len, &opts);
    free(text);
    return obj;
}

cco_parse_result_t* cco_parse_full_with_options(const char* text, const cco_parse_options_t* opts)
{
    if (!text)
        return NULL;
    cco_parse_options_t default_opts;
    if (!opts)
    {
        cco_parse_options_init(&default_opts);
        opts = &default_opts;
    }
    cco_diag_clear();

    cco_arena_t* arena = (cco_arena_t*)malloc(sizeof(cco_arena_t));
    if (!arena)
        return NULL;
    cco_arena_init(arena, opts->max_total_allocation);

    cco_parser_context_t ctx;
    cco_parser_init(&ctx, text, strlen(text), opts, arena);
    cco_symtab_t* symtab = cco_symtab_new();
    ctx.symtab = symtab;
    ctx.owns_symtab = false;

    cco_object_t* root = cco_parse_document(&ctx);
    if (!root)
    {
        cco_diag_record(CCO_ERR_PARSE, 1, 1, "cco_parse_full", "no root expression");
        cco_symtab_free(symtab);
        cco_arena_destroy(arena);
        free(arena);
        return NULL;
    }

    cco_parse_result_t* res = (cco_parse_result_t*)calloc(1, sizeof(cco_parse_result_t));
    if (!res)
    {
        cco_object_release(root);
        cco_symtab_free(symtab);
        cco_arena_destroy(arena);
        free(arena);
        return NULL;
    }
    res->root = root;
    res->symtab = symtab;
    res->arena = arena;
    return res;
}

cco_parse_result_t* cco_parse_full(const char* text)
{
    return cco_parse_full_with_options(text, NULL);
}

cco_parse_result_t* cco_parse_full_from_file_with_options(const char* filename, const cco_parse_options_t* opts)
{
    if (!filename)
        return NULL;
    cco_parse_options_t default_opts;
    if (!opts)
    {
        cco_parse_options_init(&default_opts);
        opts = &default_opts;
    }
    int chk = cco_sandbox_check_path(opts, filename);
    if (chk != CCO_OK)
        return NULL;
    size_t len = 0;
    char* text = read_file_to_string(filename, &len);
    if (!text)
        return NULL;
    cco_parse_result_t* res = cco_parse_full_with_options(text, opts);
    free(text);
    return res;
}

cco_parse_result_t* cco_parse_full_from_file(const char* filename)
{
    return cco_parse_full_from_file_with_options(filename, NULL);
}

cco_parse_result_t* cco_parse_full_from_files(const char** paths, size_t count)
{
    if (!paths || count == 0)
        return NULL;
    cco_parse_options_t opts = cco_parse_options_default();
    cco_arena_t* arena = (cco_arena_t*)malloc(sizeof(cco_arena_t));
    if (!arena)
        return NULL;
    cco_arena_init(arena, opts.max_total_allocation);

    cco_symtab_t* symtab = cco_symtab_new();
    cco_object_t* last_root = NULL;

    for (size_t i = 0; i < count; i++)
    {
        if (!paths[i])
            continue;
        int chk = cco_sandbox_check_path(&opts, paths[i]);
        if (chk != CCO_OK)
        {
            if (last_root)
                cco_object_release(last_root);
            cco_symtab_free(symtab);
            cco_arena_destroy(arena);
            free(arena);
            return NULL;
        }
        size_t len = 0;
        char* text = read_file_to_string(paths[i], &len);
        if (!text)
        {
            if (last_root)
                cco_object_release(last_root);
            cco_symtab_free(symtab);
            cco_arena_destroy(arena);
            free(arena);
            return NULL;
        }
        cco_parser_context_t ctx;
        cco_parser_init(&ctx, text, len, &opts, arena);
        ctx.symtab = symtab;
        ctx.owns_symtab = false;
        cco_object_t* root = cco_parse_document(&ctx);
        free(text);
        if (root)
        {
            if (last_root)
                cco_object_release(last_root);
            last_root = root;
        }
    }

    cco_parse_result_t* res = (cco_parse_result_t*)calloc(1, sizeof(cco_parse_result_t));
    if (!res)
    {
        if (last_root)
            cco_object_release(last_root);
        cco_symtab_free(symtab);
        cco_arena_destroy(arena);
        free(arena);
        return NULL;
    }
    res->root = last_root;
    res->symtab = symtab;
    res->arena = arena;
    return res;
}

cco_parse_result_t* cco_parse_full_froms(const char** texts, size_t count)
{
    if (!texts || count == 0)
        return NULL;
    cco_parse_options_t opts = cco_parse_options_default();
    cco_arena_t* arena = (cco_arena_t*)malloc(sizeof(cco_arena_t));
    if (!arena)
        return NULL;
    cco_arena_init(arena, opts.max_total_allocation);

    cco_symtab_t* symtab = cco_symtab_new();
    cco_object_t* last_root = NULL;

    for (size_t i = 0; i < count; i++)
    {
        if (!texts[i])
            continue;
        cco_parser_context_t ctx;
        cco_parser_init(&ctx, texts[i], strlen(texts[i]), &opts, arena);
        ctx.symtab = symtab;
        ctx.owns_symtab = false;
        cco_object_t* root = cco_parse_document(&ctx);
        if (root)
        {
            if (last_root)
                cco_object_release(last_root);
            last_root = root;
        }
    }

    cco_parse_result_t* res = (cco_parse_result_t*)calloc(1, sizeof(cco_parse_result_t));
    if (!res)
    {
        if (last_root)
            cco_object_release(last_root);
        cco_symtab_free(symtab);
        cco_arena_destroy(arena);
        free(arena);
        return NULL;
    }
    res->root = last_root;
    res->symtab = symtab;
    res->arena = arena;
    return res;
}

cco_symbol_table_t* cco_parse_result_symbols(cco_parse_result_t* res)
{
    if (!res)
        return NULL;
    return (cco_symbol_table_t*)res->symtab;
}

cco_object_t* cco_parse_result_root(cco_parse_result_t* res)
{
    if (!res)
        return NULL;
    return res->root;
}

void cco_parse_result_free(cco_parse_result_t* res)
{
    if (!res)
        return;
    if (res->root)
    {
        cco_object_release(res->root);
        res->root = NULL;
    }
    if (res->symtab)
    {
        cco_symtab_free(res->symtab);
        res->symtab = NULL;
    }
    if (res->arena)
    {
        cco_arena_destroy(res->arena);
        free(res->arena);
        res->arena = NULL;
    }
    free(res);
}

/* --- Symbols & Templates --- */

size_t cco_symbol_table_count(const cco_symbol_table_t* st)
{
    if (!st)
        return 0;
    const cco_symtab_t* sym = (const cco_symtab_t*)st;
    return sym->templates.count;
}

const char* cco_symbol_table_name(const cco_symbol_table_t* st, size_t idx)
{
    if (!st)
        return NULL;
    const cco_symtab_t* sym = (const cco_symtab_t*)st;
    if (idx >= sym->templates.count)
        return NULL;
    return sym->templates.entries[idx].name;
}

const cco_template_t* cco_symbol_table_template(const cco_symbol_table_t* st, const char* name)
{
    if (!st || !name)
        return NULL;
    return (const cco_template_t*)cco_symtab_lookup_template((cco_symtab_t*)st, name);
}

const char* cco_template_get_parent_name(const cco_template_t* t)
{
    if (!t)
        return NULL;
    const cco_template_def_t* def = (const cco_template_def_t*)t;
    return def->parent_name;
}

size_t cco_template_field_count(const cco_template_t* t)
{
    if (!t)
        return 0;
    const cco_template_def_t* def = (const cco_template_def_t*)t;
    return def->field_count;
}

const char* cco_template_field_name(const cco_template_t* t, size_t idx)
{
    if (!t)
        return NULL;
    const cco_template_def_t* def = (const cco_template_def_t*)t;
    if (idx >= def->field_count)
        return NULL;
    return def->fields[idx].name;
}

/* --- Serialization --- */

char* cco_serialize_to_string(const cco_object_t* obj, bool pretty)
{
    if (!obj)
        return NULL;

    cco_arena_t arena;
    cco_arena_init(&arena, 0);

    cco_strbuf_t buf;
    cco_strbuf_init(&buf, &arena);

    cco_error_t err = cco_serialize_buffer(obj, &buf, pretty, 2);

    char* result = NULL;
    if (err == CCO_OK && buf.data)
    {
        result = (char*)malloc(buf.length + 1);
        if (result)
        {
            memcpy(result, buf.data, buf.length);
            result[buf.length] = '\0';
        }
    }

    cco_arena_destroy(&arena);
    return result;
}

char* cco_serialize(const cco_object_t* obj)
{
    return cco_serialize_to_string(obj, false);
}

char* cco_serialize_pretty(const cco_object_t* obj, int indent)
{
    if (!obj)
        return NULL;
    cco_arena_t arena;
    cco_arena_init(&arena, 0);
    cco_strbuf_t buf;
    cco_strbuf_init(&buf, &arena);
    cco_error_t err = cco_serialize_buffer(obj, &buf, true, indent > 0 ? indent : 2);
    char* result = NULL;
    if (err == CCO_OK && buf.data)
    {
        result = (char*)malloc(buf.length + 1);
        if (result)
        {
            memcpy(result, buf.data, buf.length);
            result[buf.length] = '\0';
        }
    }
    cco_arena_destroy(&arena);
    return result;
}

char* cco_serialize_full(const cco_parse_result_t* result)
{
    if (!result)
        return NULL;
    cco_arena_t arena;
    cco_arena_init(&arena, 0);
    cco_strbuf_t buf;
    cco_strbuf_init(&buf, &arena);

    if (result->symtab)
    {
        const cco_symtab_t* st = (const cco_symtab_t*)result->symtab;
        for (size_t i = 0; i < st->typedefs.count; i++)
        {
            cco_strbuf_append(&buf, "$typedef.", 9);
            cco_strbuf_append(&buf, st->typedefs.entries[i].name, strlen(st->typedefs.entries[i].name));
            cco_strbuf_append(&buf, ": ", 2);
            cco_strbuf_append(&buf, st->typedefs.entries[i].target, strlen(st->typedefs.entries[i].target));
            cco_strbuf_append(&buf, "\n", 1);
        }
        for (size_t i = 0; i < st->templates.count; i++)
        {
            cco_strbuf_append(&buf, "$temp.", 6);
            cco_strbuf_append(&buf, st->templates.entries[i].name, strlen(st->templates.entries[i].name));
            if (st->templates.entries[i].def && st->templates.entries[i].def->parent_name)
            {
                cco_strbuf_append(&buf, " : ", 3);
                cco_strbuf_append(&buf, st->templates.entries[i].def->parent_name, strlen(st->templates.entries[i].def->parent_name));
            }
            cco_strbuf_append(&buf, ": (", 3);
            if (st->templates.entries[i].def)
            {
                for (size_t f = 0; f < st->templates.entries[i].def->field_count; f++)
                {
                    if (f > 0)
                        cco_strbuf_append(&buf, ", ", 2);
                    cco_strbuf_append(&buf, st->templates.entries[i].def->fields[f].name, strlen(st->templates.entries[i].def->fields[f].name));
                    if (st->templates.entries[i].def->fields[f].type_name)
                    {
                        cco_strbuf_append(&buf, ": ", 2);
                        cco_strbuf_append(&buf, st->templates.entries[i].def->fields[f].type_name, strlen(st->templates.entries[i].def->fields[f].type_name));
                    }
                }
            }
            cco_strbuf_append(&buf, ")\n", 2);
        }
    }

    if (result->root)
    {
        cco_serialize_buffer(result->root, &buf, true, 2);
        cco_strbuf_append(&buf, "\n", 1);
    }

    char* res_str = NULL;
    if (buf.data)
    {
        res_str = (char*)malloc(buf.length + 1);
        if (res_str)
        {
            memcpy(res_str, buf.data, buf.length);
            res_str[buf.length] = '\0';
        }
    }
    cco_arena_destroy(&arena);
    return res_str;
}

int cco_serialize_into(char* restrict out, size_t cap, const cco_object_t* restrict obj)
{
    if (!out || cap == 0 || !obj)
        return CCO_ERR_INVALID_ARG;
    char* s = cco_serialize(obj);
    if (!s)
        return CCO_ERR_NO_MEMORY;
    size_t slen = strlen(s);
    if (slen >= cap)
    {
        free(s);
        return CCO_ERR_OUT_OF_RANGE;
    }
    memcpy(out, s, slen + 1);
    free(s);
    return (int)slen;
}

int cco_save_file_pretty(const cco_object_t* obj, const char* filename, int indent)
{
    if (!obj || !filename)
        return CCO_ERR_INVALID_ARG;
    cco_parse_options_t opts = cco_parse_options_default();
    int chk = cco_sandbox_check_path(&opts, filename);
    if (chk != CCO_OK)
        return chk;

    char* s = cco_serialize_pretty(obj, indent);
    if (!s)
        return CCO_ERR_NO_MEMORY;

    FILE* f = fopen(filename, "wb");
    if (!f)
    {
        free(s);
        return CCO_ERR_IO;
    }
    fputs(s, f);
    fclose(f);
    free(s);
    return CCO_OK;
}

/* --- Expressions --- */

cco_expr_t* cco_expr_literal(cco_object_t* value)
{
    cco_expr_t* e = (cco_expr_t*)calloc(1, sizeof(cco_expr_t));
    if (!e)
        return NULL;
    e->type = CCO_EXPR_LITERAL;
    e->as.literal = value; /* transfer ownership */
    return e;
}

cco_expr_t* cco_expr_identifier(const char* name)
{
    if (!name)
        return NULL;
    return cco_expr_new_identifier(name, strlen(name));
}

cco_expr_t* cco_expr_binary(cco_expr_t* lhs, cco_op_t op, cco_expr_t* rhs)
{
    return cco_expr_new_binary(op, lhs, rhs);
}

cco_expr_t* cco_expr_unary(cco_op_t op, cco_expr_t* operand)
{
    return cco_expr_new_unary(op, operand);
}

cco_expr_t* cco_expr_coalesce(cco_expr_t* left, cco_expr_t* right)
{
    return cco_expr_new_binary(CCO_OP_COALESCE, left, right);
}

cco_expr_t* cco_expr_compound(cco_expr_t* inner)
{
    return inner;
}

cco_expr_t* cco_expr_format(cco_expr_t* string_expr)
{
    cco_expr_t* e = (cco_expr_t*)calloc(1, sizeof(cco_expr_t));
    if (!e)
    {
        cco_expr_free(string_expr);
        return NULL;
    }
    e->type = CCO_EXPR_CALL;
    e->as.call.name = strdup("$format");
    e->as.call.arg_count = 1;
    e->as.call.args = (cco_expr_t**)malloc(sizeof(cco_expr_t*));
    if (e->as.call.args)
    {
        e->as.call.args[0] = string_expr;
    }
    else
    {
        cco_expr_free(string_expr);
    }
    return e;
}

cco_expr_t* cco_expr_env(const char* var_name)
{
    cco_expr_t* e = (cco_expr_t*)calloc(1, sizeof(cco_expr_t));
    if (!e)
        return NULL;
    e->type = CCO_EXPR_CALL;
    e->as.call.name = strdup("$env");
    e->as.call.arg_count = 1;
    e->as.call.args = (cco_expr_t**)malloc(sizeof(cco_expr_t*));
    if (e->as.call.args)
    {
        e->as.call.args[0] = cco_expr_identifier(var_name);
    }
    return e;
}

cco_object_t* cco_expr_eval(const cco_expr_t* expr, cco_symbol_table_t* st, cco_object_t* self, cco_object_t* locals)
{
    if (!expr)
        return NULL;
    if (expr->type == CCO_EXPR_IDENTIFIER)
    {
        const char* name = expr->as.identifier.name;
        if (locals && locals->type == CCO_TYPE_MAP)
        {
            cco_object_t* v = cco_map_get_by_key(locals, name);
            if (v)
                return cco_object_clone(v);
        }
        if (self && self->type == CCO_TYPE_MAP)
        {
            cco_object_t* v = cco_map_get_by_key(self, name);
            if (v)
                return cco_object_clone(v);
        }
        if (st)
        {
            cco_template_def_t* td = cco_symtab_lookup_template((cco_symtab_t*)st, name);
            if (td)
            {
                cco_object_t* obj = cco_object_create(CCO_TYPE_TEMPLATE_INSTANCE);
                if (obj)
                    obj->as.tmpl_inst.template_name = strdup(name);
                return obj;
            }
        }
        return cco_object_create(CCO_TYPE_NONE);
    }
    return cco_eval_expr(NULL, (cco_expr_t*)expr);
}

/* --- Instantiation --- */

cco_object_t* cco_instantiate(cco_symbol_table_t* st, const char* name, cco_object_t** args, size_t arg_count)
{
    if (!st || !name)
        return NULL;
    cco_array_t args_arr;
    args_arr.items = args;
    args_arr.count = arg_count;
    args_arr.capacity = arg_count;
    cco_object_t* out_obj = NULL;
    cco_error_t err = cco_instantiate_positional(NULL, (cco_symtab_t*)st, NULL, name, &args_arr, &out_obj);
    if (err != CCO_OK)
    {
        return NULL;
    }
    return out_obj;
}

/* --- Diagnostics --- */

static _Thread_local cco_diag_t t_public_diags[CCO_DIAG_MAX_COUNT];

int cco_diag_count(void)
{
    size_t count = 0;
    cco_diag_get_all(&count);
    return (int)count;
}

const cco_diag_t* cco_diag_get(int i)
{
    size_t count = 0;
    const cco_diag_entry_t* entries = cco_diag_get_all(&count);
    if (i < 0 || (size_t)i >= count || !entries)
        return NULL;
    t_public_diags[i].file = "";
    t_public_diags[i].line = entries[i].line;
    t_public_diags[i].col = entries[i].col;
    t_public_diags[i].message = entries[i].message;
    t_public_diags[i].is_error = (entries[i].code != CCO_OK);
    return &t_public_diags[i];
}

void cco_diag_print(const cco_diag_t* diag, const char* source)
{
    (void)source;
    if (!diag)
        return;
    fprintf(stderr, "%sline %zu, col %zu: %s\n",
            (diag->file && diag->file[0]) ? diag->file : "",
            diag->line, diag->col,
            diag->message ? diag->message : "error");
}

void cco_diag_print_all(const char* source)
{
    int cnt = cco_diag_count();
    for (int i = 0; i < cnt; i++)
    {
        const cco_diag_t* d = cco_diag_get(i);
        cco_diag_print(d, source);
    }
}

void cco_clear_diagnostics(void)
{
    cco_diag_clear();
}

int cco_last_error(void)
{
    return (int)cco_diag_get_last_error();
}

cco_error_t cco_get_last_error(void)
{
    return cco_diag_get_last_error();
}
