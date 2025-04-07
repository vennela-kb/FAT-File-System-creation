#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <cstring>
#include "disk.h"

using namespace std;

// Constants for Ext-like filesystem
const int BLOCK_SIZE = 512;
const int SUPERBLOCK_LOCATION = 1;
const int ROOT_INODE_NUMBER = 2;
const int INODE_SIZE = 128;
const int INODES_PER_BLOCK = BLOCK_SIZE / INODE_SIZE;
const int DIRECT_BLOCKS = 12;
const int INDIRECT_BLOCKS = 1;

// Struct for superblock
struct Superblock {
    char disk_label[32];
    int block_size;
    int blocks_count;
    int inodes_count;
    int inodes_per_group;
    int first_inode_block;
    int first_data_block;
};

// Struct for inode
struct Inode {
    unsigned short mode;
    unsigned int size;
    unsigned int blocks;
    unsigned int direct_blocks[DIRECT_BLOCKS];
    unsigned int indirect_block;
    
    // Helper function to determine if this is a directory
    bool is_directory() const {
        return (mode & 0x4000) != 0;
    }
    
    // Helper function to determine if this is a regular file
    bool is_file() const {
        return (mode & 0x8000) != 0;
    }
};

// Struct for directory entry
struct DirEntry {
    unsigned int inode;
    unsigned short rec_len;
    unsigned char name_len;
    unsigned char file_type;
    char name[256];
};

class ExtFilesystem {
private:
    Disk* disk;
    Superblock superblock;
    unsigned int current_dir_inode;
    vector<string> current_path;
    
    // Read a block from disk
    void read_block(int block_num, char* buffer) {
        disk->read(block_num, buffer);
    }
    
    // Get an inode by number
    Inode get_inode(int inode_number) {
        Inode inode;
        
        // Calculate block number and offset
        int inode_index = inode_number - 1; // Inode numbers start at 1
        int block_num = superblock.first_inode_block + (inode_index / INODES_PER_BLOCK);
        int offset = (inode_index % INODES_PER_BLOCK) * INODE_SIZE;
        
        // Read the block containing the inode
        char block[BLOCK_SIZE];
        read_block(block_num, block);
        
        // Extract inode data
        memcpy(&inode, block + offset, sizeof(Inode));
        
        return inode;
    }
    
    // Read a file's contents based on its inode
    string read_file_contents(const Inode& inode) {
        string contents;
        char block[BLOCK_SIZE];
        
        // Read direct blocks
        for (int i = 0; i < DIRECT_BLOCKS && i * BLOCK_SIZE < inode.size; i++) {
            if (inode.direct_blocks[i] == 0) break;
            
            read_block(inode.direct_blocks[i], block);
            int bytes_to_read = min(BLOCK_SIZE, inode.size - i * BLOCK_SIZE);
            contents.append(block, bytes_to_read);
        }
        
        // Read indirect blocks if necessary
        if (inode.indirect_block != 0 && contents.size() < inode.size) {
            char indirect_block[BLOCK_SIZE];
            read_block(inode.indirect_block, indirect_block);
            
            unsigned int* block_pointers = (unsigned int*)indirect_block;
            int blocks_needed = (inode.size - contents.size() + BLOCK_SIZE - 1) / BLOCK_SIZE;
            int indirect_blocks_count = BLOCK_SIZE / sizeof(unsigned int);
            
            for (int i = 0; i < blocks_needed && i < indirect_blocks_count; i++) {
                if (block_pointers[i] == 0) break;
                
                read_block(block_pointers[i], block);
                int bytes_to_read = min(BLOCK_SIZE, inode.size - contents.size());
                contents.append(block, bytes_to_read);
            }
        }
        
        return contents;
    }
    
    // Get entries in a directory based on its inode
    vector<DirEntry> get_directory_entries(const Inode& dir_inode) {
        vector<DirEntry> entries;
        
        if (!dir_inode.is_directory()) {
            return entries;
        }
        
        string dir_contents = read_file_contents(dir_inode);
        const char* data = dir_contents.c_str();
        int pos = 0;
        
        while (pos < dir_inode.size) {
            DirEntry entry;
            memcpy(&entry.inode, data + pos, 4);
            memcpy(&entry.rec_len, data + pos + 4, 2);
            memcpy(&entry.name_len, data + pos + 6, 1);
            memcpy(&entry.file_type, data + pos + 7, 1);
            memcpy(entry.name, data + pos + 8, entry.name_len);
            entry.name[entry.name_len] = '\0';
            
            if (entry.inode != 0) {
                entries.push_back(entry);
            }
            
            pos += entry.rec_len;
        }
        
        return entries;
    }
    
