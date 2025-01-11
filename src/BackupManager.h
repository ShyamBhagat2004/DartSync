#ifndef BACKUPMANAGER_H
#define BACKUPMANAGER_H

#include <string>
#include <vector>
#include <cstdint> // For uintmax_t

class BackupManager {
public:
    BackupManager();

    // Performs a one-time backup (single-threaded)
    void backupOnce(const std::string& sourcePath,
                    const std::string& outputPath,
                    const std::vector<std::string>& fileTypes,
                    const std::string& keyword,
                    size_t maxFileSizeMB);

private:
    // Core backup functionality
    void performBackup(const std::string& sourcePath,
                       const std::string& outputPath,
                       const std::vector<std::string>& fileTypes,
                       const std::string& keyword,
                       size_t maxFileSizeMB);

    // Generates a versioned backup path based on current timestamp
    std::string getVersionedPath(const std::string& destination);

    // Utility: Formats byte sizes into human-readable strings (e.g., KB, MB, GB)
    std::string formatSize(uintmax_t bytes) const;
};

#endif // BACKUPMANAGER_H
