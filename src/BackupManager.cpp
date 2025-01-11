// BackupManager.cpp
#include "BackupManager.h"
#include <filesystem>
#include <iostream>
#include <chrono>
#include <thread>
#include <algorithm>
#include <fstream>
#include <iomanip> // For std::setw and std::setfill

namespace fs = std::filesystem;

BackupManager::BackupManager() {
    // Constructor implementation (if needed)
}

void BackupManager::backupOnce(const std::string& sourcePath,
                               const std::string& outputPath,
                               const std::vector<std::string>& fileTypes,
                               const std::string& keyword,
                               size_t maxFileSizeMB) {
    performBackup(sourcePath, outputPath, fileTypes, keyword, maxFileSizeMB);
}

void BackupManager::backupScheduled(const std::string& sourcePath,
                                    const std::string& outputPath,
                                    const std::vector<std::string>& fileTypes,
                                    const std::string& keyword,
                                    size_t maxFileSizeMB,
                                    const std::string& scheduleType,
                                    int intervalSeconds) {
    while (true) {
        performBackup(sourcePath, outputPath, fileTypes, keyword, maxFileSizeMB);

        // Determine sleep duration based on scheduleType
        if (scheduleType == "daily") {
            std::this_thread::sleep_for(std::chrono::hours(24));
        }
        else if (scheduleType == "weekly") {
            std::this_thread::sleep_for(std::chrono::hours(24 * 7));
        }
        else if (scheduleType == "monthly") {
            // Approximate a month as 30 days
            std::this_thread::sleep_for(std::chrono::hours(24 * 30));
        }
        else if (scheduleType == "custom") {
            std::this_thread::sleep_for(std::chrono::seconds(intervalSeconds));
        }
        else {
            std::cout << "Unknown schedule type. Defaulting to custom interval of "
                      << intervalSeconds << " seconds.\n";
            std::this_thread::sleep_for(std::chrono::seconds(intervalSeconds));
        }
    }
}

void BackupManager::performBackup(const std::string& sourcePath,
                                  const std::string& outputPath,
                                  const std::vector<std::string>& fileTypes,
                                  const std::string& keyword,
                                  size_t maxFileSizeMB) {
    try {
        // Generate a versioned backup directory
        std::string versionedOutput = getVersionedPath(outputPath);
        fs::create_directories(versionedOutput);

        // Gather all files that match the criteria
        std::vector<fs::path> filesToBackup;
        for (const auto& entry : fs::recursive_directory_iterator(sourcePath)) {
            if (fs::is_regular_file(entry.status())) {
                // File type filter
                if (!fileTypes.empty() &&
                    std::find(fileTypes.begin(), fileTypes.end(), entry.path().extension().string()) == fileTypes.end()) {
                    continue;
                }

                // Keyword filter
                if (!keyword.empty() && entry.path().filename().string().find(keyword) == std::string::npos) {
                    continue;
                }

                // File size filter
                size_t fileSizeMB = fs::file_size(entry.path()) / (1024 * 1024);
                if (maxFileSizeMB > 0 && fileSizeMB > maxFileSizeMB) {
                    continue;
                }

                filesToBackup.push_back(entry.path());
            }
        }

        size_t totalFiles = filesToBackup.size();
        if (totalFiles == 0) {
            std::cout << "No files match the backup criteria.\n";
            return;
        }

        std::cout << "Starting backup of " << totalFiles << " files...\n";

        size_t backedUpFiles = 0;
        for (const auto& filePath : filesToBackup) {
            // Calculate relative path
            fs::path relativePath = fs::relative(filePath, sourcePath);
            fs::path destination = fs::path(versionedOutput) / relativePath;
            fs::create_directories(destination.parent_path());

            // Copy file
            fs::copy_file(filePath, destination, fs::copy_options::overwrite_existing);
            backedUpFiles++;

            // Display progress
            displayProgress(backedUpFiles, totalFiles);
        }

        std::cout << "\nBackup completed successfully in directory: " << versionedOutput << "\n";
    }
    catch (const fs::filesystem_error& e) {
        std::cerr << "Filesystem error during backup: " << e.what() << "\n";
    }
}

std::string BackupManager::getVersionedPath(const std::string& destination) {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm_now;

#ifdef _WIN32
    localtime_s(&tm_now, &now_time_t);
#else
    localtime_r(&now_time_t, &tm_now);
#endif

    std::ostringstream oss;
    oss << std::put_time(&tm_now, "_%Y%m%d_%H%M%S");
    std::string timestamp = oss.str();

    fs::path destPath(destination);
    fs::path versionedPath = destPath / ("Backup" + timestamp);
    return versionedPath.string();
}

void BackupManager::displayProgress(size_t current, size_t total) {
    double progress = (static_cast<double>(current) / total) * 100.0;
    std::cout << "\rProgress: " << static_cast<int>(progress) << "%";
}