    // Find an entry in a directory by name
    DirEntry find_entry_by_name(const Inode& dir_inode, const string& name) {
        vector<DirEntry> entries = get_directory_entries(dir_inode);
        
        for (const auto& entry : entries) {
            if (name == entry.name) {
                return entry;
            }
        }
        
        DirEntry empty_entry;
        empty_entry.inode = 0;
        return empty_entry;
    }

public:
    ExtFilesystem(const string& disk_path) {
        // Open the disk
        disk = new Disk(disk_path);
        
        // Skip boot block and read superblock
        char superblock_data[BLOCK_SIZE];
        read_block(SUPERBLOCK_LOCATION, superblock_data);
        memcpy(&superblock, superblock_data, sizeof(Superblock));
        
        // Initialize to root directory
        current_dir_inode = ROOT_INODE_NUMBER;
        current_path.push_back("/");
    }
    
    ~ExtFilesystem() {
        delete disk;
    }
    
    // Get disk label
    string get_disk_label() const {
        return string(superblock.disk_label);
    }
    
    // List contents of current directory
    void list_directory() {
        Inode dir_inode = get_inode(current_dir_inode);
        vector<DirEntry> entries = get_directory_entries(dir_inode);
        
        // Print header
        cout << setw(10) << left << "Type" << setw(10) << right << "Size" << "  " << left << "Name" << endl;
        cout << string(40, '-') << endl;
        
        // Print entries
        for (const auto& entry : entries) {
            Inode entry_inode = get_inode(entry.inode);
            string type;
            
            if (entry_inode.is_directory()) {
                type = "DIR";
            } else if (entry_inode.is_file()) {
                type = "FILE";
            } else {
                type = "SPECIAL";
            }
            
            cout << setw(10) << left << type;
            
            if (entry_inode.is_file()) {
                cout << setw(10) << right << entry_inode.size;
            } else {
                cout << setw(10) << right << "-";
            }
            
            cout << "  " << left << entry.name << endl;
        }
    }
    
    // Change directory
    bool change_directory(const string& dir_name) {
        if (dir_name == "..") {
            // Handle going to parent directory
            if (current_path.size() > 1) {
                // Not at root, can go up
                current_path.pop_back();
                
                // Navigate to parent
                current_dir_inode = ROOT_INODE_NUMBER;  // Start at root
                for (size_t i = 1; i < current_path.size(); i++) {
                    Inode dir_inode = get_inode(current_dir_inode);
                    DirEntry entry = find_entry_by_name(dir_inode, current_path[i]);
                    current_dir_inode = entry.inode;
                }
                
                return true;
            }
            return false;  // Already at root
        } else if (dir_name == ".") {
            // Stay in current directory
            return true;
        } else {
            // Navigate to subdirectory
            Inode dir_inode = get_inode(current_dir_inode);
            DirEntry entry = find_entry_by_name(dir_inode, dir_name);
            
            if (entry.inode != 0) {
                Inode target_inode = get_inode(entry.inode);
                
                if (target_inode.is_directory()) {
                    current_dir_inode = entry.inode;
                    current_path.push_back(dir_name);
                    return true;
                }
            }
            
            cout << "Error: '" << dir_name << "' is not a directory or doesn't exist." << endl;
            return false;
        }
    }
    
    // Read a file's contents
    void read_file(const string& file_name) {
        Inode dir_inode = get_inode(current_dir_inode);
        DirEntry entry = find_entry_by_name(dir_inode, file_name);
        
        if (entry.inode != 0) {
            Inode file_inode = get_inode(entry.inode);
            
            if (file_inode.is_file()) {
                string contents = read_file_contents(file_inode);
                cout << contents << endl;
            } else {
                cout << "Error: '" << file_name << "' is not a regular file." << endl;
            }
        } else {
            cout << "Error: File '" << file_name << "' not found." << endl;
        }
    }
    
