#include "FileSync.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "用法: " << argv[0] << " <源目录> <目标目录>\n";
        return 1;
    }
    try {
        FileSync sync(argv[1], argv[2]);
        sync.sync();
        std::cout << "同步完成。\n";
    } catch (const std::exception& e) {
        std::cerr << "同步失败: " << e.what() << "\n";
        return 2;
    }
    return 0;
} 