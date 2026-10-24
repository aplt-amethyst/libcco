#include "../e2e_test.h"

/*
 * Tier 4: Real-World Application Scenarios
 * Verifies realistic workloads, production config patterns,
 * official repo examples, and lifecycle round-trips.
 */

static void test_tier4_official_examples(void) {
    /* Scenario 1: learn/source/constructor.cco */
    E2E_START_TEST("Scenario 1: constructor.cco custom constructors", "Tier 4: Real-World Scenarios", "M2");
    size_t len = 0;
    char* src = e2e_read_file("learn/source/constructor.cco", &len);
    E2E_CHECK_NOT_NULL(src, "learn/source/constructor.cco exists");
    if (src) {
        cco_object_t* obj = cco_parse_string(src, len, NULL);
        if (obj) {
            cco_object_t* origin = cco_map_get_by_key(obj, "origin");
            E2E_CHECK_NOT_NULL(origin, "origin instance should exist");
            cco_object_release(obj);
        } else {
            E2E_FAIL("constructor.cco failed to parse");
        }
        free(src);
    }
    E2E_END_TEST();

    /* Scenario 2: learn/source/full_demo.cco */
    E2E_START_TEST("Scenario 2: full_demo.cco complete feature demo", "Tier 4: Real-World Scenarios", "M2");
    len = 0;
    src = e2e_read_file("learn/source/full_demo.cco", &len);
    E2E_CHECK_NOT_NULL(src, "learn/source/full_demo.cco exists");
    if (src) {
        cco_object_t* obj = cco_parse_string(src, len, NULL);
        if (obj) {
            cco_object_t* app = cco_map_get_by_key(obj, "app");
            E2E_CHECK_NOT_NULL(app, "app instance should exist");
            cco_object_release(obj);
        } else {
            E2E_FAIL("full_demo.cco failed to parse");
        }
        free(src);
    }
    E2E_END_TEST();

    /* Scenario 3: learn/source/server.cco */
    E2E_START_TEST("Scenario 3: server.cco template inheritance workflow", "Tier 4: Real-World Scenarios", "M2");
    len = 0;
    src = e2e_read_file("learn/source/server.cco", &len);
    E2E_CHECK_NOT_NULL(src, "learn/source/server.cco exists");
    if (src) {
        cco_object_t* obj = cco_parse_string(src, len, NULL);
        if (obj) {
            cco_object_t* s1 = cco_map_get_by_key(obj, "s1");
            E2E_CHECK_NOT_NULL(s1, "s1 instance should exist");
            cco_object_release(obj);
        } else {
            E2E_FAIL("server.cco failed to parse");
        }
        free(src);
    }
    E2E_END_TEST();

    /* Scenario 4: learn/source/typedef_enum.cco */
    E2E_START_TEST("Scenario 4: typedef_enum.cco types and enumerations", "Tier 4: Real-World Scenarios", "M2");
    len = 0;
    src = e2e_read_file("learn/source/typedef_enum.cco", &len);
    E2E_CHECK_NOT_NULL(src, "learn/source/typedef_enum.cco exists");
    if (src) {
        cco_object_t* obj = cco_parse_string(src, len, NULL);
        if (obj) {
            cco_object_t* app = cco_map_get_by_key(obj, "app");
            E2E_CHECK_NOT_NULL(app, "app map should exist");
            if (app) {
                cco_object_t* port = cco_map_get_by_key(app, "port");
                E2E_CHECK_NOT_NULL(port, "port key should exist in app");
            }
            cco_object_release(obj);
        } else {
            E2E_FAIL("typedef_enum.cco failed to parse");
        }
        free(src);
    }
    E2E_END_TEST();

    /* Scenario 5: examples/basic.cco */
    E2E_START_TEST("Scenario 5: examples/basic.cco basic configuration", "Tier 4: Real-World Scenarios", "M2");
    len = 0;
    src = e2e_read_file("examples/basic.cco", &len);
    E2E_CHECK_NOT_NULL(src, "examples/basic.cco exists");
    if (src) {
        cco_object_t* obj = cco_parse_string(src, len, NULL);
        if (obj) {
            cco_object_t* app = cco_map_get_by_key(obj, "app");
            E2E_CHECK_NOT_NULL(app, "app key in basic.cco should exist");
            cco_object_release(obj);
        } else {
            E2E_FAIL("examples/basic.cco failed to parse");
        }
        free(src);
    }
    E2E_END_TEST();

    /* Scenario 6: Project.cco */
    E2E_START_TEST("Scenario 6: Project.cco project manifest", "Tier 4: Real-World Scenarios", "M2");
    len = 0;
    src = e2e_read_file("Project.cco", &len);
    E2E_CHECK_NOT_NULL(src, "Project.cco exists");
    if (src) {
        cco_object_t* obj = cco_parse_string(src, len, NULL);
        if (obj) {
            cco_object_t* name = cco_map_get_by_key(obj, "name");
            E2E_CHECK_NOT_NULL(name, "name key in Project.cco should exist");
            cco_object_release(obj);
        } else {
            E2E_FAIL("Project.cco failed to parse");
        }
        free(src);
    }
    E2E_END_TEST();
}

