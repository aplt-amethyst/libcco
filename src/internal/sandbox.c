#define _XOPEN_SOURCE 500
#include "sandbox.h"

#include <stdlib.h>
#include <string.h>

#include "diag.h"

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#define cco_realpath(path, resolved) _fullpath(resolved, path, _MAX_PATH)
#else
#include <limits.h>
#define cco_realpath(path, resolved) realpath(path, resolved)
#endif

int cco_sandbox_check_path(const cco_parse_options_t* opts,
                           const char* path)
{
    if (!opts || !path)
        return CCO_ERR_INVALID_ARG;

    if (!opts->restrict_filesystem)
    {
        return CCO_OK;
    }

    if (!opts->base_dir)
    {
        cco_diag_record(CCO_ERR_FORBIDDEN, 0, 0, "cco_sandbox_check_path",
                        "Filesystem restricted but no base_dir set");
        return CCO_ERR_FORBIDDEN;
    }

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

    char resolved_base[PATH_MAX];
    char resolved_path[PATH_MAX];

    if (!cco_realpath(opts->base_dir, resolved_base))
    {
        cco_diag_record(CCO_ERR_FORBIDDEN, 0, 0, "cco_sandbox_check_path",
                        "Failed to resolve base directory");
        return CCO_ERR_FORBIDDEN;
    }

    if (!cco_realpath(path, resolved_path))
    {
        cco_diag_record(CCO_ERR_FORBIDDEN, 0, 0, "cco_sandbox_check_path",
                        "Failed to resolve target path");
        return CCO_ERR_FORBIDDEN;
    }

    size_t base_len = strlen(resolved_base);

    /* Ensure the target path starts with the resolved base path */
    if (strncmp(resolved_base, resolved_path, base_len) != 0)
    {
        cco_diag_record(CCO_ERR_FORBIDDEN, 0, 0, "cco_sandbox_check_path",
                        "Path escapes sandbox base directory");
        return CCO_ERR_FORBIDDEN;
    }

    /* Ensure it doesn't just share a prefix like /sandbox matching
     * /sandbox_escape */
    if (resolved_path[base_len] != '\0' && resolved_path[base_len] != '/' &&
        resolved_path[base_len] != '\\')
    {
        if (resolved_base[base_len - 1] != '/' &&
            resolved_base[base_len - 1] != '\\')
        {
            cco_diag_record(CCO_ERR_FORBIDDEN, 0, 0, "cco_sandbox_check_path",
                            "Path prefix spoofing detected");
            return CCO_ERR_FORBIDDEN;
        }
    }

    return CCO_OK;
}
