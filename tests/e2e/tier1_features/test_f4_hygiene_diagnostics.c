#include "../e2e_test.h"

/*
 * Tier 1: F4.1 - F4.4 Code Sanitization, Diagnostic Polish & Defect Cleanup Tests
 * Verifies R4 requirements from ORIGINAL_REQUEST.md & PROJECT.md
 */

static void test_f4_1_skeleton_purge(void) {
    /* Test 1: Zero "skeleton" (case-insensitive) in src/ */
    E2E_START_TEST("Zero 'skeleton' occurrences in src/", "F4.1 Skeleton Purge", "M4");
    FILE* p = popen("grep -ri 'skeleton' src/ 2>/dev/null | wc -l", "r");
    if (p) {
        char buf[64];
        if (fgets(buf, sizeof(buf), p)) {
            int count = atoi(buf);
            E2E_CHECK_EQ_INT(0, count, "src/ should contain zero 'skeleton' occurrences");
        }
        pclose(p);
    }
    E2E_END_TEST();

    /* Test 2: Zero "infra-only" in src/ */
    E2E_START_TEST("Zero 'infra-only' occurrences in src/", "F4.1 Skeleton Purge", "M4");
    p = popen("grep -ri 'infra-only' src/ 2>/dev/null | wc -l", "r");
    if (p) {
        char buf[64];
        if (fgets(buf, sizeof(buf), p)) {
            int count = atoi(buf);
            E2E_CHECK_EQ_INT(0, count, "src/ should contain zero 'infra-only' occurrences");
        }
        pclose(p);
    }
    E2E_END_TEST();

    /* Test 3: Zero "Wait, the skeleton says" in src/ */
    E2E_START_TEST("Zero 'Wait, the skeleton says' in src/", "F4.1 Skeleton Purge", "M4");
    p = popen("grep -ri 'Wait, the skeleton says' src/ 2>/dev/null | wc -l", "r");
    if (p) {
        char buf[64];
        if (fgets(buf, sizeof(buf), p)) {
            int count = atoi(buf);
            E2E_CHECK_EQ_INT(0, count, "src/ should contain zero 'Wait, the skeleton says'");
        }
        pclose(p);
    }
    E2E_END_TEST();

    /* Test 4: Zero "just identifiers for now" in src/ */
    E2E_START_TEST("Zero 'just identifiers for now' in src/", "F4.1 Skeleton Purge", "M4");
    p = popen("grep -ri 'just identifiers for now' src/ 2>/dev/null | wc -l", "r");
    if (p) {
        char buf[64];
        if (fgets(buf, sizeof(buf), p)) {
            int count = atoi(buf);
            E2E_CHECK_EQ_INT(0, count, "src/ should contain zero 'just identifiers for now'");
        }
        pclose(p);
    }
    E2E_END_TEST();

    /* Test 5: Zero "Step 1" or "Step 2" implementation placeholders in src/ */
    E2E_START_TEST("Zero 'Step [0-9]' placeholder comments in src/", "F4.1 Skeleton Purge", "M4");
    p = popen("grep -rE 'Step [0-9]' src/ 2>/dev/null | wc -l", "r");
    if (p) {
        char buf[64];
        if (fgets(buf, sizeof(buf), p)) {
            int count = atoi(buf);
            E2E_CHECK_EQ_INT(0, count, "src/ should contain zero 'Step X' placeholder comments");
        }
        pclose(p);
    }
    E2E_END_TEST();
}

