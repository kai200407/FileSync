#pragma once
#include <string>
#include <unordered_map>
#include <filesystem>
#include <functional>
#include <mutex>
#include <vector>

struct FileInfo {
    std::string path;
    uintmax_t size;
    std::filesystem::file_time_type last_write;
    std::string hash; // 可选
};
using FileStateMap = std::unordered_map<std::string, FileInfo>;

class FileSync {
public:
    FileSync(const std::string& src, const std::string& dst);
    void sync(); // 单向增量同步
    void syncBidirectional(); // 双向同步
    void setConflictCallback(std::function<void(const std::string&)> cb);
private:
    std::string srcDir, dstDir;
    FileStateMap collectFileState(const std::string& root);
    std::string calcFileHash(const std::string& filename);
    void copyFile(const std::string& src, const std::string& dst);
    void removeFile(const std::string& path);
    void makeDir(const std::string& path);
    void logInfo(const std::string& msg);
    void logError(const std::string& msg);
    void handleConflict(const std::string& path);
    std::function<void(const std::string&)> conflictCallback;
    std::mutex mtx;
}; 