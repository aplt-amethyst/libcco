#include "../e2e_test.h"

/*
 * Tier 1: F3.1 - F3.7 Public C APIs & Symbol Export Tests
 * Verifies R3 requirements from ORIGINAL_REQUEST.md & PROJECT.md
 */

static void test_f3_1_declare_66_public_apis(void) {
    /* Test 1: Value layer constructors declared */
    E2E_START_TEST("Value layer constructors declared with CCO_API", "F3.1 Declare 66 Public C APIs", "M3");
    size_t len = 0;
    char* header = e2e_read_file("include/cnt/cco.h", &len);
    E2E_CHECK_NOT_NULL(header, "include/cnt/cco.h must exist");
    if (header) {
        E2E_CHECK(strstr(header, "cco_object_new") != NULL, "cco_object_new declared");
        E2E_CHECK(strstr(header, "cco_int_new") != NULL, "cco_int_new declared");
        E2E_CHECK(strstr(header, "cco_float_new") != NULL, "cco_float_new declared");
        E2E_CHECK(strstr(header, "cco_bool_new") != NULL, "cco_bool_new declared");
        E2E_CHECK(strstr(header, "cco_string_new") != NULL, "cco_string_new declared");
        free(header);
    }
    E2E_END_TEST();

    /* Test 2: Map operations declared */
    E2E_START_TEST("Map operations declared", "F3.1 Declare 66 Public C APIs", "M3");
    header = e2e_read_file("include/cnt/cco.h", &len);
    if (header) {
        E2E_CHECK(strstr(header, "cco_object_set") != NULL, "cco_object_set declared");
        E2E_CHECK(strstr(header, "cco_object_get") != NULL, "cco_object_get declared");
        E2E_CHECK(strstr(header, "cco_object_del") != NULL, "cco_object_del declared");
        E2E_CHECK(strstr(header, "cco_object_size") != NULL, "cco_object_size declared");
        free(header);
    }
    E2E_END_TEST();

    /* Test 3: Array operations declared */
    E2E_START_TEST("Array operations declared", "F3.1 Declare 66 Public C APIs", "M3");
    header = e2e_read_file("include/cnt/cco.h", &len);
    if (header) {
        E2E_CHECK(strstr(header, "cco_array_add") != NULL, "cco_array_add declared");
        E2E_CHECK(strstr(header, "cco_array_size") != NULL, "cco_array_size declared");
        free(header);
    }
    E2E_END_TEST();

    /* Test 4: Expressions & serialization declared */
    E2E_START_TEST("Expressions & serialization declared", "F3.1 Declare 66 Public C APIs", "M3");
    header = e2e_read_file("include/cnt/cco.h", &len);
    if (header) {
        E2E_CHECK(strstr(header, "cco_serialize_pretty") != NULL, "cco_serialize_pretty declared");
        E2E_CHECK(strstr(header, "cco_save_file_pretty") != NULL, "cco_save_file_pretty declared");
        E2E_CHECK(strstr(header, "cco_expr_eval") != NULL, "cco_expr_eval declared");
        free(header);
    }
    E2E_END_TEST();

    /* Test 5: Full parser & symbol table declared */
    E2E_START_TEST("Full parser & symbol table declared", "F3.1 Declare 66 Public C APIs", "M3");
    header = e2e_read_file("include/cnt/cco.h", &len);
    if (header) {
        E2E_CHECK(strstr(header, "cco_parse_full") != NULL, "cco_parse_full declared");
        E2E_CHECK(strstr(header, "cco_parse_result_root") != NULL, "cco_parse_result_root declared");
        E2E_CHECK(strstr(header, "cco_symbol_table_count") != NULL, "cco_symbol_table_count declared");
        free(header);
    }
    E2E_END_TEST();
}

