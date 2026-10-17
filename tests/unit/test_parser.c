#include "../test.h"
#include <cnt/cco.h>
#include "parser.h"

void test_parser(void) {
    cco_object_t* obj;
    
    /* Test 1: none is an identifier which returns error */
    obj = cco_parse_string("none", 4, NULL);
    EXPECT_TRUE(obj == NULL);
    EXPECT_EQ_INT(CCO_ERR_PARSE, cco_get_last_error());
    
    /* Test 2: true */
    obj = cco_parse_string("true", 4, NULL);
    EXPECT_TRUE(obj != NULL);
    EXPECT_EQ_INT(CCO_TYPE_BOOLEAN, cco_object_get_type(obj));
    cco_object_release(obj);
    
    /* Test 3: empty string should probably give EOF error or something, 
       but currently document expects at least something or just NONE */
    obj = cco_parse_string("", 0, NULL);
    /* Doesn't segfault */
    cco_object_release(obj);

    /* Test 4: Map starting with quoted string key */
    obj = cco_parse_string("(\"str_key\": 123)", 16, NULL);
    EXPECT_TRUE(obj != NULL);
    EXPECT_EQ_INT(CCO_TYPE_MAP, cco_object_get_type(obj));
    if (obj) {
        cco_object_t* val = cco_map_get_by_key(obj, "str_key");
        EXPECT_TRUE(val != NULL);
        int64_t ival = 0;
        EXPECT_TRUE(cco_object_get_integer(val, &ival));
        EXPECT_EQ_INT((int)ival, 123);
        cco_object_release(obj);
    }

    /* Test 5: Map starting with dotted key */
    obj = cco_parse_string("(server.port: 8080)", 19, NULL);
    EXPECT_TRUE(obj != NULL);
    EXPECT_EQ_INT(CCO_TYPE_MAP, cco_object_get_type(obj));
    if (obj) {
        cco_object_t* val = cco_map_get_by_key(obj, "server.port");
        EXPECT_TRUE(val != NULL);
        int64_t ival = 0;
        EXPECT_TRUE(cco_object_get_integer(val, &ival));
        EXPECT_EQ_INT((int)ival, 8080);
        cco_object_release(obj);
    }

    /* Test 6: Round-trip serialization with numeric string key and keyword key */
    obj = cco_parse_string("(\"123\": 10, \"true\": 20, normal: 30)", 36, NULL);
    EXPECT_TRUE(obj != NULL);
    if (obj) {
        char* ser = cco_serialize_to_string(obj, false);
        EXPECT_TRUE(ser != NULL);
        if (ser) {
            cco_object_t* re = cco_parse_string(ser, strlen(ser), NULL);
            EXPECT_TRUE(re != NULL);
            if (re) {
                cco_object_t* v1 = cco_map_get_by_key(re, "123");
                EXPECT_TRUE(v1 != NULL);
                cco_object_t* v2 = cco_map_get_by_key(re, "true");
                EXPECT_TRUE(v2 != NULL);
                cco_object_t* v3 = cco_map_get_by_key(re, "normal");
                EXPECT_TRUE(v3 != NULL);
                cco_object_release(re);
            }
            free(ser);
        }
        cco_object_release(obj);
    }

    /* Test 7: Map key with escape sequence */
    const char* edoc = "(\"key\\n1\": \"val\\n2\")";
    obj = cco_parse_string(edoc, strlen(edoc), NULL);
    EXPECT_TRUE(obj != NULL);
    if (obj) {
        cco_object_t* v = cco_map_get_by_key(obj, "key\n1");
        EXPECT_TRUE(v != NULL);
        if (v) {
            const char* s = NULL;
            size_t slen = 0;
            EXPECT_TRUE(cco_object_get_string(v, &s, &slen));
            EXPECT_TRUE(s != NULL && strcmp(s, "val\n2") == 0);
        }
        cco_object_release(obj);
    }

    /* Test 8: Map key exceeding 256 bytes */
    {
        char lkey[350];
        memset(lkey, 'x', 320);
        lkey[320] = '\0';
        char ldoc[500];
        int n = snprintf(ldoc, sizeof(ldoc), "(\"%s\": 999)", lkey);
        obj = cco_parse_string(ldoc, (size_t)n, NULL);
        EXPECT_TRUE(obj != NULL);
        if (obj) {
            cco_object_t* v = cco_map_get_by_key(obj, lkey);
            EXPECT_TRUE(v != NULL);
            int64_t val = 0;
            EXPECT_TRUE(cco_object_get_integer(v, &val));
            EXPECT_EQ_INT((int)val, 999);
            cco_object_release(obj);
        }
    }

    /* Test 9: Dotted map key with comments */
    const char* cdoc = "(a.b /* comment */ : 10, c.d /* another comment */ : 20)";
    obj = cco_parse_string(cdoc, strlen(cdoc), NULL);
    EXPECT_TRUE(obj != NULL);
    if (obj) {
        cco_object_t* v1 = cco_map_get_by_key(obj, "a.b");
        EXPECT_TRUE(v1 != NULL);
        cco_object_t* v2 = cco_map_get_by_key(obj, "c.d");
        EXPECT_TRUE(v2 != NULL);
        cco_object_release(obj);
    }

    /* Test 10: Keys with invalid dotted segments serialize quoted and roundtrip */
    {
        const char* dot_keys[] = {"foo.", "foo..bar", "foo.123", "foo.true", "foo.null"};
        for (int k = 0; k < 5; k++) {
            char ddoc[128];
            int dn = snprintf(ddoc, sizeof(ddoc), "(\"%s\": 42)", dot_keys[k]);
            cco_object_t* dobj = cco_parse_string(ddoc, (size_t)dn, NULL);
            EXPECT_TRUE(dobj != NULL);
            if (dobj) {
                char* dser = cco_serialize_to_string(dobj, false);
                EXPECT_TRUE(dser != NULL);
                if (dser) {
                    cco_object_t* dre = cco_parse_string(dser, strlen(dser), NULL);
                    EXPECT_TRUE(dre != NULL);
                    if (dre) {
                        cco_object_t* dv = cco_map_get_by_key(dre, dot_keys[k]);
                        EXPECT_TRUE(dv != NULL);
                        cco_object_release(dre);
                    }
                    free(dser);
                }
                cco_object_release(dobj);
            }
        }
    }

    /* Test 11: Empty array serialization roundtrip */
    obj = cco_parse_string("(,)", 3, NULL);
    EXPECT_TRUE(obj != NULL);
    EXPECT_EQ_INT(CCO_TYPE_ARRAY, cco_object_get_type(obj));
    if (obj) {
        EXPECT_EQ_INT((int)cco_array_get_count(obj), 0);
        char* ser = cco_serialize_to_string(obj, false);
        EXPECT_TRUE(ser != NULL);
        if (ser) {
            EXPECT_TRUE(strcmp(ser, "(,)") == 0);
            cco_object_t* re = cco_parse_string(ser, strlen(ser), NULL);
            EXPECT_TRUE(re != NULL);
            EXPECT_EQ_INT(CCO_TYPE_ARRAY, cco_object_get_type(re));
            if (re) {
                EXPECT_EQ_INT((int)cco_array_get_count(re), 0);
                cco_object_release(re);
            }
            free(ser);
        }
        cco_object_release(obj);
    }

    /* Test 12: Zero integer serialization roundtrip */
    obj = cco_parse_string("(val: 0)", 8, NULL);
    EXPECT_TRUE(obj != NULL);
    if (obj) {
        char* ser = cco_serialize_to_string(obj, false);
        EXPECT_TRUE(ser != NULL);
        if (ser) {
            cco_object_t* re = cco_parse_string(ser, strlen(ser), NULL);
            EXPECT_TRUE(re != NULL);
            if (re) {
                cco_object_t* v = cco_map_get_by_key(re, "val");
                EXPECT_TRUE(v != NULL);
                int64_t val = -1;
                EXPECT_TRUE(cco_object_get_integer(v, &val));
                EXPECT_EQ_INT((int)val, 0);
                cco_object_release(re);
            }
            free(ser);
        }
        cco_object_release(obj);
    }

    /* Test 13: Hex escape sequence \xNN */
    {
        const char* hex_src = "(msg: \"\\x41\\x42\\x43\")";
        obj = cco_parse_string(hex_src, strlen(hex_src), NULL);
        EXPECT_TRUE(obj != NULL);
        if (obj) {
            cco_object_t* v = cco_map_get_by_key(obj, "msg");
            EXPECT_TRUE(v != NULL);
            if (v) {
                const char* s = NULL;
                size_t slen = 0;
                EXPECT_TRUE(cco_object_get_string(v, &s, &slen));
                EXPECT_TRUE(s != NULL && strcmp(s, "ABC") == 0);
            }
            cco_object_release(obj);
        }
    }

    /* Test 14: Unicode escape sequence \uNNNN */
    {
        const char* u_src = "(msg: \"\\u0041\\u0042\")";
        obj = cco_parse_string(u_src, strlen(u_src), NULL);
        EXPECT_TRUE(obj != NULL);
        if (obj) {
            cco_object_t* v = cco_map_get_by_key(obj, "msg");
            EXPECT_TRUE(v != NULL);
            if (v) {
                const char* s = NULL;
                size_t slen = 0;
                EXPECT_TRUE(cco_object_get_string(v, &s, &slen));
                EXPECT_TRUE(s != NULL && strcmp(s, "AB") == 0);
            }
            cco_object_release(obj);
        }
    }

    /* Test 15: Backtick raw string literal */
    {
        const char* raw_src = "(query: `SELECT * FROM users WHERE name = \"John\"`)";
        obj = cco_parse_string(raw_src, strlen(raw_src), NULL);
        EXPECT_TRUE(obj != NULL);
        if (obj) {
            cco_object_t* v = cco_map_get_by_key(obj, "query");
            EXPECT_TRUE(v != NULL);
            if (v) {
                const char* s = NULL;
                size_t slen = 0;
                EXPECT_TRUE(cco_object_get_string(v, &s, &slen));
                EXPECT_TRUE(s != NULL && strcmp(s, "SELECT * FROM users WHERE name = \"John\"") == 0);
            }
            cco_object_release(obj);
        }
    }

    /* Test 16: Dotted key with block comment */
    {
        const char* lcomment_src = "(server.host /* primary host */\n: \"localhost\", server.port: 8080)";
        obj = cco_parse_string(lcomment_src, strlen(lcomment_src), NULL);
        EXPECT_TRUE(obj != NULL);
        if (obj) {
            cco_object_t* v1 = cco_map_get_by_key(obj, "server.host");
            EXPECT_TRUE(v1 != NULL);
            cco_object_t* v2 = cco_map_get_by_key(obj, "server.port");
            EXPECT_TRUE(v2 != NULL);
            cco_object_release(obj);
        }
    }

    /* Test 17: Lenient brackets option enforcement */
    {
        const char* json_like = "(user: { name: \"Bob\", roles: [ \"admin\" ] })";
        cco_parse_options_t strict_opts;
        cco_parse_options_init(&strict_opts);
        strict_opts.lenient_brackets = false;

        cco_object_t* strict_obj = cco_parse_string(json_like, strlen(json_like), &strict_opts);
        EXPECT_TRUE(strict_obj == NULL);
        EXPECT_EQ_INT(CCO_ERR_PARSE, cco_get_last_error());

        cco_parse_options_t lenient_opts;
        cco_parse_options_init(&lenient_opts);
        lenient_opts.lenient_brackets = true;

        cco_object_t* lenient_obj = cco_parse_string(json_like, strlen(json_like), &lenient_opts);
        EXPECT_TRUE(lenient_obj != NULL);
        if (lenient_obj) {
            EXPECT_EQ_INT(CCO_TYPE_MAP, cco_object_get_type(lenient_obj));
            cco_object_release(lenient_obj);
        }
    }
}
