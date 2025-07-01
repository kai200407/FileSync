cmake_minimum_required(VERSION 3.10)
project(FileSync)

set(CMAKE_CXX_STANDARD 17)

find_package(OpenSSL REQUIRED)

include_directories(include)

add_executable(FileSync src/main.cpp src/FileSync.cpp)
target_link_libraries(FileSync OpenSSL::Crypto) 