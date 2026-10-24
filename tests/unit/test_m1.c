#include "../test.h"
#include <cnt/cco.h>
#include <cnt/cco_options.h>
#include <cnt/cco_error.h>
#include "parser.h"
#include "lexer.h"
#include "object.h"
#include "arena.h"
#include <math.h>

static void test_m1_scientific_notation(void)
{
    /* F1.1: 1e10 */
    cco_object_t* obj = cco_parse_string("val: 1e10", 9, NULL);
    EXPECT_TRUE(obj != NULL);
    if (obj) {
        cco_object_t* val = cco_map_get_by_key(obj, "val");
        EXPECT_TRUE(val != NULL);
        if (val) {
            EXPECT_EQ_INT(CCO_TYPE_FLOAT, cco_object_get_type(val));
            double f = 0.0;
            EXPECT_TRUE(cco_object_get_float(val, &f));
            EXPECT_TRUE(fabs(f - 10000000000.0) < 1e-4);
        }
        cco_object_release(obj);
    }

    /* F1.1: 1.5e3 */
    obj = cco_parse_string("val: 1.5e3", 10, NULL);
    EXPECT_TRUE(obj != NULL);
    if (obj) {
        cco_object_t* val = cco_map_get_by_key(obj, "val");
        EXPECT_TRUE(val != NULL);
        if (val) {
            EXPECT_EQ_INT(CCO_TYPE_FLOAT, cco_object_get_type(val));
            double f = 0.0;
            EXPECT_TRUE(cco_object_get_float(val, &f));
            EXPECT_TRUE(fabs(f - 1500.0) < 1e-4);
        }
        cco_object_release(obj);
    }

    /* F1.1: 1.5e-3 */
    obj = cco_parse_string("val: 1.5e-3", 11, NULL);
    EXPECT_TRUE(obj != NULL);
    if (obj) {
        cco_object_t* val = cco_map_get_by_key(obj, "val");
        EXPECT_TRUE(val != NULL);
        if (val) {
            EXPECT_EQ_INT(CCO_TYPE_FLOAT, cco_object_get_type(val));
            double f = 0.0;
            EXPECT_TRUE(cco_object_get_float(val, &f));
            EXPECT_TRUE(fabs(f - 0.0015) < 1e-6);
        }
        cco_object_release(obj);
    }

    /* F1.1: 1e-5 */
    obj = cco_parse_string("val: 1e-5", 9, NULL);
    EXPECT_TRUE(obj != NULL);
    if (obj) {
        cco_object_t* val = cco_map_get_by_key(obj, "val");
        EXPECT_TRUE(val != NULL);
        if (val) {
            EXPECT_EQ_INT(CCO_TYPE_FLOAT, cco_object_get_type(val));
            double f = 0.0;
            EXPECT_TRUE(cco_object_get_float(val, &f));
            EXPECT_TRUE(fabs(f - 0.00001) < 1e-8);
        }
        cco_object_release(obj);
    }

    /* F1.1: 2.5E+2 */
    obj = cco_parse_string("val: 2.5E+2", 11, NULL);
    EXPECT_TRUE(obj != NULL);
    if (obj) {
        cco_object_t* val = cco_map_get_by_key(obj, "val");
        EXPECT_TRUE(val != NULL);
        if (val) {
            EXPECT_EQ_INT(CCO_TYPE_FLOAT, cco_object_get_type(val));
            double f = 0.0;
            EXPECT_TRUE(cco_object_get_float(val, &f));
            EXPECT_TRUE(fabs(f - 250.0) < 1e-4);
        }
        cco_object_release(obj);
    }

    /* F1.1: Expression $(1e3 + 5.0) */
    obj = cco_parse_string("val: $(1e3 + 5.0)", 17, NULL);
    EXPECT_TRUE(obj != NULL);
    if (obj) {
        cco_object_t* val = cco_map_get_by_key(obj, "val");
        EXPECT_TRUE(val != NULL);
        if (val) {
            EXPECT_EQ_INT(CCO_TYPE_FLOAT, cco_object_get_type(val));
            double f = 0.0;
            EXPECT_TRUE(cco_object_get_float(val, &f));
            EXPECT_TRUE(fabs(f - 1005.0) < 1e-4);
        }
        cco_object_release(obj);
    }
}

