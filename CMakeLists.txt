cmake_minimum_required(VERSION 3.12)
project(EmbedDbBenchmark LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Favor optimizations for a fair throughput demo
if (MSVC)
  add_compile_options(/O2 /DNOMINMAX)
else()
  add_compile_options(-O3 -march=native)
endif()

add_executable(embeddb
  src/main.cpp
)

# Not used. Build without CMake. See README.txt for simple compile/run instructions.
