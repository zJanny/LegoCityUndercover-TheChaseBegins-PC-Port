// Atomic file replacement of the save writer, on the platform file of the build (POSIX or Windows).
#include "save/file_replace.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

std::vector<std::uint8_t> contents(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

int entries(const std::filesystem::path& directory) {
    int count = 0;
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        (void)entry;
        ++count;
    }
    return count;
}

std::string failure(const std::string& path, const std::vector<std::uint8_t>& data) {
    try {
        chase::save::replaceFile(path, data.data(), data.size());
    } catch (const std::runtime_error& error) {
        return error.what();
    }
    return "";
}
}

int main() {
    const std::filesystem::path dir =
        std::filesystem::temp_directory_path() / ("chase-file-replace-" + std::to_string(std::random_device{}()));
    try {
        std::filesystem::remove_all(dir);
        std::filesystem::create_directories(dir);
        const std::string path = (dir / "savegame.dat").string();

        const std::vector<std::uint8_t> first = {1, 2, 3, 0, 255};
        chase::save::replaceFile(path, first.data(), first.size());
        require(contents(path) == first, "new file holds the bytes");
        require(entries(dir) == 1, "no temporary file left after a new file");

        const std::vector<std::uint8_t> second(70000, 0x5a);
        chase::save::replaceFile(path, second.data(), second.size());
        require(contents(path) == second, "existing file replaced");
        require(entries(dir) == 1, "no temporary file left after a replace");

        const std::vector<std::uint8_t> empty;
        chase::save::replaceFile(path, empty.data(), empty.size());
        require(std::filesystem::file_size(path) == 0, "empty data gives an empty file");

        const std::string missing = (dir / "missing" / "savegame.dat").string();
        const std::string created = failure(missing, first);
        require(created.rfind("cannot create " + (dir / "missing").string(), 0) == 0, "missing folder: " + created);
        require(!std::filesystem::exists(dir / "missing"), "missing folder stays missing");

        const std::filesystem::path folder = dir / "folder";
        std::filesystem::create_directories(folder);
        const std::string replaced = failure(folder.string(), first);
        require(replaced.rfind("cannot replace " + folder.string() + ": ", 0) == 0, "folder target: " + replaced);
        require(entries(dir) == 2 && std::filesystem::is_directory(folder), "failed replace removes its temporary file");

        std::filesystem::remove_all(dir);
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        std::filesystem::remove_all(dir);
        return 1;
    }
    std::cout << "file replace tests passed\n";
    return 0;
}
