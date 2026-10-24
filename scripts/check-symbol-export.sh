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

if [ "$count" -ne 88 ]; then
    echo "ERROR: Expected exactly 88 exported symbols, found $count:" >&2
    echo "$symbols" >&2
    exit 1
fi

expected_symbols=(
    "cco_arena_free"
    "cco_arena_new"
    "cco_arena_reset"
    "cco_array_add"
    "cco_array_get"
    "cco_array_get_count"
    "cco_array_get_item"
    "cco_array_new"
    "cco_array_size"
    "cco_array_wrap"
    "cco_bool_get"
    "cco_bool_new"
    "cco_clear_diagnostics"
    "cco_diag_count"
    "cco_diag_get"
    "cco_diag_print"
    "cco_diag_print_all"
    "cco_expr_binary"
    "cco_expr_coalesce"
    "cco_expr_compound"
    "cco_expr_env"
    "cco_expr_eval"
    "cco_expr_format"
    "cco_expr_free"
    "cco_expr_identifier"
    "cco_expr_literal"
    "cco_expr_unary"
    "cco_float_get"
    "cco_float_new"
    "cco_get_last_error"
    "cco_instantiate"
    "cco_int_get"
    "cco_int_new"
    "cco_last_error"
    "cco_map_get_by_key"
    "cco_map_get_count"
    "cco_map_get_key"
    "cco_map_get_value"
    "cco_map_insert"
    "cco_none_new"
    "cco_object_clone"
    "cco_object_del"
    "cco_object_equal"
    "cco_object_free"
    "cco_object_get"
    "cco_object_get_boolean"
    "cco_object_get_float"
    "cco_object_get_integer"
    "cco_object_get_string"
    "cco_object_get_type"
    "cco_object_has_key"
    "cco_object_new"
    "cco_object_release"
    "cco_object_retain"
    "cco_object_set"
    "cco_object_size"
    "cco_object_template_name"
    "cco_object_type"
    "cco_parse"
    "cco_parse_from_file"
    "cco_parse_full"
    "cco_parse_full_from_file"
    "cco_parse_full_from_file_with_options"
    "cco_parse_full_from_files"
    "cco_parse_full_froms"
    "cco_parse_full_with_options"
    "cco_parse_options_default"
    "cco_parse_options_init"
    "cco_parse_result_free"
    "cco_parse_result_root"
    "cco_parse_result_symbols"
    "cco_parse_string"
    "cco_sandbox_check_path"
    "cco_save_file_pretty"
    "cco_serialize"
    "cco_serialize_full"
    "cco_serialize_into"
    "cco_serialize_pretty"
    "cco_serialize_to_string"
    "cco_string_get"
    "cco_string_new"
    "cco_string_new_len"
    "cco_symbol_table_count"
    "cco_symbol_table_name"
    "cco_symbol_table_template"
    "cco_template_field_count"
    "cco_template_field_name"
    "cco_template_get_parent_name"
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

echo "OK: Verified exactly 88 public symbols and 0 internal leaks in $TARGET"
