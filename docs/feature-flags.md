# Compile-time Feature Flags

**Status: indev.** Seven `CCO_ENABLE_*` macros gate optional parts of
the library. Each defaults to a conservative baseline. When a flag is
off, the matching syntax is rejected at parse time and the matching API
returns `CCO_ERR_FORBIDDEN`. Nothing degrades silently.

Set the flags once at CMake configure time. Every translation unit that
touches `include/cnt/cco.h` must see the same set, because several of
them change the layout of `cco_parse_options_t`.

## The two rejection paths

Every gated feature rejects in two independent ways.

**Parse rejection.** Source containing the disabled literal syntax fails
at parse time with `CCO_ERR_FORBIDDEN`, a line, and a column. No AST
node is built. `$format("x")` under `CCO_ENABLE_FORMAT=OFF` never
reaches the parser.

**API rejection.** A caller that builds the node by hand, say with
`cco_expr_format(...)`, gets a valid node that serializes. Evaluation of
that node returns `CCO_ERR_FORBIDDEN`. The node is legitimate data; only
the evaluator refuses.

The two paths have separate tests, because a passing test on one tells
you nothing about the other.

## Flags

### `CCO_ENABLE_FORMAT` (default `OFF`)

Enables `$format(str)`, which parses an embedded string as CCO at parse
time.

With it on, `$format` shares the parent's options when
`options.format_inherits_options` is true, or uses
`options.format_options` when it is false. Nesting depth counts against
the parent's `max_nesting_depth`. A `$format` inside a `$format` fails
because depth runs out before the inner call.

With it off, source containing `$format(` is rejected at parse time.
A manually built FormatNode still serializes as `$format(...)`, and
`cco_expr_eval` on it returns `CCO_ERR_FORBIDDEN`.

The struct field `options.allow_format` exists only when this flag is on.

### `CCO_ENABLE_EVAL` (default `OFF`)

Turns `cco_expr_eval` into a real evaluator.

With it on, expressions evaluate with operator precedence from
`full.md` §23.2: `!` tightest, `|` loosest. `$(...)` produces a value.

With it off, `cco_expr_eval` returns `CCO_ERR_FORBIDDEN` on every input,
and `$(...)` in source still parses to a complete AST. Static analysis
tools that want the tree without the evaluator can build against this
configuration and read the AST directly.

### `CCO_ENABLE_COMMENT_PRESERVE` (default `OFF`)

Makes the lexer record `/* ... */` comments and the pretty serializer
emit them.

With it on, the lexer stores every comment's `{offset, length, line,
col}` in a side channel. Tokens are unchanged. Each comment attaches to
the nearest following node as a leading comment; a comment with no
following token attaches to the root as a trailing comment. Nested
cases work out naturally: `(a: 1, /* c */ b: 2)` attaches the comment
to field `b`, not to the outer map.

With it off, the lexer skips comments entirely and `cco_serialize_pretty`
ignores them.

The struct field `options.pretty_preserve_comments` exists only when this
flag is on.

### `CCO_ENABLE_ERROR_RECOVERY` (default `OFF`)

Controls whether a parse returns one diagnostic or many.

With it on, each layer (`parse_value`, `parse_field`, `parse_stmt`)
implements an error production that synchronizes on `,`, `)`, `}`, or
`]` and continues. Up to 64 diagnostics collect. The parse returns the
first error code it saw. A guard stops a loop when the same byte
position errors twice.

With it off, parsing stops at the first error. Exactly one diagnostic
lands. This is the smaller, faster default; use it unless you need the
full error report.

The struct field `options.collect_diagnostics` exists only when this flag
is on.

### `CCO_ENABLE_ENV` (default `OFF`)

Enables `$env("VAR")`, which reads an environment variable.

With it on, an unset variable yields `None`. Values never leak into
diagnostics, so a `$env` read never exposes a secret through an error
message.

With it off, source containing `$env(` is rejected at parse time. A
manually built EnvNode serializes; evaluation returns
`CCO_ERR_FORBIDDEN`.

The struct field `options.allow_env` exists only when this flag is on.

### `CCO_ENABLE_STATIC_CALLS` (default `ON`)

Enables `#T:method(args)` and `T.method(args)`.

With it on, both forms call into a static method body. The body supports
`bind`, `assign`, `$return<T>(...)`, and bare expressions. `$return`
checks against the declared signature and fails with
`CCO_ERR_TYPE_MISMATCH` on a mismatch.

With it off, both forms are parse-rejected.

The struct field `options.allow_static_calls` exists only when this flag
is on.

### `CCO_ENABLE_CONSTRUCTORS` (default `ON`)

Enables `$function.@`, which replaces a template's default constructor.

With it on, defining `$function.@` for a template makes every
instantiation of that template run the body. Inside the body,
`$this.(...)` assigns fields.

With it off, `$function.@` in source is parse-rejected.

The struct field `options.allow_constructors` exists only when this flag
is on.

### `CCO_ENABLE_FAST` (default `OFF`)

Enables high-performance fast parsing paths, inlining lexing, bypassing duplicate map key hashing, and optimizing token handling for extreme throughput.

### `CCO_ENABLE_ZERO_COPY` (default `OFF`)

Enables zero-copy DOM arena allocation for string and node buffers during parsing.

### `CCO_ENABLE_INCLUDE` (default `OFF`)

Enables `$include("path")` directive support for modular document inclusion.

### `CCO_ENABLE_STRICT_SAFETY` (default `OFF`)

Enables strict memory bounds and safety checks within options and parse contexts.

## ABI consistency

The flags change the layout of `cco_parse_options_t`. Four of them
add or remove a whole field, so a caller built with a different set sees
a struct of the wrong size. That is an ABI break, not a runtime error
you can catch. Set the flags once in your build system, propagate them
to every consumer, and never mix.

## Feature matrix in CI

CI runs six configurations to keep each flag independently covered:

1. All off.
2. All on.
3. Only `CCO_ENABLE_FORMAT`.
4. Only `CCO_ENABLE_EVAL`.
5. Only `CCO_ENABLE_COMMENT_PRESERVE`.
6. Only `CCO_ENABLE_ERROR_RECOVERY`.

The last four exist because comment preservation and error recovery were
added on user request and deserve their own coverage; the first two are
the extremes.
