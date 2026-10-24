#!/usr/bin/env bash
# run_e2e.sh - Standalone runner script for libcco E2E test suite
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

cd "$PROJECT_ROOT"

echo "==> Building libcco debug target..."
make build.debug >/dev/null

echo "==> Building E2E test suite..."
make -C tests/e2e build

echo "==> Running E2E test suite..."
./target/tests/run_e2e "$@"
