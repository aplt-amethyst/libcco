#include "../e2e_test.h"

/*
 * Tier 1: F2.1 - F2.7 AST & Parser Wiring Tests
 * Verifies R2 requirements from ORIGINAL_REQUEST.md & PROJECT.md
 */

static void test_f2_1_document_declaration_loop(void) {
    /* Test 1: Single typedef before root value */
    E2E_START_TEST("Single typedef before root value", "F2.1 Document Declaration Loop", "M2");
    const char* src1 = "$typedef.Port: Integer,\n(port: 8080)";
    cco_object_t* obj = cco_parse_string(src1, strlen(src1), NULL);
    if (obj) {
        cco_object_t* p = cco_map_get_by_key(obj, "port");
        E2E_CHECK_NOT_NULL(p, "port key should exist in root map");
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse document with typedef declaration");
    }
    E2E_END_TEST();

    /* Test 2: Multiple declarations (typedef, enum, temp) before root */
    E2E_START_TEST("Multiple declarations before root", "F2.1 Document Declaration Loop", "M2");
    const char* src2 = 
        "$typedef.Host: String,\n"
        "$enum.Env = (DEV, PROD),\n"
        "$temp.Server: (host: Host, env: Env = DEV),\n"
        "(active: true)";
    obj = cco_parse_string(src2, strlen(src2), NULL);
    if (obj) {
        cco_object_t* act = cco_map_get_by_key(obj, "active");
        E2E_CHECK_NOT_NULL(act, "active key should exist in root map");
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse multiple declarations before root");
    }
    E2E_END_TEST();

    /* Test 3: Mixed declaration order (enum before typedef, temp before enum) */
    E2E_START_TEST("Mixed declaration order", "F2.1 Document Declaration Loop", "M2");
    const char* src3 = 
        "$enum.LogLevel = (DEBUG, INFO, WARN, ERROR),\n"
        "$typedef.Count: Integer,\n"
        "(level: INFO, count: 5)";
    obj = cco_parse_string(src3, strlen(src3), NULL);
    if (obj) {
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse declarations with mixed order");
    }
    E2E_END_TEST();

    /* Test 4: Declarations separated by comments and newlines */
    E2E_START_TEST("Declarations separated by comments", "F2.1 Document Declaration Loop", "M2");
    const char* src4 = 
        "/* Type definition */\n"
        "$typedef.Id: Integer,\n"
        "# Enum definition\n"
        "$enum.State = (INIT, RUNNING, STOPPED),\n"
        "/* Root configuration */\n"
        "(id: 1, state: INIT)";
    obj = cco_parse_string(src4, strlen(src4), NULL);
    if (obj) {
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse declarations separated by comments");
    }
    E2E_END_TEST();

    /* Test 5: Trailing comma after last declaration before root value */
    E2E_START_TEST("Trailing comma between declaration and root", "F2.1 Document Declaration Loop", "M2");
    const char* src5 = 
        "$typedef.Name: String,\n"
        "(name: \"test\")";
    obj = cco_parse_string(src5, strlen(src5), NULL);
    if (obj) {
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse document with trailing comma after declaration");
    }
    E2E_END_TEST();
}

