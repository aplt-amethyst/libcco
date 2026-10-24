#include "../e2e_test.h"

/*
 * Tier 2: Boundary & Corner Cases Tests
 * Covers numeric limits, string extremes, deep recursion, empty inputs,
 * delimiter boundaries, and malformed edge cases.
 */

static void test_tier2_numeric_boundaries(void) {
    /* Test 1: INT64_MAX: 9223372036854775807 */
    E2E_START_TEST("INT64_MAX boundary", "Tier 2: Numeric Boundaries", "All");
    const char* src1 = "9223372036854775807";
    cco_object_t* obj = cco_parse_string(src1, strlen(src1), NULL);
    if (obj) {
        int64_t val = 0;
        cco_object_get_integer(obj, &val);
        E2E_CHECK_EQ_INT(9223372036854775807LL, val, "INT64_MAX mismatch");
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse INT64_MAX");
    }
    E2E_END_TEST();

    /* Test 2: INT64_MIN: -9223372036854775808 */
    E2E_START_TEST("INT64_MIN boundary", "Tier 2: Numeric Boundaries", "All");
    const char* src2 = "-9223372036854775808";
    obj = cco_parse_string(src2, strlen(src2), NULL);
    if (obj) {
        int64_t val = 0;
        cco_object_get_integer(obj, &val);
        E2E_CHECK_EQ_INT(-9223372036854775807LL - 1LL, val, "INT64_MIN mismatch");
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse INT64_MIN");
    }
    E2E_END_TEST();

    /* Test 3: Large scientific notation 1e308 */
    E2E_START_TEST("Extreme float: 1e308", "Tier 2: Numeric Boundaries", "All");
    const char* src3 = "1e308";
    obj = cco_parse_string(src3, strlen(src3), NULL);
    if (obj) {
        double val = 0.0;
        cco_object_get_float(obj, &val);
        E2E_CHECK(val > 1e300, "1e308 should parse to very large float");
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse 1e308");
    }
    E2E_END_TEST();

    /* Test 4: Small scientific notation 1e-308 */
    E2E_START_TEST("Subnormal/small float: 1e-308", "Tier 2: Numeric Boundaries", "All");
    const char* src4 = "1e-308";
    obj = cco_parse_string(src4, strlen(src4), NULL);
    if (obj) {
        double val = 0.0;
        cco_object_get_float(obj, &val);
        E2E_CHECK(val > 0.0 && val < 1e-300, "1e-308 should parse to very small positive float");
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse 1e-308");
    }
    E2E_END_TEST();

    /* Test 5: Hex boundary 0xFFFFFFFFFFFFFFFF (64-bit uint max or signed -1) */
    E2E_START_TEST("Hex 64-bit boundary 0xFFFFFFFFFFFFFFFF", "Tier 2: Numeric Boundaries", "All");
    const char* src5 = "0xFFFFFFFFFFFFFFFF";
    obj = cco_parse_string(src5, strlen(src5), NULL);
    if (obj) {
        cco_object_release(obj);
    }
    E2E_CHECK(true, "0xFFFFFFFFFFFFFFFF handled without crash");
    E2E_END_TEST();

    /* Test 6: Zeroes in decimal: 0, -0, +0, 0.0, -0.0 */
    E2E_START_TEST("Zero variations: 0, +0, -0, 0.0", "Tier 2: Numeric Boundaries", "All");
    const char* src6 = "(a: 0, b: +0, c: -0, d: 0.0)";
    obj = cco_parse_string(src6, strlen(src6), NULL);
    if (obj) {
        cco_object_t* a = cco_map_get_by_key(obj, "a");
        int64_t v = -1;
        cco_object_get_integer(a, &v);
        E2E_CHECK_EQ_INT(0, v, "Zero value should be 0");
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse zero variations");
    }
    E2E_END_TEST();
}

