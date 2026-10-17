#include "strbuf.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define STRBUF_INIT_CAPACITY 64

void cco_strbuf_init(cco_strbuf_t* sb, cco_arena_t* arena)
{
    if (sb)
    {
        sb->data = NULL;
        sb->length = 0;
        sb->capacity = 0;
        sb->arena = arena;
    }
}

static cco_error_t cco_strbuf_ensure_capacity(cco_strbuf_t* sb, size_t needed)
{
    if (sb->capacity >= needed)
    {
        return CCO_OK;
    }

    size_t new_cap =
        sb->capacity == 0 ? STRBUF_INIT_CAPACITY : sb->capacity * 2;
    if (new_cap < needed)
    {
        new_cap = needed;
    }

    /* Guard capacity against size_t overflow / 2 (as specified in limits) */
    if (new_cap > (SIZE_MAX / 2))
    {
        return CCO_ERR_NO_MEMORY;
    }

    char* new_data = (char*)cco_arena_alloc(sb->arena, new_cap);
    if (!new_data)
    {
        return CCO_ERR_NO_MEMORY;
    }

    if (sb->length > 0 && sb->data)
    {
        memcpy(new_data, sb->data, sb->length);
    }

    /* Notice: Old data is left in the arena (cannot free individually) */
    sb->data = new_data;
    sb->capacity = new_cap;
    return CCO_OK;
}

cco_error_t cco_strbuf_append(cco_strbuf_t* sb, const char* str, size_t len)
{
    if (!sb || !str)
    {
        return CCO_ERR_INVALID_ARG;
    }
    if (len == 0)
    {
        return CCO_OK;
    }

    if (SIZE_MAX - sb->length < len + 1)
    {
        return CCO_ERR_OUT_OF_RANGE;
    }

    if (sb->capacity < sb->length + len + 1)
    {
        cco_error_t err = cco_strbuf_ensure_capacity(sb, sb->length + len + 1);
        if (err != CCO_OK)
        {
            return err;
        }
    }

    /* Append */
    memcpy(sb->data + sb->length, str, len);
    sb->length += len;
    sb->data[sb->length] = '\0';

    return CCO_OK;
}

cco_error_t cco_strbuf_appendf(cco_strbuf_t* sb, const char* fmt, ...)
{
    if (!sb || !fmt)
    {
        return CCO_ERR_INVALID_ARG;
    }

    va_list args1;
    va_start(args1, fmt);
    va_list args2;
    va_copy(args2, args1);

    int needed = vsnprintf(NULL, 0, fmt, args1);
    va_end(args1);

    if (needed < 0)
    {
        va_end(args2);
        return CCO_ERR_INVALID_ARG;
    }

    if (SIZE_MAX - sb->length < (size_t)needed + 1)
    {
        va_end(args2);
        return CCO_ERR_OUT_OF_RANGE;
    }

    cco_error_t err =
        cco_strbuf_ensure_capacity(sb, sb->length + (size_t)needed + 1);
    if (err != CCO_OK)
    {
        va_end(args2);
        return err;
    }

    vsnprintf(sb->data + sb->length, (size_t)needed + 1, fmt, args2);
    sb->length += (size_t)needed;

    va_end(args2);
    return CCO_OK;
}
