
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstring>
#include <iomanip>
#include <fstream>
#include <sstream>

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>

using namespace std;

#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define BOLD    "\033[1m"

class FileExplorer {
private:
    string currentPath;
    
    // Helper function to get file type
    string getFileType(mode_t mode) {
        if (S_ISDIR(mode)) return "DIR";
        if (S_ISREG(mode)) return "FILE";
        if (S_ISLNK(mode)) return "LINK";
        if (S_ISCHR(mode)) return "CHR";
        if (S_ISBLK(mode)) return "BLK";
        if (S_ISFIFO(mode)) return "FIFO";
        if (S_ISSOCK(mode)) return "SOCK";
        return "UNKN";
    }
    
    // Helper function to get permissions string
    string getPermissions(mode_t mode) {
        string perms = "";
        perms += (mode & S_IRUSR) ? "r" : "-";
        perms += (mode & S_IWUSR) ? "w" : "-";
        perms += (mode & S_IXUSR) ? "x" : "-";
        perms += (mode & S_IRGRP) ? "r" : "-";
        perms += (mode & S_IWGRP) ? "w" : "-";
        perms += (mode & S_IXGRP) ? "x" : "-";
        perms += (mode & S_IROTH) ? "r" : "-";
        perms += (mode & S_IWOTH) ? "w" : "-";
        perms += (mode & S_IXOTH) ? "x" : "-";
        return perms;
    }
    
    // Helper function to format file size
    string formatSize(off_t size) {
        const char* units[] = {"B", "KB", "MB", "GB", "TB"};
        int unitIndex = 0;
        double fsize = size;
        
        while (fsize >= 1024 && unitIndex < 4) {
            fsize /= 1024;
            unitIndex++;
        }
        
        stringstream ss;
        ss << fixed << setprecision(2) << fsize << " " << units[unitIndex];
        return ss.str();
    }
    
    // Helper function to get username from UID
    string getUsername(uid_t uid) {
        struct passwd *pw = getpwuid(uid);
        return pw ? pw->pw_name : to_string(uid);
    }
    
    // Helper function to get group name from GID
    string getGroupname(gid_t gid) {
        struct group *gr = getgrgid(gid);
        return gr ? gr->gr_name : to_string(gid);
    }
    
    // Recursive search helper
    void searchRecursive(const string& path, const string& pattern, vector<string>& results) {
        DIR* dir = opendir(path.c_str());
        if (!dir) return;
        
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            string name = entry->d_name;
            if (name == "." || name == "..") continue;
            
            string fullPath = path + "/" + name;
            
            // Check if name matches pattern
            if (name.find(pattern) != string::npos) {
                results.push_back(fullPath);
            }
            
            // Recurse into directories
            struct stat statbuf;
            if (stat(fullPath.c_str(), &statbuf) == 0 && S_ISDIR(statbuf.st_mode)) {
                searchRecursive(fullPath, pattern, results);
            }
        }
        closedir(dir);
    }
    
    // Copy file helper
    bool copyFile(const string& src, const string& dest) {
        ifstream source(src, ios::binary);
        ofstream destination(dest, ios::binary);
        
        if (!source || !destination) {
            return false;
        }
        
        destination << source.rdbuf();
        
        // Copy permissions
        struct stat statbuf;
        if (stat(src.c_str(), &statbuf) == 0) {
            chmod(dest.c_str(), statbuf.st_mode);
        }
        
        return true;
    }

