#ifndef CNT_CCO_EXPORT_H
#define CNT_CCO_EXPORT_H

/*
 * libcco Symbol Export & Visibility Configuration
 *
 * - When building or consuming static libcco: define CCO_STATIC
 * - When building shared libcco on Windows: define CCO_EXPORTS
 * - When consuming shared libcco on Windows: dllexport is absent -> dllimport
 * - On GCC/Clang with -fvisibility=hidden: CCO_API sets default visibility
 * - CCO_LOCAL marks symbols for internal hidden visibility
 */

#if defined(CCO_STATIC)
    #define CCO_API
    #define CCO_LOCAL
#elif defined(_MSC_VER) || defined(_WIN32) || defined(__CYGWIN__)
    #if defined(CCO_EXPORTS)
        #define CCO_API __declspec(dllexport)
    #else
        #define CCO_API __declspec(dllimport)
    #endif
    #define CCO_LOCAL
#elif defined(__GNUC__) && __GNUC__ >= 4
    #define CCO_API __attribute__((visibility("default")))
    #define CCO_LOCAL __attribute__((visibility("hidden")))
#else
    #define CCO_API
    #define CCO_LOCAL
#endif

/* MSVC C99 restrict compatibility */
#if defined(_MSC_VER) && !defined(restrict)
    #define restrict __restrict
#endif

#endif /* CNT_CCO_EXPORT_H */