static void test_m1_base_literals_and_underscores(void)
{
    /* F1.2: Hex 0xFF */
    cco_object_t* obj = cco_parse_string("val: 0xFF", 9, NULL);
    EXPECT_TRUE(obj != NULL);
    if (obj) {
        cco_object_t* val = cco_map_get_by_key(obj, "val");
        EXPECT_TRUE(val != NULL);
        if (val) {
            EXPECT_EQ_INT(CCO_TYPE_INTEGER, cco_object_get_type(val));
            int64_t ival = 0;
            EXPECT_TRUE(cco_object_get_integer(val, &ival));
            EXPECT_EQ_INT(255, (int)ival);
        }
        cco_object_release(obj);
    }

    /* F1.2: Hex 0x1a */
    obj = cco_parse_string("val: 0x1a", 9, NULL);
    EXPECT_TRUE(obj != NULL);
    if (obj) {
        cco_object_t* val = cco_map_get_by_key(obj, "val");
        EXPECT_TRUE(val != NULL);
        if (val) {
            int64_t ival = 0;
            EXPECT_TRUE(cco_object_get_integer(val, &ival));
            EXPECT_EQ_INT(26, (int)ival);
        }
        cco_object_release(obj);
    }

    /* F1.2: Binary 0b1010 */
    obj = cco_parse_string("val: 0b1010", 11, NULL);
    EXPECT_TRUE(obj != NULL);
    if (obj) {
        cco_object_t* val = cco_map_get_by_key(obj, "val");
        EXPECT_TRUE(val != NULL);
        if (val) {
            EXPECT_EQ_INT(CCO_TYPE_INTEGER, cco_object_get_type(val));
            int64_t ival = 0;
            EXPECT_TRUE(cco_object_get_integer(val, &ival));
            EXPECT_EQ_INT(10, (int)ival);
        }
        cco_object_release(obj);
    }

    /* F1.2: Binary with underscores 0b1111_0000 */
    obj = cco_parse_string("val: 0b1111_0000", 16, NULL);
    EXPECT_TRUE(obj != NULL);
    if (obj) {
        cco_object_t* val = cco_map_get_by_key(obj, "val");
        EXPECT_TRUE(val != NULL);
        if (val) {
            int64_t ival = 0;
            EXPECT_TRUE(cco_object_get_integer(val, &ival));
            EXPECT_EQ_INT(240, (int)ival);
        }
        cco_object_release(obj);
    }

    /* F1.2: Octal 0o77 */
    obj = cco_parse_string("val: 0o77", 9, NULL);
    EXPECT_TRUE(obj != NULL);
    if (obj) {
        cco_object_t* val = cco_map_get_by_key(obj, "val");
        EXPECT_TRUE(val != NULL);
        if (val) {
            EXPECT_EQ_INT(CCO_TYPE_INTEGER, cco_object_get_type(val));
            int64_t ival = 0;
            EXPECT_TRUE(cco_object_get_integer(val, &ival));
            EXPECT_EQ_INT(63, (int)ival);
        }
        cco_object_release(obj);
    }

    /* F1.2: Underscores in integer 1_000_000 */
    obj = cco_parse_string("val: 1_000_000", 14, NULL);
    EXPECT_TRUE(obj != NULL);
    if (obj) {
        cco_object_t* val = cco_map_get_by_key(obj, "val");
        EXPECT_TRUE(val != NULL);
        if (val) {
            EXPECT_EQ_INT(CCO_TYPE_INTEGER, cco_object_get_type(val));
            int64_t ival = 0;
            EXPECT_TRUE(cco_object_get_integer(val, &ival));
            EXPECT_EQ_INT(1000000, (int)ival);
        }
        cco_object_release(obj);
    }

    /* F1.2: Underscores in float 1_000.5e2 */
    obj = cco_parse_string("val: 1_000.5e2", 14, NULL);
    EXPECT_TRUE(obj != NULL);
    if (obj) {
        cco_object_t* val = cco_map_get_by_key(obj, "val");
        EXPECT_TRUE(val != NULL);
        if (val) {
            EXPECT_EQ_INT(CCO_TYPE_FLOAT, cco_object_get_type(val));
            double f = 0.0;
            EXPECT_TRUE(cco_object_get_float(val, &f));
            EXPECT_TRUE(fabs(f - 100050.0) < 1e-4);
        }
        cco_object_release(obj);
    }

    /* F1.2: Expression with hex $(0xFF + 1) */
    obj = cco_parse_string("val: $(0xFF + 1)", 16, NULL);
    EXPECT_TRUE(obj != NULL);
    if (obj) {
        cco_object_t* val = cco_map_get_by_key(obj, "val");
        EXPECT_TRUE(val != NULL);
        if (val) {
            int64_t ival = 0;
            EXPECT_TRUE(cco_object_get_integer(val, &ival));
            EXPECT_EQ_INT(256, (int)ival);
        }
        cco_object_release(obj);
    }

    /* F1.2: Expression with binary and octal $(0b1010 + 0o10) */
    obj = cco_parse_string("val: $(0b1010 + 0o10)", 21, NULL);
    EXPECT_TRUE(obj != NULL);
    if (obj) {
        cco_object_t* val = cco_map_get_by_key(obj, "val");
        EXPECT_TRUE(val != NULL);
        if (val) {
            int64_t ival = 0;
            EXPECT_TRUE(cco_object_get_integer(val, &ival));
            EXPECT_EQ_INT(18, (int)ival);
        }
        cco_object_release(obj);
    }

    /* F1.2: Range operator preservation in lexer: 1..10 */
    {
        cco_arena_t arena;
        cco_arena_init(&arena, 64 * 1024);
        cco_lexer_t lexer;
        const char* rsrc = "1..10";
        cco_lexer_init(&lexer, rsrc, strlen(rsrc), NULL, &arena);
        cco_token_t t1, t2, t3;
        cco_lexer_next(&lexer, &t1);
        EXPECT_EQ_INT(CCO_TOK_INTEGER, t1.type);
        EXPECT_EQ_INT(1, (int)t1.text_len);
        cco_lexer_next(&lexer, &t2);
        EXPECT_EQ_INT(CCO_TOK_DOTDOT, t2.type);
        cco_lexer_next(&lexer, &t3);
        EXPECT_EQ_INT(CCO_TOK_INTEGER, t3.type);
        EXPECT_EQ_INT(2, (int)t3.text_len);
        cco_arena_destroy(&arena);
    }

    /* F1.2: Range operator preservation in lexer: 0..=5 */
    {
        cco_arena_t arena;
        cco_arena_init(&arena, 64 * 1024);
        cco_lexer_t lexer;
        const char* rsrc = "0..=5";
        cco_lexer_init(&lexer, rsrc, strlen(rsrc), NULL, &arena);
        cco_token_t t1, t2, t3;
        cco_lexer_next(&lexer, &t1);
        EXPECT_EQ_INT(CCO_TOK_INTEGER, t1.type);
        EXPECT_EQ_INT(1, (int)t1.text_len);
        cco_lexer_next(&lexer, &t2);
        EXPECT_EQ_INT(CCO_TOK_DOTDOTEQ, t2.type);
        cco_lexer_next(&lexer, &t3);
        EXPECT_EQ_INT(CCO_TOK_INTEGER, t3.type);
        EXPECT_EQ_INT(1, (int)t3.text_len);
        cco_arena_destroy(&arena);
    }
}

