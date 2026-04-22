#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <string>
#include <sstream>
#include <filesystem>

using namespace std;
namespace fs = std::filesystem;

const string DATA_DIR = "data";

void ensureDataDir() {
    if (!fs::exists(DATA_DIR)) {
        fs::create_directory(DATA_DIR);
    }
}

string getFilename(const string& index) {
    // Replace potentially problematic characters in filename
    string safeIndex = index;
    for (char& c : safeIndex) {
        if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|') {
            c = '_';
        }
    }
    return DATA_DIR + "/" + safeIndex + ".dat";
}

void insert(const string& index, int value) {
    ensureDataDir();
    string filename = getFilename(index);
    vector<int> values;

    // Read existing values
    if (fs::exists(filename)) {
        ifstream infile(filename, ios::binary);
        if (infile.is_open()) {
            int count;
            infile.read(reinterpret_cast<char*>(&count), sizeof(count));
            values.resize(count);
            infile.read(reinterpret_cast<char*>(values.data()), count * sizeof(int));
            infile.close();
        }
    }

    // Check if value already exists
    auto it = lower_bound(values.begin(), values.end(), value);
    if (it == values.end() || *it != value) {
        values.insert(it, value);

        // Write back to file
        ofstream outfile(filename, ios::binary);
        if (outfile.is_open()) {
            int count = values.size();
            outfile.write(reinterpret_cast<const char*>(&count), sizeof(count));
            outfile.write(reinterpret_cast<const char*>(values.data()), count * sizeof(int));
            outfile.close();
        }
    }
}

void deleteEntry(const string& index, int value) {
    ensureDataDir();
    string filename = getFilename(index);

    if (!fs::exists(filename)) {
        return;
    }

    vector<int> values;

    // Read existing values
    ifstream infile(filename, ios::binary);
    if (infile.is_open()) {
        int count;
        infile.read(reinterpret_cast<char*>(&count), sizeof(count));
        values.resize(count);
        infile.read(reinterpret_cast<char*>(values.data()), count * sizeof(int));
        infile.close();
    }

    // Remove value if exists
    auto it = lower_bound(values.begin(), values.end(), value);
    if (it != values.end() && *it == value) {
        values.erase(it);

        // Write back to file
        ofstream outfile(filename, ios::binary);
        if (outfile.is_open()) {
            int count = values.size();
            outfile.write(reinterpret_cast<const char*>(&count), sizeof(count));
            outfile.write(reinterpret_cast<const char*>(values.data()), count * sizeof(int));
            outfile.close();
        }
    }
}

void find(const string& index) {
    ensureDataDir();
    string filename = getFilename(index);

    if (!fs::exists(filename)) {
        cout << "null" << endl;
        return;
    }

    vector<int> values;

    // Read values from file
    ifstream infile(filename, ios::binary);
    if (infile.is_open()) {
        int count;
        infile.read(reinterpret_cast<char*>(&count), sizeof(count));
        values.resize(count);
        infile.read(reinterpret_cast<char*>(values.data()), count * sizeof(int));
        infile.close();
    }

    if (values.empty()) {
        cout << "null" << endl;
    } else {
        for (size_t i = 0; i < values.size(); i++) {
            if (i > 0) cout << " ";
            cout << values[i];
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