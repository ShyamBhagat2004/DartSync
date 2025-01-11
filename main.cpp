// main.cpp
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <sstream>        // Added for std::istringstream
#include <filesystem>     // Added for std::filesystem
#include "BackupManager.h"

#ifdef _WIN32
#include <windows.h>
#endif

namespace fs = std::filesystem;

// Function to get user input for file types
std::vector<std::string> getFileTypes() {
    std::vector<std::string> fileTypes;
    std::cout << "Enter file types to backup (e.g., .txt .cpp). Enter 'all' to backup all file types: ";
    std::string input;
    std::getline(std::cin, input);
    if (input == "all") {
        // Empty vector signifies all file types
        return fileTypes;
    } else {
        // Split input by spaces
        std::string type;
        std::istringstream iss(input);
        while (iss >> type) {
            if (type[0] != '.') {
                type = "." + type;
            }
            fileTypes.push_back(type);
        }
        return fileTypes;
    }
}

// Function to get a valid directory path from the user
std::string getDirectoryPath(const std::string& prompt) {
    std::string path;
    std::cout << prompt;
    std::getline(std::cin, path);

    // Remove surrounding quotes if present
    if (path.size() >= 2 && path.front() == '"' && path.back() == '"') {
        path = path.substr(1, path.size() - 2);
    }

    while (true) {
        if (!fs::exists(path)) {
            std::cout << "The path does not exist. Please enter a valid directory path: ";
        }
        else if (!fs::is_directory(path)) {
            std::cout << "The path is not a directory. Please enter a valid directory path: ";
        }
        else {
            break; // Valid path
        }

        std::getline(std::cin, path);

        // Remove surrounding quotes again
        if (path.size() >= 2 && path.front() == '"' && path.back() == '"') {
            path = path.substr(1, path.size() - 2);
        }
    }
    return path;
}

int main() {
    BackupManager backupManager;
    while (true) {
        std::cout << "=== DartSync10 Backup Service ===\n";
        std::cout << "1) Backup a folder once\n";
        std::cout << "2) Backup a folder on a schedule\n";
        std::cout << "3) Exit\n";
        std::cout << "Select an option: ";

        std::string choice;
        std::getline(std::cin, choice);

        if (choice == "1") {
            // Backup once
            std::string source = getDirectoryPath("Enter the source directory to backup: ");
            std::vector<std::string> fileTypes = getFileTypes();
            // Get output directory
            char* userProfile = getenv("USERPROFILE");
            std::string defaultDownload = userProfile ? std::string(userProfile) + "\\Downloads" : "C:\\Users\\Default\\Downloads";
            std::cout << "Enter the output directory to store backups [Default: " << defaultDownload << "]: ";
            std::string output;
            std::getline(std::cin, output);
            if (output.empty()) {
                output = defaultDownload;
            }
            backupManager.backupOnce(source, output, fileTypes);
        }
        else if (choice == "2") {
            // Backup on schedule
            std::string source = getDirectoryPath("Enter the source directory to backup: ");
            std::vector<std::string> fileTypes = getFileTypes();
            // Get output directory
            char* userProfile = getenv("USERPROFILE");
            std::string defaultDownload = userProfile ? std::string(userProfile) + "\\Downloads" : "C:\\Users\\Default\\Downloads";
            std::cout << "Enter the output directory to store backups [Default: " << defaultDownload << "]: ";
            std::string output;
            std::getline(std::cin, output);
            if (output.empty()) {
                output = defaultDownload;
            }
            // Get schedule interval
            std::cout << "Enter backup interval in seconds: ";
            int interval;
            std::cin >> interval;
            std::cin.ignore(); // Ignore remaining newline
            // Start scheduled backup
            std::cout << "Starting scheduled backups every " << interval << " seconds. Press Ctrl+C to exit.\n";
            backupManager.backupScheduled(source, output, fileTypes, interval);
        }
        else if (choice == "3") {
            std::cout << "Exiting DartSync10. Goodbye!\n";
            break;
        }
        else {
            std::cout << "Invalid option. Please try again.\n";
        }
    }
    return 0;
}
