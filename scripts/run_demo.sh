#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
SCENARIO="${1:-adaptive-buffer-comparison}"
SEED="${2:-42}"
EXPORT_PATH="${ROOT_DIR}/examples/${SCENARIO}-demo-result.json"

cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -DMEDIAFLOW_BUILD_TESTS=ON
cmake --build "${BUILD_DIR}" -j4
"${BUILD_DIR}/mediaflow" --scenario "${SCENARIO}" --seed "${SEED}" --export "${EXPORT_PATH}"

echo
echo "Demo export written to ${EXPORT_PATH}"
