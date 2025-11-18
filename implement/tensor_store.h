#ifndef TENSOR_STORE_H
#define TENSOR_STORE_H

#include "../src/database.h"
#include <vector>
#include <string>

struct TensorInfo {
    std::string name;
    int rows;
    int cols;
};

struct TensorPayload {
    long long key;
    TensorInfo info;
    std::vector<float> values;
};

class TensorStore {
public:
    explicit TensorStore(int memtableMax = 50000);

    void save(long long key, const TensorInfo& info, const std::vector<float>& data);
    bool load(long long key, TensorInfo& info, std::vector<float>& data);
    void flush();

    void saveBatch(const std::vector<TensorPayload>& batch);
    bool loadPreview(long long key, TensorInfo& info, std::vector<float>& data,
                     std::vector<float>& preview, size_t previewLimit = 8);

    static std::string summarize(const TensorInfo& info, const std::vector<float>& data,
                                 size_t previewLimit = 8);

    std::vector<TensorPayload> dryRunScenario();

private:
    Database db;

    static std::string serialize(const TensorInfo& info, const std::vector<float>& data);
    static void deserialize(const std::string& blob, TensorInfo& info, std::vector<float>& data);
};

#endif