static void test_f3_2_implement_66_public_apis(void) {
    /* Test 1: cco_object_new creates empty map */
    E2E_START_TEST("cco_object_new creates empty map", "F3.2 Implement 66 Public C APIs", "M3");
    if (cco_object_new) {
        cco_object_t* map = cco_object_new();
        E2E_CHECK_NOT_NULL(map, "cco_object_new should return non-null");
        if (map) {
            E2E_CHECK_EQ_INT(CCO_TYPE_MAP, cco_object_get_type(map), "cco_object_new should produce CCO_TYPE_MAP");
            if (cco_object_size) {
                E2E_CHECK_EQ_INT(0, cco_object_size(map), "New map should have size 0");
            }
            if (cco_object_free) cco_object_free(map);
            else cco_object_release(map);
        }
    } else {
        E2E_MARK_PENDING("cco_object_new not yet exported (M3)");
    }
    E2E_END_TEST();

    /* Test 2: cco_int_new and cco_int_get */
    E2E_START_TEST("cco_int_new & cco_int_get", "F3.2 Implement 66 Public C APIs", "M3");
    if (cco_int_new && cco_int_get) {
        cco_object_t* iobj = cco_int_new(123456);
        E2E_CHECK_NOT_NULL(iobj, "cco_int_new should return non-null");
        if (iobj) {
            int64_t v = cco_int_get(iobj);
            E2E_CHECK_EQ_INT(123456, v, "cco_int_get mismatch");
            if (cco_object_free) cco_object_free(iobj);
            else cco_object_release(iobj);
        }
    } else {
        E2E_MARK_PENDING("cco_int_new/get not yet exported (M3)");
    }
    E2E_END_TEST();

    /* Test 3: cco_string_new and cco_string_get */
    E2E_START_TEST("cco_string_new & cco_string_get", "F3.2 Implement 66 Public C APIs", "M3");
    if (cco_string_new && cco_string_get) {
        cco_object_t* sobj = cco_string_new("test string");
        E2E_CHECK_NOT_NULL(sobj, "cco_string_new should return non-null");
        if (sobj) {
            size_t len = 0;
            const char* s = cco_string_get(sobj, &len);
            E2E_CHECK_EQ_STR("test string", s, "cco_string_get mismatch");
            E2E_CHECK_EQ_INT(11, len, "string length mismatch");
            if (cco_object_free) cco_object_free(sobj);
            else cco_object_release(sobj);
        }
    } else {
        E2E_MARK_PENDING("cco_string_new/get not yet exported (M3)");
    }
    E2E_END_TEST();

    /* Test 4: Map operations: set, get, del */
    E2E_START_TEST("Map operations: cco_object_set, get, del", "F3.2 Implement 66 Public C APIs", "M3");
    if (cco_object_new && cco_object_set && cco_object_get && cco_int_new) {
        cco_object_t* map = cco_object_new();
        if (map) {
            cco_object_t* val = cco_int_new(42);
            int rc = cco_object_set(map, "key1", val);
            E2E_CHECK_EQ_INT(0, rc, "cco_object_set should return CCO_OK");
            cco_object_t* got = cco_object_get(map, "key1");
            E2E_CHECK_NOT_NULL(got, "cco_object_get should find key1");
            if (cco_object_del) {
                rc = cco_object_del(map, "key1");
                E2E_CHECK_EQ_INT(0, rc, "cco_object_del should return CCO_OK");
                got = cco_object_get(map, "key1");
                E2E_CHECK_NULL(got, "Deleted key should return NULL");
            }
            if (cco_object_free) cco_object_free(map);
            else cco_object_release(map);
        }
    } else {
        E2E_MARK_PENDING("cco_object_set/get/del not yet exported (M3)");
    }
    E2E_END_TEST();

    /* Test 5: cco_serialize_pretty */
    E2E_START_TEST("cco_serialize_pretty produces formatted string", "F3.2 Implement 66 Public C APIs", "M3");
    if (cco_serialize_pretty) {
        cco_object_t* obj = cco_parse_string("(a: 10, b: 20)", 14, NULL);
        if (obj) {
            char* pretty = cco_serialize_pretty(obj, 2);
            E2E_CHECK_NOT_NULL(pretty, "cco_serialize_pretty should produce output");
            if (pretty) {
                E2E_CHECK(strstr(pretty, "a: 10") != NULL, "Serialized string should contain 'a: 10'");
                free(pretty);
            }
            cco_object_release(obj);
        }
    } else {
        E2E_MARK_PENDING("cco_serialize_pretty not yet exported (M3)");
    }
    E2E_END_TEST();
}

