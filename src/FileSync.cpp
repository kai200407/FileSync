#include "FileSync.hpp"
#include <filesystem>
#include <fstream>
#include <openssl/sha.h>
#include <vector>
#include <iostream>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

namespace fs = std::filesystem;

FileSync::FileSync(const std::string& src, const std::string& dst)
    : srcDir(src), dstDir(dst) {
    spdlog::set_default_logger(spdlog::basic_logger_mt("sync_logger", "sync.log"));
}

FileStateMap FileSync::collectFileState(const std::string& root) {
    FileStateMap state;
    for (const auto& entry : fs::recursive_directory_iterator(root)) {
        if (entry.is_regular_file()) {
            FileInfo info;
            info.path = fs::relative(entry.path(), root).string();
            info.size = fs::file_size(entry);
            info.last_write = fs::last_write_time(entry);
            // info.hash = calcFileHash(entry.path().string()); // 可选
            state[info.path] = info;
        }
    }
    return state;
}

std::string FileSync::calcFileHash(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    std::vector<char> buffer(8192);
    while (file.read(buffer.data(), buffer.size()) || file.gcount()) {
        SHA256_Update(&ctx, buffer.data(), file.gcount());
    }
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_Final(hash, &ctx);
    std::ostringstream oss;
    for (auto b : hash) oss << std::hex << std::setw(2) << std::setfill('0') << (int)b;
    return oss.str();
}

void FileSync::copyFile(const std::string& src, const std::string& dst) {
    fs::create_directories(fs::path(dst).parent_path());
    std::ifstream in(src, std::ios::binary);
    std::ofstream out(dst, std::ios::binary);
    std::vector<char> buffer(8192);
    while (in.read(buffer.data(), buffer.size()) || in.gcount()) {
        out.write(buffer.data(), in.gcount());
    }
}

void FileSync::removeFile(const std::string& path) {
    fs::remove(path);
}

void FileSync::makeDir(const std::string& path) {
    fs::create_directories(path);
}

void FileSync::logInfo(const std::string& msg) {
    std::lock_guard<std::mutex> lock(mtx);
    spdlog::info(msg);
}
void FileSync::logError(const std::string& msg) {
    std::lock_guard<std::mutex> lock(mtx);
    spdlog::error(msg);
}
void FileSync::setConflictCallback(std::function<void(const std::string&)> cb) {
    conflictCallback = cb;
}
void FileSync::handleConflict(const std::string& path) {
    logError("冲突: " + path);
    if (conflictCallback) conflictCallback(path);
}

void FileSync::sync() {
    auto srcState = collectFileState(srcDir);
    auto dstState = collectFileState(dstDir);
    // 新增和修改
    for (const auto& [relPath, srcInfo] : srcState) {
        auto dstPath = fs::path(dstDir) / relPath;
        bool needCopy = false;
        if (dstState.find(relPath) == dstState.end()) {
            needCopy = true; // 新增
        } else {
            const auto& dstInfo = dstState[relPath];
            if (srcInfo.size != dstInfo.size || srcInfo.last_write != dstInfo.last_write) {
                needCopy = true; // 修改
            }
        }
        if (needCopy) {
            copyFile(fs::path(srcDir) / relPath, dstPath);
            logInfo("复制: " + relPath);
        }
    }
    // 删除
    for (const auto& [relPath, dstInfo] : dstState) {
        if (srcState.find(relPath) == srcState.end()) {
            auto dstPath = fs::path(dstDir) / relPath;
            removeFile(dstPath);
            logInfo("删除: " + relPath);
        }
    }
}

void FileSync::syncBidirectional() {
    auto srcState = collectFileState(srcDir);
    auto dstState = collectFileState(dstDir);
    // 合并所有路径
    std::unordered_map<std::string, int> allPaths;
    for (const auto& [p, _] : srcState) allPaths[p] = 1;
    for (const auto& [p, _] : dstState) allPaths[p] |= 2;
    for (const auto& [relPath, flag] : allPaths) {
        auto srcIt = srcState.find(relPath);
        auto dstIt = dstState.find(relPath);
        bool inSrc = srcIt != srcState.end();
        bool inDst = dstIt != dstState.end();
        if (inSrc && inDst) {
            // 冲突检测：都被修改（可用时间戳或哈希）
            if (srcIt->second.size != dstIt->second.size || srcIt->second.last_write != dstIt->second.last_write) {
                // 时间戳优先策略
                if (srcIt->second.last_write > dstIt->second.last_write) {
                    copyFile(fs::path(srcDir) / relPath, fs::path(dstDir) / relPath);
                    logInfo("A->B 更新: " + relPath);
                } else if (srcIt->second.last_write < dstIt->second.last_write) {
                    copyFile(fs::path(dstDir) / relPath, fs::path(srcDir) / relPath);
                    logInfo("B->A 更新: " + relPath);
                } else {
                    // 冲突（同一时间戳但内容不同）
                    handleConflict(relPath);
                }
            }
        } else if (inSrc && !inDst) {
            // 只在A
            copyFile(fs::path(srcDir) / relPath, fs::path(dstDir) / relPath);
            logInfo("A->B 新增: " + relPath);
        } else if (!inSrc && inDst) {
            // 只在B
            copyFile(fs::path(dstDir) / relPath, fs::path(srcDir) / relPath);
            logInfo("B->A 新增: " + relPath);
        }
    }
} 