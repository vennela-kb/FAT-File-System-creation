#ifndef EXT_FILESYSTEM_H
#define EXT_FILESYSTEM_H

#include <string>
#include <vector>
#include "Disk.h"

#define BLOCK_SIZE 512

// Define enums for file types
enum file_type {
    EXT2_S_IFREG = 0x8000,  // Regular file
    EXT2_S_IFDIR = 0x4000,  // Directory
};

// Define inode structure
struct inode {
    uint16_t mode;       // File mode
    uint32_t size;       // File size
    uint32_t block[15];  // Pointers to blocks
};

// Define directory entry structure
struct dir_entry {
    uint32_t inode;      // Inode number
    uint16_t rec_len;    // Directory entry length
    uint8_t name_len;    // Name length
    uint8_t file_type;   // File type
    char name[255];      // File name
};

// Ext-like filesystem class
class ExtFileSystem {
public:
    ExtFileSystem(Disk &disk);
    bool loadFileSystem();
    std::string getDiskLabel();
    void listDirectory();
    void changeDirectory(const std::string &dir);
    std::string getCurrentPath();
    void readFile(const std::string &file);
    void printInodeInfo(const std::string &file);

private:
    Disk &disk;
    inode root_inode;
    std::string current_path;
    std::vector<inode> inodes;

    inode getInode(uint32_t inode_number);
    std::vector<dir_entry> readDirectory(inode &dir_inode);
    inode findFileInDirectory(inode &dir_inode, const std::string &name);
};

#endif