static void test_tier2_string_boundaries(void) {
    /* Test 1: Empty string \"\" */
    E2E_START_TEST("Empty string literal \"\"", "Tier 2: String Boundaries", "All");
    const char* src1 = "\"\"";
    cco_object_t* obj = cco_parse_string(src1, 2, NULL);
    if (obj) {
        const char* s = NULL;
        size_t len = 1;
        cco_object_get_string(obj, &s, &len);
        E2E_CHECK_EQ_INT(0, len, "Empty string length should be 0");
        E2E_CHECK_EQ_STR("", s, "Empty string value mismatch");
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse empty string \"\"");
    }
    E2E_END_TEST();

    /* Test 2: 1-character string \"x\" */
    E2E_START_TEST("Single character string \"x\"", "Tier 2: String Boundaries", "All");
    const char* src2 = "\"x\"";
    obj = cco_parse_string(src2, 3, NULL);
    if (obj) {
        const char* s = NULL;
        size_t len = 0;
        cco_object_get_string(obj, &s, &len);
        E2E_CHECK_EQ_INT(1, len, "Single char string length should be 1");
        E2E_CHECK_EQ_STR("x", s, "Single char string value mismatch");
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse single character string");
    }
    E2E_END_TEST();

    /* Test 3: Standard escape sequences: \n, \t, \r, \\, \" */
    E2E_START_TEST("String escape sequences \\n \\t \\r \\\" \\\\", "Tier 2: String Boundaries", "All");
    const char* src3 = "\"line1\\nline2\\ttab\\\"quote\\\\slash\"";
    obj = cco_parse_string(src3, strlen(src3), NULL);
    if (obj) {
        const char* s = NULL;
        size_t len = 0;
        cco_object_get_string(obj, &s, &len);
        E2E_CHECK_NOT_NULL(s, "String pointer should not be NULL");
        E2E_CHECK(strstr(s, "\n") != NULL, "String should contain actual newline");
        E2E_CHECK(strstr(s, "\t") != NULL, "String should contain actual tab");
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse string with escape sequences");
    }
    E2E_END_TEST();

    /* Test 4: Large string (64 KB) */
    E2E_START_TEST("Large string (64 KB continuous)", "Tier 2: String Boundaries", "All");
    size_t big_len = 65536;
    char* big_src = (char*)malloc(big_len + 4);
    if (big_src) {
        big_src[0] = '"';
        memset(big_src + 1, 'A', big_len);
        big_src[big_len + 1] = '"';
        big_src[big_len + 2] = '\0';
        obj = cco_parse_string(big_src, big_len + 2, NULL);
        if (obj) {
            size_t out_len = 0;
            const char* s = NULL;
            cco_object_get_string(obj, &s, &out_len);
            E2E_CHECK_EQ_INT(big_len, out_len, "64KB string length mismatch");
            cco_object_release(obj);
        } else {
            E2E_FAIL("Failed to parse 64KB string");
        }
        free(big_src);
    }
    E2E_END_TEST();

    /* Test 5: Empty key in map \"\": 123 */
    E2E_START_TEST("Empty string key in map \"\": 123", "Tier 2: String Boundaries", "All");
    const char* src5 = "(\"\": 123)";
    obj = cco_parse_string(src5, strlen(src5), NULL);
    if (obj) {
        cco_object_t* item = cco_map_get_by_key(obj, "");
        E2E_CHECK_NOT_NULL(item, "Empty key lookup should find item");
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse map with empty string key");
    }
    E2E_END_TEST();
}

