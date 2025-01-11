// BackupManager.h
#ifndef BACKUP_MANAGER_H
#define BACKUP_MANAGER_H

#include <string>
#include <vector>

class BackupManager {
public:
    BackupManager();
    void backupOnce(const std::string& sourcePath, const std::string& outputPath, const std::vector<std::string>& fileTypes);
    void backupScheduled(const std::string& sourcePath, const std::string& outputPath, const std::vector<std::string>& fileTypes, int intervalSeconds);
private:
    void performBackup(const std::string& sourcePath, const std::string& outputPath, const std::vector<std::string>& fileTypes);
    std::vector<std::string> getDefaultDownloadPath();
};

#endif // BACKUP_MANAGER_H

