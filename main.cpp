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
const string INDEX_FILE = "data/index.idx";

// Cache for current operation
string currentIndex;
set<int> currentValues;
bool hasCurrent = false;

void ensureDataDir() {
    if (!fs::exists("data")) {
        fs::create_directory("data");
    }
}

// Read index positions from index file
map<string, streampos> readIndexPositions() {
    map<string, streampos> positions;

    if (fs::exists(INDEX_FILE)) {
        ifstream idxFile(INDEX_FILE, ios::binary);
        if (idxFile.is_open()) {
            int count;
            idxFile.read(reinterpret_cast<char*>(&count), sizeof(count));

            for (int i = 0; i < count; i++) {
                int len;
                idxFile.read(reinterpret_cast<char*>(&len), sizeof(len));

                string index(len, '\0');
                idxFile.read(&index[0], len);

                streampos pos;
                idxFile.read(reinterpret_cast<char*>(&pos), sizeof(pos));

                positions[index] = pos;
            }
            idxFile.close();
        }
    }

    return positions;
}

void writeIndexPositions(const map<string, streampos>& positions) {
    ofstream idxFile(INDEX_FILE, ios::binary);
    if (!idxFile.is_open()) return;

    int count = positions.size();
    idxFile.write(reinterpret_cast<const char*>(&count), sizeof(count));

    for (const auto& [index, pos] : positions) {
        int len = index.length();
        idxFile.write(reinterpret_cast<const char*>(&len), sizeof(len));
        idxFile.write(index.c_str(), len);
        idxFile.write(reinterpret_cast<const char*>(&pos), sizeof(pos));
    }

    idxFile.close();
}

// Read values for a specific index from data file
bool readValues(const string& index, set<int>& values) {
    auto positions = readIndexPositions();
    auto it = positions.find(index);

    if (it == positions.end()) {
        return false;
    }

    ifstream dbFile(DB_FILE, ios::binary);
    if (!dbFile.is_open()) return false;

    dbFile.seekg(it->second);

    int numValues;
    dbFile.read(reinterpret_cast<char*>(&numValues), sizeof(numValues));

    for (int i = 0; i < numValues; i++) {
        int val;
        dbFile.read(reinterpret_cast<char*>(&val), sizeof(val));
        values.insert(val);
    }

    dbFile.close();
    return true;
}

// Append data to end of file and update index
streampos appendData(const string& index, const set<int>& values) {
    ofstream dbFile(DB_FILE, ios::binary | ios::app);
    if (!dbFile.is_open()) return -1;

    streampos pos = dbFile.tellp();

    int numValues = values.size();
    dbFile.write(reinterpret_cast<const char*>(&numValues), sizeof(numValues));

    for (int val : values) {
        dbFile.write(reinterpret_cast<const char*>(&val), sizeof(val));
    }

    dbFile.close();
    return pos;
}

void insert(const string& index, int value) {
    ensureDataDir();

    set<int> values;

    // Read current values
    if (hasCurrent && currentIndex == index) {
        values = currentValues;
    } else {
        readValues(index, values);
    }

    values.insert(value);

    // Update current
    currentIndex = index;
    currentValues = values;
    hasCurrent = true;

    // Update index and append data
    auto positions = readIndexPositions();
    streampos pos = appendData(index, values);
    positions[index] = pos;
    writeIndexPositions(positions);
}

void deleteEntry(const string& index, int value) {
    ensureDataDir();

    set<int> values;

    // Read current values
    if (hasCurrent && currentIndex == index) {
        values = currentValues;
    } else if (!readValues(index, values)) {
        return; // Index doesn't exist
    }

    values.erase(value);

    // Update current
    currentIndex = index;
    currentValues = values;
    hasCurrent = true;

    // Update index and append data
    auto positions = readIndexPositions();
    streampos pos = appendData(index, values);
    positions[index] = pos;
    writeIndexPositions(positions);
}

void find(const string& index) {
    ensureDataDir();

    set<int> values;

    // Read values
    if (hasCurrent && currentIndex == index) {
        values = currentValues;
    } else if (!readValues(index, values)) {
        cout << "null" << endl;
        return;
    }

    // Update current
    currentIndex = index;
    currentValues = values;
    hasCurrent = true;

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