static void test_tier2_nesting_depth_boundaries(void) {
    /* Test 1: Nested maps 16 levels deep */
    E2E_START_TEST("Nested maps 16 levels deep", "Tier 2: Depth Boundaries", "All");
    char deep16[1024];
    int pos = 0;
    for (int i = 0; i < 16; i++) {
        pos += sprintf(deep16 + pos, "(k%d: ", i);
    }
    pos += sprintf(deep16 + pos, "42");
    for (int i = 0; i < 16; i++) {
        pos += sprintf(deep16 + pos, ")");
    }
    cco_object_t* obj = cco_parse_string(deep16, strlen(deep16), NULL);
    E2E_CHECK_NOT_NULL(obj, "16 levels of nesting should succeed");
    if (obj) cco_object_release(obj);
    E2E_END_TEST();

    /* Test 2: Nested maps 32 levels deep */
    E2E_START_TEST("Nested maps 32 levels deep", "Tier 2: Depth Boundaries", "All");
    char deep32[2048];
    pos = 0;
    for (int i = 0; i < 32; i++) {
        pos += sprintf(deep32 + pos, "(k%d: ", i);
    }
    pos += sprintf(deep32 + pos, "100");
    for (int i = 0; i < 32; i++) {
        pos += sprintf(deep32 + pos, ")");
    }
    obj = cco_parse_string(deep32, strlen(deep32), NULL);
    E2E_CHECK_NOT_NULL(obj, "32 levels of nesting should succeed");
    if (obj) cco_object_release(obj);
    E2E_END_TEST();

    /* Test 3: Nested arrays 20 levels deep */
    E2E_START_TEST("Nested arrays 20 levels deep", "Tier 2: Depth Boundaries", "All");
    char arr20[1024];
    pos = 0;
    for (int i = 0; i < 20; i++) pos += sprintf(arr20 + pos, "(");
    pos += sprintf(arr20 + pos, "1");
    for (int i = 0; i < 20; i++) pos += sprintf(arr20 + pos, ")");
    obj = cco_parse_string(arr20, strlen(arr20), NULL);
    E2E_CHECK_NOT_NULL(obj, "20 levels of array nesting should succeed");
    if (obj) cco_object_release(obj);
    E2E_END_TEST();

    /* Test 4: Extreme recursion depth (e.g. 256 levels) fails gracefully without stack overflow */
    E2E_START_TEST("Extreme depth 256 levels terminates safely", "Tier 2: Depth Boundaries", "All");
    char deep256[4096];
    pos = 0;
    for (int i = 0; i < 256; i++) pos += sprintf(deep256 + pos, "(k: ");
    pos += sprintf(deep256 + pos, "1");
    for (int i = 0; i < 256; i++) pos += sprintf(deep256 + pos, ")");
    cco_clear_diagnostics();
    obj = cco_parse_string(deep256, strlen(deep256), NULL);
    /* May succeed or fail with depth exceeded, but MUST NOT crash with SIGSEGV */
    if (obj) cco_object_release(obj);
    E2E_CHECK(true, "Handled 256 recursion depth safely");
    E2E_END_TEST();

    /* Test 5: Alternating map/array nesting */
    E2E_START_TEST("Alternating map/array nesting 10 levels", "Tier 2: Depth Boundaries", "All");
    const char* alt_src = "(a: ((b: ((c: (d: 1)))))))";
    obj = cco_parse_string(alt_src, strlen(alt_src), NULL);
    E2E_CHECK_NOT_NULL(obj, "Alternating nesting should succeed");
    if (obj) cco_object_release(obj);
    E2E_END_TEST();
}

static void test_tier2_container_size_boundaries(void) {
    /* Test 1: Empty map () */
    E2E_START_TEST("Empty map ()", "Tier 2: Container Boundaries", "All");
    const char* src1 = "()";
    cco_object_t* obj = cco_parse_string(src1, 2, NULL);
    if (obj) {
        int t = cco_object_get_type(obj);
        E2E_CHECK(t == CCO_TYPE_MAP || t == CCO_TYPE_ARRAY, "Empty () is valid container");
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse empty container ()");
    }
    E2E_END_TEST();

    /* Test 2: Map with 500 keys */
    E2E_START_TEST("Map with 500 keys", "Tier 2: Container Boundaries", "All");
    size_t cap = 32768;
    char* big_map = (char*)malloc(cap);
    if (big_map) {
        int pos = sprintf(big_map, "(");
        for (int i = 0; i < 500; i++) {
            pos += sprintf(big_map + pos, "k%d: %d%s", i, i, (i == 499) ? "" : ", ");
        }
        pos += sprintf(big_map + pos, ")");
        obj = cco_parse_string(big_map, pos, NULL);
        if (obj) {
            size_t cnt = cco_map_get_count(obj);
            E2E_CHECK_EQ_INT(500, cnt, "500-key map count mismatch");
            cco_object_t* k100 = cco_map_get_by_key(obj, "k100");
            E2E_CHECK_NOT_NULL(k100, "k100 lookup should succeed");
            cco_object_release(obj);
        } else {
            E2E_FAIL("Failed to parse 500-key map");
        }
        free(big_map);
    }
    E2E_END_TEST();

    /* Test 3: Array with 500 elements */
    E2E_START_TEST("Array with 500 elements", "Tier 2: Container Boundaries", "All");
    size_t arr_cap = 16384;
    char* big_arr = (char*)malloc(arr_cap);
    if (big_arr) {
        int pos = sprintf(big_arr, "(");
        for (int i = 0; i < 500; i++) {
            pos += sprintf(big_arr + pos, "%d%s", i, (i == 499) ? "" : ", ");
        }
        pos += sprintf(big_arr + pos, ")");
        obj = cco_parse_string(big_arr, pos, NULL);
        if (obj) {
            size_t cnt = cco_array_get_count(obj);
            E2E_CHECK_EQ_INT(500, cnt, "500-element array count mismatch");
            cco_object_release(obj);
        } else {
            E2E_FAIL("Failed to parse 500-element array");
        }
        free(big_arr);
    }
    E2E_END_TEST();

    /* Test 4: Single element array (42) */
    E2E_START_TEST("Single element array (42)", "Tier 2: Container Boundaries", "All");
    const char* src4 = "(42)";
    obj = cco_parse_string(src4, 4, NULL);
    if (obj) {
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse single element array");
    }
    E2E_END_TEST();

    /* Test 5: Single key map (single_key: 1) */
    E2E_START_TEST("Single key map (single_key: 1)", "Tier 2: Container Boundaries", "All");
    const char* src5 = "(single_key: 1)";
    obj = cco_parse_string(src5, 15, NULL);
    if (obj) {
        cco_object_t* item = cco_map_get_by_key(obj, "single_key");
        E2E_CHECK_NOT_NULL(item, "single_key lookup should succeed");
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse single key map");
    }
    E2E_END_TEST();
}

