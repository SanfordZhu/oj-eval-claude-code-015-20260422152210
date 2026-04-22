#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <string>
#include <sstream>
#include <filesystem>
#include <unordered_map>
#include <set>

using namespace std;
namespace fs = std::filesystem;

const string DATA_DIR = "data";
const string INDEX_FILE = DATA_DIR + "/index.dat";
const int BUCKET_SIZE = 1000; // Group indices into buckets

void ensureDataDir() {
    if (!fs::exists(DATA_DIR)) {
        fs::create_directory(DATA_DIR);
    }
}

int getBucketId(const string& index) {
    // Simple hash function to distribute indices
    int hash = 0;
    for (char c : index) {
        hash = (hash * 31 + c) % 1000000;
    }
    return hash / BUCKET_SIZE;
}

string getBucketFilename(int bucketId) {
    return DATA_DIR + "/bucket_" + to_string(bucketId) + ".dat";
}

struct IndexData {
    string index;
    set<int> values;
};

void writeBucket(int bucketId, const vector<IndexData>& bucketData) {
    string filename = getBucketFilename(bucketId);
    ofstream outfile(filename, ios::binary);
    if (!outfile.is_open()) return;

    int count = bucketData.size();
    outfile.write(reinterpret_cast<const char*>(&count), sizeof(count));

    for (const auto& data : bucketData) {
        int indexLen = data.index.length();
        outfile.write(reinterpret_cast<const char*>(&indexLen), sizeof(indexLen));
        outfile.write(data.index.c_str(), indexLen);

        int valueCount = data.values.size();
        outfile.write(reinterpret_cast<const char*>(&valueCount), sizeof(valueCount));
        for (int val : data.values) {
            outfile.write(reinterpret_cast<const char*>(&val), sizeof(val));
        }
    }
    outfile.close();
}

vector<IndexData> readBucket(int bucketId) {
    vector<IndexData> bucketData;
    string filename = getBucketFilename(bucketId);

    if (!fs::exists(filename)) {
        return bucketData;
    }

    ifstream infile(filename, ios::binary);
    if (!infile.is_open()) return bucketData;

    int count;
    infile.read(reinterpret_cast<char*>(&count), sizeof(count));
    bucketData.reserve(count);

    for (int i = 0; i < count; i++) {
        IndexData data;

        int indexLen;
        infile.read(reinterpret_cast<char*>(&indexLen), sizeof(indexLen));
        data.index.resize(indexLen);
        infile.read(&data.index[0], indexLen);

        int valueCount;
        infile.read(reinterpret_cast<char*>(&valueCount), sizeof(valueCount));
        for (int j = 0; j < valueCount; j++) {
            int val;
            infile.read(reinterpret_cast<char*>(&val), sizeof(val));
            data.values.insert(val);
        }

        bucketData.push_back(std::move(data));
    }
    infile.close();

    return bucketData;
}

void insert(const string& index, int value) {
    ensureDataDir();
    int bucketId = getBucketId(index);
    vector<IndexData> bucketData = readBucket(bucketId);

    // Find the index in bucket
    auto it = find_if(bucketData.begin(), bucketData.end(),
                      [&index](const IndexData& d) { return d.index == index; });

    if (it == bucketData.end()) {
        // New index
        IndexData newData;
        newData.index = index;
        newData.values.insert(value);
        bucketData.push_back(std::move(newData));
    } else {
        // Existing index
        it->values.insert(value);
    }

    writeBucket(bucketId, bucketData);
}

void deleteEntry(const string& index, int value) {
    ensureDataDir();
    int bucketId = getBucketId(index);
    vector<IndexData> bucketData = readBucket(bucketId);

    // Find the index in bucket
    auto it = find_if(bucketData.begin(), bucketData.end(),
                      [&index](const IndexData& d) { return d.index == index; });

    if (it != bucketData.end()) {
        // Remove value if exists
        it->values.erase(value);

        // If no values left, remove the index
        if (it->values.empty()) {
            bucketData.erase(it);
        }

        writeBucket(bucketId, bucketData);
    }
}

void find(const string& index) {
    ensureDataDir();
    int bucketId = getBucketId(index);
    vector<IndexData> bucketData = readBucket(bucketId);

    // Find the index in bucket
    auto it = find_if(bucketData.begin(), bucketData.end(),
                      [&index](const IndexData& d) { return d.index == index; });

    if (it == bucketData.end() || it->values.empty()) {
        cout << "null" << endl;
    } else {
        bool first = true;
        for (int val : it->values) {
            if (!first) cout << " ";
            cout << val;
            first = false;
        }
        cout << endl;
    }
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int n;
    cin >> n;
    cin.ignore();

    for (int i = 0; i < n; i++) {
        string line;
        getline(cin, line);
        istringstream iss(line);

        string command;
        iss >> command;

        if (command == "insert") {
            string index;
            int value;
            iss >> index >> value;
            insert(index, value);
        } else if (command == "delete") {
            string index;
            int value;
            iss >> index >> value;
            deleteEntry(index, value);
        } else if (command == "find") {
            string index;
            iss >> index;
            find(index);
        }
    }

    return 0;
}