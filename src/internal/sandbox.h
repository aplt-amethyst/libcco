#ifndef CNT_CCO_INTERNAL_SANDBOX_H
#define CNT_CCO_INTERNAL_SANDBOX_H

#include <stddef.h>

#include <cnt/cco_error.h>
#include <cnt/cco_export.h>
#include <cnt/cco_options.h>

/* Validates if a requested file path is within the allowed base directory.
   Returns CCO_OK if allowed, CCO_ERR_FORBIDDEN otherwise. */
CCO_API int cco_sandbox_check_path(const cco_parse_options_t* opts,
                                   const char* path);

#endif /* CNT_CCO_INTERNAL_SANDBOX_H */
