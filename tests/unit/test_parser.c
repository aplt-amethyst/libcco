#include "../test.h"
#include <cnt/cco.h>
#include "parser.h"

void test_parser(void) {
    cco_object_t* obj;
    
    /* Test 1: none is an identifier which returns error */
    obj = cco_parse_string("none", 4, NULL);
    printf("Finished line %d\n", __LINE__); fflush(stdout); EXPECT_TRUE(obj == NULL);
    EXPECT_EQ_INT(CCO_ERR_PARSE, cco_get_last_error());
    
    /* Test 2: true */
    obj = cco_parse_string("true", 4, NULL);
    printf("Finished line %d\n", __LINE__); fflush(stdout); EXPECT_TRUE(obj != NULL);
    EXPECT_EQ_INT(CCO_TYPE_BOOLEAN, cco_object_get_type(obj));
    cco_object_release(obj);
    
    /* Test 3: empty string should probably give EOF error or something, 
       but currently document expects at least something or just NONE */
    obj = cco_parse_string("", 0, NULL);
    /* Doesn't segfault */
    cco_object_release(obj);
}
