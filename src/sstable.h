#ifndef SSTABLE_H
#define SSTABLE_H

#include <string>
#include "memtable.h"
using namespace std;

class Sstable {
public:
    static bool writeFile(const string& name, const MemTable& mem);
    static bool readValue(const string& name, long long key, string& value);
};

#endif
