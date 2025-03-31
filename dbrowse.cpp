#include <iostream>
#include <string>
#include "Disk.h"
#include "ExtFileSystem.h"

using namespace std;

Disk disk("disk.img", 512);
ExtFileSystem fs(disk);

void printHelp() {
    cout << "Available Commands:" << endl;
    cout << "  dir            - List contents of current directory" << endl;
    cout << "  cd <dir>       - Change directory" << endl;
    cout << "  read <file>    - Read and display file contents" << endl;
    cout << "  pwd            - Print current directory path" << endl;
    cout << "  stat <file>    - Print inode information for a file" << endl;
    cout << "  help           - Show available commands" << endl;
    cout << "  exit           - Exit program" << endl;
}

int main() {
    if (!fs.loadFileSystem()) {
        cout << "Failed to load file system." << endl;
        return 1;
    }

    cout << "Welcome to ExtBrowse! Disk label: " << fs.getDiskLabel() << endl;
    printHelp();

    string command;
    while (true) {
        cout << "> ";
        getline(cin, command);
        
        if (command == "exit") break;
        else if (command == "help") printHelp();
        else if (command == "pwd") cout << fs.getCurrentPath() << endl;
        else if (command.rfind("dir", 0) == 0) fs.listDirectory();
        else if (command.rfind("cd ", 0) == 0) fs.changeDirectory(command.substr(3));
        else if (command.rfind("read ", 0) == 0) fs.readFile(command.substr(5));
        else if (command.rfind("stat ", 0) == 0) fs.printInodeInfo(command.substr(5));
        else cout << "Invalid command. Type 'help' for a list of commands." << endl;
    }

    disk.printStats();
    return 0;
}
