#ifndef WAL_H
#define WAL_H

#include <string>
#include <fstream>
using namespace std;

class Wal {
public:
    explicit Wal(const string& name);
    ~Wal();
    void write(long long key, const string& value);
    void reset();
private:
    string fileName;
    ofstream out;
};

#endif
