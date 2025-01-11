#include "BackupManager.h"
#include <filesystem>
#include <iostream>
#include <chrono>
#include <thread>
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <mutex>

namespace fs = std::filesystem;

// Optional mutex for console output
static std::mutex coutMutex;

BackupManager::BackupManager() {
    // Constructor
}

void BackupManager::backupOnce(const std::string& sourcePath,
                               const std::string& outputPath,
                               const std::vector<std::string>& fileTypes,
                               const std::string& keyword,
                               size_t maxFileSizeMB)
{
    performBackup(sourcePath, outputPath, fileTypes, keyword, maxFileSizeMB);
}

void BackupManager::performBackup(const std::string& sourcePath,
                                  const std::string& outputPath,
                                  const std::vector<std::string>& fileTypes,
                                  const std::string& keyword,
                                  size_t maxFileSizeMB)
{
    try {
        {
            std::lock_guard<std::mutex> lock(coutMutex);
            std::cout << "Generating versioned backup directory...\n";
        }

        // Create or get versioned directory
        std::string versionedOutput = getVersionedPath(outputPath);
        fs::create_directories(versionedOutput);

        {
            std::lock_guard<std::mutex> lock(coutMutex);
            std::cout << "Backup directory created at: " << versionedOutput << "\n";
            std::cout << "Scanning for files to backup...\n";
        }

        // Gather files
        std::vector<fs::path> filesToBackup;
        uintmax_t totalBytes = 0;

        for (const auto& entry : fs::recursive_directory_iterator(sourcePath)) {
            if (fs::is_regular_file(entry.path())) {
                // File type filter
                if (!fileTypes.empty()) {
                    auto ext = entry.path().extension().string();
                    if (std::find(fileTypes.begin(), fileTypes.end(), ext) == fileTypes.end())
                        continue;
                }

                // Keyword filter
                if (!keyword.empty()) {
                    auto fname = entry.path().filename().string();
                    if (fname.find(keyword) == std::string::npos)
                        continue;
                }

                // Max file size filter
                uintmax_t fileSize = fs::file_size(entry.path());
                auto fileSizeMB = static_cast<size_t>(fileSize / (1024 * 1024));
                if (maxFileSizeMB > 0 && fileSizeMB > maxFileSizeMB)
                    continue;

                filesToBackup.push_back(entry.path());
                totalBytes += fileSize;
            }
        }

        {
            std::lock_guard<std::mutex> lock(coutMutex);
            std::cout << "Total files to backup: " << filesToBackup.size() << "\n";
            std::cout << "Total size to backup: " << formatSize(totalBytes) << "\n";
        }

        if (filesToBackup.empty()) {
            std::lock_guard<std::mutex> lock(coutMutex);
            std::cout << "No files match the backup criteria.\n";
            return;
        }

        {
            std::lock_guard<std::mutex> lock(coutMutex);
            std::cout << "Starting backup of " << filesToBackup.size() << " files...\n";
        }

        // Single-thread copying
        uintmax_t bytesCopied = 0;
        for (const auto& filePath : filesToBackup) {
            try {
                fs::path relativePath = fs::relative(filePath, sourcePath);
                fs::path destination = fs::path(versionedOutput) / relativePath;
                fs::create_directories(destination.parent_path());

                // Print which file is being copied (only filename)
                {
                    std::lock_guard<std::mutex> lock(coutMutex);
                    std::cout << "Copying file: " << filePath.filename().string() << "\n";
                }

                fs::copy_file(filePath, destination, fs::copy_options::overwrite_existing);
                bytesCopied += fs::file_size(filePath);

                // If you want progress, you could do something like:
                /*
                double progress = static_cast<double>(bytesCopied) / totalBytes * 100.0;
                {
                    std::lock_guard<std::mutex> lock(coutMutex);
                    std::cout << "Progress: " << progress << "%\n";
                }
                */
            }
            catch (const fs::filesystem_error& e) {
                std::lock_guard<std::mutex> lock(coutMutex);
                std::cerr << "Failed to copy " << filePath << ": " << e.what() << "\n";
            }
        }

        {
            std::lock_guard<std::mutex> lock(coutMutex);
            std::cout << "\nBackup completed successfully in directory: " << versionedOutput << "\n";
        }
    }
    catch (const fs::filesystem_error& e) {
        std::lock_guard<std::mutex> lock(coutMutex);
        std::cerr << "Filesystem error during backup: " << e.what() << "\n";
    }
    catch (const std::exception& e) {
        std::lock_guard<std::mutex> lock(coutMutex);
        std::cerr << "General error during backup: " << e.what() << "\n";
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

std::string BackupManager::formatSize(uintmax_t bytes) const {
    const char* sizes[] = { "B", "KB", "MB", "GB", "TB" };
    int order = 0;
    double len = static_cast<double>(bytes);

    while (len >= 1024.0 && order < 4) {
        order++;
        len /= 1024.0;
    }

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << len << " " << sizes[order];
    return oss.str();
}
