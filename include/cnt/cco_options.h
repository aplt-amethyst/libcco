#ifndef CNT_CCO_OPTIONS_H
#define CNT_CCO_OPTIONS_H

#include <stdbool.h>
#include <stddef.h>
#include "cnt/cco_export.h"

/*
 * Compile-time feature flags recognized by libcco build configuration:
 * CCO_ENABLE_COMMENT_PRESERVE, CCO_ENABLE_CONSTRUCTORS, CCO_ENABLE_ENV,
 * CCO_ENABLE_ERROR_RECOVERY, CCO_ENABLE_EVAL, CCO_ENABLE_FAST,
 * CCO_ENABLE_FORMAT, CCO_ENABLE_INCLUDE, CCO_ENABLE_STATIC_CALLS,
 * CCO_ENABLE_STRICT_SAFETY, CCO_ENABLE_ZERO_COPY.
 */

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct cco_parse_options_s
{
    size_t max_depth;
    size_t max_string_length;
    size_t max_document_size;
    size_t max_total_allocation;
    size_t max_instantiations;
    size_t max_steps;
    struct cco_arena_s* custom_arena;

    bool allow_colon_instantiation;
    bool lenient_brackets;
    bool strict_typing;
    bool restrict_filesystem;
    const char* base_dir;

    bool allow_format_builtin;
    bool allow_eval;
    bool preserve_comments;
    bool enable_error_recovery;
    bool allow_env_builtin;
    bool allow_static_calls;
    bool allow_constructors;
    bool strict_safety_parsing;
    bool allow_include;
    bool fast_mode; /* Bypasses duplicate checks and hashes for extreme DOM parsing */
    bool zero_copy_dom;
} cco_parse_options_t;

/* Initializes options with default values */
CCO_API void cco_parse_options_init(cco_parse_options_t* opts);

#ifdef __cplusplus
}
#endif

#endif /* CNT_CCO_OPTIONS_H */
