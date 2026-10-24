#include "../e2e_test.h"

/* 
 * Tier 1: F1.1 - F1.7 Lexer, Literal Engine & Comments Tests
 * Verifies R1 requirements from ORIGINAL_REQUEST.md & PROJECT.md
 */

static void test_f1_1_scientific_notation(void) {
    /* Test 1: 1e10 -> double value 1e10 */
    E2E_START_TEST("1e10 integer mantissa positive exponent", "F1.1 Scientific Notation", "M1");
    cco_object_t* obj = cco_parse_string("1e10", 4, NULL);
    if (obj) {
        double val = 0.0;
        int type = cco_object_get_type(obj);
        bool has_flt = cco_object_get_float(obj, &val);
        E2E_CHECK(type == CCO_TYPE_FLOAT, "1e10 should be of float type");
        E2E_CHECK(has_flt, "cco_object_get_float should succeed on 1e10");
        E2E_CHECK_EQ_DOUBLE(1e10, val, 1e-4, "1e10 value mismatch");
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse 1e10");
    }
    E2E_END_TEST();

    /* Test 2: 1.5e3 -> 1500.0 */
    E2E_START_TEST("1.5e3 float mantissa positive exponent", "F1.1 Scientific Notation", "M1");
    obj = cco_parse_string("1.5e3", 5, NULL);
    if (obj) {
        double val = 0.0;
        cco_object_get_float(obj, &val);
        E2E_CHECK_EQ_DOUBLE(1500.0, val, 1e-4, "1.5e3 should equal 1500.0");
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse 1.5e3");
    }
    E2E_END_TEST();

    /* Test 3: 1.5e-3 -> 0.0015 */
    E2E_START_TEST("1.5e-3 float mantissa negative exponent", "F1.1 Scientific Notation", "M1");
    obj = cco_parse_string("1.5e-3", 6, NULL);
    if (obj) {
        double val = 0.0;
        cco_object_get_float(obj, &val);
        E2E_CHECK_EQ_DOUBLE(0.0015, val, 1e-7, "1.5e-3 should equal 0.0015");
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse 1.5e-3");
    }
    E2E_END_TEST();

    /* Test 4: 1e-5 -> 0.00001 */
    E2E_START_TEST("1e-5 integer mantissa negative exponent", "F1.1 Scientific Notation", "M1");
    obj = cco_parse_string("1e-5", 4, NULL);
    if (obj) {
        double val = 0.0;
        cco_object_get_float(obj, &val);
        E2E_CHECK_EQ_DOUBLE(0.00001, val, 1e-8, "1e-5 should equal 0.00001");
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse 1e-5");
    }
    E2E_END_TEST();

    /* Test 5: Capital E and explicit positive sign: 2.5E+4 -> 25000.0 */
    E2E_START_TEST("2.5E+4 capital E with positive sign", "F1.1 Scientific Notation", "M1");
    obj = cco_parse_string("2.5E+4", 6, NULL);
    if (obj) {
        double val = 0.0;
        cco_object_get_float(obj, &val);
        E2E_CHECK_EQ_DOUBLE(25000.0, val, 1e-4, "2.5E+4 should equal 25000.0");
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse 2.5E+4");
    }
    E2E_END_TEST();

    /* Test 6: Scientific notation as map value (scientific_key: 3.2e2) */
    E2E_START_TEST("scientific notation inside map value", "F1.1 Scientific Notation", "M1");
    const char* map_src = "(rate: 3.2e2)";
    obj = cco_parse_string(map_src, strlen(map_src), NULL);
    if (obj) {
        cco_object_t* item = cco_map_get_by_key(obj, "rate");
        E2E_CHECK_NOT_NULL(item, "map key 'rate' should exist");
        if (item) {
            double val = 0.0;
            cco_object_get_float(item, &val);
            E2E_CHECK_EQ_DOUBLE(320.0, val, 1e-4, "rate should equal 320.0");
        }
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse map with scientific value");
    }
    E2E_END_TEST();
}

