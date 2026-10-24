#ifndef CNT_CCO_INTERNAL_SERIALIZER_H
#define CNT_CCO_INTERNAL_SERIALIZER_H

#include "object.h"
#include "strbuf.h"

typedef struct
{
    bool pretty;        /* True for pretty print, false for compact */
    int indent_spaces;  /* Number of spaces per indentation level */
    int current_indent; /* Current indentation depth */
    int depth;          /* Current recursion depth */
    cco_strbuf_t* out;  /* Output string buffer */
} cco_serializer_context_t;

cco_error_t cco_serialize_buffer(const cco_object_t* obj, cco_strbuf_t* out_buf,
                                 bool pretty, int indent_spaces);

#endif /* CNT_CCO_INTERNAL_SERIALIZER_H */