    // Print working directory
    void print_working_directory() {
        if (current_path.size() == 1) {
            cout << "/" << endl;
        } else {
            for (const auto& dir : current_path) {
                if (dir != "/") {
                    cout << "/" << dir;
                }
            }
            cout << endl;
        }
    }
    
    // Print inode information (stat)
    void print_inode_info(const string& name) {
        Inode dir_inode = get_inode(current_dir_inode);
        DirEntry entry = find_entry_by_name(dir_inode, name);
        
        if (entry.inode != 0) {
            Inode file_inode = get_inode(entry.inode);
            
            cout << "Inode Information for '" << name << "':" << endl;
            cout << "------------------------" << endl;
            cout << "Inode Number: " << entry.inode << endl;
            
            cout << "Type: ";
            if (file_inode.is_directory()) {
                cout << "Directory";
            } else if (file_inode.is_file()) {
                cout << "Regular File";
            } else {
                cout << "Special File";
            }
            cout << endl;
            
            cout << "Mode: 0x" << hex << file_inode.mode << dec << endl;
            cout << "Size: " << file_inode.size << " bytes" << endl;
            cout << "Blocks: " << file_inode.blocks << endl;
            
            cout << "Direct Blocks:" << endl;
            for (int i = 0; i < DIRECT_BLOCKS; i++) {
                if (file_inode.direct_blocks[i] != 0) {
                    cout << "  [" << i << "]: " << file_inode.direct_blocks[i] << endl;
                }
            }
            
            if (file_inode.indirect_block != 0) {
                cout << "Indirect Block: " << file_inode.indirect_block << endl;
                
                // Display contents of indirect block
                char indirect_block[BLOCK_SIZE];
                read_block(file_inode.indirect_block, indirect_block);
                unsigned int* block_pointers = (unsigned int*)indirect_block;
                
                int count = 0;
                for (int i = 0; i < (int)(BLOCK_SIZE / sizeof(unsigned int)); i++) {
                    if (block_pointers[i] != 0) {
                        count++;
                    }
                }
                
                cout << "  Contains " << count << " block pointers" << endl;
            }
        } else {
            cout << "Error: '" << name << "' not found." << endl;
        }
    }
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cout << "Usage: " << argv[0] << " <disk_image>" << endl;
        return 1;
    }
    
    string disk_path = argv[1];
    ExtFilesystem fs(disk_path);
    
    cout << "Ext Filesystem Browser" << endl;
    cout << "Disk Label: " << fs.get_disk_label() << endl;
    cout << "Type 'help' for available commands" << endl;
    
    bool running = true;
    while (running) {
        string command;
        cout << "> ";
        getline(cin, command);
        
        istringstream iss(command);
        string cmd;
        iss >> cmd;
        
        if (cmd == "dir" || cmd == "ls") {
            fs.list_directory();
        }
        else if (cmd == "cd") {
            string dir_name;
            iss >> dir_name;
            if (dir_name.empty()) {
                cout << "Usage: cd <directory>" << endl;
            } else {
                fs.change_directory(dir_name);
            }
        }
        else if (cmd == "read") {
            string file_name;
            iss >> file_name;
            if (file_name.empty()) {
                cout << "Usage: read <file>" << endl;
            } else {
                fs.read_file(file_name);
            }
        }
        else if (cmd == "pwd") {
            fs.print_working_directory();
        }
        else if (cmd == "stat") {
            string name;
            iss >> name;
            if (name.empty()) {
                cout << "Usage: stat <file>" << endl;
            } else {
                fs.print_inode_info(name);
            }
        }
        else if (cmd == "exit" || cmd == "quit") {
            running = false;
        }
        else if (cmd == "help") {
            cout << "Available commands:" << endl;
            cout << "  dir               - List contents of current directory" << endl;
            cout << "  cd <dir>          - Change directory" << endl;
            cout << "  read <file>       - Read and print the contents of a file" << endl;
            cout << "  pwd               - Print the current working directory" << endl;
            cout << "  stat <file>       - Print the inode information for a file" << endl;
            cout << "  help              - Show this help message" << endl;
            cout << "  exit, quit        - Exit the program" << endl;
        }
        else if (!cmd.empty()) {
            cout << "Unknown command: " << cmd << endl;
            cout << "Type 'help' for available commands" << endl;
        }
    }
    
    return 0;
}