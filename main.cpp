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

const string DB_FILE = "data/database.db";

// Cache only the most recently accessed index
string cachedIndex;
set<int> cachedValues;
bool cacheValid = false;

void ensureDataDir() {
    if (!fs::exists("data")) {
        fs::create_directory("data");
    }
}

// Read specific index from file
bool readIndexFromFile(const string& index, set<int>& values) {
    if (!fs::exists(DB_FILE)) {
        return false;
    }

    ifstream infile(DB_FILE, ios::binary);
    if (!infile.is_open()) return false;

    int numIndices;
    infile.read(reinterpret_cast<char*>(&numIndices), sizeof(numIndices));

    for (int i = 0; i < numIndices; i++) {
        int indexLen;
        infile.read(reinterpret_cast<char*>(&indexLen), sizeof(indexLen));

        string idx(indexLen, '\0');
        infile.read(&idx[0], indexLen);

        int numValues;
        infile.read(reinterpret_cast<char*>(&numValues), sizeof(numValues));

        if (idx == index) {
            // Found the index
            for (int j = 0; j < numValues; j++) {
                int val;
                infile.read(reinterpret_cast<char*>(&val), sizeof(val));
                values.insert(val);
            }
            infile.close();
            return true;
        } else {
            // Skip this index's values
            infile.seekg(numValues * sizeof(int), ios::cur);
        }
    }

    infile.close();
    return false;
}

// Write entire file with updated values for specific index
void updateIndexInFile(const string& index, const set<int>& values, bool deleteIndex = false) {
    // First, read all indices except the one being updated
    map<string, set<int>> otherIndices;

    if (fs::exists(DB_FILE)) {
        ifstream infile(DB_FILE, ios::binary);
        if (infile.is_open()) {
            int numIndices;
            infile.read(reinterpret_cast<char*>(&numIndices), sizeof(numIndices));

            for (int i = 0; i < numIndices; i++) {
                int indexLen;
                infile.read(reinterpret_cast<char*>(&indexLen), sizeof(indexLen));

                string idx(indexLen, '\0');
                infile.read(&idx[0], indexLen);

                int numValues;
                infile.read(reinterpret_cast<char*>(&numValues), sizeof(numValues));

                if (idx != index) {
                    set<int> vals;
                    for (int j = 0; j < numValues; j++) {
                        int val;
                        infile.read(reinterpret_cast<char*>(&val), sizeof(val));
                        vals.insert(val);
                    }
                    otherIndices[idx] = vals;
                } else {
                    // Skip the index being updated
                    infile.seekg(numValues * sizeof(int), ios::cur);
                }
            }
            infile.close();
        }
    }

    // Write back all indices
    ofstream outfile(DB_FILE, ios::binary);
    if (!outfile.is_open()) return;

    int totalIndices = otherIndices.size() + (deleteIndex || values.empty() ? 0 : 1);
    outfile.write(reinterpret_cast<const char*>(&totalIndices), sizeof(totalIndices));

    for (const auto& [idx, vals] : otherIndices) {
        int indexLen = idx.length();
        outfile.write(reinterpret_cast<const char*>(&indexLen), sizeof(indexLen));
        outfile.write(idx.c_str(), indexLen);

        int numValues = vals.size();
        outfile.write(reinterpret_cast<const char*>(&numValues), sizeof(numValues));

        for (int val : vals) {
            outfile.write(reinterpret_cast<const char*>(&val), sizeof(val));
        }
    }

    // Add the updated index if not deleting
    if (!deleteIndex && !values.empty()) {
        int indexLen = index.length();
        outfile.write(reinterpret_cast<const char*>(&indexLen), sizeof(indexLen));
        outfile.write(index.c_str(), indexLen);

        int numValues = values.size();
        outfile.write(reinterpret_cast<const char*>(&numValues), sizeof(numValues));

        for (int val : values) {
            outfile.write(reinterpret_cast<const char*>(&val), sizeof(val));
        }
    }

    outfile.close();
}

void insert(const string& index, int value) {
    ensureDataDir();

    set<int> values;

    // Use cache if available
    if (cacheValid && cachedIndex == index) {
        values = cachedValues;
    } else {
        readIndexFromFile(index, values);
    }

    values.insert(value);

    // Update cache
    cachedIndex = index;
    cachedValues = values;
    cacheValid = true;

    updateIndexInFile(index, values);
}

void deleteEntry(const string& index, int value) {
    ensureDataDir();

    set<int> values;

    // Use cache if available
    if (cacheValid && cachedIndex == index) {
        values = cachedValues;
    } else {
        if (!readIndexFromFile(index, values)) {
            return; // Index doesn't exist
        }
    }

    values.erase(value);

    // Update cache
    cachedIndex = index;
    cachedValues = values;
    cacheValid = true;

    updateIndexInFile(index, values, values.empty());
}

void find(const string& index) {
    ensureDataDir();

    set<int> values;

    // Use cache if available
    if (cacheValid && cachedIndex == index) {
        values = cachedValues;
    } else {
        if (!readIndexFromFile(index, values)) {
            cout << "null" << endl;
            return;
        }
        // Update cache
        cachedIndex = index;
        cachedValues = values;
        cacheValid = true;
    }

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