static void test_m1_signed_numbers(void)
{
    /* F1.3: Signed floats: -1.5, +1.5 */
    cco_object_t* obj = cco_parse_string("(a: -1.5, b: +1.5)", 18, NULL);
    EXPECT_TRUE(obj != NULL);
    if (obj) {
        cco_object_t* va = cco_map_get_by_key(obj, "a");
        cco_object_t* vb = cco_map_get_by_key(obj, "b");
        EXPECT_TRUE(va != NULL);
        EXPECT_TRUE(vb != NULL);
        if (va && vb) {
            double fa = 0.0, fb = 0.0;
            EXPECT_TRUE(cco_object_get_float(va, &fa));
            EXPECT_TRUE(cco_object_get_float(vb, &fb));
            EXPECT_TRUE(fabs(fa - (-1.5)) < 1e-6);
            EXPECT_TRUE(fabs(fb - 1.5) < 1e-6);
        }
        cco_object_release(obj);
    }

    /* F1.3: Signed integers: -10, +10 */
    obj = cco_parse_string("(a: -10, b: +10)", 16, NULL);
    EXPECT_TRUE(obj != NULL);
    if (obj) {
        cco_object_t* va = cco_map_get_by_key(obj, "a");
        cco_object_t* vb = cco_map_get_by_key(obj, "b");
        EXPECT_TRUE(va != NULL);
        EXPECT_TRUE(vb != NULL);
        if (va && vb) {
            int64_t ia = 0, ib = 0;
            EXPECT_TRUE(cco_object_get_integer(va, &ia));
            EXPECT_TRUE(cco_object_get_integer(vb, &ib));
            EXPECT_EQ_INT(-10, (int)ia);
            EXPECT_EQ_INT(10, (int)ib);
        }
        cco_object_release(obj);
    }

    /* F1.3: Signed hex integers: -0xFF, +0xFF */
    obj = cco_parse_string("(a: -0xFF, b: +0xFF)", 20, NULL);
    EXPECT_TRUE(obj != NULL);
    if (obj) {
        cco_object_t* va = cco_map_get_by_key(obj, "a");
        cco_object_t* vb = cco_map_get_by_key(obj, "b");
        EXPECT_TRUE(va != NULL);
        EXPECT_TRUE(vb != NULL);
        if (va && vb) {
            int64_t ia = 0, ib = 0;
            EXPECT_TRUE(cco_object_get_integer(va, &ia));
            EXPECT_TRUE(cco_object_get_integer(vb, &ib));
            EXPECT_EQ_INT(-255, (int)ia);
            EXPECT_EQ_INT(255, (int)ib);
        }
        cco_object_release(obj);
    }

    /* F1.3: Expressions with signed numbers */
    obj = cco_parse_string("val: $(-1.5 + 2.0)", 18, NULL);
    EXPECT_TRUE(obj != NULL);
    if (obj) {
        cco_object_t* val = cco_map_get_by_key(obj, "val");
        EXPECT_TRUE(val != NULL);
        if (val) {
            double f = 0.0;
            EXPECT_TRUE(cco_object_get_float(val, &f));
            EXPECT_TRUE(fabs(f - 0.5) < 1e-6);
        }
        cco_object_release(obj);
    }

    obj = cco_parse_string("val: $(+1.5 * 2.0)", 18, NULL);
    EXPECT_TRUE(obj != NULL);
    if (obj) {
        cco_object_t* val = cco_map_get_by_key(obj, "val");
        EXPECT_TRUE(val != NULL);
        if (val) {
            double f = 0.0;
            EXPECT_TRUE(cco_object_get_float(val, &f));
            EXPECT_TRUE(fabs(f - 3.0) < 1e-6);
        }
        cco_object_release(obj);
    }
}

