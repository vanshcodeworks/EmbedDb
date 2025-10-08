#!/usr/bin/env bash
set -euo pipefail

mkdir -p build

CXX="${CXX:-}"
if [[ -z "${CXX}" ]]; then
  if command -v g++ >/dev/null 2>&1; then CXX=g++;
  elif command -v clang++ >/dev/null 2>&1; then CXX=clang++;
  else echo "No C++ compiler found (g++ or clang++)."; exit 1; fi
fi

echo "Using compiler: ${CXX}"
${CXX} -O3 -march=native -std=c++17 -DNDEBUG -o build/embeddb src/main.cpp

echo "Running: ./build/embeddb ${*:-}"
./build/embeddb "$@"
