#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <string>
#include <sstream>
#include <filesystem>
#include <set>
#include <cstring>

using namespace std;
namespace fs = std::filesystem;

const string DB_FILE = "data/database.db";
const int BLOCK_SIZE = 4096;  // 4KB blocks
const int MAX_VALUES_PER_BLOCK = 100;  // Max values per index in a block

void ensureDataDir() {
    if (!fs::exists("data")) {
        fs::create_directory("data");
    }
}

// Block format:
// - index string (256 bytes max)
// - value count (4 bytes)
// - values (4 bytes each, up to MAX_VALUES_PER_BLOCK)
// - next block position for this index (8 bytes, -1 if no next)
// Total: ~660 bytes per block

struct Block {
    char index[256];
    int valueCount;
    int values[MAX_VALUES_PER_BLOCK];
    int64_t nextBlock;

    Block() : valueCount(0), nextBlock(-1) {
        memset(index, 0, sizeof(index));
        memset(values, 0, sizeof(values));
    }
};

streampos allocateBlock() {
    ofstream file(DB_FILE, ios::binary | ios::app);
    if (!file.is_open()) return -1;

    streampos pos = file.tellp();
    Block block;
    file.write(reinterpret_cast<char*>(&block), sizeof(Block));
    file.close();

    return pos;
}

Block readBlock(streampos pos) {
    Block block;
    ifstream file(DB_FILE, ios::binary);
    if (!file.is_open()) return block;

    file.seekg(pos);
    file.read(reinterpret_cast<char*>(&block), sizeof(Block));
    file.close();

    return block;
}

void writeBlock(streampos pos, const Block& block) {
    fstream file(DB_FILE, ios::binary | ios::in | ios::out);
    if (!file.is_open()) return;

    file.seekp(pos);
    file.write(reinterpret_cast<const char*>(&block), sizeof(Block));
    file.close();
}

void insert(const string& index, int value) {
    ensureDataDir();

    // Find the first block for this index
    streampos currentPos = 0;
    Block currentBlock;
    bool found = false;

    if (fs::exists(DB_FILE)) {
        ifstream file(DB_FILE, ios::binary);
        if (file.is_open()) {
            file.seekg(0, ios::end);
            streampos fileSize = file.tellg();
            file.seekg(0);

            while (file.tellg() < fileSize) {
                streampos pos = file.tellg();
                Block block;
                file.read(reinterpret_cast<char*>(&block), sizeof(Block));

                if (string(block.index) == index) {
                    currentPos = pos;
                    currentBlock = block;
                    found = true;

                    // Go to last block in chain
                    while (block.nextBlock != -1) {
                        file.seekg(block.nextBlock);
                        pos = block.nextBlock;
                        file.read(reinterpret_cast<char*>(&block), sizeof(Block));
                        currentPos = pos;
                        currentBlock = block;
                    }
                    break;
                }
            }
            file.close();
        }
    }

    if (found) {
        // Check if value already exists in this block
        for (int i = 0; i < currentBlock.valueCount; i++) {
            if (currentBlock.values[i] == value) {
                return; // Value already exists
            }
        }

        // Add to current block if space
        if (currentBlock.valueCount < MAX_VALUES_PER_BLOCK) {
            currentBlock.values[currentBlock.valueCount++] = value;
            // Sort values
            sort(currentBlock.values, currentBlock.values + currentBlock.valueCount);
            writeBlock(currentPos, currentBlock);
        } else {
            // Allocate new block
            streampos newPos = allocateBlock();
            if (newPos != -1) {
                Block newBlock;
                strncpy(newBlock.index, index.c_str(), sizeof(newBlock.index) - 1);
                newBlock.values[0] = value;
                newBlock.valueCount = 1;

                currentBlock.nextBlock = newPos;
                writeBlock(currentPos, currentBlock);
                writeBlock(newPos, newBlock);
            }
        }
    } else {
        // Create first block for this index
        streampos pos = allocateBlock();
        if (pos != -1) {
            Block block;
            strncpy(block.index, index.c_str(), sizeof(block.index) - 1);
            block.values[0] = value;
            block.valueCount = 1;
            writeBlock(pos, block);
        }
    }
}

void deleteEntry(const string& index, int value) {
    ensureDataDir();

    if (!fs::exists(DB_FILE)) return;

    ifstream file(DB_FILE, ios::binary);
    if (!file.is_open()) return;

    file.seekg(0, ios::end);
    streampos fileSize = file.tellg();
    file.seekg(0);

    vector<pair<streampos, Block>> allBlocks;
    bool found = false;

    // Read all blocks
    while (file.tellg() < fileSize) {
        streampos pos = file.tellg();
        Block block;
        file.read(reinterpret_cast<char*>(&block), sizeof(Block));
        allBlocks.emplace_back(pos, block);

        if (string(block.index) == index) {
            // Remove value if exists
            for (int i = 0; i < block.valueCount; i++) {
                if (block.values[i] == value) {
                    // Shift remaining values
                    for (int j = i; j < block.valueCount - 1; j++) {
                        block.values[j] = block.values[j + 1];
                    }
                    block.valueCount--;
                    allBlocks.back().second = block;
                    found = true;
                    break;
                }
            }
        }
    }
    file.close();

    if (found) {
        // Rewrite file without empty blocks
        ofstream outFile(DB_FILE, ios::binary);
        if (outFile.is_open()) {
            for (const auto& [pos, block] : allBlocks) {
                if (block.valueCount > 0) {
                    outFile.write(reinterpret_cast<const char*>(&block), sizeof(Block));
                }
            }
            outFile.close();
        }
    }
}

void find(const string& index) {
    ensureDataDir();

    if (!fs::exists(DB_FILE)) {
        cout << "null" << endl;
        return;
    }

    ifstream file(DB_FILE, ios::binary);
    if (!file.is_open()) {
        cout << "null" << endl;
        return;
    }

    file.seekg(0, ios::end);
    streampos fileSize = file.tellg();
    file.seekg(0);

    set<int> allValues;

    // Scan all blocks for this index
    while (file.tellg() < fileSize) {
        Block block;
        file.read(reinterpret_cast<char*>(&block), sizeof(Block));

        if (string(block.index) == index) {
            for (int i = 0; i < block.valueCount; i++) {
                allValues.insert(block.values[i]);
            }

            // Follow chain
            streampos nextPos = block.nextBlock;
            while (nextPos != -1) {
                file.seekg(nextPos);
                file.read(reinterpret_cast<char*>(&block), sizeof(Block));
                for (int i = 0; i < block.valueCount; i++) {
                    allValues.insert(block.values[i]);
                }
                nextPos = block.nextBlock;
            }
            break;
        }
    }

    file.close();

    if (allValues.empty()) {
        cout << "null" << endl;
    } else {
        bool first = true;
        for (int val : allValues) {
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