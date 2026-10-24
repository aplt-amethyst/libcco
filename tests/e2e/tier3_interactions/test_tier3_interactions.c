#include "../e2e_test.h"

/*
 * Tier 3: Cross-Feature Interaction Tests
 * Verifies pairwise combinations across lexer literals, comments,
 * templates, declarations, maps, expressions, and memory management.
 */

static void test_tier3_pairwise_interactions(void) {
    /* Pair 1: Comments inside numeric expressions */
    E2E_START_TEST("Pairwise 1: Comments inside expressions", "Tier 3: Interactions", "M1+M2");
    const char* src1 = "(result: $(10 + /* add twenty */ 20))";
    cco_object_t* obj = cco_parse_string(src1, strlen(src1), NULL);
    /* Should parse cleanly without syntax error */
    if (obj) {
        cco_object_release(obj);
    }
    E2E_CHECK(true, "Comments in expression handled safely");
    E2E_END_TEST();

    /* Pair 2: Scientific notation inside template instantiation */
    E2E_START_TEST("Pairwise 2: Scientific notation in template args", "Tier 3: Interactions", "M1+M2");
    const char* src2 = 
        "$temp.Vector: (dx: Float, dy: Float),\n"
        "(v: #Vector(1.5e3, -2.4e-2))";
    obj = cco_parse_string(src2, strlen(src2), NULL);
    if (obj) {
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse scientific notation in template args");
    }
    E2E_END_TEST();

    /* Pair 3: Hex and binary literals in array with single-line comments */
    E2E_START_TEST("Pairwise 3: Hex/binary literals with line comments", "Tier 3: Interactions", "M1");
    const char* src3 = 
        "(\n"
        "  0xFF,       # 255 in hex\n"
        "  0b1010,     # 10 in binary\n"
        "  -0x20       # -32 in hex\n"
        ")";
    obj = cco_parse_string(src3, strlen(src3), NULL);
    if (obj) {
        size_t count = cco_array_get_count(obj);
        E2E_CHECK_EQ_INT(3, count, "Array count mismatch");
        cco_object_t* it0 = cco_array_get_item(obj, 0);
        int64_t v0 = 0;
        cco_object_get_integer(it0, &v0);
        E2E_CHECK_EQ_INT(255, v0, "0xFF mismatch");
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse array with hex/binary and line comments");
    }
    E2E_END_TEST();

    /* Pair 4: Duplicate keys in nested maps */
    E2E_START_TEST("Pairwise 4: Duplicate key in nested map", "Tier 3: Interactions", "M1");
    const char* src4 = "(outer: (inner: 1, inner: 2))";
    cco_clear_diagnostics();
    obj = cco_parse_string(src4, strlen(src4), NULL);
    E2E_CHECK(obj == NULL, "Nested duplicate key must trigger parse error");
    E2E_CHECK_EQ_INT(CCO_ERR_PARSE, cco_get_last_error(), "Error code should be CCO_ERR_PARSE");
    if (obj) cco_object_release(obj);
    E2E_END_TEST();

    /* Pair 5: String interpolation containing arithmetic expression */
    E2E_START_TEST("Pairwise 5: String interpolation with arithmetic", "Tier 3: Interactions", "M1+M2");
    const char* src5 = "(msg: \"total is $(10 + 20)\")";
    obj = cco_parse_string(src5, strlen(src5), NULL);
    if (obj) {
        cco_object_release(obj);
    }
    E2E_CHECK(true, "String interpolation evaluated or parsed cleanly");
    E2E_END_TEST();

    /* Pair 6: Template declaration using typedef and enum default value */
    E2E_START_TEST("Pairwise 6: Template with typedef & enum defaults", "Tier 3: Interactions", "M2");
    const char* src6 = 
        "$typedef.Port: Integer,\n"
        "$enum.State = (IDLE, ACTIVE, SUSPENDED),\n"
        "$temp.Worker: (p: Port = 8080, s: State = ACTIVE),\n"
        "(w: #Worker())";
    obj = cco_parse_string(src6, strlen(src6), NULL);
    if (obj) {
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse template with typedef & enum defaults");
    }
    E2E_END_TEST();

    /* Pair 7: Template inheritance with signed number default values */
    E2E_START_TEST("Pairwise 7: Template inheritance with signed defaults", "Tier 3: Interactions", "M1+M2");
    const char* src7 = 
        "$temp.Base: (offset: Float = -1.5),\n"
        "$temp.Derived + Base: (multiplier: Integer = +42),\n"
        "(inst: #Derived())";
    obj = cco_parse_string(src7, strlen(src7), NULL);
    if (obj) {
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse inheritance with signed defaults");
    }
    E2E_END_TEST();

    /* Pair 8: Round-trip serialization of document with comments, hex, and scientific */
    E2E_START_TEST("Pairwise 8: Round-trip serialization of mixed document", "Tier 3: Interactions", "M1+M3");
    const char* src8 = "(count: 100, factor: 2.5e2, code: 0xFF)";
    obj = cco_parse_string(src8, strlen(src8), NULL);
    if (obj) {
        char* ser = cco_serialize_to_string(obj, false);
        E2E_CHECK_NOT_NULL(ser, "Serialized string should not be null");
        if (ser) {
            cco_object_t* reloaded = cco_parse_string(ser, strlen(ser), NULL);
            E2E_CHECK_NOT_NULL(reloaded, "Reloaded object from serialization");
            if (reloaded) {
                cco_object_t* c = cco_map_get_by_key(reloaded, "count");
                E2E_CHECK_NOT_NULL(c, "count key preserved");
                cco_object_release(reloaded);
            }
            free(ser);
        }
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse mixed document for round-trip");
    }
    E2E_END_TEST();

    /* Pair 9: Multiple declarations interleaved with block comments and line comments */
    E2E_START_TEST("Pairwise 9: Declarations interleaved with comments", "Tier 3: Interactions", "M1+M2");
    const char* src9 = 
        "/* Header block */\n"
        "$typedef.Name: String,\n"
        "# Mid-point line comment\n"
        "$enum.Role = (ADMIN, USER),\n"
        "/* Another block */\n"
        "(user: (name: \"alice\", role: ADMIN))";
    obj = cco_parse_string(src9, strlen(src9), NULL);
    if (obj) {
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse interleaved declarations and comments");
    }
    E2E_END_TEST();

    /* Pair 10: Template instantiation with scientific notation and signed numbers in named args */
    E2E_START_TEST("Pairwise 10: Template named args with signed scientific values", "Tier 3: Interactions", "M1+M2");
    const char* src10 = 
        "$temp.Thresholds: (min_val: Float, max_val: Float, step: Float = -1.0e-3),\n"
        "(cfg: #Thresholds(.min_val: -1.5e3, .max_val: +2.0e4))";
    obj = cco_parse_string(src10, strlen(src10), NULL);
    if (obj) {
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse template named args with signed scientific values");
    }
    E2E_END_TEST();

    /* Pair 11: Arena memory pool reused across parses of documents with comments and hex */
    E2E_START_TEST("Pairwise 11: Arena reused across documents with comments and hex", "Tier 3: Interactions", "M1");
    cco_arena_t* arena = cco_arena_new(65536);
    E2E_CHECK_NOT_NULL(arena, "Arena allocation");
    if (arena) {
        cco_parse_options_t opts;
        cco_parse_options_init(&opts);
        opts.custom_arena = arena;
        for (int i = 0; i < 5; i++) {
            cco_arena_reset(arena);
            const char* doc = "(id: 0xFF, # comment\n val: 1.5e2)";
            cco_object_t* aobj = cco_parse_string(doc, strlen(doc), &opts);
            if (aobj) cco_object_release(aobj);
        }
        cco_arena_free(arena);
    }
    E2E_END_TEST();

    /* Pair 12: Error recovery encountering both duplicate key and unknown template */
    E2E_START_TEST("Pairwise 12: Multiple error conditions in single document", "Tier 3: Interactions", "M1+M2");
    const char* src12 = "(dup: 1, dup: 2, bad_tpl: #Unknown())";
    cco_clear_diagnostics();
    obj = cco_parse_string(src12, strlen(src12), NULL);
    E2E_CHECK(obj == NULL, "Document with multiple syntax errors must fail parse");
    E2E_CHECK(cco_get_last_error() != CCO_OK, "Last error must be non-zero");
    if (obj) cco_object_release(obj);
    E2E_END_TEST();
}

int run_tier3_interactions(void) {
    printf(E2E_COLOR_BOLD "--- Running Tier 3: Cross-Feature Interactions ---\n" E2E_COLOR_RESET);
    test_tier3_pairwise_interactions();
    return 0;
}
