EmbedDb minimal insert benchmark (no CMake)

Requirements:
- A C++17 compiler (g++/clang++ on Linux/macOS, MSVC on Windows)

Source:
- src/main.cpp

Quick start (Linux/macOS):
1) mkdir -p build
2) g++ -O3 -march=native -std=c++17 -DNDEBUG -o build/embeddb src/main.cpp
3) ./build/embeddb 5000000 32 1 100000

Quick start (Windows - MSVC Developer Command Prompt):
1) if not exist build mkdir build
2) cl /O2 /std:c++17 /DNDEBUG /EHsc /Fe:build\embeddb.exe src\main.cpp
3) build\embeddb.exe 5000000 32 1 100000

New argument:
- memtable_max: flush MemTable to an SSTable when it reaches this many entries (default 100000).

What files are created:
- wal.log: write-ahead log of recent writes (replayed on startup for recovery).
- sst_*.sst: immutable sorted tables written on flush (newest have higher numbers).

Reset the demo:
- Delete wal.log and sst_*.sst to start fresh.

Notes:
- Args: ops value_size shuffle(1/0) memtable_max
- Shuffle randomizes key insert order to emulate tree rebalancing cost.
- Use -march=native only if compiling for the current machine.