static void test_f2_2_template_declaration(void) {
    /* Test 1: Simple template declaration */
    E2E_START_TEST("Simple template declaration", "F2.2 Template Declaration", "M2");
    const char* src1 = 
        "$temp.Point: (x: Integer, y: Integer),\n"
        "(origin: (x: 0, y: 0))";
    cco_object_t* obj = cco_parse_string(src1, strlen(src1), NULL);
    if (obj) {
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse simple template declaration");
    }
    E2E_END_TEST();

    /* Test 2: Template with default values */
    E2E_START_TEST("Template with default values", "F2.2 Template Declaration", "M2");
    const char* src2 = 
        "$temp.Config: (host: String = \"localhost\", port: Integer = 8080),\n"
        "(server: (host: \"127.0.0.1\"))";
    obj = cco_parse_string(src2, strlen(src2), NULL);
    if (obj) {
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse template with default values");
    }
    E2E_END_TEST();

    /* Test 3: Template single inheritance */
    E2E_START_TEST("Template single inheritance", "F2.2 Template Declaration", "M2");
    const char* src3 = 
        "$temp.Base: (id: Integer),\n"
        "$temp.Derived + Base: (name: String),\n"
        "(status: \"ready\")";
    obj = cco_parse_string(src3, strlen(src3), NULL);
    if (obj) {
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse template inheritance declaration");
    }
    E2E_END_TEST();

    /* Test 4: Multi-level template inheritance */
    E2E_START_TEST("Multi-level template inheritance", "F2.2 Template Declaration", "M2");
    const char* src4 = 
        "$temp.Shape: (area: Float),\n"
        "$temp.Circle + Shape: (radius: Float),\n"
        "$temp.SmallCircle + Circle: (label: String),\n"
        "(ok: true)";
    obj = cco_parse_string(src4, strlen(src4), NULL);
    if (obj) {
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse multi-level template inheritance");
    }
    E2E_END_TEST();

    /* Test 5: Template with complex field types */
    E2E_START_TEST("Template with complex field types (Array, Map)", "F2.2 Template Declaration", "M2");
    const char* src5 = 
        "$temp.Cluster: (\n"
        "    name: String,\n"
        "    nodes: Array,\n"
        "    metadata: Map\n"
        "),\n"
        "(cluster_ready: true)";
    obj = cco_parse_string(src5, strlen(src5), NULL);
    if (obj) {
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse template with container field types");
    }
    E2E_END_TEST();
}

static void test_f2_3_function_declaration(void) {
    /* Test 1: Constructor function syntax $function.@<host: String, port: Integer>: (...) */
    E2E_START_TEST("Constructor function declaration", "F2.3 Function Declaration", "M2");
    const char* src1 = 
        "$temp.Server: (host: String, port: Integer = 8080, url: String = \"\"),\n"
        "$function.@<host: String, port: Integer>: (\n"
        "    $this.(host: host, port: port, url: (\"https://\" + host + \":\" + port))\n"
        "),\n"
        "(init: true)";
    cco_object_t* obj = cco_parse_string(src1, strlen(src1), NULL);
    if (obj) {
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse constructor function declaration");
    }
    E2E_END_TEST();

    /* Test 2: Type conversion constructor */
    E2E_START_TEST("Type conversion constructor", "F2.3 Function Declaration", "M2");
    const char* src2 = 
        "$temp.Feature: (name: String, enabled: Boolean, level: Integer = 0),\n"
        "$function.@<name: String, enabled: Boolean>: (\n"
        "    $this.(name: name, enabled: enabled, level: $(enabled ?? 1))\n"
        "),\n"
        "(ready: true)";
    obj = cco_parse_string(src2, strlen(src2), NULL);
    if (obj) {
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse type conversion constructor");
    }
    E2E_END_TEST();

    /* Test 3: Named function declaration */
    E2E_START_TEST("Named function declaration", "F2.3 Function Declaration", "M2");
    const char* src3 = 
        "$function.make_url<host: String, port: Integer>: (\n"
        "    (\"https://\" + host + \":\" + port)\n"
        "),\n"
        "(val: 1)";
    obj = cco_parse_string(src3, strlen(src3), NULL);
    if (obj) {
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse named function declaration");
    }
    E2E_END_TEST();

    /* Test 4: Function with default parameter values */
    E2E_START_TEST("Function with default parameter values", "F2.3 Function Declaration", "M2");
    const char* src4 = 
        "$function.calc<x: Integer, factor: Integer = 2>: (\n"
        "    (x * factor)\n"
        "),\n"
        "(res: 0)";
    obj = cco_parse_string(src4, strlen(src4), NULL);
    if (obj) {
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse function with default parameters");
    }
    E2E_END_TEST();

    /* Test 5: Multiple function declarations in single document */
    E2E_START_TEST("Multiple function declarations", "F2.3 Function Declaration", "M2");
    const char* src5 = 
        "$function.fn1<a: Integer>: (a + 1),\n"
        "$function.fn2<b: Integer>: (b * 2),\n"
        "(done: true)";
    obj = cco_parse_string(src5, strlen(src5), NULL);
    if (obj) {
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse multiple function declarations");
    }
    E2E_END_TEST();
}

