#ifndef MEMTABLE_H
#define MEMTABLE_H

#include <map>
#include <string>
using namespace std;

class MemTable {
public:
    void put(long long key, const string& value);
    bool get(long long key, string& value) const;
    int size() const;
    void clear();
    const map<long long, string>& items() const;
private:
    map<long long, string> data;
};

#endif