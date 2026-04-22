#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <string>
#include <sstream>
#include <filesystem>
#include <set>

using namespace std;
namespace fs = std::filesystem;

const string DB_FILE = "data/database.db";

void ensureDataDir() {
    if (!fs::exists("data")) {
        fs::create_directory("data");
    }
}

// Simple format: index length, index string, value count, values
// All operations read the entire file and rewrite it

void insert(const string& index, int value) {
    ensureDataDir();

    vector<pair<string, set<int>>> data;

    // Read existing data
    if (fs::exists(DB_FILE)) {
        ifstream infile(DB_FILE, ios::binary);
        if (infile.is_open()) {
            while (infile.peek() != EOF) {
                int indexLen;
                infile.read(reinterpret_cast<char*>(&indexLen), sizeof(indexLen));
                if (infile.eof()) break;

                string idx(indexLen, '\0');
                infile.read(&idx[0], indexLen);

                int numValues;
                infile.read(reinterpret_cast<char*>(&numValues), sizeof(numValues));

                set<int> values;
                for (int i = 0; i < numValues; i++) {
                    int val;
                    infile.read(reinterpret_cast<char*>(&val), sizeof(val));
                    values.insert(val);
                }

                data.emplace_back(idx, values);
            }
            infile.close();
        }
    }

    // Update or add
    bool found = false;
    for (auto& [idx, values] : data) {
        if (idx == index) {
            values.insert(value);
            found = true;
            break;
        }
    }

    if (!found) {
        set<int> newValues;
        newValues.insert(value);
        data.emplace_back(index, newValues);
    }

    // Write back
    ofstream outfile(DB_FILE, ios::binary);
    if (outfile.is_open()) {
        for (const auto& [idx, values] : data) {
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

    vector<pair<string, set<int>>> data;

    // Read existing data
    ifstream infile(DB_FILE, ios::binary);
    if (infile.is_open()) {
        while (infile.peek() != EOF) {
            int indexLen;
            infile.read(reinterpret_cast<char*>(&indexLen), sizeof(indexLen));
            if (infile.eof()) break;

            string idx(indexLen, '\0');
            infile.read(&idx[0], indexLen);

            int numValues;
            infile.read(reinterpret_cast<char*>(&numValues), sizeof(numValues));

            set<int> values;
            for (int i = 0; i < numValues; i++) {
                int val;
                infile.read(reinterpret_cast<char*>(&val), sizeof(val));
                values.insert(val);
            }

            data.emplace_back(idx, values);
        }
        infile.close();
    }

    // Update
    for (auto& [idx, values] : data) {
        if (idx == index) {
            values.erase(value);
            break;
        }
    }

    // Remove empty entries
    data.erase(remove_if(data.begin(), data.end(),
                        [](const auto& p) { return p.second.empty(); }),
              data.end());

    // Write back
    ofstream outfile(DB_FILE, ios::binary);
    if (outfile.is_open()) {
        for (const auto& [idx, values] : data) {
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

    set<int> foundValues;

    // Scan through file linearly
    ifstream infile(DB_FILE, ios::binary);
    if (infile.is_open()) {
        while (infile.peek() != EOF) {
            int indexLen;
            infile.read(reinterpret_cast<char*>(&indexLen), sizeof(indexLen));
            if (infile.eof()) break;

            string idx(indexLen, '\0');
            infile.read(&idx[0], indexLen);

            int numValues;
            infile.read(reinterpret_cast<char*>(&numValues), sizeof(numValues));

            if (idx == index) {
                // Found our index
                for (int i = 0; i < numValues; i++) {
                    int val;
                    infile.read(reinterpret_cast<char*>(&val), sizeof(val));
                    foundValues.insert(val);
                }
                break; // No need to continue
            } else {
                // Skip values
                infile.seekg(numValues * sizeof(int), ios::cur);
            }
        }
        infile.close();
    }

    if (foundValues.empty()) {
        cout << "null" << endl;
    } else {
        bool first = true;
        for (int val : foundValues) {
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