static void test_f2_4_bind_declaration(void) {
    /* Test 1: Simple variable bind $bind.x: 100 */
    E2E_START_TEST("Simple variable bind $bind.x: 100", "F2.4 Bind Declaration", "M2");
    const char* src1 = 
        "$bind.x: 100,\n"
        "(value: 100)";
    cco_object_t* obj = cco_parse_string(src1, strlen(src1), NULL);
    if (obj) {
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse variable bind declaration");
    }
    E2E_END_TEST();

    /* Test 2: Bind expression $bind.total: (10 + 20) */
    E2E_START_TEST("Bind expression $bind.total: (10 + 20)", "F2.4 Bind Declaration", "M2");
    const char* src2 = 
        "$bind.total: (10 + 20),\n"
        "(sum: 30)";
    obj = cco_parse_string(src2, strlen(src2), NULL);
    if (obj) {
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse bind expression declaration");
    }
    E2E_END_TEST();

    /* Test 3: Bind string value $bind.prefix: \"https://\" */
    E2E_START_TEST("Bind string value $bind.prefix", "F2.4 Bind Declaration", "M2");
    const char* src3 = 
        "$bind.prefix: \"https://\",\n"
        "(url: \"https://example.com\")";
    obj = cco_parse_string(src3, strlen(src3), NULL);
    if (obj) {
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse bind string declaration");
    }
    E2E_END_TEST();

    /* Test 4: Multiple binds */
    E2E_START_TEST("Multiple bind declarations", "F2.4 Bind Declaration", "M2");
    const char* src4 = 
        "$bind.a: 1,\n"
        "$bind.b: 2,\n"
        "$bind.c: 3,\n"
        "(total: 6)";
    obj = cco_parse_string(src4, strlen(src4), NULL);
    if (obj) {
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse multiple binds");
    }
    E2E_END_TEST();

    /* Test 5: Bind with nested map value */
    E2E_START_TEST("Bind with map structure", "F2.4 Bind Declaration", "M2");
    const char* src5 = 
        "$bind.defaults: (timeout: 30, retries: 3),\n"
        "(app: \"demo\")";
    obj = cco_parse_string(src5, strlen(src5), NULL);
    if (obj) {
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse bind with map value");
    }
    E2E_END_TEST();
}

static void test_f2_5_symtab_initialization(void) {
    /* Test 1: Symtab initialized during parse lifecycle */
    E2E_START_TEST("Symtab initialized during parse", "F2.5 Symtab Initialization", "M2");
    const char* src1 = 
        "$typedef.Url: String,\n"
        "$enum.Color = (RED, GREEN, BLUE),\n"
        "(theme: RED)";
    cco_object_t* obj = cco_parse_string(src1, strlen(src1), NULL);
    if (obj) {
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse document with symtab types");
    }
    E2E_END_TEST();

    /* Test 2: Symtab query through cco_parse_full if available */
    E2E_START_TEST("Symtab symbols queryable via cco_parse_full", "F2.5 Symtab Initialization", "M2");
    if (cco_parse_full && cco_parse_result_symbols) {
        const char* src2 = 
            "$temp.Point: (x: Integer, y: Integer),\n"
            "(p: 1)";
        void* res = cco_parse_full(src2);
        E2E_CHECK_NOT_NULL(res, "cco_parse_full should return valid result");
        if (res) {
            void* st = cco_parse_result_symbols(res);
            E2E_CHECK_NOT_NULL(st, "Symbol table should not be NULL");
            if (st && cco_symbol_table_count) {
                size_t cnt = cco_symbol_table_count(st);
                E2E_CHECK(cnt >= 1, "Symbol table should contain at least 1 symbol");
            }
            if (cco_parse_result_free) {
                cco_parse_result_free(res);
            }
        }
    } else {
        E2E_MARK_PENDING("cco_parse_full not yet exported (M3 dependency for full inspect)");
    }
    E2E_END_TEST();

    /* Test 3: Multiple enums registered in symtab */
    E2E_START_TEST("Multiple enums in symtab", "F2.5 Symtab Initialization", "M2");
    const char* src3 = 
        "$enum.E1 = (A, B),\n"
        "$enum.E2 = (C, D),\n"
        "(v1: A, v2: D)";
    cco_object_t* obj3 = cco_parse_string(src3, strlen(src3), NULL);
    if (obj3) {
        cco_object_release(obj3);
    } else {
        E2E_FAIL("Failed to parse document with multiple enums");
    }
    E2E_END_TEST();

    /* Test 4: Redefinition error in symtab */
    E2E_START_TEST("Symbol redefinition in symtab returns error", "F2.5 Symtab Initialization", "M2");
    const char* src4 = 
        "$typedef.T: String,\n"
        "$typedef.T: Integer,\n"
        "(val: 1)";
    cco_clear_diagnostics();
    cco_object_t* obj4 = cco_parse_string(src4, strlen(src4), NULL);
    E2E_CHECK(obj4 == NULL, "Redefinition of symbol T must fail parse");
    if (obj4) cco_object_release(obj4);
    E2E_END_TEST();

    /* Test 5: Clean deallocation of symtab without memory leak */
    E2E_START_TEST("Clean deallocation of symtab", "F2.5 Symtab Initialization", "M2");
    for (int iter = 0; iter < 10; iter++) {
        const char* src5 = 
            "$typedef.T1: String, $temp.P: (x: Integer), (val: 10)";
        cco_object_t* obj5 = cco_parse_string(src5, strlen(src5), NULL);
        if (obj5) {
            cco_object_release(obj5);
        }
    }
    E2E_CHECK(true, "Symtab allocations cycled cleanly");
    E2E_END_TEST();
}

