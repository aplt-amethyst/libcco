#include "e2e_test.h"

e2e_context_t g_e2e_ctx = {
    .total = 0,
    .passed = 0,
    .failed = 0,
    .pending = 0,
    .skipped = 0,
    .current_failed = 0,
    .current_pending = false,
    .current_name = NULL,
    .current_feature = NULL,
    .current_milestone = NULL,
    .filter_feature = NULL,
    .filter_milestone = NULL,
    .verbose = false
};

static void print_banner(void) {
    printf("\n");
    printf(E2E_COLOR_CYAN E2E_COLOR_BOLD "====================================================================\n" E2E_COLOR_RESET);
    printf(E2E_COLOR_CYAN E2E_COLOR_BOLD "          libcco Industrial-Grade End-to-End Test Suite             \n" E2E_COLOR_RESET);
    printf(E2E_COLOR_CYAN E2E_COLOR_BOLD "====================================================================\n" E2E_COLOR_RESET);
    printf("Tiers: [T1] Feature Coverage | [T2] Boundaries | [T3] Interactions | [T4] Scenarios\n\n");
}

static void print_usage(const char* prog) {
    printf("Usage: %s [options]\n", prog);
    printf("Options:\n");
    printf("  --all            Run all test tiers (default)\n");
    printf("  --tier1          Run Tier 1 only (Feature Coverage F1-F4)\n");
    printf("  --tier2          Run Tier 2 only (Boundary & Corner Cases)\n");
    printf("  --tier3          Run Tier 3 only (Cross-Feature Interactions)\n");
    printf("  --tier4          Run Tier 4 only (Real-World Scenarios)\n");
    printf("  -v, --verbose    Show verbose test execution output\n");
    printf("  -h, --help       Show this help message\n");
}

int main(int argc, char** argv) {
    bool run_t1 = false;
    bool run_t2 = false;
    bool run_t3 = false;
    bool run_t4 = false;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--tier1") == 0) {
            run_t1 = true;
        } else if (strcmp(argv[i], "--tier2") == 0) {
            run_t2 = true;
        } else if (strcmp(argv[i], "--tier3") == 0) {
            run_t3 = true;
        } else if (strcmp(argv[i], "--tier4") == 0) {
            run_t4 = true;
        } else if (strcmp(argv[i], "--all") == 0) {
            run_t1 = run_t2 = run_t3 = run_t4 = true;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            g_e2e_ctx.verbose = true;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }

    /* Default is to run all tiers */
    if (!run_t1 && !run_t2 && !run_t3 && !run_t4) {
        run_t1 = run_t2 = run_t3 = run_t4 = true;
    }

    print_banner();
    clock_t start_time = clock();

    if (run_t1) {
        run_tier1_lexer_literals();
        run_tier1_ast_parser();
        run_tier1_api_surface();
        run_tier1_hygiene_diagnostics();
    }

    if (run_t2) {
        run_tier2_boundaries();
    }

    if (run_t3) {
        run_tier3_interactions();
    }

    if (run_t4) {
        run_tier4_scenarios();
    }

    clock_t end_time = clock();
    double elapsed_sec = (double)(end_time - start_time) / CLOCKS_PER_SEC;

    printf("\n" E2E_COLOR_CYAN E2E_COLOR_BOLD "====================================================================\n" E2E_COLOR_RESET);
    printf(E2E_COLOR_BOLD "                      E2E Test Execution Summary                    \n" E2E_COLOR_RESET);
    printf(E2E_COLOR_CYAN E2E_COLOR_BOLD "====================================================================\n" E2E_COLOR_RESET);
    printf("  Total Test Cases : %d\n", g_e2e_ctx.total);
    printf("  " E2E_COLOR_GREEN "Passed           : %d" E2E_COLOR_RESET "\n", g_e2e_ctx.passed);
    printf("  " E2E_COLOR_RED   "Failed           : %d" E2E_COLOR_RESET "\n", g_e2e_ctx.failed);
    printf("  " E2E_COLOR_YELLOW"Pending (Milestone): %d" E2E_COLOR_RESET "\n", g_e2e_ctx.pending);
    printf("  Elapsed Time     : %.3f s\n", elapsed_sec);
    printf(E2E_COLOR_CYAN E2E_COLOR_BOLD "====================================================================\n" E2E_COLOR_RESET);

    if (g_e2e_ctx.failed == 0) {
        printf("  " E2E_COLOR_GREEN E2E_COLOR_BOLD "RESULT: ALL EXECUTED E2E TESTS PASSED!\n" E2E_COLOR_RESET);
        return 0;
    } else {
        printf("  " E2E_COLOR_RED E2E_COLOR_BOLD "RESULT: %d TEST(S) FAILED (requires milestone fixes)\n" E2E_COLOR_RESET, g_e2e_ctx.failed);
        return 1;
    }
}
