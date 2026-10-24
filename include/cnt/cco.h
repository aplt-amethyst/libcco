#ifndef CNT_CCO_H
#define CNT_CCO_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "cnt/cco_export.h"
#include <cnt/cco_error.h>
#include <cnt/cco_options.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /* Opaque handles */
    typedef struct cco_object_s cco_object_t;
    typedef struct cco_array_s cco_array_t;
    typedef struct cco_parse_result_s cco_parse_result_t;
    typedef struct cco_symbol_table_s cco_symbol_table_t;
    typedef struct cco_template_s cco_template_t;
    typedef struct cco_expr_s cco_expr_t;
    typedef struct cco_arena_s cco_arena_t;

    /* Object Types */
    typedef enum
    {
        CCO_TYPE_NONE = 0,
        CCO_TYPE_BOOLEAN = 1,
        CCO_TYPE_INTEGER = 2,
        CCO_TYPE_FLOAT = 3,
        CCO_TYPE_STRING = 4,
        CCO_TYPE_ARRAY = 5,
        CCO_TYPE_MAP = 6,
        CCO_TYPE_TEMPLATE_INSTANCE = 7,
        CCO_TYPE_TYPEDEF = 8,
        CCO_TYPE_ENUM = 9,
        CCO_TYPE_PARAM = 10,
        CCO_TYPE_RAW_EXPR = 11
    } cco_type_t;

    /* Expression Operators */
    typedef enum
    {
        CCO_OP_NONE = 0,
        CCO_OP_OR,
        CCO_OP_AND,
        CCO_OP_BIT_OR,
        CCO_OP_BIT_XOR,
        CCO_OP_BIT_AND,
        CCO_OP_EQ,
        CCO_OP_NEQ,
        CCO_OP_LT,
        CCO_OP_LTE,
        CCO_OP_GT,
        CCO_OP_GTE,
        CCO_OP_COALESCE,
        CCO_OP_ADD,
        CCO_OP_SUB,
        CCO_OP_MUL,
        CCO_OP_DIV,
        CCO_OP_POW,
        CCO_OP_NOT,
        CCO_OP_BIT_NOT,
        CCO_OP_NEG,
        CCO_OP_POS,
        CCO_OP_LEQ = CCO_OP_LTE,
        CCO_OP_GEQ = CCO_OP_GTE
    } cco_op_t;

    /* Diagnostics Struct */
    typedef struct cco_diag
    {
        const char* file;
        size_t      line;
        size_t      col;
        const char* message;
        int         is_error;
    } cco_diag_t;

    /* --- Memory Pool (Arena) API --- */
    CCO_API cco_arena_t* cco_arena_new(size_t max_limit);
    CCO_API void cco_arena_reset(cco_arena_t* arena);
    CCO_API void cco_arena_free(cco_arena_t* arena);

    /* --- Value Construction APIs --- */
    CCO_API cco_object_t* cco_object_new(void);
    CCO_API cco_object_t* cco_none_new(void);
    CCO_API cco_object_t* cco_int_new(int64_t v);
    CCO_API cco_object_t* cco_float_new(double v);
    CCO_API cco_object_t* cco_bool_new(bool v);
    CCO_API cco_object_t* cco_string_new(const char* s);
    CCO_API cco_object_t* cco_string_new_len(const char* s, size_t n);
    CCO_API cco_array_t* cco_array_new(void);
    CCO_API cco_object_t* cco_array_wrap(cco_array_t* arr);

    /* --- Refcount & Lifetime --- */
    CCO_API void cco_object_retain(cco_object_t* obj);
    CCO_API void cco_object_release(cco_object_t* obj);
    CCO_API void cco_object_free(cco_object_t* obj);

    /* --- Inspection & Getters --- */
    CCO_API cco_type_t cco_object_type(const cco_object_t* obj);
    CCO_API int cco_object_get_type(const cco_object_t* obj);
    CCO_API int64_t cco_int_get(const cco_object_t* obj);
    CCO_API double cco_float_get(const cco_object_t* obj);
    CCO_API bool cco_bool_get(const cco_object_t* obj);
    CCO_API const char* cco_string_get(const cco_object_t* obj, size_t* len_out);
    CCO_API bool cco_object_get_boolean(const cco_object_t* obj, bool* out_val);
    CCO_API bool cco_object_get_integer(const cco_object_t* obj, int64_t* out_val);
    CCO_API bool cco_object_get_float(const cco_object_t* obj, double* out_val);
    CCO_API bool cco_object_get_string(const cco_object_t* obj, const char** out_val, size_t* out_len);

    /* --- Map Operations --- */
    CCO_API int cco_object_set(cco_object_t* obj, const char* key, cco_object_t* value);
    CCO_API cco_object_t* cco_object_get(const cco_object_t* obj, const char* key);
    CCO_API int cco_object_has_key(const cco_object_t* obj, const char* key);
    CCO_API int cco_object_del(cco_object_t* obj, const char* key);
    CCO_API size_t cco_object_size(const cco_object_t* obj);
    CCO_API cco_error_t cco_map_insert(cco_object_t* map, const char* key, cco_object_t* value);
    CCO_API size_t cco_map_get_count(const cco_object_t* obj);
    CCO_API const char* cco_map_get_key(const cco_object_t* obj, size_t index);
    CCO_API cco_object_t* cco_map_get_value(const cco_object_t* obj, size_t index);
    CCO_API cco_object_t* cco_map_get_by_key(const cco_object_t* obj, const char* key);

    /* --- Array Operations --- */
    CCO_API int cco_array_add(cco_array_t* arr, cco_object_t* value);
    CCO_API cco_object_t* cco_array_get(const cco_array_t* arr, size_t index);
    CCO_API size_t cco_array_size(const cco_array_t* arr);
    CCO_API size_t cco_array_get_count(const cco_object_t* obj);
    CCO_API cco_object_t* cco_array_get_item(const cco_object_t* obj, size_t index);

    /* --- Deep Copy, Equality & Metadata --- */
    CCO_API cco_object_t* cco_object_clone(const cco_object_t* obj);
    CCO_API bool cco_object_equal(const cco_object_t* a, const cco_object_t* b);
    CCO_API const char* cco_object_template_name(const cco_object_t* obj);

    /* --- Parsing APIs --- */
    CCO_API cco_object_t* cco_parse(const char* text);
    CCO_API cco_object_t* cco_parse_from_file(const char* filename);
    CCO_API cco_object_t* cco_parse_string(const char* src, size_t len, const cco_parse_options_t* opts);
    CCO_API cco_parse_result_t* cco_parse_full(const char* text);
    CCO_API cco_parse_result_t* cco_parse_full_from_file(const char* filename);
    CCO_API cco_parse_result_t* cco_parse_full_with_options(const char* text, const cco_parse_options_t* opts);
    CCO_API cco_parse_result_t* cco_parse_full_from_file_with_options(const char* filename, const cco_parse_options_t* opts);
    CCO_API cco_parse_result_t* cco_parse_full_from_files(const char** paths, size_t count);
    CCO_API cco_parse_result_t* cco_parse_full_froms(const char** texts, size_t count);
    CCO_API cco_symbol_table_t* cco_parse_result_symbols(cco_parse_result_t* res);
    CCO_API cco_object_t* cco_parse_result_root(cco_parse_result_t* res);
    CCO_API void cco_parse_result_free(cco_parse_result_t* res);

    /* --- Symbols & Templates --- */
    CCO_API size_t cco_symbol_table_count(const cco_symbol_table_t* st);
    CCO_API const char* cco_symbol_table_name(const cco_symbol_table_t* st, size_t idx);
    CCO_API const cco_template_t* cco_symbol_table_template(const cco_symbol_table_t* st, const char* name);
    CCO_API const char* cco_template_get_parent_name(const cco_template_t* t);
    CCO_API size_t cco_template_field_count(const cco_template_t* t);
    CCO_API const char* cco_template_field_name(const cco_template_t* t, size_t idx);

    /* --- Serialization --- */
    CCO_API char* cco_serialize(const cco_object_t* obj);
    CCO_API char* cco_serialize_pretty(const cco_object_t* obj, int indent);
    CCO_API char* cco_serialize_full(const cco_parse_result_t* result);
    CCO_API int cco_serialize_into(char* out, size_t cap, const cco_object_t* obj);
    CCO_API int cco_save_file_pretty(const cco_object_t* obj, const char* filename, int indent);
    CCO_API char* cco_serialize_to_string(const cco_object_t* obj, bool pretty);

    /* --- Expressions --- */
    CCO_API cco_expr_t* cco_expr_literal(cco_object_t* value);
    CCO_API cco_expr_t* cco_expr_identifier(const char* name);
    CCO_API cco_expr_t* cco_expr_binary(cco_expr_t* lhs, cco_op_t op, cco_expr_t* rhs);
    CCO_API cco_expr_t* cco_expr_unary(cco_op_t op, cco_expr_t* operand);
    CCO_API cco_expr_t* cco_expr_coalesce(cco_expr_t* left, cco_expr_t* right);
    CCO_API cco_expr_t* cco_expr_compound(cco_expr_t* inner);
    CCO_API cco_expr_t* cco_expr_format(cco_expr_t* string_expr);
    CCO_API cco_expr_t* cco_expr_env(const char* var_name);
    CCO_API void cco_expr_free(cco_expr_t* expr);
    CCO_API cco_object_t* cco_expr_eval(const cco_expr_t* expr, cco_symbol_table_t* st, cco_object_t* self, cco_object_t* locals);

    /* --- Instantiation --- */
    CCO_API cco_object_t* cco_instantiate(cco_symbol_table_t* st, const char* name, cco_object_t** args, size_t arg_count);

    /* --- Diagnostics --- */
    CCO_API int cco_diag_count(void);
    CCO_API const cco_diag_t* cco_diag_get(int i);
    CCO_API void cco_diag_print(const cco_diag_t* diag, const char* source);
    CCO_API void cco_diag_print_all(const char* source);
    CCO_API void cco_clear_diagnostics(void);
    CCO_API int cco_last_error(void);
    CCO_API cco_error_t cco_get_last_error(void);

    /* --- Options & Sandbox --- */
    CCO_API cco_parse_options_t cco_parse_options_default(void);
    CCO_API int cco_sandbox_check_path(const cco_parse_options_t* opts, const char* path);

#ifdef __cplusplus
}
#endif

#endif /* CNT_CCO_H */
