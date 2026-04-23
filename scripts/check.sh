#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build-tests"

cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -DMEDIAFLOW_BUILD_TESTS=ON
cmake --build "${BUILD_DIR}" -j4
ctest --test-dir "${BUILD_DIR}" --output-on-failure

cd "${ROOT_DIR}/ui"
npm install
npm run build
