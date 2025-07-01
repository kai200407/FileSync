# FileSync

一个用 C++ 实现的高效文件同步工具，支持单向和增量同步，基于 C++17 `std::filesystem` 和 OpenSSL 哈希。

## 功能特性
- 单向/增量同步
- 支持文件/目录的创建、删除、修改、重命名
- 大文件分块同步
- 文件哈希校验（SHA-256）
- 错误处理与日志

## 编译

```bash
mkdir build && cd build
cmake ..
make
```

## 运行

```bash
./FileSync <源目录> <目标目录>
``` 