static void test_m1_hang_fix(void)
{
    /* F1.4: Lone dollar string */
    cco_object_t* obj = cco_parse_string("str: \"$\"", 8, NULL);
    EXPECT_TRUE(obj != NULL);
    if (obj) {
        cco_object_t* val = cco_map_get_by_key(obj, "str");
        EXPECT_TRUE(val != NULL);
        if (val) {
            const char* s = NULL;
            size_t slen = 0;
            EXPECT_TRUE(cco_object_get_string(val, &s, &slen));
            EXPECT_TRUE(s != NULL && strcmp(s, "$") == 0);
        }
        cco_object_release(obj);
    }

    /* F1.4: Dollar in string word "a$b" */
    obj = cco_parse_string("str: \"a$b\"", 10, NULL);
    EXPECT_TRUE(obj != NULL);
    if (obj) {
        cco_object_t* val = cco_map_get_by_key(obj, "str");
        EXPECT_TRUE(val != NULL);
        if (val) {
            const char* s = NULL;
            size_t slen = 0;
            EXPECT_TRUE(cco_object_get_string(val, &s, &slen));
            EXPECT_TRUE(s != NULL && strcmp(s, "a$b") == 0);
        }
        cco_object_release(obj);
    }

    /* F1.4: Dollar as currency "price is $5" */
    obj = cco_parse_string("str: \"price is $5\"", 18, NULL);
    EXPECT_TRUE(obj != NULL);
    if (obj) {
        cco_object_t* val = cco_map_get_by_key(obj, "str");
        EXPECT_TRUE(val != NULL);
        if (val) {
            const char* s = NULL;
            size_t slen = 0;
            EXPECT_TRUE(cco_object_get_string(val, &s, &slen));
            EXPECT_TRUE(s != NULL && strcmp(s, "price is $5") == 0);
        }
        cco_object_release(obj);
    }

    /* F1.4: Double dollar "$$" */
    obj = cco_parse_string("str: \"$$\"", 9, NULL);
    EXPECT_TRUE(obj != NULL);
    if (obj) {
        cco_object_t* val = cco_map_get_by_key(obj, "str");
        EXPECT_TRUE(val != NULL);
        if (val) {
            const char* s = NULL;
            size_t slen = 0;
            EXPECT_TRUE(cco_object_get_string(val, &s, &slen));
            EXPECT_TRUE(s != NULL && strcmp(s, "$$") == 0);
        }
        cco_object_release(obj);
    }

    /* F1.4: Truncated interpolation does not hang, reports error */
    obj = cco_parse_string("str: \"$(1 + 2\"", 14, NULL);
    EXPECT_TRUE(obj == NULL);
    EXPECT_EQ_INT(CCO_ERR_PARSE, cco_get_last_error());

    /* F1.4: Lone dollar at top level */
    obj = cco_parse_string("$", 1, NULL);
    EXPECT_TRUE(obj == NULL);
    EXPECT_EQ_INT(CCO_ERR_PARSE, cco_get_last_error());
}

