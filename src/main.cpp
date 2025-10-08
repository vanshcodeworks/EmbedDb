#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <functional>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <numeric>
#include <random>
#include <sstream>
#include <string>
#include <vector>

// =========================
// In-memory MemTable
// =========================
// Very basic ordered MemTable using std::map (kept sorted for easy SSTable flush)
class MemTable {
public:
    using Key = uint64_t;
    using Value = std::string;

    void put(Key k, Value v) { table_[k] = std::move(v); }

    const Value* get(Key k) const {
        auto it = table_.find(k);
        if (it == table_.end()) return nullptr;
        return &it->second;
    }

    size_t size() const { return table_.size(); }

    const std::map<Key, Value>& data() const { return table_; }

    void clear() { table_.clear(); }
    
private:
    std::map<Key, Value> table_;
};

// =========================
// Write-Ahead Log (WAL)
// =========================
// Append-only binary log for durability.
// Record format: [u8 type=1 (Put)][u64 key][u32 value_len][value bytes]
class WAL {
public:
    explicit WAL(const std::string& path) : path_(path) {
        out_.open(path_, std::ios::binary | std::ios::app);
    }

    ~WAL() { if (out_.is_open()) out_.close(); }

    void appendPut(uint64_t key, const std::string& value) {
        uint8_t type = 1; // Put
        uint32_t len = static_cast<uint32_t>(value.size());
        out_.write(reinterpret_cast<const char*>(&type), sizeof(type));
        out_.write(reinterpret_cast<const char*>(&key), sizeof(key));
        out_.write(reinterpret_cast<const char*>(&len), sizeof(len));
        if (len) out_.write(value.data(), len);
        // Demo: rely on OS buffering; real systems control fsync timings.
    }

    // Rebuild MemTable from WAL (simple best-effort replay)
    template <typename PutFn>
    void replay(PutFn put) {
        std::ifstream in(path_, std::ios::binary);
        if (!in) return; // No WAL yet
        while (true) {
            uint8_t type = 0;
            uint64_t key = 0;
            uint32_t len = 0;
            if (!in.read(reinterpret_cast<char*>(&type), sizeof(type))) break;
            if (!in.read(reinterpret_cast<char*>(&key), sizeof(key))) break;
            if (!in.read(reinterpret_cast<char*>(&len), sizeof(len))) break;
            std::string value;
            value.resize(len);
            if (len > 0) {
                if (!in.read(&value[0], len)) break; // truncated
            }
            if (type == 1) {
                put(key, value);
            }
        }
    }

    // Truncate WAL after a successful MemTable flush
    void reset() {
        if (out_.is_open()) out_.close();
        std::ofstream trunc(path_, std::ios::binary | std::ios::trunc);
        trunc.close();
        out_.open(path_, std::ios::binary | std::ios::app);
    }

private:
    std::string path_;
    std::ofstream out_;
};

// =========================
// SSTable I/O (very simple)
// =========================
// Immutable sorted table on disk.
// File format: [u64 count][repeat count times: u64 key, u32 len, bytes]
class SSTableIO {
public:
    static bool write(const std::string& path, const MemTable& mt) {
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        if (!out) return false;
        uint64_t count = static_cast<uint64_t>(mt.size());
        out.write(reinterpret_cast<const char*>(&count), sizeof(count));
        for (const auto& kv : mt.data()) {
            uint64_t key = kv.first;
            const std::string& value = kv.second;
            uint32_t len = static_cast<uint32_t>(value.size());
            out.write(reinterpret_cast<const char*>(&key), sizeof(key));
            out.write(reinterpret_cast<const char*>(&len), sizeof(len));
            if (len) out.write(value.data(), len);
        }
        return true;
    }

    // Naive scan lookup (newest-to-oldest SSTables helps)
    static bool get(const std::string& path, uint64_t key, std::string& out_value) {
        std::ifstream in(path, std::ios::binary);
        if (!in) return false;
        uint64_t count = 0;
        if (!in.read(reinterpret_cast<char*>(&count), sizeof(count))) return false;
        for (uint64_t i = 0; i < count; ++i) {
            uint64_t k = 0; uint32_t len = 0;
            if (!in.read(reinterpret_cast<char*>(&k), sizeof(k))) return false;
            if (!in.read(reinterpret_cast<char*>(&len), sizeof(len))) return false;
            if (k == key) {
                out_value.resize(len);
                if (len) in.read(&out_value[0], len); else out_value.clear();
                return true;
            } else {
                if (len) in.seekg(len, std::ios::cur); // skip
            }
        }
        return false;
    }
};

