#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <string>
#include <sstream>
#include <filesystem>
#include <map>
#include <set>

using namespace std;
namespace fs = std::filesystem;

const string DATA_FILE = "data/database.dat";
const string INDEX_FILE = "data/index.dat";

void ensureDataDir() {
    if (!fs::exists("data")) {
        fs::create_directory("data");
    }
}

struct IndexEntry {
    string index;
    streampos dataPos;  // Position in data file
    int dataSize;       // Size of data in bytes
};

void writeDataFile(streampos pos, const set<int>& values) {
    // Read existing data file if it exists
    vector<char> buffer;
    streampos fileSize = 0;

    if (fs::exists(DATA_FILE)) {
        ifstream infile(DATA_FILE, ios::binary);
        if (infile.is_open()) {
            infile.seekg(0, ios::end);
            fileSize = infile.tellg();
            infile.seekg(0, ios::beg);

            buffer.resize(fileSize);
            infile.read(buffer.data(), fileSize);
            infile.close();
        }
    }

    // Prepare new data
    vector<char> newData;
    int count = values.size();
    newData.reserve(sizeof(count) + count * sizeof(int));

    newData.insert(newData.end(), reinterpret_cast<const char*>(&count),
                   reinterpret_cast<const char*>(&count) + sizeof(count));

    for (int val : values) {
        newData.insert(newData.end(), reinterpret_cast<const char*>(&val),
                       reinterpret_cast<const char*>(&val) + sizeof(val));
    }

    // Write back
    ofstream outfile(DATA_FILE, ios::binary);
    if (outfile.is_open()) {
        // Write data before position
        if (pos > 0 && pos <= fileSize) {
            outfile.write(buffer.data(), pos);
        }

        // Write new data
        outfile.write(newData.data(), newData.size());

        // Write remaining data after position
        if (pos < fileSize) {
            outfile.write(buffer.data() + pos, fileSize - pos);
        }

        outfile.close();
    }
}

set<int> readDataFile(streampos pos, int size) {
    set<int> values;

    if (!fs::exists(DATA_FILE) || size <= 0) {
        return values;
    }

    ifstream infile(DATA_FILE, ios::binary);
    if (!infile.is_open()) return values;

    infile.seekg(pos);

    int count;
    infile.read(reinterpret_cast<char*>(&count), sizeof(count));

    for (int i = 0; i < count; i++) {
        int val;
        infile.read(reinterpret_cast<char*>(&val), sizeof(val));
        values.insert(val);
    }

    infile.close();
    return values;
}

vector<IndexEntry> readIndex() {
    vector<IndexEntry> index;

    if (!fs::exists(INDEX_FILE)) {
        return index;
    }

    ifstream infile(INDEX_FILE, ios::binary);
    if (!infile.is_open()) return index;

    int count;
    infile.read(reinterpret_cast<char*>(&count), sizeof(count));
    index.reserve(count);

    for (int i = 0; i < count; i++) {
        IndexEntry entry;

        int indexLen;
        infile.read(reinterpret_cast<char*>(&indexLen), sizeof(indexLen));
        entry.index.resize(indexLen);
        infile.read(&entry.index[0], indexLen);

        infile.read(reinterpret_cast<char*>(&entry.dataPos), sizeof(entry.dataPos));
        infile.read(reinterpret_cast<char*>(&entry.dataSize), sizeof(entry.dataSize));

        index.push_back(entry);
    }

    infile.close();
    return index;
}

void writeIndex(const vector<IndexEntry>& index) {
    ofstream outfile(INDEX_FILE, ios::binary);
    if (!outfile.is_open()) return;

    int count = index.size();
    outfile.write(reinterpret_cast<const char*>(&count), sizeof(count));

    for (const auto& entry : index) {
        int indexLen = entry.index.length();
        outfile.write(reinterpret_cast<const char*>(&indexLen), sizeof(indexLen));
        outfile.write(entry.index.c_str(), indexLen);

        outfile.write(reinterpret_cast<const char*>(&entry.dataPos), sizeof(entry.dataPos));
        outfile.write(reinterpret_cast<const char*>(&entry.dataSize), sizeof(entry.dataSize));
    }

    outfile.close();
}

void insert(const string& index, int value) {
    ensureDataDir();

    vector<IndexEntry> indexData = readIndex();

    // Find existing entry
    auto it = find_if(indexData.begin(), indexData.end(),
                      [&index](const IndexEntry& e) { return e.index == index; });

    if (it != indexData.end()) {
        // Update existing entry
        set<int> values = readDataFile(it->dataPos, it->dataSize);
        values.insert(value);

        // Rewrite data at same position
        writeDataFile(it->dataPos, values);
    } else {
        // Add new entry at end
        set<int> newValues;
        newValues.insert(value);

        // Get current file size as position
        streampos pos = 0;
        if (fs::exists(DATA_FILE)) {
            ifstream infile(DATA_FILE, ios::binary | ios::ate);
            pos = infile.tellg();
            infile.close();
        }

        writeDataFile(pos, newValues);

        IndexEntry newEntry;
        newEntry.index = index;
        newEntry.dataPos = pos;
        newEntry.dataSize = sizeof(int) + newValues.size() * sizeof(int);

        indexData.push_back(newEntry);
        writeIndex(indexData);
    }
}

void deleteEntry(const string& index, int value) {
    ensureDataDir();

    vector<IndexEntry> indexData = readIndex();

    auto it = find_if(indexData.begin(), indexData.end(),
                      [&index](const IndexEntry& e) { return e.index == index; });

    if (it != indexData.end()) {
        set<int> values = readDataFile(it->dataPos, it->dataSize);
        values.erase(value);

        if (values.empty()) {
            // Remove entry from index
            indexData.erase(it);
            writeIndex(indexData);
        } else {
            // Update data
            writeDataFile(it->dataPos, values);
        }
    }
}

void find(const string& index) {
    ensureDataDir();

    vector<IndexEntry> indexData = readIndex();

    auto it = find_if(indexData.begin(), indexData.end(),
                      [&index](const IndexEntry& e) { return e.index == index; });

    if (it == indexData.end()) {
        cout << "null" << endl;
        return;
    }

    set<int> values = readDataFile(it->dataPos, it->dataSize);

    if (values.empty()) {
        cout << "null" << endl;
    } else {
        bool first = true;
        for (int val : values) {
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