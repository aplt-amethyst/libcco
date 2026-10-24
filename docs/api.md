# libcco Public API Reference

**Status: indev.** This is the authoritative reference for the public
surface in `include/cnt/cco.h` and `include/cnt/cco.hpp`. Any PR that
adds, removes, or renames a public symbol MUST update this file in the
same commit. CI enforces it (`scripts/check-api-doc.sh`).

## Conventions

Ownership is spelled out per function with three words:

- `new` - caller owns the returned pointer.
- `transfer` - callee takes ownership, caller must NOT free.
- `borrowed` - callee keeps it valid only while the parent lives.

Every failure returns `NULL`, a negative `CCO_ERR_*` code, or a
documented sentinel, and pushes a diagnostic into the thread-local list.

Diagnostics and error state are thread-local. Every other call is safe
to use concurrently on distinct objects. `refcount` is not atomic;
sharing a single object across threads needs external synchronization.

## Version

```c
#define CCO_VERSION_MAJOR 0
#define CCO_VERSION_MINOR 1
#define CCO_VERSION_PATCH 0
```

## Error codes

```c
typedef enum
{
    CCO_OK                 =  0,
    CCO_ERR_PARSE          = -1,
    CCO_ERR_NOT_FOUND      = -2,
    CCO_ERR_TYPE_MISMATCH  = -3,
    CCO_ERR_OUT_OF_RANGE   = -4,
    CCO_ERR_IO             = -5,
    CCO_ERR_FORBIDDEN      = -6,
    CCO_ERR_NOMEM          = -7,
    CCO_ERR_DEPTH_EXCEEDED = -8,
    CCO_ERR_INVALID_ARG    = -9
} cco_err_t;
```

`CCO_ERR_FORBIDDEN` is returned by every code path that reaches a
capability disabled at compile time. It is never a silent fallback.

## Types

```c
typedef struct cco_object_s       cco_object_t;
typedef struct cco_array_s        cco_array_t;
typedef struct cco_parse_result_s cco_parse_result_t;
typedef struct cco_symbol_table_s cco_symbol_table_t;
typedef struct cco_template_s     cco_template_t;
typedef struct cco_expr_s         cco_expr_t;

typedef enum
{
    CCO_TYPE_NONE,
    CCO_TYPE_BOOL,
    CCO_TYPE_INT,
    CCO_TYPE_FLOAT,
    CCO_TYPE_STRING,
    CCO_TYPE_ARRAY,
    CCO_TYPE_MAP,
    CCO_TYPE_TEMPLATE_INSTANCE
} cco_type_t;
```

All handles are opaque. `sizeof(cco_object_t)` is asserted `<= 64`
at compile time.

## Value layer

### Construction

```c
cco_object_t* cco_object_new(void);
cco_object_t* cco_none_new(void);
cco_object_t* cco_int_new(int64_t v);
cco_object_t* cco_float_new(double v);
cco_object_t* cco_bool_new(bool v);
cco_object_t* cco_string_new(const char* s);
cco_object_t* cco_string_new_len(const char* s, size_t n);
cco_array_t*  cco_array_new(void);
cco_object_t* cco_array_wrap(cco_array_t* arr);
```

Every constructor returns `new`. `NULL` means allocation failed.

`cco_object_new` builds an empty map. `cco_array_new` builds an empty
array handle; `cco_array_wrap` turns it into an object usable as a
value. After wrap, do not call `cco_array_*` on the raw pointer.

`cco_string_new` reads to the first NUL. `cco_string_new_len` reads
exactly `n` bytes and supports embedded `\0`; serialization escapes
them as `\x00`.

### Refcount

```c
void cco_object_retain(cco_object_t* obj);
void cco_object_free(cco_object_t* obj);
```

`refcount` starts at 1. `cco_object_free` decrements and destroys at 0.
Passing `NULL` is a no-op. `refcount` is not atomic.

### Inspection and getters

```c
cco_type_t  cco_object_type(const cco_object_t* obj);
int64_t     cco_int_get(const cco_object_t* obj);
double      cco_float_get(const cco_object_t* obj);
bool        cco_bool_get(const cco_object_t* obj);
const char* cco_string_get(const cco_object_t* obj, size_t* len_out);
```

Getters on the wrong type return 0, `0.0`, `false`, or `NULL`, and push
`CCO_ERR_TYPE_MISMATCH`. `cco_string_get` returns a borrowed pointer
valid until `obj` is freed; `len_out` may be `NULL`.

### Map operations

```c
int           cco_object_set(cco_object_t* obj, const char* key, cco_object_t* value);
cco_object_t* cco_object_get(const cco_object_t* obj, const char* key);
int           cco_object_has_key(const cco_object_t* obj, const char* key);
int           cco_object_del(cco_object_t* obj, const char* key);
size_t        cco_object_size(const cco_object_t* obj);
```