// =========================
// Tiny DB wiring WAL + MemTable + SSTables
// =========================
class DB {
public:
    explicit DB(size_t memtable_max_entries)
        : wal_("wal.log"), memtable_max_(memtable_max_entries) {
        // Recover from WAL
        wal_.replay([this](uint64_t k, const std::string& v){ mem_.put(k, v); });
    }

    void put(uint64_t key, const std::string& value) {
        wal_.appendPut(key, value);
        mem_.put(key, value);
        if (mem_.size() >= memtable_max_) flushToSSTable();
    }

    bool get(uint64_t key, std::string& value) const {
        if (const auto* v = mem_.get(key)) { value = *v; return true; }
        for (int i = static_cast<int>(sstables_.size()) - 1; i >= 0; --i) {
            if (SSTableIO::get(sstables_[static_cast<size_t>(i)], key, value)) return true;
        }
        return false;
    }

    void flushToSSTable() {
        if (mem_.size() == 0) return;
        std::string path = nextSSTablePath();
        if (SSTableIO::write(path, mem_)) {
            sstables_.push_back(path);
            std::cout << "Flushed " << mem_.size() << " entries to " << path << "\n";
            mem_.clear();
            wal_.reset();
        } else {
            std::cerr << "Failed to write SSTable: " << path << "\n";
        }
    }

    size_t memtableSize() const { return mem_.size(); }
    size_t sstableCount() const { return sstables_.size(); }

private:
    std::string nextSSTablePath() {
        std::ostringstream oss;
        oss << "sst_" << std::setw(6) << std::setfill('0') << sst_seq_++ << ".sst";
        return oss.str();
    }

    WAL wal_;
    MemTable mem_;
    size_t memtable_max_ = 100000;
    std::vector<std::string> sstables_;
    uint32_t sst_seq_ = 0;
};

// =========================
// Benchmark helpers
// =========================
static inline uint64_t now_ns() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
               std::chrono::high_resolution_clock::now().time_since_epoch())
        .count();
}

struct BenchResult { double seconds{}; double mops{}; double ns_per_op{}; };

// End-to-end write path: WAL + MemTable + periodic flush to SSTable
BenchResult bench_inserts(size_t n, size_t value_size, bool shuffle_keys, size_t memtable_max) {
    DB db(memtable_max);

    std::vector<uint64_t> keys(n);
    std::iota(keys.begin(), keys.end(), 1ULL);
    if (shuffle_keys) { std::mt19937_64 rng(123456); std::shuffle(keys.begin(), keys.end(), rng); }

    std::string value(value_size, 'x');

    uint64_t t0 = now_ns();
    for (auto k : keys) db.put(k, value);
    db.flushToSSTable(); // ensure remaining in-memory data is persisted
    uint64_t t1 = now_ns();

    double secs = (t1 - t0) / 1e9;
    double nsop = (t1 - t0) / static_cast<double>(n);
    double mops = (n / 1e6) / secs;

    std::cout << "SSTables created: " << db.sstableCount()
              << ", MemTable entries remaining: " << db.memtableSize() << "\n";

    return BenchResult{secs, mops, nsop};
}

// =========================
// Program entry
// =========================
int main(int argc, char** argv) {
    // Args: [ops] [value_size] [shuffle] [memtable_max]
    size_t ops = 50000;      // default 5M inserts
    size_t value_size = 32;    // bytes per value
    bool shuffle = true;       // random key order stresses tree balancing
    size_t memtable_max = 10000; // flush when MemTable reaches this many entries

    if (argc > 1) ops = std::stoull(argv[1]);
    if (argc > 2) value_size = std::stoull(argv[2]);
    if (argc > 3) shuffle = (std::strcmp(argv[3], "0") != 0);
    if (argc > 4) memtable_max = std::stoull(argv[4]);

    std::cout << "EmbedDb minimal LSM insert demo (WAL + MemTable + SSTable)\n";
    std::cout << "ops=" << ops
              << ", value_size=" << value_size
              << ", shuffle_keys=" << (shuffle ? "true" : "false")
              << ", memtable_max=" << memtable_max
              << "\n";

    auto r = bench_inserts(ops, value_size, shuffle, memtable_max);

    std::cout << std::fixed << std::setprecision(3)
              << "Time: " << r.seconds << " s\n"
              << "Throughput: " << r.mops << " Mops/s\n"
              << "Latency: " << r.ns_per_op << " ns/op\n";

    std::cout << "Note: This demo writes wal.log and sst_*.sst in the current folder. Delete them to reset.\n";
    return 0;
}
