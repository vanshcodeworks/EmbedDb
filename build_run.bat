@echo off
setlocal ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION

REM Build and run the benchmark using MSVC (cl) if available, otherwise try g++
if not exist build mkdir build >nul 2>&1

where cl >nul 2>&1
if %errorlevel%==0 (
  echo Detected MSVC cl. Building...
  cl /nologo /O2 /std:c++17 /DNDEBUG /EHsc /Fe:build\embeddb.exe src\main.cpp || goto :eof
  echo Running: build\embeddb.exe %*
  build\embeddb.exe %*
  goto :eof
)

where g++ >nul 2>&1
if %errorlevel%==0 (
  echo Detected g++. Building...
  g++ -O3 -march=native -std=c++17 -DNDEBUG -o build\embeddb.exe src\main.cpp || goto :eof
  echo Running: build\embeddb.exe %*
  build\embeddb.exe %*
  goto :eof
)

echo No compatible compiler found. Please install MSVC (cl) or MinGW g++.
