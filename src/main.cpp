#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include <filesystem>
#include <chrono>
#include <thread>
#include "BackupManager.h"

namespace fs = std::filesystem;

// Display help message
void displayHelp() {
    std::cout << "DartSync Backup Service\n\n";
    std::cout << "Usage:\n";
    std::cout << "  DartSync.exe <command> [options]\n\n";
    std::cout << "Commands:\n";
    std::cout << "  backup_once        Perform a single backup\n";
    std::cout << "  backup_scheduled   Perform backups on a schedule\n";
    std::cout << "  -h, --help         Display this help message\n\n";
    std::cout << "Options for 'backup_once':\n";
    std::cout << "  -s, --source <path>       Source directory to backup (required)\n";
    std::cout << "  -d, --dest <path>         Destination directory to store backups (required)\n";
    std::cout << "  -f, --filetypes <types>   File types to include (e.g., .txt .cpp). Use 'all' for all types\n";
    std::cout << "  -k, --keyword <keyword>   Keyword to filter files by name\n";
    std::cout << "  -m, --maxsize <MB>        Maximum file size in megabytes (0 for no limit)\n\n";
    std::cout << "Options for 'backup_scheduled':\n";
    std::cout << "  -s, --source <path>       Source directory to backup (required)\n";
    std::cout << "  -d, --dest <path>         Destination directory to store backups (required)\n";
    std::cout << "  -f, --filetypes <types>   File types to include (e.g., .txt .cpp). Use 'all' for all types\n";
    std::cout << "  -k, --keyword <keyword>   Keyword to filter files by name\n";
    std::cout << "  -m, --maxsize <MB>        Maximum file size in megabytes (0 for no limit)\n";
    std::cout << "  -t, --schedule <type>     Schedule type: daily, weekly, monthly, custom (required)\n";
    std::cout << "  -i, --interval <seconds>  Interval in seconds for 'custom' schedule\n\n";
    std::cout << "Examples:\n";
    std::cout << "  DartSync.exe backup_once --source \"C:\\MyFolder\" --dest \"D:\\Backup\" --filetypes .txt .cpp --keyword Report --maxsize 10\n";
    std::cout << "  DartSync.exe backup_scheduled --source \"C:\\MyFolder\" --dest \"D:\\Backup\" --filetypes all --schedule daily\n";
    std::cout << "  DartSync.exe backup_scheduled --source \"C:\\MyFolder\" --dest \"D:\\Backup\" --filetypes .txt --schedule custom --interval 3600\n";
}

// Parse file types from space-separated tokens
std::vector<std::string> parseFileTypes(const std::string& input) {
    std::vector<std::string> fileTypes;
    if (input == "all") {
        // Empty vector => all file types
        return fileTypes;
    } else {
        std::istringstream iss(input);
        std::string type;
        while (iss >> type) {
            // Ensure a leading dot
            if (!type.empty() && type[0] != '.') {
                type = "." + type;
            }
            fileTypes.push_back(type);
        }
    }
    return fileTypes;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Error: No command provided.\n\n";
        displayHelp();
        return 1;
    }

    std::string command = argv[1];
    BackupManager backupManager;

    if (command == "-h" || command == "--help") {
        displayHelp();
        return 0;
    }
    else if (command == "backup_once" || command == "backup_scheduled") {
        // Defaults
        std::string sourcePath;
        std::string outputPath;
        std::vector<std::string> fileTypes;
        std::string keyword;
        size_t maxFileSizeMB = 0;
        std::string scheduleType;
        int intervalSeconds = 0;

        // Parse command-line args
        for (int i = 2; i < argc; ++i) {
            std::string arg = argv[i];
            if ((arg == "-s") || (arg == "--source")) {
                if (i + 1 < argc) {
                    sourcePath = argv[++i];
                } else {
                    std::cerr << "Error: --source requires a value.\n";
                    return 1;
                }
            }
            else if ((arg == "-d") || (arg == "--dest")) {
                if (i + 1 < argc) {
                    outputPath = argv[++i];
                } else {
                    std::cerr << "Error: --dest requires a value.\n";
                    return 1;
                }
            }
            else if ((arg == "-f") || (arg == "--filetypes")) {
                if (i + 1 < argc) {
                    std::string typesInput = argv[++i];
                    fileTypes = parseFileTypes(typesInput);
                } else {
                    std::cerr << "Error: --filetypes requires a value.\n";
                    return 1;
                }
            }
            else if ((arg == "-k") || (arg == "--keyword")) {
                if (i + 1 < argc) {
                    keyword = argv[++i];
                } else {
                    std::cerr << "Error: --keyword requires a value.\n";
                    return 1;
                }
            }
            else if ((arg == "-m") || (arg == "--maxsize")) {
                if (i + 1 < argc) {
                    try {
                        maxFileSizeMB = std::stoul(argv[++i]);
                    }
                    catch (...) {
                        std::cerr << "Error: --maxsize requires a numerical value.\n";
                        return 1;
                    }
                } else {
                    std::cerr << "Error: --maxsize requires a value.\n";
                    return 1;
                }
            }
            else if ((arg == "-t") || (arg == "--schedule")) {
                if (i + 1 < argc) {
                    scheduleType = argv[++i];
                    std::transform(scheduleType.begin(), scheduleType.end(),
                                   scheduleType.begin(), ::tolower);
                } else {
                    std::cerr << "Error: --schedule requires a value.\n";
                    return 1;
                }
            }
            else if ((arg == "-i") || (arg == "--interval")) {
                if (i + 1 < argc) {
                    try {
                        intervalSeconds = std::stoi(argv[++i]);
                        if (intervalSeconds <= 0) {
                            std::cerr << "Error: --interval must be positive.\n";
                            return 1;
                        }
                    }
                    catch (...) {
                        std::cerr << "Error: --interval requires a numerical value.\n";
                        return 1;
                    }
                } else {
                    std::cerr << "Error: --interval requires a value.\n";
                    return 1;
                }
            }
            else {
                std::cerr << "Error: Unknown option '" << arg << "'.\n";
                displayHelp();
                return 1;
            }
        }

        // Check required options
        if (sourcePath.empty()) {
            std::cerr << "Error: Source directory is required.\n";
            return 1;
        }
        if (outputPath.empty()) {
            std::cerr << "Error: Destination directory is required.\n";
            return 1;
        }

        // Execute the command
        if (command == "backup_once") {
            backupManager.backupOnce(sourcePath, outputPath, fileTypes, keyword, maxFileSizeMB);
        } else {
            // "backup_scheduled"
            if (scheduleType.empty()) {
                std::cerr << "Error: Schedule type is required.\n";
                return 1;
            }
            if (scheduleType != "daily" && scheduleType != "weekly" &&
                scheduleType != "monthly" && scheduleType != "custom") {
                std::cerr << "Error: Invalid schedule type.\n";
                return 1;
            }
            if (scheduleType == "custom" && intervalSeconds == 0) {
                std::cerr << "Error: --interval is required for custom schedule.\n";
                return 1;
            }

            std::cout << "Starting scheduled backups (" << scheduleType << ")\n";
            backupManager.backupScheduled(
                sourcePath, outputPath, fileTypes, keyword, maxFileSizeMB,
                scheduleType, intervalSeconds
            );
        }
    }
    else {
        std::cerr << "Error: Unknown command '" << command << "'.\n\n";
        displayHelp();
        return 1;
    }

    return 0;
}
