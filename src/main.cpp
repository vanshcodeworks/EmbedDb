#include "database.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <chrono>
#include <iomanip>
#include <fstream>
#include <sstream>
#include "../implement/tensor_store.h"
using namespace std;

// getting current time in cpp
//https://stackoverflow.com/questions/36751133/proper-method-of-using-stdchrono (yha se liya chrono)

double getCurrentTime() {
    auto now = chrono::high_resolution_clock::now();
    auto ns = chrono::duration_cast<chrono::nanoseconds>(now.time_since_epoch());
    return ns.count() / 1e9; // ns to s conversion
}


void runBenchmark(int numOps, int valueSize, bool shuffleKeys, int memtableMax) {
    Database db(memtableMax);
    
    vector<long long> keys(numOps);
    for (int i = 0; i < numOps; i++) {
        keys[i] = i + 1;
    }
    


    
    if (shuffleKeys) {
        //Provides random number facilities like  (Mersenne Twister RNG).
        mt19937 rng(12345);
        // using random inport yha shuffle kr denge jisse easy na ho put krna like 1 , 2 , 3,4 ki jagah 200 , 2 , 500
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

static bool parseNumber(const string& text, int& out) {
    try {
        out = stoi(text);
        return true;
    } catch (...) {
        cerr << "Invalid numeric argument: " << text << endl;
        return false;
    }
}

static string trim(const string& s) {
    const char* ws = " \t\r\n";
    size_t b = s.find_first_not_of(ws);
    size_t e = s.find_last_not_of(ws);
    if (b == string::npos) return "";
    return s.substr(b, e - b + 1);
}

void runTensorDryRunDemo() {
    cout << "Tensor dry run demo\n";
    TensorStore store;
    auto replay = store.dryRunScenario();
    for (const auto& item : replay) {
        cout << "  key " << item.key << ": "
             << TensorStore::summarize(item.info, item.values) << endl;
    }
}

void runTensorTrainingDemo() {
    cout << "Tiny DL training demo\n";
    TensorStore store;
    TensorInfo info{"dense_weights", 1, 4};
    vector<float> weights{0.1f, -0.2f, 0.05f, 0.3f};
    vector<float> grad{0.01f, -0.03f, 0.02f, -0.04f};
    float lr = 0.5f;
    for (int epoch = 0; epoch < 3; ++epoch) {
        for (size_t i = 0; i < weights.size(); ++i) {
            weights[i] -= lr * grad[i];
        }
        store.save(1000 + epoch, info, weights);
        cout << "  epoch " << epoch << ": "
             << TensorStore::summarize(info, weights) << endl;
    }
    store.flush();
    cout << "Weights persisted into SST files.\n";
}

void runTensorIngestDemo(const string& rawPath) {
    ifstream file(rawPath);
    if (!file) {
        cerr << "Cannot open tensor file: " << rawPath << "\n"
             << "Hint: run implement/python_demo/dl_model_demo.py or pass an absolute path.\n";
        return;
    }

    string header, values;
    if (!getline(file, header) || !getline(file, values)) {
        cerr << "Tensor file must contain two lines (header + values)." << endl;
        return;
    }

    stringstream hs(header);
    vector<string> parts;
    string part;
    while (getline(hs, part, ',')) parts.push_back(trim(part));
    if (parts.size() < 4) { cerr << "Header needs key,name,rows,cols." << endl; return; }

    long long key = stoll(parts[0]);
    TensorInfo info{parts[1], stoi(parts[2]), stoi(parts[3])};

    vector<float> floats;
    stringstream vs(values);
    while (getline(vs, part, ',')) {
        part = trim(part);
        if (!part.empty()) floats.push_back(stof(part));
    }

    TensorStore store;
    store.save(key, info, floats);
    store.flush();
    cout << "Ingested tensor key " << key << ": "
         << TensorStore::summarize(info, floats) << endl;
}

int main(int argc, char* argv[]) {
    int numOps = 50000;
    int valueSize = 32;
    bool shuffleKeys = true;
    int memtableMax = 10000;
    
    bool tensorDryRun = false;
    bool tensorTrain = false;
    string tensorIngestFile;
    vector<string> numericArgs;

    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "--run-dryrun" || arg == "--tensor-dryrun" ||
            arg == "-dry" || arg == "-dryrun") { tensorDryRun = true; continue; }
        if (arg == "--tensor-train") { tensorTrain = true; continue; }
        if (arg == "--tensor-ingest" && i + 1 < argc) {
            tensorIngestFile = argv[++i];
            continue;
        }
        numericArgs.push_back(arg);
    }

    if (tensorDryRun || tensorTrain || !tensorIngestFile.empty()) {
        if (tensorDryRun) runTensorDryRunDemo();
        if (tensorTrain) runTensorTrainingDemo();
        if (!tensorIngestFile.empty()) runTensorIngestDemo(tensorIngestFile);
        return 0;
    }

    size_t idx = 0;
    auto nextArg = [&](int& target) {
        if (idx < numericArgs.size()) {
            if (!parseNumber(numericArgs[idx], target)) exit(1);
            ++idx;
        }
    };

    nextArg(numOps);
    nextArg(valueSize);
    // ...existing parsing for shuffle/memtable using numericArgs + bounds...

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
