// BackupManager.h
#ifndef BACKUP_MANAGER_H
#define BACKUP_MANAGER_H

#include <string>
#include <vector>

class BackupManager {
public:
    BackupManager();

    // Perform a single backup with all filtering options
    void backupOnce(const std::string& sourcePath,
                   const std::string& outputPath,
                   const std::vector<std::string>& fileTypes,
                   const std::string& keyword,
                   size_t maxFileSizeMB);

    // Perform scheduled backups with advanced scheduling options
    void backupScheduled(const std::string& sourcePath,
                         const std::string& outputPath,
                         const std::vector<std::string>& fileTypes,
                         const std::string& keyword,
                         size_t maxFileSizeMB,
                         const std::string& scheduleType,
                         int intervalSeconds); // interval represents seconds for custom

private:
    // Core backup function applying all filters
    void performBackup(const std::string& sourcePath,
                      const std::string& outputPath,
                      const std::vector<std::string>& fileTypes,
                      const std::string& keyword,
                      size_t maxFileSizeMB);

    // Generates a versioned backup directory name based on the current timestamp
    std::string getVersionedPath(const std::string& destination);

    // Displays real-time progress updates
    void displayProgress(size_t current, size_t total);
};

#endif // BACKUP_MANAGER_H