static void test_f1_2_hex_binary_octal(void) {
    /* Test 1: Hex 0xFF -> 255 */
    E2E_START_TEST("0xFF hex literal", "F1.2 Hex, Binary, Octal", "M1");
    cco_object_t* obj = cco_parse_string("0xFF", 4, NULL);
    if (obj) {
        int64_t val = 0;
        cco_object_get_integer(obj, &val);
        E2E_CHECK_EQ_INT(255, val, "0xFF should evaluate to 255");
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse 0xFF");
    }
    E2E_END_TEST();

    /* Test 2: Hex multi-digit mixed case 0x1A2b -> 6699 */
    E2E_START_TEST("0x1A2b hex mixed case", "F1.2 Hex, Binary, Octal", "M1");
    obj = cco_parse_string("0x1A2b", 6, NULL);
    if (obj) {
        int64_t val = 0;
        cco_object_get_integer(obj, &val);
        E2E_CHECK_EQ_INT(6699, val, "0x1A2b should evaluate to 6699");
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse 0x1A2b");
    }
    E2E_END_TEST();

    /* Test 3: Binary 0b1010 -> 10 */
    E2E_START_TEST("0b1010 binary literal", "F1.2 Hex, Binary, Octal", "M1");
    obj = cco_parse_string("0b1010", 6, NULL);
    if (obj) {
        int64_t val = 0;
        cco_object_get_integer(obj, &val);
        E2E_CHECK_EQ_INT(10, val, "0b1010 should evaluate to 10");
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse 0b1010");
    }
    E2E_END_TEST();

    /* Test 4: Binary 8-bit byte 0b11110000 -> 240 */
    E2E_START_TEST("0b11110000 8-bit binary literal", "F1.2 Hex, Binary, Octal", "M1");
    obj = cco_parse_string("0b11110000", 10, NULL);
    if (obj) {
        int64_t val = 0;
        cco_object_get_integer(obj, &val);
        E2E_CHECK_EQ_INT(240, val, "0b11110000 should evaluate to 240");
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse 0b11110000");
    }
    E2E_END_TEST();

    /* Test 5: Octal 0o77 -> 63 */
    E2E_START_TEST("0o77 octal literal", "F1.2 Hex, Binary, Octal", "M1");
    obj = cco_parse_string("0o77", 4, NULL);
    if (obj) {
        int64_t val = 0;
        cco_object_get_integer(obj, &val);
        E2E_CHECK_EQ_INT(63, val, "0o77 should evaluate to 63");
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse 0o77");
    }
    E2E_END_TEST();

    /* Test 6: Zeroes in alternate bases 0x0, 0b0, 0o0 */
    E2E_START_TEST("0x0, 0b0, 0o0 base zero representations", "F1.2 Hex, Binary, Octal", "M1");
    const char* arr_src = "(0x0, 0b0, 0o0)";
    obj = cco_parse_string(arr_src, strlen(arr_src), NULL);
    if (obj) {
        size_t count = cco_array_get_count(obj);
        E2E_CHECK_EQ_INT(3, count, "Array should contain 3 elements");
        for (size_t i = 0; i < count; i++) {
            cco_object_t* item = cco_array_get_item(obj, i);
            int64_t v = -1;
            cco_object_get_integer(item, &v);
            E2E_CHECK_EQ_INT(0, v, "Zero in base should evaluate to 0");
        }
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse array of base zeroes");
    }
    E2E_END_TEST();
}