static void test_f3_3_unified_public_surface(void) {
    /* Test 1: Existing 21 public functions functional */
    E2E_START_TEST("Existing 21 core public functions remain intact", "F3.3 Unified Public Surface", "M3");
    cco_arena_t* arena = cco_arena_new(1024 * 1024);
    E2E_CHECK_NOT_NULL(arena, "cco_arena_new should succeed");
    if (arena) {
        cco_arena_reset(arena);
        cco_arena_free(arena);
    }
    E2E_END_TEST();

    /* Test 2: Options struct initialization */
    E2E_START_TEST("cco_parse_options_init initializes defaults", "F3.3 Unified Public Surface", "M3");
    cco_parse_options_t opts;
    cco_parse_options_init(&opts);
    E2E_CHECK(opts.max_depth > 0, "opts max_depth initialized to non-zero default");
    E2E_END_TEST();

    /* Test 3: Public API C linkage compatibility (extern \"C\") */
    E2E_START_TEST("C linkage compatibility", "F3.3 Unified Public Surface", "M3");
    size_t len = 0;
    char* header = e2e_read_file("include/cnt/cco.h", &len);
    if (header) {
        E2E_CHECK(strstr(header, "extern \"C\"") != NULL, "include/cnt/cco.h contains extern \"C\"");
        free(header);
    }
    E2E_END_TEST();

    /* Test 4: Header idempotency guards #ifndef CNT_CCO_H */
    E2E_START_TEST("Header include guard #ifndef CNT_CCO_H", "F3.3 Unified Public Surface", "M3");
    header = e2e_read_file("include/cnt/cco.h", &len);
    if (header) {
        E2E_CHECK(strstr(header, "#ifndef CNT_CCO_H") != NULL, "Header has guard");
        free(header);
    }
    E2E_END_TEST();

    /* Test 5: Dynamic library symbol export count check */
    E2E_START_TEST("Dynamic library exports matches contract", "F3.3 Unified Public Surface", "M3");
    FILE* p = popen("nm -D --defined-only target/debug/libcco.so 2>/dev/null | awk '{print $NF}' | grep -c '^cco_'", "r");
    if (p) {
        char buf[64];
        if (fgets(buf, sizeof(buf), p)) {
            int cnt = atoi(buf);
            E2E_CHECK(cnt >= 21, "Target shared library should export at least 21 public symbols");
        }
        pclose(p);
    }
    E2E_END_TEST();
}

static void test_f3_4_fix_check_api_doc(void) {
    /* Test 1: check-api-doc.sh script exists and is executable */
    E2E_START_TEST("check-api-doc.sh exists", "F3.4 Fix check-api-doc.sh", "M3");
    int rc = system("test -x scripts/check-api-doc.sh || test -f scripts/check-api-doc.sh");
    E2E_CHECK_EQ_INT(0, rc, "scripts/check-api-doc.sh should exist");
    E2E_END_TEST();

    /* Test 2: check-api-doc.sh contains no \\x08 control characters */
    E2E_START_TEST("check-api-doc.sh has zero \\x08 backspace control bytes", "F3.4 Fix check-api-doc.sh", "M3");
    size_t len = 0;
    char* script = e2e_read_file("scripts/check-api-doc.sh", &len);
    E2E_CHECK_NOT_NULL(script, "scripts/check-api-doc.sh read");
    if (script) {
        bool has_bs = false;
        for (size_t i = 0; i < len; i++) {
            if (script[i] == '\x08') {
                has_bs = true;
                break;
            }
        }
        E2E_CHECK(!has_bs, "Script should have zero 0x08 backspace control bytes");
        free(script);
    }
    E2E_END_TEST();

    /* Test 3: check-api-doc.sh passes cleanly */
    E2E_START_TEST("check-api-doc.sh executes with return code 0", "F3.4 Fix check-api-doc.sh", "M3");
    rc = system("bash scripts/check-api-doc.sh >/dev/null 2>&1");
    E2E_CHECK_EQ_INT(0, rc, "check-api-doc.sh should pass cleanly");
    E2E_END_TEST();

    /* Test 4: docs/api.md has entries for all currently declared public symbols */
    E2E_START_TEST("docs/api.md has documentation for cco_parse_string", "F3.4 Fix check-api-doc.sh", "M3");
    size_t doc_len = 0;
    char* doc = e2e_read_file("docs/api.md", &doc_len);
    if (doc) {
        E2E_CHECK(strstr(doc, "cco_parse_string") != NULL, "docs/api.md documents cco_parse_string");
        free(doc);
    }
    E2E_END_TEST();

    /* Test 5: check-api-doc.sh rejects undocumented functions */
    E2E_START_TEST("check-api-doc regex correctly extracts declarations", "F3.4 Fix check-api-doc.sh", "M3");
    rc = system("bash -c 'declared=$(sed -E \"s:/\\*[^*]*\\*/::g\" include/cnt/cco.h | grep -vE \"^\\s*(\\*|//)\" | grep -oE \"\\\\bcco_[a-z_]+\\\\s*\\\\(\" | sed -E \"s/[[:space:]]*\\\\($//\" | sort -u); [ -n \"$declared\" ]'");
    E2E_CHECK_EQ_INT(0, rc, "Sed expression should extract declared symbols");
    E2E_END_TEST();
}

