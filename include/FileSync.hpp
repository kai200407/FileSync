#pragma once
#include <string>
#include <unordered_map>
#include <filesystem>

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
private:
    std::string srcDir, dstDir;
    FileStateMap collectFileState(const std::string& root);
    std::string calcFileHash(const std::string& filename);
    void copyFile(const std::string& src, const std::string& dst);
    void removeFile(const std::string& path);
    void makeDir(const std::string& path);
}; 