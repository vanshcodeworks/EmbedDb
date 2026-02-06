#ifndef DATABASE_H
#define DATABASE_H

#include <vector>
#include <string>
#include "memtable.h"
#include "wal.h"
#include "sstable.h"
using namespace std;

class Database {
public:
    explicit Database(int maxMem = 100000);
    // New: replay-from-WAL constructor hook (for future extension)
    template<typename ReplayFn>
    Database(int maxMem, ReplayFn replayHook)
        : wal("wal.log"), maxMem(maxMem), sstSeq(0) {
        // For now just call the hook so demo code can print something.
        // In a fuller version this could read WAL and re-populate mem.
        replayHook();
    }
    void put(long long key, const string& value);
    bool get(long long key, string& value) const;
    void flush();
    int memSize() const;
    int sstableCount() const;
private:
    void flushMem();
    string nextSstable();
    MemTable mem;
    Wal wal;
    vector<string> sstFiles;
    int maxMem;
    int sstSeq;
};

#endif
