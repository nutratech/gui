#!/bin/bash
# Build AppImage from project root
# Assumes linuxdeploy and linuxdeploy-plugin-qt are in PATH
set -e

cd "$(dirname "$0")/.." || exit 1

cmake -B build -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
cmake --build build --target appimage