public:
    FileExplorer() {
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) != nullptr) {
            currentPath = cwd;
        } else {
            currentPath = "/";
        }
    }
    
    // Day 1: List files in current directory
    void listFiles(bool detailed = false) {
        DIR* dir = opendir(currentPath.c_str());
        if (!dir) {
            cout << RED << "Error: Cannot open directory: " << strerror(errno) << RESET << endl;
            return;
        }
        
        cout << BOLD << CYAN << "\nCurrent Directory: " << currentPath << RESET << endl;
        cout << string(80, '=') << endl;
        
        if (detailed) {
            cout << left << setw(12) << "Permissions" 
                 << setw(10) << "Owner" 
                 << setw(10) << "Group" 
                 << setw(12) << "Size" 
                 << setw(6) << "Type"
                 << "Name" << endl;
            cout << string(80, '-') << endl;
        }
        
        vector<string> files, dirs;
        struct dirent* entry;
        
        while ((entry = readdir(dir)) != nullptr) {
            string name = entry->d_name;
            if (name == ".") continue;
            
            string fullPath = currentPath + "/" + name;
            struct stat statbuf;
            
            if (stat(fullPath.c_str(), &statbuf) == 0) {
                if (detailed) {
                    cout << left << setw(12) << getPermissions(statbuf.st_mode)
                         << setw(10) << getUsername(statbuf.st_uid)
                         << setw(10) << getGroupname(statbuf.st_gid)
                         << setw(12) << formatSize(statbuf.st_size)
                         << setw(6) << getFileType(statbuf.st_mode);
                    
                    if (S_ISDIR(statbuf.st_mode)) {
                        cout << BLUE << name << "/" << RESET << endl;
                    } else if (statbuf.st_mode & S_IXUSR) {
                        cout << GREEN << name << RESET << endl;
                    } else {
                        cout << name << endl;
                    }
                } else {
                    if (S_ISDIR(statbuf.st_mode)) {
                        dirs.push_back(name);
                    } else {
                        files.push_back(name);
                    }
                }
            }
        }
        
        if (!detailed) {
            // Sort and display
            sort(dirs.begin(), dirs.end());
            sort(files.begin(), files.end());
            
            for (const auto& d : dirs) {
                cout << BLUE << d << "/" << RESET << "  ";
            }
            cout << endl;
            for (const auto& f : files) {
                cout << f << "  ";
            }
            cout << endl;
        }
        
        closedir(dir);
        cout << string(80, '=') << endl;
    }
    
    // Day 2: Navigate to directory
    bool changeDirectory(const string& path) {
        string newPath;
        
        if (path == "..") {
            // Go to parent directory
            size_t pos = currentPath.find_last_of('/');
            if (pos != string::npos && pos != 0) {
                newPath = currentPath.substr(0, pos);
            } else {
                newPath = "/";
            }
        } else if (path[0] == '/') {
            // Absolute path
            newPath = path;
        } else {
            // Relative path
            newPath = currentPath + "/" + path;
        }
        
        struct stat statbuf;
        if (stat(newPath.c_str(), &statbuf) == 0 && S_ISDIR(statbuf.st_mode)) {
            currentPath = newPath;
            return true;
        }
        
        cout << RED << "Error: Directory not found or not accessible" << RESET << endl;
        return false;
    }
    
    string getCurrentPath() const {
        return currentPath;
    }
    
    // Day 3: Create directory
    bool createDirectory(const string& name) {
        string fullPath = currentPath + "/" + name;
        if (mkdir(fullPath.c_str(), 0755) == 0) {
            cout << GREEN << "Directory created successfully: " << name << RESET << endl;
            return true;
        }
        cout << RED << "Error: Cannot create directory: " << strerror(errno) << RESET << endl;
        return false;
    }
    
    // Day 3: Create file
    bool createFile(const string& name) {
        string fullPath = currentPath + "/" + name;
        ofstream file(fullPath);
        if (file) {
            cout << GREEN << "File created successfully: " << name << RESET << endl;
            return true;
        }
        cout << RED << "Error: Cannot create file" << RESET << endl;
        return false;
    }
    
    // Day 3: Delete file or directory
    bool deleteItem(const string& name) {
        string fullPath = currentPath + "/" + name;
        struct stat statbuf;
        
        if (stat(fullPath.c_str(), &statbuf) != 0) {
            cout << RED << "Error: Item not found" << RESET << endl;
            return false;
        }
        
        if (S_ISDIR(statbuf.st_mode)) {
            if (rmdir(fullPath.c_str()) == 0) {
                cout << GREEN << "Directory deleted: " << name << RESET << endl;
                return true;
            }
            cout << RED << "Error: Cannot delete directory (may not be empty): " 
                 << strerror(errno) << RESET << endl;
        } else {
            if (unlink(fullPath.c_str()) == 0) {
                cout << GREEN << "File deleted: " << name << RESET << endl;
                return true;
            }
            cout << RED << "Error: Cannot delete file: " << strerror(errno) << RESET << endl;
        }
        return false;
    }
    
    // Day 3: Copy file
    bool copy(const string& src, const string& dest) {
        string srcPath = currentPath + "/" + src;
        string destPath = currentPath + "/" + dest;
        
        struct stat statbuf;
        if (stat(srcPath.c_str(), &statbuf) != 0) {
            cout << RED << "Error: Source file not found" << RESET << endl;
            return false;
        }
        
        if (!S_ISREG(statbuf.st_mode)) {
            cout << RED << "Error: Can only copy regular files" << RESET << endl;
            return false;
        }
        
        if (copyFile(srcPath, destPath)) {
            cout << GREEN << "File copied successfully" << RESET << endl;
            return true;
        }
        
        cout << RED << "Error: Cannot copy file" << RESET << endl;
        return false;
    }
    
    // Day 3: Move/rename file
    bool move(const string& src, const string& dest) {
        string srcPath = currentPath + "/" + src;
        string destPath = currentPath + "/" + dest;
        
        if (rename(srcPath.c_str(), destPath.c_str()) == 0) {
            cout << GREEN << "Item moved/renamed successfully" << RESET << endl;
            return true;
        }
        
        cout << RED << "Error: Cannot move/rename: " << strerror(errno) << RESET << endl;
        return false;
    }
    
    // Day 4: Search for files
    void search(const string& pattern) {
        cout << YELLOW << "Searching for: " << pattern << RESET << endl;
        vector<string> results;
        searchRecursive(currentPath, pattern, results);
        
        if (results.empty()) {
            cout << "No files found matching the pattern." << endl;
        } else {
            cout << GREEN << "Found " << results.size() << " match(es):" << RESET << endl;
            for (const auto& result : results) {
                cout << "  " << result << endl;
            }
        }
    }
    
    // Day 5: Change permissions
    bool changePermissions(const string& name, const string& perms) {
        string fullPath = currentPath + "/" + name;
        
        // Convert permission string to mode_t
        mode_t mode = 0;
        if (perms.length() == 3) {
            mode = strtol(perms.c_str(), nullptr, 8);
        } else {
            cout << RED << "Error: Permission format should be octal (e.g., 755)" << RESET << endl;
            return false;
        }
        
        if (chmod(fullPath.c_str(), mode) == 0) {
            cout << GREEN << "Permissions changed successfully" << RESET << endl;
            return true;
        }
        
        cout << RED << "Error: Cannot change permissions: " << strerror(errno) << RESET << endl;
        return false;
    }
    
    // Day 5: Change owner
    bool changeOwner(const string& name, const string& owner) {
        string fullPath = currentPath + "/" + name;
        
        struct passwd *pw = getpwnam(owner.c_str());
        if (!pw) {
            cout << RED << "Error: User not found" << RESET << endl;
            return false;
        }
        
        if (chown(fullPath.c_str(), pw->pw_uid, -1) == 0) {
            cout << GREEN << "Owner changed successfully" << RESET << endl;
            return true;
        }
        
        cout << RED << "Error: Cannot change owner: " << strerror(errno) << RESET << endl;
        return false;
    }
    
    // Display help
    void showHelp() {
        cout << BOLD << CYAN << "\n=== File Explorer Commands ===" << RESET << endl;
        cout << YELLOW << "Navigation:" << RESET << endl;
        cout << "  ls              - List files in current directory" << endl;
        cout << "  ll              - List files with detailed information" << endl;
        cout << "  cd <dir>        - Change directory" << endl;
        cout << "  pwd             - Print current directory" << endl;
        cout << endl;
        cout << YELLOW << "File Operations:" << RESET << endl;
        cout << "  mkdir <name>    - Create directory" << endl;
        cout << "  touch <name>    - Create file" << endl;
        cout << "  rm <name>       - Delete file or directory" << endl;
        cout << "  cp <src> <dst>  - Copy file" << endl;
        cout << "  mv <src> <dst>  - Move/rename file" << endl;
        cout << endl;
        cout << YELLOW << "Search:" << RESET << endl;
        cout << "  find <pattern>  - Search for files" << endl;
        cout << endl;
        cout << YELLOW << "Permissions:" << RESET << endl;
        cout << "  chmod <file> <mode> - Change permissions (e.g., chmod file.txt 755)" << endl;
        cout << "  chown <file> <user> - Change owner" << endl;
        cout << endl;
        cout << YELLOW << "Other:" << RESET << endl;
        cout << "  help            - Show this help" << endl;
        cout << "  exit            - Exit the application" << endl;
        cout << string(50, '=') << endl;
    }
};

