#include "sstable.h"
#include <fstream>
using namespace std;


bool Sstable::writeFile(const string& name, const MemTable& mem) {

    ofstream f(name, ios::binary);

    if (!f.is_open()) return false;

    // Write how many entries we will store (header).
    long long count = mem.size();
    
    f.write((const char*)&count, sizeof(count));// This tells the reader how many key–values are stored.

    for (auto& p : mem.items()) { //Write each key-value pair
        long long k = p.first;           // the key to write
        const string& v = p.second;      // the value to write
        int len = (int)v.size();         // number of bytes in value

        // Write key as fixed-size binary
        f.write((const char*)&k, sizeof(k));
        // Write value length so we can read exact number of bytes later
        f.write((const char*)&len, sizeof(len));
        // Write value bytes if there is any content
        if (len > 0) f.write(v.data(), len); //Write key, value length, and bytes
    }
    return true;
}

bool Sstable::readValue(const string& name, long long key, string& value) {
    // Open file for binary reading.
    ifstream f(name, ios::binary);
    if (!f.is_open()) return false;

    // Read the number of records stored in this SSTable.

    long long count = 0;
    if (!f.read((char*)&count, sizeof(count))) return false;

    // Scan all records one by one (simple linear search).
    for (long long i = 0; i < count; i++) {
        long long k;      // key in file
        int len;          // length of the value that follows

        // Read key and value length; fail if we can't read them fully.
        if (!f.read((char*)&k, sizeof(k))) return false;
        if (!f.read((char*)&len, sizeof(len))) return false;

        if (k == key) {
            // If key matches, read the value bytes into output string.
            value.resize(len);
            if (len > 0) f.read(&value[0], len);
            return true;  // found

        } else {
            // If key does not match, skip forward by len bytes to the next record.
            
            if (len > 0) f.seekg(len, ios::cur);
        }
    }
    // If we reach here, key was not found in this file.
    return false;
}
