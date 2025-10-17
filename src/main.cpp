#include "database.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <chrono>
#include <iomanip>
using namespace std;

double getCurrentTime() {
    auto now = chrono::high_resolution_clock::now();
    auto ns = chrono::duration_cast<chrono::nanoseconds>(now.time_since_epoch());
    return ns.count() / 1e9;
}


void runBenchmark(int numOps, int valueSize, bool shuffleKeys, int memtableMax) {
    Database db(memtableMax);
    
    vector<long long> keys(numOps);
    for (int i = 0; i < numOps; i++) {
        keys[i] = i + 1;
    }
    
    if (shuffleKeys) {
        mt19937 rng(12345);
        shuffle(keys.begin(), keys.end(), rng);
    }
    
    string value(valueSize, 'x');
    
    double startTime = getCurrentTime();
    
    for (long long key : keys) {
        db.put(key, value);
    }
    db.flush();
    
    double endTime = getCurrentTime();
    double totalTime = endTime - startTime;
    double opsPerSecond = numOps / totalTime / 1000000.0;
    double nsPerOp = (totalTime * 1e9) / numOps;
    
    cout << "Results:" << endl;
    cout << "  SSTables: " << db.sstableCount() << endl;
    cout << "  Remaining in memory: " << db.memSize() << endl;
    cout << fixed << setprecision(3);
    cout << "  Time: " << totalTime << " seconds" << endl;
    cout << "  Throughput: " << opsPerSecond << " Mops/s" << endl;
    cout << "  Latency: " << nsPerOp << " ns/op" << endl;
}

int main(int argc, char* argv[]) {
    int numOps = 50000;
    int valueSize = 32;
    bool shuffleKeys = true;
    int memtableMax = 10000;
    
    if (argc > 1) numOps = stoi(argv[1]);
    if (argc > 2) valueSize = stoi(argv[2]);
    if (argc > 3) shuffleKeys = (string(argv[3]) != "0");
    if (argc > 4) memtableMax = stoi(argv[4]);
    
    cout << "EmbedDb Benchmark" << endl;
    cout << "Operations: " << numOps << endl;
    cout << "Value size: " << valueSize << " bytes" << endl;
    cout << "Shuffle keys: " << (shuffleKeys ? "yes" : "no") << endl;
    cout << "Memtable max: " << memtableMax << endl;
    cout << endl;
    
    runBenchmark(numOps, valueSize, shuffleKeys, memtableMax);
    
    cout << "benchmark done pls delete for run again";    
    return 0;
}
