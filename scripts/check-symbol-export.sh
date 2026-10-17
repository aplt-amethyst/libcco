#!/usr/bin/env bash
set -euo pipefail

# check-symbol-export.sh: Validate exported symbols of libcco shared library
# Ensures zero internal symbol leakage and exact match against public API contract.

TARGET="${1:-target/debug/libcco.so}"

if [ ! -f "$TARGET" ]; then
    # If no explicit argument was provided, try alternate locations: target/release or build/
    if [ "$#" -eq 0 ] && [ -f "target/release/libcco.so" ]; then
        TARGET="target/release/libcco.so"
    elif [ "$#" -eq 0 ] && [ -f "build/libcco.so" ]; then
        TARGET="build/libcco.so"
    else
        echo "ERROR: Shared library target '$TARGET' not found. Build it first." >&2
        exit 1
    fi
fi

NM_CMD="${NM:-nm}"
if ! command -v "$NM_CMD" >/dev/null 2>&1; then
    echo "ERROR: $NM_CMD command not found" >&2
    exit 1
fi

echo "==> Inspecting dynamic symbols in $TARGET..."
symbols=$("$NM_CMD" -D --defined-only "$TARGET" 2>/dev/null | awk '{print $NF}' | sort)
count=$(echo "$symbols" | grep -c . || true)

if [ "$count" -ne 21 ]; then
    echo "ERROR: Expected exactly 21 exported symbols, found $count:" >&2
    echo "$symbols" >&2
    exit 1
fi

expected_symbols=(
    "cco_arena_free"
    "cco_arena_new"
    "cco_arena_reset"
    "cco_array_get_count"
    "cco_array_get_item"
    "cco_clear_diagnostics"
    "cco_get_last_error"
    "cco_map_get_by_key"
    "cco_map_get_count"
    "cco_map_get_key"
    "cco_map_get_value"
    "cco_object_get_boolean"
    "cco_object_get_float"
    "cco_object_get_integer"
    "cco_object_get_string"
    "cco_object_get_type"
    "cco_object_release"
    "cco_object_retain"
    "cco_parse_options_init"
    "cco_parse_string"
    "cco_serialize_to_string"
)

# Verify each expected symbol exists
for sym in "${expected_symbols[@]}"; do
    if ! echo "$symbols" | grep -qx "$sym"; then
        echo "ERROR: Missing expected public symbol: $sym" >&2
        exit 1
    fi
done

# Verify no unexpected symbol exists
for sym in $symbols; do
    found=0
    for exp in "${expected_symbols[@]}"; do
        if [ "$sym" = "$exp" ]; then
            found=1
            break
        fi
    done
    if [ "$found" -eq 0 ]; then
        echo "ERROR: Leaked unexpected symbol: $sym" >&2
        exit 1
    fi
done

echo "OK: Verified exactly 21 public symbols and 0 internal leaks in $TARGET"