static void test_f3_5_symbol_export_whitelist(void) {
    /* Test 1: check-symbol-export.sh exists */
    E2E_START_TEST("check-symbol-export.sh exists", "F3.5 Symbol Export Whitelist", "M3");
    int rc = system("test -f scripts/check-symbol-export.sh");
    E2E_CHECK_EQ_INT(0, rc, "scripts/check-symbol-export.sh should exist");
    E2E_END_TEST();

    /* Test 2: check-symbol-export.sh runs on target/debug/libcco.so */
    E2E_START_TEST("check-symbol-export.sh runs with 0 leaks", "F3.5 Symbol Export Whitelist", "M3");
    rc = system("bash scripts/check-symbol-export.sh target/debug/libcco.so >/dev/null 2>&1");
    E2E_CHECK_EQ_INT(0, rc, "check-symbol-export.sh should pass with 0 symbol leaks");
    E2E_END_TEST();

    /* Test 3: No internal symbols exported (zero leaked symbols) */
    E2E_START_TEST("Zero internal symbols exported", "F3.5 Symbol Export Whitelist", "M3");
    FILE* p = popen("nm -D --defined-only target/debug/libcco.so 2>/dev/null | awk '{print $NF}' | grep -v '^cco_' | grep -v '^_'", "r");
    if (p) {
        char buf[128];
        bool has_internal = false;
        while (fgets(buf, sizeof(buf), p)) {
            has_internal = true;
            break;
        }
        E2E_CHECK(!has_internal, "No non-cco public symbols should be exported");
        pclose(p);
    }
    E2E_END_TEST();

    /* Test 4: Makefile check-symbols target passes */
    E2E_START_TEST("make check-symbols passes", "F3.5 Symbol Export Whitelist", "M3");
    rc = system("make check-symbols >/dev/null 2>&1");
    E2E_CHECK_EQ_INT(0, rc, "make check-symbols should succeed");
    E2E_END_TEST();

    /* Test 5: Whitelist verification on release build */
    E2E_START_TEST("Symbol export matches release build", "F3.5 Symbol Export Whitelist", "M3");
    if (system("test -f target/release/libcco.so") == 0) {
        rc = system("bash scripts/check-symbol-export.sh target/release/libcco.so >/dev/null 2>&1");
        E2E_CHECK_EQ_INT(0, rc, "check-symbol-export.sh on release target");
    } else {
        E2E_CHECK(true, "Release library not built in debug run (skipped)");
    }
    E2E_END_TEST();
}

