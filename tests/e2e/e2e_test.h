#ifndef E2E_TEST_H
#define E2E_TEST_H

#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <time.h>
#include <errno.h>

#include <cnt/cco.h>
#include <cnt/cco_options.h>
#include <cnt/cco_error.h>

/* ANSI Colors for beautiful test reporting */
#define E2E_COLOR_RESET   "\033[0m"
#define E2E_COLOR_BOLD    "\033[1m"
#define E2E_COLOR_RED     "\033[31m"
#define E2E_COLOR_GREEN   "\033[32m"
#define E2E_COLOR_YELLOW  "\033[33m"
#define E2E_COLOR_BLUE    "\033[34m"
#define E2E_COLOR_MAGENTA "\033[35m"
#define E2E_COLOR_CYAN    "\033[36m"



/* E2E Test Runner State */
typedef struct {
    int total;
    int passed;
    int failed;
    int pending;
    int skipped;
    int current_failed;
    bool current_pending;
    const char* current_name;
    const char* current_feature;
    const char* current_milestone;
    const char* filter_feature;
    const char* filter_milestone;
    bool verbose;
} e2e_context_t;

extern e2e_context_t g_e2e_ctx;

/* Test Lifecycle Macros */
#define E2E_START_TEST(name_str, feature_str, milestone_str) \
    do { \
        g_e2e_ctx.total++; \
        g_e2e_ctx.current_failed = 0; \
        g_e2e_ctx.current_pending = false; \
        g_e2e_ctx.current_name = (name_str); \
        g_e2e_ctx.current_feature = (feature_str); \
        g_e2e_ctx.current_milestone = (milestone_str); \
        if (g_e2e_ctx.verbose) { \
            printf("  RUN  [%s] %s (%s)\n", (feature_str), (name_str), (milestone_str)); \
        } \
    } while(0)

#define E2E_END_TEST() \
    do { \
        if (g_e2e_ctx.current_pending) { \
            g_e2e_ctx.pending++; \
            if (g_e2e_ctx.verbose) { \
                printf("  " E2E_COLOR_YELLOW "PENDING" E2E_COLOR_RESET " [%s] %s\n", \
                       g_e2e_ctx.current_feature, g_e2e_ctx.current_name); \
            } \
        } else if (g_e2e_ctx.current_failed == 0) { \
            g_e2e_ctx.passed++; \
            if (g_e2e_ctx.verbose) { \
                printf("  " E2E_COLOR_GREEN "PASS" E2E_COLOR_RESET "    [%s] %s\n", \
                       g_e2e_ctx.current_feature, g_e2e_ctx.current_name); \
            } \
        } else { \
            g_e2e_ctx.failed++; \
            printf("  " E2E_COLOR_RED "FAIL" E2E_COLOR_RESET "    [%s] %s (%s)\n", \
                   g_e2e_ctx.current_feature, g_e2e_ctx.current_name, g_e2e_ctx.current_milestone); \
        } \
    } while(0)

#define E2E_MARK_PENDING(reason) \
    do { \
        g_e2e_ctx.current_pending = true; \
        if (g_e2e_ctx.verbose) { \
            printf("       -> Pending reason: %s\n", (reason)); \
        } \
    } while(0)

#define E2E_FAIL(msg) \
    do { \
        g_e2e_ctx.current_failed++; \
        fprintf(stderr, "    " E2E_COLOR_RED "Assertion Failed" E2E_COLOR_RESET " at %s:%d: %s\n", \
                __FILE__, __LINE__, (msg)); \
    } while(0)

#define E2E_CHECK(cond, msg) \
    do { \
        if (!(cond)) { \
            E2E_FAIL(msg); \
        } \
    } while(0)

#define E2E_CHECK_EQ_INT(expected, actual, msg) \
    do { \
        int64_t _e = (int64_t)(expected); \
        int64_t _a = (int64_t)(actual); \
        if (_e != _a) { \
            g_e2e_ctx.current_failed++; \
            fprintf(stderr, "    " E2E_COLOR_RED "FAIL" E2E_COLOR_RESET " %s:%d: %s (expected %ld, got %ld)\n", \
                    __FILE__, __LINE__, (msg), (long)_e, (long)_a); \
        } \
    } while(0)

#define E2E_CHECK_EQ_DOUBLE(expected, actual, eps, msg) \
    do { \
        double _e = (double)(expected); \
        double _a = (double)(actual); \
        double _diff = fabs(_e - _a); \
        if (_diff > (eps)) { \
            g_e2e_ctx.current_failed++; \
            fprintf(stderr, "    " E2E_COLOR_RED "FAIL" E2E_COLOR_RESET " %s:%d: %s (expected %.6g, got %.6g, diff %.6g > eps %.6g)\n", \
                    __FILE__, __LINE__, (msg), _e, _a, _diff, (double)(eps)); \
        } \
    } while(0)

#define E2E_CHECK_EQ_STR(expected, actual, msg) \
    do { \
        const char* _e = (expected); \
        const char* _a = (actual); \
        if (!_e || !_a || strcmp(_e, _a) != 0) { \
            g_e2e_ctx.current_failed++; \
            fprintf(stderr, "    " E2E_COLOR_RED "FAIL" E2E_COLOR_RESET " %s:%d: %s (expected '%s', got '%s')\n", \
                    __FILE__, __LINE__, (msg), _e ? _e : "NULL", _a ? _a : "NULL"); \
        } \
    } while(0)

#define E2E_CHECK_NOT_NULL(ptr, msg) \
    do { \
        if ((ptr) == NULL) { \
            g_e2e_ctx.current_failed++; \
            fprintf(stderr, "    " E2E_COLOR_RED "FAIL" E2E_COLOR_RESET " %s:%d: %s (got NULL)\n", \
                    __FILE__, __LINE__, (msg)); \
        } \
    } while(0)

#define E2E_CHECK_NULL(ptr, msg) \
    do { \
        if ((ptr) != NULL) { \
            g_e2e_ctx.current_failed++; \
            fprintf(stderr, "    " E2E_COLOR_RED "FAIL" E2E_COLOR_RESET " %s:%d: %s (expected NULL, got %p)\n", \
                    __FILE__, __LINE__, (msg), (void*)(ptr)); \
        } \
    } while(0)

/* Utility: Read entire file into heap buffer */
static inline char* e2e_read_file(const char* path, size_t* out_len) {
    FILE* f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (len < 0) {
        fclose(f);
        return NULL;
    }
    char* buf = (char*)malloc(len + 1);
    if (!buf) {
        fclose(f);
        return NULL;
    }
    size_t read_bytes = fread(buf, 1, len, f);
    buf[read_bytes] = '\0';
    if (out_len) *out_len = read_bytes;
    fclose(f);
    return buf;
}

/* Tier entry points */
int run_tier1_lexer_literals(void);
int run_tier1_ast_parser(void);
int run_tier1_api_surface(void);
int run_tier1_hygiene_diagnostics(void);
int run_tier2_boundaries(void);
int run_tier3_interactions(void);
int run_tier4_scenarios(void);

#endif /* E2E_TEST_H */
