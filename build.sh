#!/bin/bash
set -e

mkdir -p build
cd build
cmake .. -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build .
ln -sf build/compile_commands.json ../compile_commands.json
