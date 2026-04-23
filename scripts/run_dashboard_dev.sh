#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
PORT="${MEDIAFLOW_PORT:-8080}"

cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -DMEDIAFLOW_BUILD_TESTS=ON
cmake --build "${BUILD_DIR}" -j4

"${BUILD_DIR}/mediaflow" --serve --port "${PORT}" &
BACKEND_PID=$!
trap 'kill "${BACKEND_PID}" >/dev/null 2>&1 || true' EXIT

cd "${ROOT_DIR}/ui"
npm install
npm run dev
