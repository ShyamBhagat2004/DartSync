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

void BackupManager::backupScheduled(const std::string& sourcePath,
                                    const std::string& outputPath,
                                    const std::vector<std::string>& fileTypes,
                                    const std::string& keyword,
                                    size_t maxFileSizeMB,
                                    const std::string& scheduleType,
                                    int intervalSeconds)
{
    while (true) {
        performBackup(sourcePath, outputPath, fileTypes, keyword, maxFileSizeMB);

        if (scheduleType == "daily") {
            std::this_thread::sleep_for(std::chrono::hours(24));
        }
        else if (scheduleType == "weekly") {
            std::this_thread::sleep_for(std::chrono::hours(24 * 7));
        }
        else if (scheduleType == "monthly") {
            // approximate 30 days
            std::this_thread::sleep_for(std::chrono::hours(24 * 30));
        }
        else if (scheduleType == "custom") {
            std::this_thread::sleep_for(std::chrono::seconds(intervalSeconds));
        }
        else {
            std::lock_guard<std::mutex> lock(coutMutex);
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
                                  size_t maxFileSizeMB)
{
    try {
        {
            std::lock_guard<std::mutex> lock(coutMutex);
            std::cout << "Generating versioned backup directory...\n";
        }

        std::string versionedOutput = getVersionedPath(outputPath);
        fs::create_directories(versionedOutput);

        {
            std::lock_guard<std::mutex> lock(coutMutex);
            std::cout << "Backup directory created at: " << versionedOutput << "\n";
            std::cout << "Scanning for files to backup...\n";
        }

        std::vector<fs::path> filesToBackup;
        uintmax_t totalBytes = 0;

        for (const auto& entry : fs::recursive_directory_iterator(sourcePath)) {
            if (fs::is_regular_file(entry.status())) {
                // File type filter
                if (!fileTypes.empty() &&
                    std::find(fileTypes.begin(), fileTypes.end(),
                              entry.path().extension().string()) == fileTypes.end()) {
                    continue;
                }

                // Keyword filter
                if (!keyword.empty() &&
                    entry.path().filename().string().find(keyword) == std::string::npos) {
                    continue;
                }

                // File size filter
                uintmax_t fileSize = fs::file_size(entry.path());
                size_t fileSizeMBCalc = (size_t)(fileSize / (1024 * 1024));
                if (maxFileSizeMB > 0 && fileSizeMBCalc > maxFileSizeMB) {
                    continue;
                }

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

        uintmax_t bytesCopied = 0;
        for (const auto& filePath : filesToBackup) {
            try {
                fs::path relativePath = fs::relative(filePath, sourcePath);
                fs::path destination = fs::path(versionedOutput) / relativePath;
                fs::create_directories(destination.parent_path());

                {
                    std::lock_guard<std::mutex> lock(coutMutex);
                    std::cout << "Copying file: "
                              << filePath.filename().string()
                              << " to "
                              << destination.filename().string() << "\n";
                }

                fs::copy_file(filePath, destination, fs::copy_options::overwrite_existing);

                uintmax_t fileSize = fs::file_size(filePath);
                bytesCopied += fileSize;

                displayProgress(bytesCopied, totalBytes);
            }
            catch (const fs::filesystem_error& e) {
                std::lock_guard<std::mutex> lock(coutMutex);
                std::cerr << "\nFailed to copy "
                          << filePath.filename().string()
                          << ": " << e.what() << "\n";
            }
            catch (const std::exception& e) {
                std::lock_guard<std::mutex> lock(coutMutex);
                std::cerr << "\nGeneral error copying "
                          << filePath.filename().string()
                          << ": " << e.what() << "\n";
            }
        }

        {
            std::lock_guard<std::mutex> lock(coutMutex);
            std::cout << "\nBackup completed successfully in directory: "
                      << versionedOutput << "\n";
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

void BackupManager::displayProgress(uintmax_t bytesCopied, uintmax_t totalBytes) {
    if (totalBytes == 0) return;

    double progress = (double)bytesCopied / totalBytes;
    double percentage = progress * 100.0;

    static int lastPercentage = -1;
    int currentPercentage = (int)percentage;
    if (currentPercentage != lastPercentage) {
        const int barWidth = 50;
        int pos = (int)(barWidth * progress);

        std::lock_guard<std::mutex> lock(coutMutex);
        std::cout << "\rProgress: [";
        for (int i = 0; i < barWidth; ++i) {
            if (i < pos) std::cout << "=";
            else if (i == pos) std::cout << ">";
            else std::cout << " ";
        }
        std::cout << "] " << std::fixed << std::setprecision(2)
                  << percentage << "%" << std::flush;

        lastPercentage = currentPercentage;
    }
}

std::string BackupManager::formatSize(uintmax_t bytes) const {
    const char* sizes[] = { "B", "KB", "MB", "GB", "TB" };
    int order = 0;
    double len = (double)bytes;
    while (len >= 1024.0 && order < 4) {
        order++;
        len /= 1024.0;
    }
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << len << " " << sizes[order];
    return oss.str();
}