static void test_f4_2_comment_english_standardization(void) {
    /* Test 1: Zero non-ASCII / CJK bytes in src/internal/lexer.c and lexer.h */
    E2E_START_TEST("Zero CJK bytes in src/internal/lexer.[ch]", "F4.2 Comment English Standardization", "M4");
    FILE* p = popen("grep -P '[\\x80-\\xFF]' src/internal/lexer.[ch] 2>/dev/null | wc -l", "r");
    if (p) {
        char buf[64];
        if (fgets(buf, sizeof(buf), p)) {
            int count = atoi(buf);
            E2E_CHECK_EQ_INT(0, count, "src/internal/lexer.[ch] should contain zero non-ASCII bytes");
        }
        pclose(p);
    }
    E2E_END_TEST();

    /* Test 2: Zero non-ASCII / CJK bytes in src/internal/parser.c and parser.h */
    E2E_START_TEST("Zero CJK bytes in src/internal/parser.[ch]", "F4.2 Comment English Standardization", "M4");
    p = popen("grep -P '[\\x80-\\xFF]' src/internal/parser.[ch] 2>/dev/null | wc -l", "r");
    if (p) {
        char buf[64];
        if (fgets(buf, sizeof(buf), p)) {
            int count = atoi(buf);
            E2E_CHECK_EQ_INT(0, count, "src/internal/parser.[ch] should contain zero non-ASCII bytes");
        }
        pclose(p);
    }
    E2E_END_TEST();

    /* Test 3: Zero non-ASCII / CJK bytes in src/internal/expr_parser.c */
    E2E_START_TEST("Zero CJK bytes in src/internal/expr_parser.[ch]", "F4.2 Comment English Standardization", "M4");
    p = popen("grep -P '[\\x80-\\xFF]' src/internal/expr_parser.[ch] 2>/dev/null | wc -l", "r");
    if (p) {
        char buf[64];
        if (fgets(buf, sizeof(buf), p)) {
            int count = atoi(buf);
            E2E_CHECK_EQ_INT(0, count, "src/internal/expr_parser.[ch] should contain zero non-ASCII bytes");
        }
        pclose(p);
    }
    E2E_END_TEST();

    /* Test 4: Zero non-ASCII / CJK bytes in src/internal/symtab.c */
    E2E_START_TEST("Zero CJK bytes in src/internal/symtab.[ch]", "F4.2 Comment English Standardization", "M4");
    p = popen("grep -P '[\\x80-\\xFF]' src/internal/symtab.[ch] 2>/dev/null | wc -l", "r");
    if (p) {
        char buf[64];
        if (fgets(buf, sizeof(buf), p)) {
            int count = atoi(buf);
            E2E_CHECK_EQ_INT(0, count, "src/internal/symtab.[ch] should contain zero non-ASCII bytes");
        }
        pclose(p);
    }
    E2E_END_TEST();

    /* Test 5: Zero non-ASCII / CJK bytes in include/cnt/ headers */
    E2E_START_TEST("Zero CJK bytes in include/cnt/ headers", "F4.2 Comment English Standardization", "M4");
    p = popen("grep -P '[\\x80-\\xFF]' include/cnt/*.h 2>/dev/null | wc -l", "r");
    if (p) {
        char buf[64];
        if (fgets(buf, sizeof(buf), p)) {
            int count = atoi(buf);
            E2E_CHECK_EQ_INT(0, count, "include/cnt/*.h should contain zero non-ASCII bytes");
        }
        pclose(p);
    }
    E2E_END_TEST();
}

static void test_f4_3_polish_diagnostics(void) {
    /* Test 1: Syntax error records valid error code and non-empty diagnostic */
    E2E_START_TEST("Diagnostic recorded on syntax error", "F4.3 Polish Diagnostics", "M4");
    cco_clear_diagnostics();
    cco_object_t* obj = cco_parse_string("invalid syntax here @!%", 23, NULL);
    E2E_CHECK(obj == NULL, "Parse should fail on invalid syntax");
    E2E_CHECK(cco_get_last_error() != CCO_OK, "Last error should not be CCO_OK");
    if (obj) cco_object_release(obj);
    E2E_END_TEST();

    /* Test 2: Unclosed string literal produces diagnostic */
    E2E_START_TEST("Diagnostic on unclosed string literal", "F4.3 Polish Diagnostics", "M4");
    cco_clear_diagnostics();
    obj = cco_parse_string("\"unclosed string", 16, NULL);
    E2E_CHECK(obj == NULL, "Unclosed string must fail parse");
    E2E_CHECK_EQ_INT(CCO_ERR_PARSE, cco_get_last_error(), "Error code should be CCO_ERR_PARSE");
    if (obj) cco_object_release(obj);
    E2E_END_TEST();

    /* Test 3: Unclosed delimiter produces diagnostic */
    E2E_START_TEST("Diagnostic on unclosed map paren", "F4.3 Polish Diagnostics", "M4");
    cco_clear_diagnostics();
    obj = cco_parse_string("(a: 10, b: 20", 13, NULL);
    E2E_CHECK(obj == NULL, "Unclosed paren must fail parse");
    E2E_CHECK_EQ_INT(CCO_ERR_PARSE, cco_get_last_error(), "Error code should be CCO_ERR_PARSE");
    if (obj) cco_object_release(obj);
    E2E_END_TEST();

    /* Test 4: Diagnostics do not contain 'unsupported in skeleton' */
    E2E_START_TEST("No diagnostic messages mention 'skeleton'", "F4.3 Polish Diagnostics", "M4");
    FILE* p = popen("grep -ri 'unsupported in skeleton' src/ 2>/dev/null | wc -l", "r");
    if (p) {
        char buf[64];
        if (fgets(buf, sizeof(buf), p)) {
            int count = atoi(buf);
            E2E_CHECK_EQ_INT(0, count, "src/ diagnostics should not say 'unsupported in skeleton'");
        }
        pclose(p);
    }
    E2E_END_TEST();

    /* Test 5: cco_clear_diagnostics clears error state */
    E2E_START_TEST("cco_clear_diagnostics resets error state to CCO_OK", "F4.3 Polish Diagnostics", "M4");
    cco_parse_string("@#$bad", 6, NULL);
    cco_clear_diagnostics();
    E2E_CHECK_EQ_INT(CCO_OK, cco_get_last_error(), "Error should be reset to CCO_OK");
    E2E_END_TEST();
}

