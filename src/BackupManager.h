#ifndef BACKUPMANAGER_H
#define BACKUPMANAGER_H

#include <string>
#include <vector>
#include <cstdint> // For uintmax_t

class BackupManager {
public:
    BackupManager();

    // Performs a one-time backup
    void backupOnce(const std::string& sourcePath,
                    const std::string& outputPath,
                    const std::vector<std::string>& fileTypes,
                    const std::string& keyword,
                    size_t maxFileSizeMB);

    // Performs a scheduled backup based on the scheduleType and interval
    void backupScheduled(const std::string& sourcePath,
                         const std::string& outputPath,
                         const std::vector<std::string>& fileTypes,
                         const std::string& keyword,
                         size_t maxFileSizeMB,
                         const std::string& scheduleType,
                         int intervalSeconds);

private:
    // Core backup functionality
    void performBackup(const std::string& sourcePath,
                       const std::string& outputPath,
                       const std::vector<std::string>& fileTypes,
                       const std::string& keyword,
                       size_t maxFileSizeMB);

    // Generates a versioned backup path based on the current timestamp
    std::string getVersionedPath(const std::string& destination);

    // Displays the backup progress based on bytes copied
    void displayProgress(uintmax_t bytesCopied, uintmax_t totalBytes);

    // Formats byte sizes into human-readable strings (e.g., KB, MB, GB)
    std::string formatSize(uintmax_t bytes) const;
};

#endif // BACKUPMANAGER_H