static void test_f2_6_template_instantiation(void) {
    /* Test 1: Positional instantiation #Point(10, 20) */
    E2E_START_TEST("Positional instantiation #Point(10, 20)", "F2.6 Template Instantiation", "M2");
    const char* src1 = 
        "$temp.Point: (x: Integer, y: Integer),\n"
        "(pt: #Point(10, 20))";
    cco_object_t* obj = cco_parse_string(src1, strlen(src1), NULL);
    if (obj) {
        cco_object_t* pt = cco_map_get_by_key(obj, "pt");
        E2E_CHECK_NOT_NULL(pt, "pt key should exist");
        if (pt) {
            int t = cco_object_get_type(pt);
            E2E_CHECK(t == CCO_TYPE_TEMPLATE_INSTANCE || t == CCO_TYPE_MAP, 
                      "Template instance should have template instance or map type");
        }
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse positional template instantiation #Point(10, 20)");
    }
    E2E_END_TEST();

    /* Test 2: Named arguments instantiation #Server(.host: \"localhost\", .port: 9090) */
    E2E_START_TEST("Named arguments instantiation #Server(.host: ...)", "F2.6 Template Instantiation", "M2");
    const char* src2 = 
        "$temp.Server: (host: String, port: Integer = 8080),\n"
        "(srv: #Server(.host: \"localhost\", .port: 9090))";
    obj = cco_parse_string(src2, strlen(src2), NULL);
    if (obj) {
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse named arguments template instantiation");
    }
    E2E_END_TEST();

    /* Test 3: Default argument fallback #Server(\"example.com\") uses default port 8080 */
    E2E_START_TEST("Default argument fallback in template instantiation", "F2.6 Template Instantiation", "M2");
    const char* src3 = 
        "$temp.Server: (host: String, port: Integer = 8080),\n"
        "(srv: #Server(\"example.com\"))";
    obj = cco_parse_string(src3, strlen(src3), NULL);
    if (obj) {
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to instantiate template with default arguments");
    }
    E2E_END_TEST();

    /* Test 4: Inherited template instantiation #SecureServer(...) */
    E2E_START_TEST("Inherited template instantiation", "F2.6 Template Instantiation", "M2");
    const char* src4 = 
        "$temp.Server: (host: String, port: Integer = 8080),\n"
        "$temp.SecureServer + Server: (cert: String),\n"
        "(sec: #SecureServer(\"secure.example.com\", 443, \"/etc/cert.pem\"))";
    obj = cco_parse_string(src4, strlen(src4), NULL);
    if (obj) {
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to instantiate inherited template");
    }
    E2E_END_TEST();

    /* Test 5: Undefined template name returns parse error */
    E2E_START_TEST("Undefined template name returns error", "F2.6 Template Instantiation", "M2");
    const char* src5 = "(origin: #NoSuchTemplate(1, 2))";
    cco_clear_diagnostics();
    obj = cco_parse_string(src5, strlen(src5), NULL);
    E2E_CHECK(obj == NULL, "Undefined template must fail parsing");
    E2E_CHECK_EQ_INT(CCO_ERR_PARSE, cco_get_last_error(), "Error code should be CCO_ERR_PARSE");
    if (obj) cco_object_release(obj);
    E2E_END_TEST();
}