static void test_f1_3_signed_numbers(void) {
    /* Test 1: Negative float -1.5 */
    E2E_START_TEST("-1.5 signed negative float", "F1.3 Signed Numbers", "M1");
    cco_object_t* obj = cco_parse_string("-1.5", 4, NULL);
    if (obj) {
        double val = 0.0;
        cco_object_get_float(obj, &val);
        E2E_CHECK_EQ_DOUBLE(-1.5, val, 1e-4, "-1.5 mismatch");
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse -1.5");
    }
    E2E_END_TEST();

    /* Test 2: Explicit positive float +1.5 */
    E2E_START_TEST("+1.5 signed positive float", "F1.3 Signed Numbers", "M1");
    obj = cco_parse_string("+1.5", 4, NULL);
    if (obj) {
        double val = 0.0;
        cco_object_get_float(obj, &val);
        E2E_CHECK_EQ_DOUBLE(1.5, val, 1e-4, "+1.5 mismatch");
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse +1.5");
    }
    E2E_END_TEST();

    /* Test 3: Negative integer -42 */
    E2E_START_TEST("-42 signed negative integer", "F1.3 Signed Numbers", "M1");
    obj = cco_parse_string("-42", 3, NULL);
    if (obj) {
        int64_t val = 0;
        cco_object_get_integer(obj, &val);
        E2E_CHECK_EQ_INT(-42, val, "-42 mismatch");
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse -42");
    }
    E2E_END_TEST();

    /* Test 4: Positive integer +100 */
    E2E_START_TEST("+100 signed positive integer", "F1.3 Signed Numbers", "M1");
    obj = cco_parse_string("+100", 4, NULL);
    if (obj) {
        int64_t val = 0;
        cco_object_get_integer(obj, &val);
        E2E_CHECK_EQ_INT(100, val, "+100 mismatch");
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse +100");
    }
    E2E_END_TEST();

    /* Test 5: Signed scientific notation -1.5e3 -> -1500.0 */
    E2E_START_TEST("-1.5e3 signed negative scientific", "F1.3 Signed Numbers", "M1");
    obj = cco_parse_string("-1.5e3", 6, NULL);
    if (obj) {
        double val = 0.0;
        cco_object_get_float(obj, &val);
        E2E_CHECK_EQ_DOUBLE(-1500.0, val, 1e-4, "-1.5e3 mismatch");
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse -1.5e3");
    }
    E2E_END_TEST();

    /* Test 6: Signed hex -0x10 -> -16 */
    E2E_START_TEST("-0x10 signed negative hex", "F1.3 Signed Numbers", "M1");
    obj = cco_parse_string("-0x10", 5, NULL);
    if (obj) {
        int64_t val = 0;
        cco_object_get_integer(obj, &val);
        E2E_CHECK_EQ_INT(-16, val, "-0x10 mismatch");
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse -0x10");
    }
    E2E_END_TEST();
}

static void test_f1_4_fix_parser_hang(void) {
    /* Test 1: Lone '$' in string interpolation terminates immediately without hang */
    E2E_START_TEST("\"$\" lone dollar in string terminates without hang", "F1.4 Fix Parser Hang", "M1");
    cco_object_t* obj = cco_parse_string("\"$\"", 3, NULL);
    /* String with lone $ might be valid literal "$" or parse error, but MUST NOT hang! */
    if (obj) {
        cco_object_release(obj);
    }
    E2E_CHECK(true, "Execution returned without infinite loop");
    E2E_END_TEST();

    /* Test 2: Lone '$' inside text: "prefix $ suffix" */
    E2E_START_TEST("\"prefix $ suffix\" text with dollar", "F1.4 Fix Parser Hang", "M1");
    obj = cco_parse_string("\"prefix $ suffix\"", 17, NULL);
    if (obj) {
        cco_object_release(obj);
    }
    E2E_CHECK(true, "Execution returned without infinite loop");
    E2E_END_TEST();

    /* Test 3: Unclosed interpolation "$(1 + 2" terminates with parse error */
    E2E_START_TEST("\"$(1 + 2\" unclosed interpolation syntax error", "F1.4 Fix Parser Hang", "M1");
    obj = cco_parse_string("\"$(1 + 2\"", 9, NULL);
    E2E_CHECK(obj == NULL, "Unclosed interpolation must fail parsing");
    if (obj) cco_object_release(obj);
    E2E_END_TEST();

    /* Test 4: Truncated interpolation "$(" terminates cleanly */
    E2E_START_TEST("\"$(\" truncated interpolation terminates cleanly", "F1.4 Fix Parser Hang", "M1");
    obj = cco_parse_string("\"$(\"", 4, NULL);
    E2E_CHECK(obj == NULL, "Truncated interpolation must fail parsing");
    if (obj) cco_object_release(obj);
    E2E_END_TEST();

    /* Test 5: Double dollar "$$" */
    E2E_START_TEST("\"$$\" double dollar terminates cleanly", "F1.4 Fix Parser Hang", "M1");
    obj = cco_parse_string("\"$$\"", 4, NULL);
    if (obj) cco_object_release(obj);
    E2E_CHECK(true, "Returned without hang");
    E2E_END_TEST();
}