`cco_object_set` takes ownership of `value` (transfer). Overwriting an
existing key frees the old value. `cco_object_get` returns borrowed;
`NULL` when the key is missing. `cco_object_has_key` returns 1 or 0.
`cco_object_del` frees the value and returns `CCO_OK`, or
`CCO_ERR_NOT_FOUND` when the key is absent.

### Array operations

```c
int           cco_array_add(cco_array_t* arr, cco_object_t* value);
cco_object_t* cco_array_get(const cco_array_t* arr, size_t index);
size_t        cco_array_size(const cco_array_t* arr);
```

`cco_array_add` takes ownership of `value` (transfer).
`cco_array_get` returns borrowed; `NULL` when `index` is out of range.

### Deep copy and equality

```c
cco_object_t* cco_object_clone(const cco_object_t* obj);
bool          cco_object_equal(const cco_object_t* a, const cco_object_t* b);
```

`cco_object_clone` returns a fully independent `new` object.
`cco_object_equal` compares recursively. Strings compare by length and
`memcmp`, so embedded NUL bytes compare correctly.

### Template metadata

```c
const char* cco_object_template_name(const cco_object_t* obj);
```

Returns the template name for a `CCO_TYPE_TEMPLATE_INSTANCE`, `NULL`
otherwise. Borrowed pointer.

## Parsing

### Simple

```c
cco_object_t* cco_parse(const char* text);
cco_object_t* cco_parse_from_file(const char* filename);
```

Parses a root value and discards declarations. Returns `new` object or
`NULL`. Diagnostics collect in the thread-local list.

### Full

```c
cco_parse_result_t* cco_parse_full(const char* text);
cco_parse_result_t* cco_parse_full_from_file(const char* filename);
cco_parse_result_t* cco_parse_full_with_options(const char* text,
                                                const cco_parse_options_t* opts);
cco_parse_result_t* cco_parse_full_from_file_with_options(const char* filename,
                                                          const cco_parse_options_t* opts);
```

Returns both the root value and the symbol table. When the input has no
root expression, `cco_parse_full` fails with the diagnostic
`"no root expression"`; use `cco_parse_full_froms` if root-less input is
valid for your case.

### Multi-source

```c
cco_parse_result_t* cco_parse_full_from_files(const char** paths, size_t count);
cco_parse_result_t* cco_parse_full_froms(const char** texts, size_t count);
```

Declarations accumulate across all inputs. Root is taken from the last
input that produced one. `cco_parse_full_froms` accepts root-less inputs:
`cco_parse_result_root` then returns `NULL` and the symbol table is
still complete. This is the schema-only workflow.

### Inspecting a result

```c
cco_symbol_table_t* cco_parse_result_symbols(cco_parse_result_t* res);
cco_object_t*       cco_parse_result_root(cco_parse_result_t* res);
void                cco_parse_result_free(cco_parse_result_t* res);
```

Both getters return borrowed pointers valid until
`cco_parse_result_free`. That call releases the arena, the symbol table,
and the root object together.

## Symbols and templates

```c
size_t                cco_symbol_table_count(const cco_symbol_table_t* st);
const char*           cco_symbol_table_name(const cco_symbol_table_t* st, size_t idx);
const cco_template_t* cco_symbol_table_template(const cco_symbol_table_t* st,
                                                const char* name);

const char* cco_template_get_parent_name(const cco_template_t* t);
size_t      cco_template_field_count(const cco_template_t* t);
const char* cco_template_field_name(const cco_template_t* t, size_t idx);
```

Every returned pointer is borrowed, valid for the life of the owning
parse result. `cco_symbol_table_template` returns `NULL` when the name
is unknown. `cco_template_get_parent_name` returns `NULL` for a root
template.

## Serialization

```c
char* cco_serialize(const cco_object_t* obj);
char* cco_serialize_pretty(const cco_object_t* obj, int indent);
char* cco_serialize_full(const cco_parse_result_t* result);
int   cco_serialize_into(char* restrict out, size_t cap,
                         const cco_object_t* restrict obj);
int   cco_save_file_pretty(const cco_object_t* obj,
                           const char* filename, int indent);
```

`cco_serialize` and `cco_serialize_pretty` return a `new` C string; the
caller frees it with `free`. `indent` counts spaces per level; pass 2
for the convention used across the docs.

`cco_serialize_full` emits every declaration (`$typedef`, `$enum`,
`$temp`, `$function`) followed by the root value. When
`cco_parse_result_root` is `NULL` the output contains only declarations.

`cco_serialize_into` writes into a caller buffer and returns a negative
error code when `cap` is too small. `cco_save_file_pretty` writes to
disk, and returns `CCO_ERR_FORBIDDEN` when the sandbox rejects the path.

