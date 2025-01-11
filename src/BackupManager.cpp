#include "BackupManager.h"
#include <filesystem>
#include <iostream>
#include <chrono>
#include <thread>
#include <algorithm>
#include <fstream>
#include <iomanip>   // For std::setw, std::setprecision
#include <sstream>   // For std::ostringstream
#include <mutex>     // For std::mutex
#include <atomic>    // For std::atomic
#include <queue>     // For std::queue

namespace fs = std::filesystem;

// Global mutex for thread-safe console output
std::mutex coutMutex;

BackupManager::BackupManager() {
    // Constructor (if needed)
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

        // Gather matching files and compute total size
        std::vector<fs::path> filesToBackup;
        uintmax_t totalBytes = 0;

        for (const auto& entry : fs::recursive_directory_iterator(sourcePath)) {
            // Only regular files
            if (fs::is_regular_file(entry.path())) {
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
                size_t fileSizeMB = static_cast<size_t>(fileSize / (1024 * 1024));
                if (maxFileSizeMB > 0 && fileSizeMB > maxFileSizeMB) {
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

        // Put files in a queue for multithreaded copying
        std::queue<fs::path> fileQueue;
        for (const auto& filePath : filesToBackup) {
            fileQueue.push(filePath);
        }

        // For concurrency
        std::atomic<uintmax_t> bytesCopied{0};
        std::mutex queueMutex;

        // Worker thread lambda
        auto worker = [&](int threadId) {
            while (true) {
                fs::path filePath;
                {
                    std::lock_guard<std::mutex> lk(queueMutex);
                    if (fileQueue.empty()) {
                        break;
                    }
                    filePath = fileQueue.front();
                    fileQueue.pop();
                }

                try {
                    // Only show the filename, not the full path
                    std::string fileName = filePath.filename().string();

                    // Print the file name being copied
                    {
                        std::lock_guard<std::mutex> lock(coutMutex);
                        std::cout << "Copying file: " << fileName << "\n";
                    }

                    // Compute destination path
                    fs::path relativePath = fs::relative(filePath, sourcePath);
                    fs::path destination = fs::path(versionedOutput) / relativePath;
                    fs::create_directories(destination.parent_path());

                    // Copy the file
                    fs::copy_file(filePath, destination, fs::copy_options::overwrite_existing);

                    // Update the total bytes copied
                    uintmax_t fileSize = fs::file_size(filePath);
                    bytesCopied.fetch_add(fileSize, std::memory_order_relaxed);

                    // Display progress on new line each time percentage changes
                    displayProgress(bytesCopied.load(std::memory_order_relaxed), totalBytes);
                }
                catch (const fs::filesystem_error& e) {
                    std::lock_guard<std::mutex> lock(coutMutex);
                    std::cerr << "[Thread " << threadId << "] Failed to copy "
                              << filePath.filename().string() << ": " << e.what() << "\n";
                }
                catch (const std::exception& e) {
                    std::lock_guard<std::mutex> lock(coutMutex);
                    std::cerr << "[Thread " << threadId << "] Error copying "
                              << filePath.filename().string() << ": " << e.what() << "\n";
                }
            }
        };

        // Spawn threads
        unsigned int numThreads = std::max(1u, std::thread::hardware_concurrency());
        std::vector<std::thread> threads;
        threads.reserve(numThreads);

        for (unsigned int i = 0; i < numThreads; ++i) {
            threads.emplace_back(worker, i);
        }

        // Join threads
        for (auto& t : threads) {
            if (t.joinable()) {
                t.join();
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

// Displays progress on a new line each time the integer percentage changes
void BackupManager::displayProgress(uintmax_t bytesCopied, uintmax_t totalBytes) {
    if (totalBytes == 0) return; // Avoid division by zero

    double progress = static_cast<double>(bytesCopied) / totalBytes;
    double percentage = progress * 100.0;

    // We'll use a static mutex so progress updates don't collide
    static std::mutex progressMutex;
    std::lock_guard<std::mutex> lock(progressMutex);

    // Only update on integer percentage change
    static int lastPercentage = -1;
    int currentPercentage = static_cast<int>(percentage);

    if (currentPercentage != lastPercentage) {
        const int barWidth = 50;
        int pos = static_cast<int>(barWidth * progress);

        // Print a new line for each progress update
        std::cout << "Progress: [";
        for (int i = 0; i < barWidth; ++i) {
            if (i < pos) std::cout << "=";
            else if (i == pos) std::cout << ">";
            else std::cout << " ";
        }
        std::cout << "] " << std::fixed << std::setprecision(2)
                  << percentage << "%" << std::endl;

        lastPercentage = currentPercentage;
    }
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