static void test_f1_5_single_line_comments(void) {
    /* Test 1: # comment before root value */
    E2E_START_TEST("# comment before root value", "F1.5 Single-Line Comments", "M1");
    const char* src = "# This is a comment\n42";
    cco_object_t* obj = cco_parse_string(src, strlen(src), NULL);
    if (obj) {
        int64_t val = 0;
        cco_object_get_integer(obj, &val);
        E2E_CHECK_EQ_INT(42, val, "Value after comment should be 42");
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse document with leading # comment");
    }
    E2E_END_TEST();

    /* Test 2: # comment at end of line in map */
    E2E_START_TEST("# comment inline in map", "F1.5 Single-Line Comments", "M1");
    const char* src2 = "(host: \"localhost\" # server host\n, port: 8080 # port\n)";
    obj = cco_parse_string(src2, strlen(src2), NULL);
    if (obj) {
        cco_object_t* p = cco_map_get_by_key(obj, "port");
        E2E_CHECK_NOT_NULL(p, "port key should exist");
        if (p) {
            int64_t val = 0;
            cco_object_get_integer(p, &val);
            E2E_CHECK_EQ_INT(8080, val, "port should be 8080");
        }
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse map with inline # comments");
    }
    E2E_END_TEST();

    /* Test 3: Multiple consecutive # comments */
    E2E_START_TEST("Multiple consecutive # comments", "F1.5 Single-Line Comments", "M1");
    const char* src3 = "# Line 1\n# Line 2\n# Line 3\n\"hello\"";
    obj = cco_parse_string(src3, strlen(src3), NULL);
    if (obj) {
        const char* s = NULL;
        size_t len = 0;
        cco_object_get_string(obj, &s, &len);
        E2E_CHECK_EQ_STR("hello", s, "String after comments should be 'hello'");
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse consecutive comments");
    }
    E2E_END_TEST();

    /* Test 4: Empty single line comment #\n */
    E2E_START_TEST("Empty single line comment #\\n", "F1.5 Single-Line Comments", "M1");
    const char* src4 = "#\n(x: 10)";
    obj = cco_parse_string(src4, strlen(src4), NULL);
    if (obj) {
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse empty comment #\\n");
    }
    E2E_END_TEST();

    /* Test 5: Single-line comment inside array */
    E2E_START_TEST("Single-line comment inside array elements", "F1.5 Single-Line Comments", "M1");
    const char* src5 = "(\n  1, # first item\n  2, # second item\n  3  # third item\n)";
    obj = cco_parse_string(src5, strlen(src5), NULL);
    if (obj) {
        size_t count = cco_array_get_count(obj);
        E2E_CHECK_EQ_INT(3, count, "Array should have 3 items");
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse array with inline comments");
    }
    E2E_END_TEST();
}

static void test_f1_6_block_comments(void) {
    /* Test 1: Simple block comment */
    E2E_START_TEST("/* comment */ before value", "F1.6 Block Comments", "M1");
    const char* src = "/* simple block */ 99";
    cco_object_t* obj = cco_parse_string(src, strlen(src), NULL);
    if (obj) {
        int64_t val = 0;
        cco_object_get_integer(obj, &val);
        E2E_CHECK_EQ_INT(99, val, "Value should be 99");
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse block comment");
    }
    E2E_END_TEST();

    /* Test 2: Multiline block comment */
    E2E_START_TEST("Multiline block comment /* ... */", "F1.6 Block Comments", "M1");
    const char* src2 = "/*\n  Line 1\n  Line 2\n*/ (ok: true)";
    obj = cco_parse_string(src2, strlen(src2), NULL);
    if (obj) {
        cco_object_t* ok_val = cco_map_get_by_key(obj, "ok");
        E2E_CHECK_NOT_NULL(ok_val, "ok key should exist");
        if (ok_val) {
            bool b = false;
            cco_object_get_boolean(ok_val, &b);
            E2E_CHECK(b == true, "ok should be true");
        }
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse multiline block comment");
    }
    E2E_END_TEST();

    /* Test 3: Empty block comment */
    E2E_START_TEST("Empty block comment /**/", "F1.6 Block Comments", "M1");
    const char* src3 = "/**/ [1, 2]";
    obj = cco_parse_string(src3, strlen(src3), NULL);
    if (obj) {
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse empty block comment");
    }
    E2E_END_TEST();

    /* Test 4: Block comment with multiple asterisks */
    E2E_START_TEST("Block comment with multiple asterisks /**** ****/", "F1.6 Block Comments", "M1");
    const char* src4 = "/*********************/ 123";
    obj = cco_parse_string(src4, strlen(src4), NULL);
    if (obj) {
        int64_t val = 0;
        cco_object_get_integer(obj, &val);
        E2E_CHECK_EQ_INT(123, val, "Value should be 123");
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse multi-asterisk comment");
    }
    E2E_END_TEST();

    /* Test 5: Block comment between map entries */
    E2E_START_TEST("Block comment between map entries", "F1.6 Block Comments", "M1");
    const char* src5 = "(a: 1 /* entry divider */, b: 2)";
    obj = cco_parse_string(src5, strlen(src5), NULL);
    if (obj) {
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse map with embedded block comment");
    }
    E2E_END_TEST();
}

