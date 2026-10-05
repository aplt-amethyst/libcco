#include <stdlib.h>
#include <string.h>

#include <cnt/cco.h>

#include "arena.h"
#include "diag.h"
#include "parser.h"
#include "serializer.h"
#include "strbuf.h"

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
#ifdef CCO_ENABLE_FAST
    opts->fast_mode = true;
#endif
#ifdef CCO_ENABLE_FORMAT
    opts->allow_format_builtin = true;
#endif
#ifdef CCO_ENABLE_EVAL
    opts->allow_eval = true;
#endif
#ifdef CCO_ENABLE_ENV
    opts->allow_env_builtin = true;
#endif
#ifdef CCO_ENABLE_STATIC_CALLS
    opts->allow_static_calls = true;
#endif
#ifdef CCO_ENABLE_CONSTRUCTORS
    opts->allow_constructors = true;
#endif
#ifdef CCO_ENABLE_STRICT_SAFETY
    opts->strict_safety_parsing = false;
#endif
#ifdef CCO_ENABLE_INCLUDE
    opts->allow_include = true;
#endif
}

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

    cco_arena_t arena;
    cco_arena_init(&arena, opts->max_total_allocation);

    cco_parser_context_t ctx;
    cco_parser_init(&ctx, src, len, opts, &arena);

    cco_object_t* root = cco_parse_document(&ctx);

    cco_arena_destroy(&arena);
    return root;
}

char* cco_serialize_to_string(const cco_object_t* obj, bool pretty)
{
    if (!obj)
        return NULL;

    cco_arena_t arena;
    cco_arena_init(&arena, 0);

    cco_strbuf_t buf;
    cco_strbuf_init(&buf, &arena);

    cco_error_t err = cco_serialize(obj, &buf, pretty);

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

int cco_object_get_type(const cco_object_t* obj)
{
    if (!obj)
        return 0; /* CCO_TYPE_NONE */
    return (int)obj->type;
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

bool cco_object_get_string(const cco_object_t* obj, const char** out_val,
                           size_t* out_len)
{
    if (!obj || obj->type != CCO_TYPE_STRING)
        return false;
    if (out_val)
        *out_val = obj->as.string.data;
    if (out_len)
        *out_len = obj->as.string.length;
    return true;
}

size_t cco_array_get_count(const cco_object_t* obj)
{
    if (!obj || obj->type != CCO_TYPE_ARRAY || 0)
        return 0;
    return obj->as.array.count;
}

cco_object_t* cco_array_get_item(const cco_object_t* obj, size_t index)
{
    if (!obj || obj->type != CCO_TYPE_ARRAY || 0)
        return NULL;
    if (index >= obj->as.array.count)
        return NULL;
    return obj->as.array.items[index];
}

size_t cco_map_get_count(const cco_object_t* obj)
{
    if (!obj || obj->type != CCO_TYPE_MAP || 0)
        return 0;
    return obj->as.map.count;
}

const char* cco_map_get_key(const cco_object_t* obj, size_t index)
{
    if (!obj || obj->type != CCO_TYPE_MAP || 0)
        return NULL;
    if (index >= obj->as.map.count)
        return NULL;
    return obj->as.map.keys[index];
}

cco_object_t* cco_map_get_value(const cco_object_t* obj, size_t index)
{
    if (!obj || obj->type != CCO_TYPE_MAP || 0)
        return NULL;
    if (index >= obj->as.map.count)
        return NULL;
    return obj->as.map.values[index];
}

cco_object_t* cco_map_get_by_key(const cco_object_t* obj, const char* key)
{
    if (!obj || obj->type != CCO_TYPE_MAP || 0 || !key)
        return NULL;
    for (size_t i = 0; i < obj->as.map.count; i++)
    {
        if (strcmp(obj->as.map.keys[i], key) == 0)
        {
            return obj->as.map.values[i];
        }
    }
    return NULL;
}

cco_error_t cco_get_last_error(void) { return cco_diag_get_last_error(); }

void cco_clear_diagnostics(void) { cco_diag_clear(); }
