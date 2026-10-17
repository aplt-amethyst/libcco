#include "serializer.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

static cco_error_t serialize_string(const char* str, size_t len,
                                    cco_serializer_context_t* ctx)
{
    cco_error_t err = cco_strbuf_append(ctx->out, "\"", 1);
    if (err != CCO_OK)
        return err;

    size_t start = 0;
    for (size_t i = 0; i < len; i++)
    {
        char c = str[i];
        const char* esc = NULL;
        size_t esc_len = 0;
        char hex_buf[8];

        switch (c)
        {
        case '\n': esc = "\\n"; esc_len = 2; break;
        case '\t': esc = "\\t"; esc_len = 2; break;
        case '\r': esc = "\\r"; esc_len = 2; break;
        case '\\': esc = "\\\\"; esc_len = 2; break;
        case '"':  esc = "\\\""; esc_len = 2; break;
        default:
            if ((unsigned char)c < 32)
            {
                snprintf(hex_buf, sizeof(hex_buf), "\\x%02X", (unsigned char)c);
                esc = hex_buf;
                esc_len = 4;
            }
            break;
        }

        if (esc)
        {
            if (i > start)
            {
                err = cco_strbuf_append(ctx->out, str + start, i - start);
                if (err != CCO_OK)
                    return err;
            }
            err = cco_strbuf_append(ctx->out, esc, esc_len);
            if (err != CCO_OK)
                return err;
            start = i + 1;
        }
    }

    if (len > start)
    {
        err = cco_strbuf_append(ctx->out, str + start, len - start);
        if (err != CCO_OK)
            return err;
    }

    return cco_strbuf_append(ctx->out, "\"", 1);
}

static void print_indent(cco_serializer_context_t* ctx)
{
    if (!ctx->pretty || ctx->current_indent <= 0)
        return;
    static const char spaces[65] = "                                                                ";
    int remaining = ctx->current_indent;
    while (remaining > 0)
    {
        int chunk = remaining > 64 ? 64 : remaining;
        cco_strbuf_append(ctx->out, spaces, (size_t)chunk);
        remaining -= chunk;
    }
}

static inline size_t fast_i64toa(int64_t v, char* buf)
{
    if (v == 0)
    {
        buf[0] = '0';
        buf[1] = '\0';
        return 1;
    }
    char temp[32];
    int p = 0;
    bool neg = false;
    uint64_t uv;
    if (v < 0)
    {
        neg = true;
        uv = (uint64_t)(-(v + 1)) + 1;
    }
    else
    {
        uv = (uint64_t)v;
    }
    while (uv > 0)
    {
        temp[p++] = (char)('0' + (uv % 10));
        uv /= 10;
    }
    size_t out = 0;
    if (neg)
        buf[out++] = '-';
    while (p > 0)
    {
        buf[out++] = temp[--p];
    }
    buf[out] = '\0';
    return out;
}