int main() {
    FileExplorer explorer;
    string command;
    
    cout << BOLD << GREEN;
    cout << "╔════════════════════════════════════════╗" << endl;
    cout << "║   Linux File Explorer Application      ║" << endl;
    cout << "║   Type 'help' for available commands   ║" << endl;
    cout << "╚════════════════════════════════════════╝" << RESET << endl;
    
    while (true) {
        cout << BOLD << BLUE << "\n[" << explorer.getCurrentPath() << "]$ " << RESET;
        getline(cin, command);
        
        if (command.empty()) continue;
        
        stringstream ss(command);
        string cmd;
        ss >> cmd;
        
        if (cmd == "exit" || cmd == "quit") {
            cout << GREEN << "Goodbye!" << RESET << endl;
            break;
        }
        else if (cmd == "help") {
            explorer.showHelp();
        }
        else if (cmd == "ls") {
            explorer.listFiles(false);
        }
        else if (cmd == "ll") {
            explorer.listFiles(true);
        }
        else if (cmd == "pwd") {
            cout << explorer.getCurrentPath() << endl;
        }
        else if (cmd == "cd") {
            string path;
            ss >> path;
            if (path.empty()) {
                cout << RED << "Usage: cd <directory>" << RESET << endl;
            } else {
                explorer.changeDirectory(path);
            }
        }
        else if (cmd == "mkdir") {
            string name;
            ss >> name;
            if (name.empty()) {
                cout << RED << "Usage: mkdir <directory_name>" << RESET << endl;
            } else {
                explorer.createDirectory(name);
            }
        }
        else if (cmd == "touch") {
            string name;
            ss >> name;
            if (name.empty()) {
                cout << RED << "Usage: touch <file_name>" << RESET << endl;
            } else {
                explorer.createFile(name);
            }
        }
        else if (cmd == "rm") {
            string name;
            ss >> name;
            if (name.empty()) {
                cout << RED << "Usage: rm <file_or_directory>" << RESET << endl;
            } else {
                explorer.deleteItem(name);
            }
        }
        else if (cmd == "cp") {
            string src, dest;
            ss >> src >> dest;
            if (src.empty() || dest.empty()) {
                cout << RED << "Usage: cp <source> <destination>" << RESET << endl;
            } else {
                explorer.copy(src, dest);
            }
        }
        else if (cmd == "mv") {
            string src, dest;
            ss >> src >> dest;
            if (src.empty() || dest.empty()) {
                cout << RED << "Usage: mv <source> <destination>" << RESET << endl;
            } else {
                explorer.move(src, dest);
            }
        }
        else if (cmd == "find") {
            string pattern;
            ss >> pattern;
            if (pattern.empty()) {
                cout << RED << "Usage: find <pattern>" << RESET << endl;
            } else {
                explorer.search(pattern);
            }
        }
        else if (cmd == "chmod") {
            string name, perms;
            ss >> name >> perms;
            if (name.empty() || perms.empty()) {
                cout << RED << "Usage: chmod <file> <permissions>" << RESET << endl;
            } else {
                explorer.changePermissions(name, perms);
            }
        }
        else if (cmd == "chown") {
            string name, owner;
            ss >> name >> owner;
            if (name.empty() || owner.empty()) {
                cout << RED << "Usage: chown <file> <owner>" << RESET << endl;
            } else {
                explorer.changeOwner(name, owner);
            }
        }
        else {
            cout << RED << "Unknown command: " << cmd << RESET << endl;
            cout << "Type 'help' for available commands" << endl;
        }
    }
    
    return 0;
}
