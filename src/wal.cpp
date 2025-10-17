#include "wal.h"
#include <iostream>

Wal::Wal(const std::string& name) : fileName(name) {
    out.open(fileName, ios::binary | ios::app);
}

Wal::~Wal() {
    if (out.is_open()) out.close();
}

void Wal::write(long long key, const std::string& value) {
    int len = (int)value.size();
    out.write((const char*)&key, sizeof(key));
    out.write((const char*)&len, sizeof(len));
    if (len > 0) out.write(value.data(), len);
    out.flush();
}

void Wal::reset() {
    if (out.is_open()) out.close();
    out.open(fileName, ios::binary | ios::trunc);
}