static bool is_valid_unquoted_key(const char* key, size_t len)
{
    if (!key || len == 0)
        return false;

    size_t seg_start = 0;
    while (seg_start < len)
    {
        size_t seg_end = seg_start;
        while (seg_end < len && key[seg_end] != '.')
        {
            seg_end++;
        }
        size_t seg_len = seg_end - seg_start;
        if (seg_len == 0)
            return false; /* Leading dot, consecutive dot, or trailing dot */

        char c0 = key[seg_start];
        if (!((c0 >= 'a' && c0 <= 'z') || (c0 >= 'A' && c0 <= 'Z') || c0 == '_'))
            return false;

        for (size_t i = seg_start + 1; i < seg_end; i++)
        {
            char ch = key[i];
            if (!((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
                  (ch >= '0' && ch <= '9') || ch == '_'))
            {
                return false;
            }
        }

        if ((seg_len == 4 && (memcmp(key + seg_start, "true", 4) == 0 ||
                              memcmp(key + seg_start, "null", 4) == 0 ||
                              memcmp(key + seg_start, "None", 4) == 0)) ||
            (seg_len == 5 && memcmp(key + seg_start, "false", 5) == 0) ||
            (seg_len == 7 && memcmp(key + seg_start, "$return", 7) == 0) ||
            (seg_len == 5 && memcmp(key + seg_start, "$this", 5) == 0))
        {
            return false;
        }

        if (seg_end < len && key[seg_end] == '.')
        {
            seg_start = seg_end + 1;
            if (seg_start == len)
                return false; /* Trailing dot */
        }
        else
        {
            break;
        }
    }
    return true;
}

static cco_error_t serialize_recursive_inner(const cco_object_t* obj,
                                             cco_serializer_context_t* ctx);

static cco_error_t serialize_recursive(const cco_object_t* obj,
                                       cco_serializer_context_t* ctx)
{
    if (!obj)
        return CCO_ERR_INVALID_ARG;
    if (ctx->depth > 1000)
        return CCO_ERR_OUT_OF_RANGE;
    ctx->depth++;
    cco_error_t err = serialize_recursive_inner(obj, ctx);
    ctx->depth--;
    return err;
}

static cco_error_t serialize_recursive_inner(const cco_object_t* obj,
                                             cco_serializer_context_t* ctx)
{
    if (!obj)
        return CCO_ERR_INVALID_ARG;
    cco_error_t err = CCO_OK;

    switch (obj->type)
    {
    case CCO_TYPE_NONE:
        err = cco_strbuf_append(ctx->out, "None", 4);
        break;
    case CCO_TYPE_BOOLEAN:
        if (obj->as.boolean)
            err = cco_strbuf_append(ctx->out, "true", 4);
        else
            err = cco_strbuf_append(ctx->out, "false", 5);
        break;
    case CCO_TYPE_INTEGER: {
        char num_buf[32];
        size_t n = fast_i64toa(obj->as.integer, num_buf);
        err = cco_strbuf_append(ctx->out, num_buf, n);
        break;
    }
    case CCO_TYPE_FLOAT: {
        char num_buf[64];
        int n = snprintf(num_buf, sizeof(num_buf), "%g", obj->as.floating);
        if (n > 0)
            err = cco_strbuf_append(ctx->out, num_buf, (size_t)n);
        break;
    }
    case CCO_TYPE_STRING:
        err = serialize_string(obj->as.string.data, obj->as.string.length, ctx);
        break;
    case CCO_TYPE_ARRAY:
        if (obj->as.array.count == 0)
        {
            err = cco_strbuf_append(ctx->out, "(,)", 3);
            break;
        }
        err = cco_strbuf_append(ctx->out, "(", 1);
        if (err != CCO_OK)
            return err;
        if (ctx->pretty)
        {
            cco_strbuf_append(ctx->out, "\n", 1);
            ctx->current_indent += ctx->indent_spaces;
        }
        for (size_t i = 0; i < obj->as.array.count; i++)
        {
            if (ctx->pretty)
                print_indent(ctx);
            err = serialize_recursive(obj->as.array.items[i], ctx);
            if (err != CCO_OK)
                return err;
            if (i + 1 < obj->as.array.count)
            {
                err = cco_strbuf_append(ctx->out, ctx->pretty ? ",\n" : ",",
                                        ctx->pretty ? 2 : 1);
                if (err != CCO_OK)
                    return err;
            }
            else if (ctx->pretty)
            {
                cco_strbuf_append(ctx->out, "\n", 1);
            }
        }
        if (ctx->pretty)
        {
            ctx->current_indent -= ctx->indent_spaces;
            print_indent(ctx);
        }
        err = cco_strbuf_append(ctx->out, ")", 1);
        break;
    case CCO_TYPE_MAP:
        err = cco_strbuf_append(ctx->out, "(", 1);
        if (err != CCO_OK)
            return err;
        if (ctx->pretty && obj->as.map.count > 0)
        {
            cco_strbuf_append(ctx->out, "\n", 1);
            ctx->current_indent += ctx->indent_spaces;
        }
        for (size_t i = 0; i < obj->as.map.count; i++)
        {
            if (ctx->pretty)
                print_indent(ctx);
            const char* key = obj->as.map.keys[i];
            size_t klen = key ? strlen(key) : 0;
            bool is_ident = is_valid_unquoted_key(key, klen);
            if (is_ident)
            {
                err = cco_strbuf_append(ctx->out, key, klen);
            }
            else
            {
                err = serialize_string(key ? key : "", klen, ctx);
            }
            if (err != CCO_OK)
                return err;
            err = cco_strbuf_append(ctx->out, ctx->pretty ? ": " : ":",
                                    ctx->pretty ? 2 : 1);
            if (err != CCO_OK)
                return err;
            err = serialize_recursive(obj->as.map.values[i], ctx);
            if (err != CCO_OK)
                return err;
            if (i + 1 < obj->as.map.count)
            {
                err = cco_strbuf_append(ctx->out, ctx->pretty ? ",\n" : ",",
                                        ctx->pretty ? 2 : 1);
                if (err != CCO_OK)
                    return err;
            }
            else if (ctx->pretty)
            {
                cco_strbuf_append(ctx->out, "\n", 1);
            }
        }
        if (ctx->pretty && obj->as.map.count > 0)
        {
            ctx->current_indent -= ctx->indent_spaces;
            print_indent(ctx);
        }
        err = cco_strbuf_append(ctx->out, ")", 1);
        break;
    case CCO_TYPE_TEMPLATE_INSTANCE:
        err = cco_strbuf_append(ctx->out, "#", 1);
        if (err == CCO_OK && obj->as.tmpl_inst.template_name)
        {
            err = cco_strbuf_append(ctx->out, obj->as.tmpl_inst.template_name,
                                    strlen(obj->as.tmpl_inst.template_name));
        }
        if (err == CCO_OK)
        {
            err = cco_strbuf_append(ctx->out, "(", 1);
            if (err == CCO_OK)
            {
                for (size_t i = 0; i < obj->as.tmpl_inst.fields.count; i++)
                {
                    const char* key = obj->as.tmpl_inst.fields.keys[i];
                    size_t klen = key ? strlen(key) : 0;
                    if (klen > 0 && key[0] != '.')
                    {
                        err = cco_strbuf_append(ctx->out, ".", 1);
                        if (err != CCO_OK) break;
                    }
                    err = cco_strbuf_append(ctx->out, key ? key : "", klen);
                    if (err != CCO_OK) break;
                    err = cco_strbuf_append(ctx->out, ctx->pretty ? ": " : ":", ctx->pretty ? 2 : 1);
                    if (err != CCO_OK) break;
                    err = serialize_recursive(obj->as.tmpl_inst.fields.values[i], ctx);
                    if (err != CCO_OK) break;
                    if (i + 1 < obj->as.tmpl_inst.fields.count)
                    {
                        err = cco_strbuf_append(ctx->out, ctx->pretty ? ", " : ",", ctx->pretty ? 2 : 1);
                        if (err != CCO_OK) break;
                    }
                }
                if (err == CCO_OK)
                {
                    err = cco_strbuf_append(ctx->out, ")", 1);
                }
            }
        }
        break;
    default:
        err = CCO_ERR_TYPE_MISMATCH;
        break;
    }
    return err;
}

cco_error_t cco_serialize(const cco_object_t* obj, cco_strbuf_t* out_buf,
                          bool pretty)
{
    if (!obj || !out_buf)
        return CCO_ERR_INVALID_ARG;
    cco_serializer_context_t ctx;
    ctx.pretty = pretty;
    ctx.indent_spaces = 4;
    ctx.current_indent = 0;
    ctx.depth = 0;
    ctx.out = out_buf;

    return serialize_recursive(obj, &ctx);
}