Strings escape `"`, `\\`, `\n`, `\t`, `\r`, control characters, and
embedded NUL as `\x00`. Template instances print as `#Name(...)` or
`#Name: (...)` depending on whether they were built positionally or by
name. Enum values print by name when a symbol table is available, by
integer otherwise.

With `CCO_ENABLE_COMMENT_PRESERVE=ON` and
`options.pretty_preserve_comments=true`, `cco_serialize_pretty` writes
each node's `leading_comments` and the root's `trailing_comments`,
copying them verbatim from source.

## Expressions

```c
typedef enum
{
    CCO_OP_ADD, CCO_OP_SUB, CCO_OP_MUL, CCO_OP_DIV,
    CCO_OP_EQ, CCO_OP_NEQ, CCO_OP_LT, CCO_OP_GT, CCO_OP_LEQ, CCO_OP_GEQ,
    CCO_OP_AND, CCO_OP_OR, CCO_OP_NOT,
    CCO_OP_COALESCE
} cco_op_t;

cco_expr_t* cco_expr_literal(cco_object_t* value);
cco_expr_t* cco_expr_identifier(const char* name);
cco_expr_t* cco_expr_binary(cco_expr_t* lhs, cco_op_t op, cco_expr_t* rhs);
cco_expr_t* cco_expr_unary(cco_op_t op, cco_expr_t* operand);
cco_expr_t* cco_expr_coalesce(cco_expr_t* left, cco_expr_t* right);
cco_expr_t* cco_expr_compound(cco_expr_t* inner);
cco_expr_t* cco_expr_format(cco_expr_t* string_expr);
cco_expr_t* cco_expr_env(const char* var_name);
void        cco_expr_free(cco_expr_t* expr);

cco_object_t* cco_expr_eval(const cco_expr_t* expr,
                            cco_symbol_table_t* st,
                            cco_object_t* self,
                            cco_object_t* locals);
```

`cco_expr_literal` takes ownership of `value` (transfer).
`cco_expr_free` releases the tree.

`cco_expr_format` and `cco_expr_env` are declared and constructible
regardless of the `CCO_ENABLE_FORMAT` and `CCO_ENABLE_ENV` switches.
The node they build is valid data and serializes; only evaluation
refuses.

Three switches change the behavior of `cco_expr_eval`:

| Switch | Effect |
|---|---|
| `CCO_ENABLE_FORMAT=OFF` | Evaluates a FormatNode to `CCO_ERR_FORBIDDEN`. Parse of `$format(` in source also rejects. Two independent paths. |
| `CCO_ENABLE_ENV=OFF` | Same shape: parse rejects `$env(`, eval rejects a manually built EnvNode. |
| `CCO_ENABLE_EVAL=OFF` | Every `cco_expr_eval` call returns `CCO_ERR_FORBIDDEN`. `$(...)` in source still parses and produces a full AST. |

With `CCO_ENABLE_EVAL=ON`, evaluation follows operator precedence from
`full.md` §23.2: `!` binds tightest, `|` binds loosest.

When `st` is `NULL`, identifier lookup uses only `self` and `locals`.
When both are `NULL`, identifier lookup fails. The return value is
`new`.

## Instantiation

```c
cco_object_t* cco_instantiate(cco_symbol_table_t* st, const char* name,
                              cco_object_t** args, size_t arg_count);
```

Positional template instantiation. `args` are consumed (transfer). The
result is a `new` object of type `CCO_TYPE_TEMPLATE_INSTANCE`.
Respects `options.max_instantiations` and returns `NULL` with
`CCO_ERR_OUT_OF_RANGE` when exceeded. With
`CCO_ENABLE_CONSTRUCTORS=OFF`, returns `NULL` and pushes
`CCO_ERR_FORBIDDEN`.

## Diagnostics

```c
typedef struct cco_diag
{
    const char* file;
    size_t      line;
    size_t      col;
    const char* message;
    int         is_error;
} cco_diag_t;

int               cco_diag_count(void);
const cco_diag_t* cco_diag_get(int i);
void              cco_diag_print(const cco_diag_t* diag, const char* source);
void              cco_diag_print_all(const char* source);
void              cco_clear_diagnostics(void);
int               cco_last_error(void);
```

All state is thread-local. `cco_diag_get` returns a borrowed pointer
valid until the next `cco_diag_add` or `cco_clear_diagnostics` on the
same thread.

With `CCO_ENABLE_ERROR_RECOVERY=OFF`, at most one diagnostic collects
per parse. With `=ON`, up to 64 collect and the parse returns the first
error code encountered.

`cco_last_error` returns the first error code recorded since the last
clear, or `CCO_OK`.

## Options

```c
typedef struct cco_parse_options cco_parse_options_t;
cco_parse_options_t cco_parse_options_default(void);
```