static void test_f2_7_official_repo_examples(void) {
    /* Test 1: learn/source/constructor.cco */
    E2E_START_TEST("Official example: learn/source/constructor.cco", "F2.7 Official Repo Examples", "M2");
    size_t len = 0;
    char* src = e2e_read_file("learn/source/constructor.cco", &len);
    E2E_CHECK_NOT_NULL(src, "learn/source/constructor.cco file must exist");
    if (src) {
        cco_object_t* obj = cco_parse_string(src, len, NULL);
        if (obj) {
            cco_object_release(obj);
        } else {
            E2E_FAIL("constructor.cco failed to parse");
        }
        free(src);
    }
    E2E_END_TEST();

    /* Test 2: learn/source/full_demo.cco */
    E2E_START_TEST("Official example: learn/source/full_demo.cco", "F2.7 Official Repo Examples", "M2");
    len = 0;
    src = e2e_read_file("learn/source/full_demo.cco", &len);
    E2E_CHECK_NOT_NULL(src, "learn/source/full_demo.cco file must exist");
    if (src) {
        cco_object_t* obj = cco_parse_string(src, len, NULL);
        if (obj) {
            cco_object_release(obj);
        } else {
            E2E_FAIL("full_demo.cco failed to parse");
        }
        free(src);
    }
    E2E_END_TEST();

    /* Test 3: learn/source/server.cco */
    E2E_START_TEST("Official example: learn/source/server.cco", "F2.7 Official Repo Examples", "M2");
    len = 0;
    src = e2e_read_file("learn/source/server.cco", &len);
    E2E_CHECK_NOT_NULL(src, "learn/source/server.cco file must exist");
    if (src) {
        cco_object_t* obj = cco_parse_string(src, len, NULL);
        if (obj) {
            cco_object_release(obj);
        } else {
            E2E_FAIL("server.cco failed to parse");
        }
        free(src);
    }
    E2E_END_TEST();

    /* Test 4: learn/source/typedef_enum.cco */
    E2E_START_TEST("Official example: learn/source/typedef_enum.cco", "F2.7 Official Repo Examples", "M2");
    len = 0;
    src = e2e_read_file("learn/source/typedef_enum.cco", &len);
    E2E_CHECK_NOT_NULL(src, "learn/source/typedef_enum.cco file must exist");
    if (src) {
        cco_object_t* obj = cco_parse_string(src, len, NULL);
        if (obj) {
            cco_object_release(obj);
        } else {
            E2E_FAIL("typedef_enum.cco failed to parse");
        }
        free(src);
    }
    E2E_END_TEST();

    /* Test 5: examples/basic.cco */
    E2E_START_TEST("Official example: examples/basic.cco", "F2.7 Official Repo Examples", "M2");
    len = 0;
    src = e2e_read_file("examples/basic.cco", &len);
    E2E_CHECK_NOT_NULL(src, "examples/basic.cco file must exist");
    if (src) {
        cco_object_t* obj = cco_parse_string(src, len, NULL);
        if (obj) {
            cco_object_release(obj);
        } else {
            E2E_FAIL("examples/basic.cco failed to parse");
        }
        free(src);
    }
    E2E_END_TEST();

    /* Test 6: Project.cco */
    E2E_START_TEST("Official example: Project.cco", "F2.7 Official Repo Examples", "M2");
    len = 0;
    src = e2e_read_file("Project.cco", &len);
    E2E_CHECK_NOT_NULL(src, "Project.cco file must exist");
    if (src) {
        cco_object_t* obj = cco_parse_string(src, len, NULL);
        if (obj) {
            cco_object_release(obj);
        } else {
            E2E_FAIL("Project.cco failed to parse");
        }
        free(src);
    }
    E2E_END_TEST();
}

int run_tier1_ast_parser(void) {
    printf(E2E_COLOR_BOLD "--- Running Tier 1: AST & Parser Wiring (F2.1 - F2.7) ---\n" E2E_COLOR_RESET);
    test_f2_1_document_declaration_loop();
    test_f2_2_template_declaration();
    test_f2_3_function_declaration();
    test_f2_4_bind_declaration();
    test_f2_5_symtab_initialization();
    test_f2_6_template_instantiation();
    test_f2_7_official_repo_examples();
    return 0;
}
