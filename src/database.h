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
