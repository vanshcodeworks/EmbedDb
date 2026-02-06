# EmbedDb

A high-performance LSM-Tree based key-value storage engine built in C++17.

## Live Demo

Check out the interactive demo at: **[embeddb.vanshcodeworks.com](https://embeddb.vanshcodeworks.com)**

## Performance 

- **128K ops/sec** - Sequential write throughput
- **7.8μs latency** - Per-operation latency
- **3MB/s bandwidth** - Write bandwidth with 32-byte values

## Architecture

EmbedDb implements the LSM-Tree (Log-Structured Merge-Tree) architecture with:

- **Memtable**: In-memory sorted map for fast writes
- **Write-Ahead Log (WAL)**: Crash recovery and durability
- **SSTables**: Immutable on-disk sorted files

## Building

```bash
g++ -std=c++17 -O2 src\main.cpp src\database.cpp src\memtable.cpp src\wal.cpp src\sstable.cpp -o embeddb.exe
```

## Running Benchmarks

```bash
# Format: ./embeddb [operations] [value_size] [shuffle] [memtable_max]
./embeddb.exe 1000000 32 1 100000
```

**Parameters:**
- `operations`: Number of write operations
- `value_size`: Size of each value in bytes
- `shuffle`: Randomize keys (0=sequential, 1=random)
- `memtable_max`: Max memtable entries before flush

## Learn More

Visit the [demo site](https://embeddb.vanshcodeworks.com) for:
- Detailed architecture diagrams
- Benchmark results
- Implementation deep-dive blog post

## 📄 License

MIT License - see LICENSE file for details

## 👨‍💻 Author

Built by [VanshCodeWorks](https://vanshcodeworks.com)