static void test_tier4_workload_scenarios(void) {
    /* Scenario 7: Microservice Config Scenario */
    E2E_START_TEST("Scenario 7: Production microservice configuration", "Tier 4: Real-World Scenarios", "M1");
    const char* microservice_cco = 
        "# Production microservice configuration\n"
        "(\n"
        "  service: (\n"
        "    name: \"auth-gateway\",\n"
        "    version: \"2.4.1\",\n"
        "    port: 8443,\n"
        "    debug: false,\n"
        "    rate_limit: 1.5e3, # 1500 requests per second\n"
        "    log_level: 0x03    # bitmask flags\n"
        "  ),\n"
        "  database: (\n"
        "    host: \"db-primary.internal\",\n"
        "    port: 5432,\n"
        "    pool_min: 5,\n"
        "    pool_max: 50,\n"
        "    timeout_ms: 2500,\n"
        "    ssl: true\n"
        "  ),\n"
        "  redis: (\n"
        "    hosts: (\"redis-01:6379\", \"redis-02:6379\"),\n"
        "    db: 0,\n"
        "    ttl_seconds: 3600\n"
        "  ),\n"
        "  features: (\n"
        "    jwt_v2: true,\n"
        "    mfa_required: true,\n"
        "    beta_access: false\n"
        "  )\n"
        ")\n";
    cco_object_t* obj = cco_parse_string(microservice_cco, strlen(microservice_cco), NULL);
    if (obj) {
        cco_object_t* svc = cco_map_get_by_key(obj, "service");
        E2E_CHECK_NOT_NULL(svc, "service map should exist");
        if (svc) {
            cco_object_t* port = cco_map_get_by_key(svc, "port");
            int64_t pval = 0;
            cco_object_get_integer(port, &pval);
            E2E_CHECK_EQ_INT(8443, pval, "port should equal 8443");
        }
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse microservice configuration");
    }
    E2E_END_TEST();

    /* Scenario 8: CI/CD Pipeline Configuration */
    E2E_START_TEST("Scenario 8: CI/CD Pipeline configuration", "Tier 4: Real-World Scenarios", "M1");
    const char* cicd_cco = 
        "# CI/CD workflow pipeline definition\n"
        "(\n"
        "  pipeline: \"libcco-industrial-ci\",\n"
        "  stages: (\"lint\", \"build\", \"test\", \"package\"),\n"
        "  matrix: (\n"
        "    os: (\"ubuntu-22.04\", \"ubuntu-24.04\", \"debian-12\"),\n"
        "    compiler: (\"gcc-11\", \"gcc-13\", \"clang-16\", \"clang-18\")\n"
        "  ),\n"
        "  timeout_minutes: 30,\n"
        "  retry_count: 2,\n"
        "  artifacts: (\n"
        "    paths: (\"target/debug/libcco.so\", \"target/tests/run_e2e\"),\n"
        "    expire_in: \"7 days\"\n"
        "  )\n"
        ")\n";
    obj = cco_parse_string(cicd_cco, strlen(cicd_cco), NULL);
    if (obj) {
        cco_object_t* stages = cco_map_get_by_key(obj, "stages");
        E2E_CHECK_NOT_NULL(stages, "stages array should exist");
        if (stages) {
            size_t count = cco_array_get_count(stages);
            E2E_CHECK_EQ_INT(4, count, "4 stages expected");
        }
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse CI/CD configuration");
    }
    E2E_END_TEST();

    /* Scenario 9: High-Throughput Batch Workload with Arena Reuse */
    E2E_START_TEST("Scenario 9: High-throughput batch parse (500 iterations)", "Tier 4: Real-World Scenarios", "M1");
    cco_arena_t* arena = cco_arena_new(1024 * 1024);
    E2E_CHECK_NOT_NULL(arena, "Arena should be allocated");
    if (arena) {
        cco_parse_options_t opts;
        cco_parse_options_init(&opts);
        opts.custom_arena = arena;
        
        clock_t start = clock();
        const char* sample_data = "(id: 100, name: \"batch_item\", status: true, score: 98.6)";
        size_t slen = strlen(sample_data);
        int success_count = 0;
        
        for (int i = 0; i < 500; i++) {
            cco_arena_reset(arena);
            cco_object_t* batch_obj = cco_parse_string(sample_data, slen, &opts);
            if (batch_obj) {
                success_count++;
                cco_object_release(batch_obj);
            }
        }
        clock_t end = clock();
        double elapsed_ms = (double)(end - start) * 1000.0 / CLOCKS_PER_SEC;
        
        E2E_CHECK_EQ_INT(500, success_count, "All 500 batch parses should succeed");
        if (g_e2e_ctx.verbose) {
            printf("       -> 500 iterations in %.2f ms (%.2f us/op)\n", 
                   elapsed_ms, elapsed_ms * 1000.0 / 500.0);
        }
        cco_arena_free(arena);
    }
    E2E_END_TEST();

    /* Scenario 10: Full Round-Trip Mutation Scenario */
    E2E_START_TEST("Scenario 10: Full parse -> serialize -> reparse roundtrip", "Tier 4: Real-World Scenarios", "M1");
    const char* original_doc = 
        "(server: (host: \"127.0.0.1\", port: 9000, secure: true, max_conns: 1000))";
    obj = cco_parse_string(original_doc, strlen(original_doc), NULL);
    if (obj) {
        char* serialized = cco_serialize_to_string(obj, false);
        E2E_CHECK_NOT_NULL(serialized, "Serialization should succeed");
        if (serialized) {
            cco_object_t* reloaded = cco_parse_string(serialized, strlen(serialized), NULL);
            E2E_CHECK_NOT_NULL(reloaded, "Reparse serialized text should succeed");
            if (reloaded) {
                cco_object_t* srv = cco_map_get_by_key(reloaded, "server");
                E2E_CHECK_NOT_NULL(srv, "server map preserved in roundtrip");
                if (srv) {
                    cco_object_t* port = cco_map_get_by_key(srv, "port");
                    int64_t p = 0;
                    cco_object_get_integer(port, &p);
                    E2E_CHECK_EQ_INT(9000, p, "port preserved across roundtrip");
                }
                cco_object_release(reloaded);
            }
            free(serialized);
        }
        cco_object_release(obj);
    } else {
        E2E_FAIL("Failed to parse initial roundtrip doc");
    }
    E2E_END_TEST();
}

int run_tier4_scenarios(void) {
    printf(E2E_COLOR_BOLD "--- Running Tier 4: Real-World Application Scenarios ---\n" E2E_COLOR_RESET);
    test_tier4_official_examples();
    test_tier4_workload_scenarios();
    return 0;
}
