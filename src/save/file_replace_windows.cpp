#include "save/file_replace.hpp"
#include <filesystem>
#include <random>
#include <stdexcept>
#include <system_error>
#include <windows.h>

namespace chase::save {
namespace {
std::runtime_error systemError(const std::string& what, const std::string& path, DWORD code) {
    return std::runtime_error(what + " " + path + ": " + std::system_category().message(int(code)));
}

std::string suffix(std::mt19937& random) {
    static const char kDigits[] = "0123456789abcdefghijklmnopqrstuvwxyz";
    std::string text(6, '0');
    for (char& c : text) c = kDigits[random() % 36];
    return text;
}

void writeAll(HANDLE file, const std::uint8_t* data, std::size_t size, const std::string& path) {
    while (size > 0) {
        DWORD chunk = size > 0x40000000u ? 0x40000000u : DWORD(size);
        DWORD written = 0;
        if (!WriteFile(file, data, chunk, &written, nullptr)) throw systemError("cannot write", path, GetLastError());
        data += written;
        size -= written;
    }
}
}

void replaceFile(const std::string& path, const std::uint8_t* data, std::size_t size) {
    std::filesystem::path target(path);
    std::filesystem::path directory = target.has_parent_path() ? target.parent_path() : ".";
    std::mt19937 random(std::random_device{}());
    std::filesystem::path temporary;
    HANDLE file = INVALID_HANDLE_VALUE;
    for (int attempt = 0; attempt < 100 && file == INVALID_HANDLE_VALUE; ++attempt) {
        temporary = directory / (target.filename().string() + ".tmp-" + suffix(random));
        file = CreateFileW(temporary.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file != INVALID_HANDLE_VALUE) break;
        const DWORD error = GetLastError();
        if (error != ERROR_FILE_EXISTS) throw systemError("cannot create", temporary.string(), error);
    }
    if (file == INVALID_HANDLE_VALUE) throw systemError("cannot create", temporary.string(), ERROR_FILE_EXISTS);
    try {
        writeAll(file, data, size, temporary.string());
        if (!FlushFileBuffers(file)) throw systemError("cannot sync", temporary.string(), GetLastError());
        HANDLE closing = file;
        file = INVALID_HANDLE_VALUE;
        if (!CloseHandle(closing)) throw systemError("cannot close", temporary.string(), GetLastError());
        if (!MoveFileExW(temporary.c_str(), target.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
            throw systemError("cannot replace", path, GetLastError());
    } catch (...) {
        if (file != INVALID_HANDLE_VALUE) CloseHandle(file);
        DeleteFileW(temporary.c_str());
        throw;
    }
}

}