Full field list, defaults, and enforcement points live in
[options.md](options.md). The struct layout depends on the
`CCO_ENABLE_*` compile-time set; the caller's translation units MUST
match the library.

## Sandbox

```c
int cco_sandbox_check_path(const cco_parse_options_t* opts, const char* path);
```

Returns `CCO_OK` when the path is permitted, `CCO_ERR_FORBIDDEN` when
rejected, `CCO_ERR_IO` when `realpath` fails. With
`opts->base_dir == NULL` and `restrict_fs == true`, the current working
directory and its descendants pass. With `restrict_fs == false`, every
path passes.

## C++ wrapper

`include/cnt/cco.hpp` exposes:

- `cnt::Value` - RAII wrapper over `cco_object_t*`
- `cnt::Array` - builder over `cco_array_t*`
- `cnt::ArrayProxy` / `cnt::ValueProxy` - implicit conversions from
  `long long`, `bool`, `std::string`, `const char*`
- `cnt::ParseResult` - returned by `cnt::parse(src)`; has `root()` and
  `serialize_pretty(n)`
- `cnt::get_diagnostics()` / `cnt::clear_diagnostics()` - namespace
  functions
- Map iteration yields `std::pair<const char*, cnt::Value>` for range-for

Exception mapping:

| Failure | Exception |
|---|---|
| Parse error | `std::runtime_error` |
| Type mismatch | `std::logic_error` |
| Missing key | `std::out_of_range` |

Target C++11. Only the standard library is used.

## Ownership summary

| Returns | Caller frees |
|---|---|
| `cco_object_new`, `cco_none_new`, `cco_int_new`, `cco_float_new`, `cco_bool_new`, `cco_string_new*`, `cco_array_new`, `cco_array_wrap` | yes |
| `cco_object_clone` | yes |
| `cco_parse*`, `cco_parse_full*` | yes (`cco_object_free` or `cco_parse_result_free`) |
| `cco_serialize*` | yes (`free`) |
| `cco_expr_*_new` and `cco_expr_literal`, `cco_expr_identifier`, `cco_expr_binary`, `cco_expr_unary`, `cco_expr_coalesce`, `cco_expr_compound`, `cco_expr_format`, `cco_expr_env` | yes (`cco_expr_free`) |
| `cco_expr_eval`, `cco_instantiate` | yes (`cco_object_free`) |
| `cco_object_get`, `cco_array_get`, `cco_parse_result_*`, `cco_symbol_table_*`, `cco_template_*`, `cco_string_get`, `cco_object_template_name`, `cco_diag_get` | no (borrowed) |

## Arena and Low-Level DOM Inspection API

### Memory Arena API
```c
cco_arena_t* cco_arena_new(size_t max_limit);
void         cco_arena_reset(cco_arena_t* arena);
void         cco_arena_free(cco_arena_t* arena);
```
Memory pools manage chunked allocations across parse cycles. `cco_arena_reset` enables zero-heap allocation parsing.

### Low-Level Parsing and Serialization
```c
cco_object_t* cco_parse_string(const char* src, size_t len, const cco_parse_options_t* opts);
char*         cco_serialize_to_string(const cco_object_t* obj, bool pretty);
```
`cco_parse_string` parses an explicit buffer slice with custom options. `cco_serialize_to_string` produces a heap-allocated serialized string.

### Low-Level Object Inspection & Memory
```c
int         cco_object_get_type(const cco_object_t* obj);
void        cco_object_release(cco_object_t* obj);
bool        cco_object_get_boolean(const cco_object_t* obj, bool* out_val);
bool        cco_object_get_integer(const cco_object_t* obj, int64_t* out_val);
bool        cco_object_get_float(const cco_object_t* obj, double* out_val);
bool        cco_object_get_string(const cco_object_t* obj, const char** out_val, size_t* out_len);
cco_error_t cco_get_last_error(void);
```
Provides type code extraction and out-pointer value extraction returning `bool` success flags. `cco_object_release` decrements refcount and frees at zero. `cco_get_last_error` queries the first recorded thread-local error code.

### Low-Level Map and Array Inspection
```c
cco_error_t   cco_map_insert(cco_object_t* map, const char* key, cco_object_t* value);
size_t        cco_map_get_count(const cco_object_t* obj);
const char*   cco_map_get_key(const cco_object_t* obj, size_t index);
cco_object_t* cco_map_get_value(const cco_object_t* obj, size_t index);
cco_object_t* cco_map_get_by_key(const cco_object_t* obj, const char* key);

size_t        cco_array_get_count(const cco_object_t* obj);
cco_object_t* cco_array_get_item(const cco_object_t* obj, size_t index);
```
Index-based and key-based traversal primitives for map and array objects. `cco_map_insert` directly inserts a key-value pair.
