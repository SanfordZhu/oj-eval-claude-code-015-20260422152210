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

void ensureDataDir() {
    if (!fs::exists("data")) {
        fs::create_directory("data");
    }
}

// Simple file format:
// Header: number of indices (4 bytes)
// For each index:
//   - index length (4 bytes)
//   - index string
//   - number of values (4 bytes)
//   - values (4 bytes each)

void insert(const string& index, int value) {
    ensureDataDir();

    // Read entire database
    map<string, set<int>> db;

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

                set<int> values;
                for (int j = 0; j < numValues; j++) {
                    int val;
                    infile.read(reinterpret_cast<char*>(&val), sizeof(val));
                    values.insert(val);
                }

                db[idx] = values;
            }
            infile.close();
        }
    }

    // Update database
    db[index].insert(value);

    // Write entire database back
    ofstream outfile(DB_FILE, ios::binary);
    if (outfile.is_open()) {
        int numIndices = db.size();
        outfile.write(reinterpret_cast<const char*>(&numIndices), sizeof(numIndices));

        for (const auto& [idx, values] : db) {
            int indexLen = idx.length();
            outfile.write(reinterpret_cast<const char*>(&indexLen), sizeof(indexLen));
            outfile.write(idx.c_str(), indexLen);

            int numValues = values.size();
            outfile.write(reinterpret_cast<const char*>(&numValues), sizeof(numValues));

            for (int val : values) {
                outfile.write(reinterpret_cast<const char*>(&val), sizeof(val));
            }
        }
        outfile.close();
    }
}

void deleteEntry(const string& index, int value) {
    ensureDataDir();

    if (!fs::exists(DB_FILE)) {
        return;
    }

    // Read entire database
    map<string, set<int>> db;

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

            set<int> values;
            for (int j = 0; j < numValues; j++) {
                int val;
                infile.read(reinterpret_cast<char*>(&val), sizeof(val));
                values.insert(val);
            }

            db[idx] = values;
        }
        infile.close();
    }

    // Update database
    auto it = db.find(index);
    if (it != db.end()) {
        it->second.erase(value);
        if (it->second.empty()) {
            db.erase(it);
        }
    }

    // Write entire database back
    ofstream outfile(DB_FILE, ios::binary);
    if (outfile.is_open()) {
        int numIndices = db.size();
        outfile.write(reinterpret_cast<const char*>(&numIndices), sizeof(numIndices));

        for (const auto& [idx, values] : db) {
            int indexLen = idx.length();
            outfile.write(reinterpret_cast<const char*>(&indexLen), sizeof(indexLen));
            outfile.write(idx.c_str(), indexLen);

            int numValues = values.size();
            outfile.write(reinterpret_cast<const char*>(&numValues), sizeof(numValues));

            for (int val : values) {
                outfile.write(reinterpret_cast<const char*>(&val), sizeof(val));
            }
        }
        outfile.close();
    }
}

void find(const string& index) {
    ensureDataDir();

    if (!fs::exists(DB_FILE)) {
        cout << "null" << endl;
        return;
    }

    // Read entire database
    map<string, set<int>> db;

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

            set<int> values;
            for (int j = 0; j < numValues; j++) {
                int val;
                infile.read(reinterpret_cast<char*>(&val), sizeof(val));
                values.insert(val);
            }

            db[idx] = values;
        }
        infile.close();
    }

    // Find and output
    auto it = db.find(index);
    if (it == db.end() || it->second.empty()) {
        cout << "null" << endl;
    } else {
        bool first = true;
        for (int val : it->second) {
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