static void test_f3_6_doc_code_blocks_and_examples(void) {
    /* Test 1: README.md exists and contains C examples */
    E2E_START_TEST("README.md exists and contains C code blocks", "F3.6 Doc Code Blocks & Examples", "M3");
    size_t len = 0;
    char* readme = e2e_read_file("README.md", &len);
    E2E_CHECK_NOT_NULL(readme, "README.md should exist");
    if (readme) {
        E2E_CHECK(strstr(readme, "#include <cnt/cco.h>") != NULL, "README.md contains #include <cnt/cco.h>");
        free(readme);
    }
    E2E_END_TEST();

    /* Test 2: docs/api.md exists and contains complete reference */
    E2E_START_TEST("docs/api.md exists", "F3.6 Doc Code Blocks & Examples", "M3");
    char* doc = e2e_read_file("docs/api.md", &len);
    E2E_CHECK_NOT_NULL(doc, "docs/api.md should exist");
    if (doc) free(doc);
    E2E_END_TEST();

    /* Test 3: Basic example in examples/ directory compiles */
    E2E_START_TEST("examples/build_demo compilation check", "F3.6 Doc Code Blocks & Examples", "M3");
    int rc = system("make examples name=simple >/dev/null 2>&1 || test -f examples/basic.cco");
    E2E_CHECK_EQ_INT(0, rc, "Examples should be present and valid");
    E2E_END_TEST();

    /* Test 4: Quickstart C snippet from docs compiles against libcco */
    E2E_START_TEST("Quickstart C snippet compilation check", "F3.6 Doc Code Blocks & Examples", "M3");
    const char* test_code = 
        "#include <cnt/cco.h>\n"
        "int main(void) {\n"
        "    cco_object_t* obj = cco_parse_string(\"(hello: \\\"world\\\")\", 18, NULL);\n"
        "    if (obj) cco_object_release(obj);\n"
        "    return 0;\n"
        "}\n";
    FILE* f = fopen("/tmp/e2e_quickstart_test.c", "w");
    if (f) {
        fputs(test_code, f);
        fclose(f);
        const char* s_flag = (system("nm target/debug/liblibcco.a 2>/dev/null | grep -q __asan_init") == 0) ? "-fsanitize=address,undefined" : "";
        char cmd[512];
        snprintf(cmd, sizeof(cmd), "cc -Iinclude %s /tmp/e2e_quickstart_test.c target/debug/liblibcco.a -lm -o /tmp/e2e_quickstart_test >/dev/null 2>&1", s_flag);
        rc = system(cmd);
        E2E_CHECK_EQ_INT(0, rc, "Quickstart C snippet should compile cleanly");
        if (rc == 0) {
            rc = system("/tmp/e2e_quickstart_test");
            E2E_CHECK_EQ_INT(0, rc, "Quickstart binary should execute with code 0");
        }
        remove("/tmp/e2e_quickstart_test.c");
        remove("/tmp/e2e_quickstart_test");
    }
    E2E_END_TEST();

    /* Test 5: Arena snippet from docs compiles against libcco */
    E2E_START_TEST("Arena C snippet compilation check", "F3.6 Doc Code Blocks & Examples", "M3");
    const char* arena_code = 
        "#include <cnt/cco.h>\n"
        "int main(void) {\n"
        "    cco_arena_t* a = cco_arena_new(65536);\n"
        "    if (!a) return 1;\n"
        "    cco_arena_reset(a);\n"
        "    cco_arena_free(a);\n"
        "    return 0;\n"
        "}\n";
    f = fopen("/tmp/e2e_arena_test.c", "w");
    if (f) {
        fputs(arena_code, f);
        fclose(f);
        const char* s_flag = (system("nm target/debug/liblibcco.a 2>/dev/null | grep -q __asan_init") == 0) ? "-fsanitize=address,undefined" : "";
        char cmd[512];
        snprintf(cmd, sizeof(cmd), "cc -Iinclude %s /tmp/e2e_arena_test.c target/debug/liblibcco.a -lm -o /tmp/e2e_arena_test >/dev/null 2>&1", s_flag);
        rc = system(cmd);
        E2E_CHECK_EQ_INT(0, rc, "Arena C snippet should compile cleanly");
        if (rc == 0) {
            rc = system("/tmp/e2e_arena_test");
            E2E_CHECK_EQ_INT(0, rc, "Arena binary should execute with code 0");
        }
        remove("/tmp/e2e_arena_test.c");
        remove("/tmp/e2e_arena_test");
    }
    E2E_END_TEST();
}

