#include "wal.h"
#include <iostream>

Wal::Wal(const std::string& name) : fileName(name) {
    // Open the log in append + binary mode so we never overwrite existing data accidentally.
    out.open(fileName, ios::binary | ios::app);
}

Wal::~Wal() {
    // Close the file if it is open this is deconstructor used in this.
    if (out.is_open()) out.close();
}

void Wal::write(long long key, const std::string& value) {
    // Compute the number of bytes to write for the value.
    // because if lost we can find next value ewith this

    int len = (int)value.size();

    // key as raw bytes (fixed-size long long).
    out.write((const char*)&key, sizeof(key));

    // value length (fixed-size int).
    out.write((const char*)&len, sizeof(len));

    // value bytes 
    if (len > 0) {
        out.write(value.data(), len);
    }

    out.flush();
}

void Wal::reset() {
    // Close current handle to release the append mode.
    if (out.is_open()) out.close();

    // Reopen in truncate mode to clear the file (set size to 0).
    out.open(fileName, ios::binary | ios::trunc);
}
