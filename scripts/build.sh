#!/bin/bash
set -e

SRC_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$SRC_DIR/build}"
BUILD_TYPE="${BUILD_TYPE:-Release}"

if [ "${1,,}" = "debug" ]; then
    BUILD_TYPE="Debug"
    shift
fi

cmake -B "$BUILD_DIR" -S "$SRC_DIR" -DCMAKE_BUILD_TYPE="$BUILD_TYPE" "$@"
cmake --build "$BUILD_DIR" --parallel "$(nproc)"
