#include "FileSync.hpp"
#include <filesystem>
#include <fstream>
#include <openssl/sha.h>
#include <vector>
#include <iostream>

namespace fs = std::filesystem;

FileSync::FileSync(const std::string& src, const std::string& dst)
    : srcDir(src), dstDir(dst) {}

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
            std::cout << "复制: " << relPath << std::endl;
        }
    }
    // 删除
    for (const auto& [relPath, dstInfo] : dstState) {
        if (srcState.find(relPath) == srcState.end()) {
            auto dstPath = fs::path(dstDir) / relPath;
            removeFile(dstPath);
            std::cout << "删除: " << relPath << std::endl;
        }
    }
} 