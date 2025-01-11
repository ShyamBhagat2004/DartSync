// BackupManager.cpp
#include "BackupManager.h"
#include <filesystem>
#include <iostream>
#include <chrono>
#include <thread>
#include <windows.h> // For Windows-specific operations

namespace fs = std::filesystem;

BackupManager::BackupManager() {
    // Constructor implementation (if needed)
}

void BackupManager::backupOnce(const std::string& sourcePath, const std::string& outputPath, const std::vector<std::string>& fileTypes) {
    performBackup(sourcePath, outputPath, fileTypes);
}

void BackupManager::backupScheduled(const std::string& sourcePath, const std::string& outputPath, const std::vector<std::string>& fileTypes, int intervalSeconds) {
    while (true) {
        performBackup(sourcePath, outputPath, fileTypes);
        std::this_thread::sleep_for(std::chrono::seconds(intervalSeconds));
    }
}

void BackupManager::performBackup(const std::string& sourcePath, const std::string& outputPath, const std::vector<std::string>& fileTypes) {
    try {
        fs::create_directories(outputPath);
        for (const auto& entry : fs::recursive_directory_iterator(sourcePath)) {
            if (fs::is_regular_file(entry.status())) {
                if (fileTypes.empty() || std::find(fileTypes.begin(), fileTypes.end(), entry.path().extension().string()) != fileTypes.end()) {
                    fs::path relativePath = fs::relative(entry.path(), sourcePath);
                    fs::path destination = fs::path(outputPath) / relativePath;
                    fs::create_directories(destination.parent_path());
                    fs::copy_file(entry.path(), destination, fs::copy_options::overwrite_existing);
                    std::cout << "Backed up: " << entry.path() << " to " << destination << std::endl;
                }
            }
        }
        std::cout << "Backup completed successfully." << std::endl;
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << std::endl;
    }
}

std::vector<std::string> BackupManager::getDefaultDownloadPath() {
    char* userProfile = getenv("USERPROFILE");
    if (userProfile) {
        return { std::string(userProfile) + "\\Downloads" };
    }
    return { "C:\\Users\\Default\\Downloads" };
}