static void test_f1_7_duplicate_key_rejection(void) {
    /* Test 1: Immediate duplicate key returns CCO_ERR_PARSE */
    E2E_START_TEST("Immediate duplicate key (a: 1, a: 2)", "F1.7 Duplicate Key Rejection", "M1");
    cco_clear_diagnostics();
    cco_object_t* obj = cco_parse_string("(a: 1, a: 2)", 12, NULL);
    E2E_CHECK(obj == NULL, "Parsing duplicate key must return NULL");
    E2E_CHECK_EQ_INT(CCO_ERR_PARSE, cco_get_last_error(), "Error code should be CCO_ERR_PARSE");
    if (obj) cco_object_release(obj);
    E2E_END_TEST();

    /* Test 2: Separated duplicate key (x: 10, y: 20, x: 30) */
    E2E_START_TEST("Separated duplicate key (x: 10, y: 20, x: 30)", "F1.7 Duplicate Key Rejection", "M1");
    cco_clear_diagnostics();
    obj = cco_parse_string("(x: 10, y: 20, x: 30)", 21, NULL);
    E2E_CHECK(obj == NULL, "Separated duplicate key must return NULL");
    E2E_CHECK_EQ_INT(CCO_ERR_PARSE, cco_get_last_error(), "Error code should be CCO_ERR_PARSE");
    if (obj) cco_object_release(obj);
    E2E_END_TEST();

    /* Test 3: Dotted duplicate key (a.b: 1, a.b: 2) */
    E2E_START_TEST("Dotted duplicate key (a.b: 1, a.b: 2)", "F1.7 Duplicate Key Rejection", "M1");
    cco_clear_diagnostics();
    obj = cco_parse_string("(a.b: 1, a.b: 2)", 16, NULL);
    E2E_CHECK(obj == NULL, "Dotted duplicate key must return NULL");
    E2E_CHECK_EQ_INT(CCO_ERR_PARSE, cco_get_last_error(), "Error code should be CCO_ERR_PARSE");
    if (obj) cco_object_release(obj);
    E2E_END_TEST();

    /* Test 4: Quoted duplicate key (\"key\": 1, \"key\": 2) */
    E2E_START_TEST("Quoted duplicate key (\"key\": 1, \"key\": 2)", "F1.7 Duplicate Key Rejection", "M1");
    cco_clear_diagnostics();
    obj = cco_parse_string("(\"key\": 1, \"key\": 2)", 20, NULL);
    E2E_CHECK(obj == NULL, "Quoted duplicate key must return NULL");
    E2E_CHECK_EQ_INT(CCO_ERR_PARSE, cco_get_last_error(), "Error code should be CCO_ERR_PARSE");
    if (obj) cco_object_release(obj);
    E2E_END_TEST();

    /* Test 5: Mixed unquoted and quoted duplicate (foo: 1, \"foo\": 2) */
    E2E_START_TEST("Mixed unquoted and quoted duplicate (foo: 1, \"foo\": 2)", "F1.7 Duplicate Key Rejection", "M1");
    cco_clear_diagnostics();
    obj = cco_parse_string("(foo: 1, \"foo\": 2)", 18, NULL);
    E2E_CHECK(obj == NULL, "Mixed unquoted and quoted duplicate key must return NULL");
    E2E_CHECK_EQ_INT(CCO_ERR_PARSE, cco_get_last_error(), "Error code should be CCO_ERR_PARSE");
    if (obj) cco_object_release(obj);
    E2E_END_TEST();
}

int run_tier1_lexer_literals(void) {
    printf(E2E_COLOR_BOLD "--- Running Tier 1: Lexer & Literals (F1.1 - F1.7) ---\n" E2E_COLOR_RESET);
    test_f1_1_scientific_notation();
    test_f1_2_hex_binary_octal();
    test_f1_3_signed_numbers();
    test_f1_4_fix_parser_hang();
    test_f1_5_single_line_comments();
    test_f1_6_block_comments();
    test_f1_7_duplicate_key_rejection();
    return 0;
}
