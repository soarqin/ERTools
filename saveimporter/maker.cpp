#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#undef WIN32_LEAN_AND_MEAN
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>

int wmain(int argc, wchar_t *argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: saveimporter_maker <input> <output>" << std::endl;
        return -1;
    }
    std::ifstream exe("saveimporter.exe", std::ios::binary);
    if (!exe.is_open()) {
        std::cerr << "Failed to open saveimporter.exe" << std::endl;
        return -1;
    }
    std::ifstream file(argv[1], std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open input file" << std::endl;
        return -1;
    }
    std::ofstream out(argv[2], std::ios::binary);
    if (!out.is_open()) {
        std::cerr << "Failed to open output file" << std::endl;
        return -1;
    }
    exe.seekg(0, std::ios::end);
    auto size = exe.tellg();
    exe.seekg(0, std::ios::beg);
    std::vector<uint8_t> data(size);
    exe.read(reinterpret_cast<char *>(data.data()), size);
    out.write(reinterpret_cast<char *>(data.data()), size);
    file.seekg(0, std::ios::end);
    size = file.tellg();
    file.seekg(0, std::ios::beg);
    data.resize(size);
    file.read(reinterpret_cast<char *>(data.data()), size);
    out.write(reinterpret_cast<char *>(data.data()), size);
    out.write(reinterpret_cast<char *>(&size), 4);
    return 0;
}