static void test_m1_comments(void)
{
    /* F1.5: Single-line comment before map entry */
    cco_object_t* obj = cco_parse_string("# initial comment\na: 1\n", 23, NULL);
    EXPECT_TRUE(obj != NULL);
    if (obj) {
        cco_object_t* val = cco_map_get_by_key(obj, "a");
        EXPECT_TRUE(val != NULL);
        int64_t ival = 0;
        EXPECT_TRUE(cco_object_get_integer(val, &ival));
        EXPECT_EQ_INT(1, (int)ival);
        cco_object_release(obj);
    }

    /* F1.5: Inline single-line comment */
    obj = cco_parse_string("a: 1, # inline comment\nb: 2", 27, NULL);
    EXPECT_TRUE(obj != NULL);
    if (obj) {
        cco_object_t* va = cco_map_get_by_key(obj, "a");
        cco_object_t* vb = cco_map_get_by_key(obj, "b");
        EXPECT_TRUE(va != NULL);
        EXPECT_TRUE(vb != NULL);
        cco_object_release(obj);
    }

    /* F1.5: Single-line comment without space after # */
    obj = cco_parse_string("#TODO: note\na: 42\n", 18, NULL);
    EXPECT_TRUE(obj != NULL);
    if (obj) {
        cco_object_t* val = cco_map_get_by_key(obj, "a");
        EXPECT_TRUE(val != NULL);
        cco_object_release(obj);
    }

    /* F1.5: Disambiguation of #Template(...) vs comment */
    {
        cco_arena_t arena;
        cco_arena_init(&arena, 64 * 1024);
        cco_lexer_t lexer;
        const char* tsrc = "origin: #Point(0.0, 0.0)";
        cco_lexer_init(&lexer, tsrc, strlen(tsrc), NULL, &arena);
        cco_token_t tok;
        cco_lexer_next(&lexer, &tok); /* origin */
        EXPECT_EQ_INT(CCO_TOK_IDENTIFIER, tok.type);
        cco_lexer_next(&lexer, &tok); /* : */
        EXPECT_EQ_INT(CCO_TOK_COLON, tok.type);
        cco_lexer_next(&lexer, &tok); /* # */
        EXPECT_EQ_INT(CCO_TOK_HASH, tok.type);
        cco_lexer_next(&lexer, &tok); /* Point */
        EXPECT_EQ_INT(CCO_TOK_IDENTIFIER, tok.type);
        cco_arena_destroy(&arena);
    }

    /* F1.5: Single-line comment within dotted map key */
    obj = cco_parse_string("(server # inline\n.port: 8080)", 30, NULL);
    EXPECT_TRUE(obj != NULL);
    if (obj) {
        cco_object_t* val = cco_map_get_by_key(obj, "server.port");
        EXPECT_TRUE(val != NULL);
        if (val) {
            int64_t ival = 0;
            EXPECT_TRUE(cco_object_get_integer(val, &ival));
            EXPECT_EQ_INT(8080, (int)ival);
        }
        cco_object_release(obj);
    }

    /* F1.6: Block comment */
    obj = cco_parse_string("/* block comment */ a: 1", 24, NULL);
    EXPECT_TRUE(obj != NULL);
    if (obj) {
        cco_object_t* val = cco_map_get_by_key(obj, "a");
        EXPECT_TRUE(val != NULL);
        cco_object_release(obj);
    }

    /* F1.6: Unterminated block comment returns error */
    obj = cco_parse_string("/* unterminated comment\na: 1", 28, NULL);
    EXPECT_TRUE(obj == NULL);
    EXPECT_EQ_INT(CCO_ERR_PARSE, cco_get_last_error());
}