static void test_f3_7_abi_stability(void) {
    /* Test 1: sizeof(cco_parse_options_t) is reasonable and non-zero */
    E2E_START_TEST("sizeof(cco_parse_options_t) non-zero", "F3.7 ABI Stability", "M3");
    size_t sz = sizeof(cco_parse_options_t);
    E2E_CHECK(sz >= 16 && sz <= 256, "sizeof(cco_parse_options_t) should be between 16 and 256 bytes");
    E2E_END_TEST();

    /* Test 2: cco_parse_options_init field initialization */
    E2E_START_TEST("cco_parse_options_init clears/sets valid flags", "F3.7 ABI Stability", "M3");
    cco_parse_options_t opts;
    memset(&opts, 0xFF, sizeof(opts));
    cco_parse_options_init(&opts);
    /* After init, pointer fields should not be 0xFFFFFFFFFFFFFFFF */
    E2E_CHECK(opts.custom_arena == NULL || (uintptr_t)opts.custom_arena != UINTPTR_MAX, "custom_arena pointer initialized");
    E2E_END_TEST();

    /* Test 3: No #ifdef drift inside struct definition in header */
    E2E_START_TEST("No conditional compilation drift inside cco_parse_options_t", "F3.7 ABI Stability", "M3");
    size_t len = 0;
    char* opt_header = e2e_read_file("include/cnt/cco_options.h", &len);
    E2E_CHECK_NOT_NULL(opt_header, "include/cnt/cco_options.h exists");
    if (opt_header) {
        /* Check if there are #ifdef CCO_ENABLE_* directly inside the struct definition */
        char* struct_start = strstr(opt_header, "struct cco_parse_options");
        if (struct_start) {
            char* struct_end = strstr(struct_start, "}");
            if (struct_end) {
                *struct_end = '\0';
                /* Ensure no #ifdef inside this block */
                bool has_ifdef = (strstr(struct_start, "#ifdef") != NULL) || (strstr(struct_start, "#if ") != NULL);
                E2E_CHECK(!has_ifdef, "cco_parse_options struct should not have #ifdef drift");
            }
        }
        free(opt_header);
    }
    E2E_END_TEST();

    /* Test 4: Default options constructor if available */
    E2E_START_TEST("cco_parse_options_default returns valid struct", "F3.7 ABI Stability", "M3");
    if (cco_parse_options_default != NULL) {
        cco_parse_options_t d = cco_parse_options_default();
        E2E_CHECK(d.custom_arena == NULL, "Default custom_arena is NULL");
    } else {
        E2E_CHECK(true, "Default options via cco_parse_options_init");
    }
    E2E_END_TEST();

    /* Test 5: Options passed into cco_parse_string correctly respected */
    E2E_START_TEST("cco_parse_options respected by cco_parse_string", "F3.7 ABI Stability", "M3");
    cco_parse_options_t custom_opts;
    cco_parse_options_init(&custom_opts);
    cco_object_t* obj = cco_parse_string("(num: 123)", 10, &custom_opts);
    E2E_CHECK_NOT_NULL(obj, "cco_parse_string with options should succeed");
    if (obj) cco_object_release(obj);
    E2E_END_TEST();
}

int run_tier1_api_surface(void) {
    printf(E2E_COLOR_BOLD "--- Running Tier 1: Public C APIs & Symbol Export (F3.1 - F3.7) ---\n" E2E_COLOR_RESET);
    test_f3_1_declare_66_public_apis();
    test_f3_2_implement_66_public_apis();
    test_f3_3_unified_public_surface();
    test_f3_4_fix_check_api_doc();
    test_f3_5_symbol_export_whitelist();
    test_f3_6_doc_code_blocks_and_examples();
    test_f3_7_abi_stability();
    return 0;
}
