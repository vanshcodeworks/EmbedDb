#include "tensor_store.h"
#include <cstring>
#include <algorithm>
#include <sstream>
using namespace std;

/*
Fast tensor use cases:
1) ML feature caching between CPU ↔ GPU stages.
2) Storing rolling sensor snapshots from IoT pipelines.
3) Persisting intermediate inference outputs for audit or replay.
This helper keeps everything simple while riding on the existing Database engine.
*/

TensorStore::TensorStore(int memtableMax) : db(memtableMax) {}

void TensorStore::save(long long key, const TensorInfo& info, const vector<float>& data) {
    string payload = serialize(info, data);
    db.put(key, payload);
}

bool TensorStore::load(long long key, TensorInfo& info, vector<float>& data) {
    string payload;
    if (!db.get(key, payload)) return false;
    deserialize(payload, info, data);
    return true;
}

void TensorStore::flush() { db.flush(); }

void TensorStore::saveBatch(const vector<TensorPayload>& batch) {
    for (const auto& item : batch) save(item.key, item.info, item.values);
}

bool TensorStore::loadPreview(long long key, TensorInfo& info, vector<float>& data,
                     vector<float>& preview, size_t previewLimit) {
        if (!load(key, info, data)) return false;
        size_t take = min(previewLimit, data.size());
        preview.assign(data.begin(), data.begin() + take);
        return true;
    }

string TensorStore::summarize(const TensorInfo& info, const vector<float>& data,
                            size_t previewLimit) {
        ostringstream oss;
        oss << info.name << " (" << info.rows << "x" << info.cols << ") -> ";
        size_t take = min(previewLimit, data.size());
        for (size_t i = 0; i < take; ++i) {
            if (i) oss << ", ";
            oss << data[i];
        }
        if (data.size() > take) oss << ", ...";
        return oss.str();
    }

vector<TensorPayload> TensorStore::dryRunScenario() {
        vector<TensorPayload> script = {
            {101, {"camera_frame", 32, 32}, vector<float>(1024, 0.25f)},
            {202, {"latent_block", 1, 256}, vector<float>(256, 0.9f)}
        };
        saveBatch(script);
        flush();
        vector<TensorPayload> replay;
        for (auto& item : script) {
            TensorInfo info;
            vector<float> data;
            if (load(item.key, info, data)) {
                replay.push_back({item.key, info, data});
            }
        }
        return replay;
    }

string TensorStore::serialize(const TensorInfo& info, const vector<float>& data) {
    int nameLen = (int)info.name.size();
    int count = (int)data.size();
    size_t bytes = sizeof(int) * 4 + nameLen + sizeof(float) * count;
    string out(bytes, '\0');
    char* ptr = out.empty() ? nullptr : &out[0];
    if (!ptr) return out;

    memcpy(ptr, &nameLen, sizeof(int));        ptr += sizeof(int);
    memcpy(ptr, info.name.data(), nameLen);    ptr += nameLen;
    memcpy(ptr, &info.rows, sizeof(int));      ptr += sizeof(int);
    memcpy(ptr, &info.cols, sizeof(int));      ptr += sizeof(int);
    memcpy(ptr, &count, sizeof(int));          ptr += sizeof(int);
    if (count > 0) memcpy(ptr, data.data(), sizeof(float) * count);
    return out;
}

void TensorStore::deserialize(const string& blob, TensorInfo& info, vector<float>& data) {
        const char* ptr = blob.data();
        int nameLen;
        memcpy(&nameLen, ptr, sizeof(int));        ptr += sizeof(int);

        info.name.assign(ptr, nameLen);            ptr += nameLen;
        memcpy(&info.rows, ptr, sizeof(int));      ptr += sizeof(int);
        memcpy(&info.cols, ptr, sizeof(int));      ptr += sizeof(int);

        int count;
        memcpy(&count, ptr, sizeof(int));          ptr += sizeof(int);
        data.resize(count);
        if (count > 0) memcpy(data.data(), ptr, sizeof(float) * count);
    }
/*
Example (pseudo-use):
int main() {
    TensorStore store;
    TensorInfo meta{"layer1_weights", 64, 128};
    vector<float> tensor(meta.rows * meta.cols, 0.5f);
    store.save(42, meta, tensor);

    TensorInfo loadedInfo;
    vector<float> loadedData;
    if (store.load(42, loadedInfo, loadedData)) {
        // ready for fast tensor access
    }
}
    
*/