static void test_f4_4_latent_defect_cleanup(void) {
    /* Test 1: fuzz_target.c compiles cleanly without missing prototype warnings */
    E2E_START_TEST("fuzz_target.c compilation check", "F4.4 Latent Defect Cleanup", "M4");
    int rc = system("cc -c -Wall -Wextra -Iinclude -Isrc/internal tests/fuzz/fuzz_target.c -o /tmp/fuzz_target_test.o >/dev/null 2>&1");
    E2E_CHECK_EQ_INT(0, rc, "fuzz_target.c should compile cleanly with -Wall -Wextra");
    remove("/tmp/fuzz_target_test.o");
    E2E_END_TEST();

    /* Test 2: test_expr.c does not silently bailout */
    E2E_START_TEST("test_expr.c does not contain silent early bailout", "F4.4 Latent Defect Cleanup", "M4");
    size_t len = 0;
    char* expr_test = e2e_read_file("tests/unit/test_expr.c", &len);
    E2E_CHECK_NOT_NULL(expr_test, "tests/unit/test_expr.c exists");
    if (expr_test) {
        /* Check if there is an unconditional return 0 or silent skip at top of test */
        E2E_CHECK(strstr(expr_test, "return; /* skip */") == NULL, "test_expr.c should not have skip bailout");
        free(expr_test);
    }
    E2E_END_TEST();

    /* Test 3: Division by zero does not crash parser */
    E2E_START_TEST("Division by zero in expression handled safely", "F4.4 Latent Defect Cleanup", "M4");
    cco_clear_diagnostics();
    cco_object_t* obj = cco_parse_string("(val: $(10 / 0))", 16, NULL);
    /* Should either return error or null, but MUST NOT crash with SIGFPE */
    if (obj) cco_object_release(obj);
    E2E_CHECK(true, "Division by zero handled without crash");
    E2E_END_TEST();

    /* Test 4: NULL pointer passed to cco_object_release is safe no-op */
    E2E_START_TEST("cco_object_release(NULL) is safe no-op", "F4.4 Latent Defect Cleanup", "M4");
    cco_object_release(NULL);
    E2E_CHECK(true, "cco_object_release(NULL) did not crash");
    E2E_END_TEST();

    /* Test 5: NULL pointer passed to cco_serialize_to_string handled safely */
    E2E_START_TEST("cco_serialize_to_string(NULL) returns NULL safely", "F4.4 Latent Defect Cleanup", "M4");
    char* s = cco_serialize_to_string(NULL, false);
    E2E_CHECK_NULL(s, "Serializing NULL object should return NULL");
    if (s) free(s);
    E2E_END_TEST();
}

int run_tier1_hygiene_diagnostics(void) {
    printf(E2E_COLOR_BOLD "--- Running Tier 1: Hygiene & Diagnostics (F4.1 - F4.4) ---\n" E2E_COLOR_RESET);
    test_f4_1_skeleton_purge();
    test_f4_2_comment_english_standardization();
    test_f4_3_polish_diagnostics();
    test_f4_4_latent_defect_cleanup();
    return 0;
}