static void test_m1_duplicate_keys(void)
{
    /* F1.7: Simple duplicate key in parenthesized map */
    cco_object_t* obj = cco_parse_string("(a: 1, a: 2)", 12, NULL);
    EXPECT_TRUE(obj == NULL);
    EXPECT_EQ_INT(CCO_ERR_PARSE, cco_get_last_error());

    /* F1.7: Duplicate string key */
    obj = cco_parse_string("(\"key\": 1, \"key\": 2)", 20, NULL);
    EXPECT_TRUE(obj == NULL);
    EXPECT_EQ_INT(CCO_ERR_PARSE, cco_get_last_error());

    /* F1.7: Duplicate dotted key */
    obj = cco_parse_string("(a.b: 1, a.b: 2)", 16, NULL);
    EXPECT_TRUE(obj == NULL);
    EXPECT_EQ_INT(CCO_ERR_PARSE, cco_get_last_error());

    /* F1.7: Different dotted keys are accepted */
    obj = cco_parse_string("(a.b: 1, a.c: 2)", 16, NULL);
    EXPECT_TRUE(obj != NULL);
    if (obj) {
        EXPECT_EQ_INT(2, (int)obj->as.map.count);
        cco_object_release(obj);
    }

    /* F1.7: Duplicate key in top-level unbracketed map */
    obj = cco_parse_string("a: 1, a: 2", 10, NULL);
    EXPECT_TRUE(obj == NULL);
    EXPECT_EQ_INT(CCO_ERR_PARSE, cco_get_last_error());

    /* F1.7: Duplicate key in nested map */
    obj = cco_parse_string("(sub: (x: 10, x: 20))", 21, NULL);
    EXPECT_TRUE(obj == NULL);
    EXPECT_EQ_INT(CCO_ERR_PARSE, cco_get_last_error());

    /* F1.7: Positive control - unique keys */
    obj = cco_parse_string("(a: 1, b: 2, c: 3)", 18, NULL);
    EXPECT_TRUE(obj != NULL);
    if (obj) {
        EXPECT_EQ_INT(3, (int)obj->as.map.count);
        cco_object_release(obj);
    }

    /* F1.7: Direct API cco_map_has_key */
    cco_object_t* map = cco_object_create(CCO_TYPE_MAP);
    EXPECT_TRUE(map != NULL);
    if (map) {
        EXPECT_TRUE(!cco_map_has_key(map, "foo"));
        cco_object_t* v1 = cco_object_create(CCO_TYPE_INTEGER);
        v1->as.integer = 100;
        EXPECT_EQ_INT(CCO_OK, cco_map_insert(map, "foo", v1));
        EXPECT_TRUE(cco_map_has_key(map, "foo"));
        EXPECT_TRUE(!cco_map_has_key(map, "bar"));

        /* F1.7: Direct API cco_map_insert duplicate rejection */
        cco_object_t* v2 = cco_object_create(CCO_TYPE_INTEGER);
        v2->as.integer = 200;
        EXPECT_EQ_INT(CCO_ERR_PARSE, cco_map_insert(map, "foo", v2));
        cco_object_release(v2);

        /* F1.7: Direct API cco_map_set updates existing key */
        cco_object_t* v3 = cco_object_create(CCO_TYPE_INTEGER);
        v3->as.integer = 300;
        EXPECT_EQ_INT(CCO_OK, cco_map_set(map, "foo", v3));
        cco_object_t* retrieved = cco_map_get_by_key(map, "foo");
        EXPECT_TRUE(retrieved != NULL);
        if (retrieved) {
            int64_t ival = 0;
            EXPECT_TRUE(cco_object_get_integer(retrieved, &ival));
            EXPECT_EQ_INT(300, (int)ival);
        }
        cco_object_release(v3);
        cco_object_release(v1);
        cco_object_release(map);
    }
}

void test_m1_all(void)
{
    test_m1_scientific_notation();
    test_m1_base_literals_and_underscores();
    test_m1_signed_numbers();
    test_m1_hang_fix();
    test_m1_comments();
    test_m1_duplicate_keys();
}
