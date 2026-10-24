#!/usr/bin/env bash
set -euo pipefail

# Extract every public function name declared in the public header and
# verify that docs/api.md mentions each one. The header scan is limited
# to declaration lines (leading non-space, non-comment character), so
# illustrative mentions in comments like "e.g. cco_expr_format(...)" do
# not count as declarations.
#
# The API doc scan is intentionally loose (any cco_foo mention counts),
# so the check biases towards passing rather than false-failing an
# early build where no public functions exist yet.

header=include/cnt/cco.h
api_doc=docs/api.md

if [ ! -f "$header" ]; then
    echo "ERROR: $header not found" >&2
    exit 1
fi
if [ ! -f "$api_doc" ]; then
    echo "ERROR: $api_doc not found" >&2
    exit 1
fi

# Strip /* */ block comments, then keep only lines that begin with a
# non-space, non-`*`, non-`/` character. That leaves actual declarations
# while dropping prose and comment continuations.
declared=$(sed -E 's:/\*[^*]*\*/::g' "$header"     | grep -vE '^\s*(\*|//)'     | grep -oE '\bcco_[a-z_]+\s*\('     | sed -E 's/[[:space:]]*\($//'     | sort -u || true)

if [ -z "$declared" ]; then
    echo "OK: no public functions declared yet (indev stage)"
    exit 0
fi

# Every cco_foo token mentioned anywhere in the doc counts as documented.
documented=$(grep -oE '\bcco_[a-z_]+' "$api_doc" | sort -u || true)

missing=$(comm -23 <(echo "$declared") <(echo "$documented"))
if [ -n "$missing" ]; then
    echo "ERROR: public symbols declared in $header but not documented in $api_doc:" >&2
    echo "$missing" >&2
    exit 1
fi

echo "OK: every public symbol in $header appears in $api_doc"
