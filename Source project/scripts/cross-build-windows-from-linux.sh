#!/bin/bash
# How the bundled Windows binary was produced: clang-cl + lld-link + xwin (MSVC CRT/SDK).
# Requires: clang, lld, llvm, cmake, ninja, and `xwin --accept-license splat --output $XWIN_DIR`.
set -e
cd "$(dirname "$0")/.."
: "${XWIN_DIR:?set XWIN_DIR to the xwin splat output}"
cmake -B build-win -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=cmake/clang-cl-msvc.cmake -DXWIN_DIR="$XWIN_DIR"
cmake --build build-win --target VelvetSpace_VST3