static void test_tier2_syntax_boundaries(void) {
    /* Test 1: Zero byte / empty input */
    E2E_START_TEST("Zero byte empty string input", "Tier 2: Syntax Boundaries", "All");
    cco_object_t* obj = cco_parse_string("", 0, NULL);
    /* Safe return (NULL or empty node) without crash */
    if (obj) cco_object_release(obj);
    E2E_CHECK(true, "Zero byte input handled safely");
    E2E_END_TEST();

    /* Test 2: Whitespace only input */
    E2E_START_TEST("Whitespace-only input", "Tier 2: Syntax Boundaries", "All");
    const char* ws = "   \t\n\r\n   ";
    obj = cco_parse_string(ws, strlen(ws), NULL);
    if (obj) cco_object_release(obj);
    E2E_CHECK(true, "Whitespace-only input handled safely");
    E2E_END_TEST();

    /* Test 3: Comment only input */
    E2E_START_TEST("Comment-only input", "Tier 2: Syntax Boundaries", "All");
    const char* cmt = "# Single line\n/* Block */\n";
    obj = cco_parse_string(cmt, strlen(cmt), NULL);
    if (obj) cco_object_release(obj);
    E2E_CHECK(true, "Comment-only input handled safely");
    E2E_END_TEST();

    /* Test 4: Windows CRLF \\r\\n line endings */
    E2E_START_TEST("CRLF line endings", "Tier 2: Syntax Boundaries", "All");
    const char* crlf = "(\r\n  key: \"value\",\r\n  num: 42\r\n)\r\n";
    obj = cco_parse_string(crlf, strlen(crlf), NULL);
    if (obj) {
        cco_object_t* val = cco_map_get_by_key(obj, "key");
        E2E_CHECK_NOT_NULL(val, "CRLF parse succeeded");
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse CRLF input");
    }
    E2E_END_TEST();

    /* Test 5: Missing colon in map (a 1) syntax error */
    E2E_START_TEST("Missing colon in map (a 1)", "Tier 2: Syntax Boundaries", "All");
    cco_clear_diagnostics();
    obj = cco_parse_string("(a 1)", 5, NULL);
    E2E_CHECK(obj == NULL, "Missing colon should fail parse");
    E2E_CHECK_EQ_INT(CCO_ERR_PARSE, cco_get_last_error(), "Error should be CCO_ERR_PARSE");
    if (obj) cco_object_release(obj);
    E2E_END_TEST();

    /* Test 6: Unmatched opening paren (a: 1 */
    E2E_START_TEST("Unmatched opening paren (a: 1", "Tier 2: Syntax Boundaries", "All");
    cco_clear_diagnostics();
    obj = cco_parse_string("(a: 1", 5, NULL);
    E2E_CHECK(obj == NULL, "Unmatched open paren should fail parse");
    if (obj) cco_object_release(obj);
    E2E_END_TEST();
}

int run_tier2_boundaries(void) {
    printf(E2E_COLOR_BOLD "--- Running Tier 2: Boundary & Corner Cases ---\n" E2E_COLOR_RESET);
    test_tier2_numeric_boundaries();
    test_tier2_string_boundaries();
    test_tier2_nesting_depth_boundaries();
    test_tier2_container_size_boundaries();
    test_tier2_syntax_boundaries();
    return 0;
}
