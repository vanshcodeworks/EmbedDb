#include "sstable.h"
#include <fstream>
using namespace std;

bool Sstable::writeFile(const string& name, const MemTable& mem) {
    ofstream f(name, ios::binary);
    if (!f.is_open()) return false;
    long long count = mem.size();
    f.write((const char*)&count, sizeof(count));
    for (auto& p : mem.items()) {
        long long k = p.first;
        const string& v = p.second;
        int len = (int)v.size();
        f.write((const char*)&k, sizeof(k));
        f.write((const char*)&len, sizeof(len));
        if (len > 0) f.write(v.data(), len);
    }
    return true;
}

bool Sstable::readValue(const string& name, long long key, string& value) {
    ifstream f(name, ios::binary);
    if (!f.is_open()) return false;
    long long count = 0;
    if (!f.read((char*)&count, sizeof(count))) return false;
    for (long long i = 0; i < count; i++) {
        long long k; int len;
        if (!f.read((char*)&k, sizeof(k))) return false;
        if (!f.read((char*)&len, sizeof(len))) return false;
        if (k == key) {
            value.resize(len);
            if (len > 0) f.read(&value[0], len);
            return true;
        } else {
            if (len > 0) f.seekg(len, ios::cur);
        }
    }
    return false;
}
