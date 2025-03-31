#include "ExtFileSystem.h"
#include <iostream>
#include <stdexcept>

ExtFileSystem::ExtFileSystem(Disk &disk) : disk(disk), current_path("/") {}

bool ExtFileSystem::loadFileSystem() {
    // Load the root inode
    root_inode = getInode(2);  // Typically inode 2 is the root in Ext-like filesystems
    return true;
}

std::string ExtFileSystem::getDiskLabel() {
    // For simple implementation, return a hardcoded label
    return "EXT Disk";
}

void ExtFileSystem::listDirectory() {
    inode current_inode = getInode(2);  // For simplicity, always list root directory
    std::vector<dir_entry> entries = readDirectory(current_inode);

    for (const auto &entry : entries) {
        std::string type = (entry.file_type == EXT2_S_IFDIR) ? "DIR" : "FILE";
        std::cout << type << "\t" << entry.name << std::endl;
    }
}

void ExtFileSystem::changeDirectory(const std::string &dir) {
    // Implement directory change logic
}

std::string ExtFileSystem::getCurrentPath() {
    return current_path;
}

void ExtFileSystem::readFile(const std::string &file) {
    // Implement file reading logic
}

void ExtFileSystem::printInodeInfo(const std::string &file) {
    // Implement inode information printing logic
}

inode ExtFileSystem::getInode(uint32_t inode_number) {
    inode result;
    // Implement logic to read inode from disk
    return result;
}

std::vector<dir_entry> ExtFileSystem::readDirectory(inode &dir_inode) {
    std::vector<dir_entry> entries;
    // Implement logic to read directory entries from disk
    return entries;
}

inode ExtFileSystem::findFileInDirectory(inode &dir_inode, const std::string &name) {
    inode result;
    // Implement logic to find file in directory
    return result;
}
