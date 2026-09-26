#include "test.h"
#include <stdio.h>

int tests_run = 0;
int tests_passed = 0;

/* Declare test suites */
extern void test_arena(void);
extern void test_strbuf(void);
extern void test_lexer(void);
extern void test_parser(void);
extern void test_scope_all(void);
extern void test_expr(void);

int main(void) {
    printf("Starting libcco unit tests...\n");

    printf("test_arena\n"); fflush(stdout); test_arena();
    printf("test_strbuf\n"); fflush(stdout); test_strbuf();
    printf("test_lexer\n"); fflush(stdout); test_lexer();
    printf("test_parser\n"); fflush(stdout); test_parser();
    printf("test_scope_all\n"); fflush(stdout); test_scope_all();
    printf("test_expr\n"); fflush(stdout); test_expr();

    printf("========================================\n");
    printf("Tests run:    %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_run - tests_passed);

    if (tests_run == tests_passed) {
        printf("SUCCESS!\n");
        return 0;
    } else {
        printf("FAILURE!\n");
        return 1;
    }
}
