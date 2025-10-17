#include "memtable.h"

void MemTable::put(long long key, const string& value) {
     data[key] = value; 
    }

bool MemTable::get(long long key, string& value) const {
    auto it = data.find(key);
    if (it == data.end()) return false;
    value = it->second;
    return true;
}

int MemTable::size() const {
    return (int)data.size(); 
}

void MemTable::clear() { 
    data.clear(); 
}

const map<long long, string>& MemTable::items() const { 
    return data; 
}
