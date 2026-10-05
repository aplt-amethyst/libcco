#ifndef CNT_CCO_OPTIONS_H
#define CNT_CCO_OPTIONS_H

#include <stdbool.h>
#include <stddef.h>

typedef struct
{
    size_t max_depth;
    size_t max_string_length;
    size_t max_document_size;
    size_t max_total_allocation;
    size_t max_instantiations;
    size_t max_steps;

    bool allow_colon_instantiation;
    bool lenient_brackets;
    bool strict_typing;
    bool restrict_filesystem;
    const char* base_dir;

#ifdef CCO_ENABLE_FORMAT
    bool allow_format_builtin;
#endif

#ifdef CCO_ENABLE_EVAL
    bool allow_eval;
#endif

#ifdef CCO_ENABLE_COMMENT_PRESERVE
    bool preserve_comments;
#endif

#ifdef CCO_ENABLE_ERROR_RECOVERY
    bool enable_error_recovery;
#endif

#ifdef CCO_ENABLE_ENV
    bool allow_env_builtin;
#endif

#ifdef CCO_ENABLE_STATIC_CALLS
    bool allow_static_calls;
#endif

#ifdef CCO_ENABLE_CONSTRUCTORS
    bool allow_constructors;
#endif

#ifdef CCO_ENABLE_STRICT_SAFETY
    bool strict_safety_parsing;
#endif

#ifdef CCO_ENABLE_INCLUDE
    bool allow_include;
#endif

} cco_parse_options_t;

/* Initializes options with default values */
void cco_parse_options_init(cco_parse_options_t* opts);

#endif /* CNT_CCO_OPTIONS_H */
