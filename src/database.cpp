#include "database.h"
#include <iostream>
#include <sstream>
#include <iomanip>
using namespace std;

Database::Database(int maxMem) : wal("wal.log"), maxMem(maxMem), sstSeq(0) {}

void Database::put(long long key, const string& value) {
    wal.write(key, value);
    mem.put(key, value);
    if (mem.size() >= maxMem) flushMem();
}

bool Database::get(long long key, string& value) const {
    if (mem.get(key, value)) return true;
    for (int i = (int)sstFiles.size() - 1; i >= 0; i--) {
        if (Sstable::readValue(sstFiles[(size_t)i], key, value)) return true;
    }
    return false;
}

void Database::flush() {
    if (mem.size() > 0) flushMem();
}

int Database::memSize() const { return mem.size(); }
int Database::sstableCount() const { return (int)sstFiles.size(); }

void Database::flushMem() {
    string name = nextSstable();
    if (Sstable::writeFile(name, mem)) {
        sstFiles.push_back(name);
        cout << "flushed " << mem.size() << " to " << name << endl;
        mem.clear();
        wal.reset();
    }
}

string Database::nextSstable() {
    ostringstream os;
    os << "sst_" << setw(6) << setfill('0') << sstSeq++ << ".sst";
    return os.str();
}
