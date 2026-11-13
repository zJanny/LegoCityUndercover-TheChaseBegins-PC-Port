// Default save path from APPDATA (Windows environment): %APPDATA%\chase\savegame.dat, WINPLAT-1.
#include "save/save.hpp"
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>

namespace {
void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

void setVariable(const char* name, const char* value) {
    if (_putenv_s(name, value) != 0) throw std::runtime_error(std::string("cannot set ") + name);
}

std::string failureOfDefaultSavePath() {
    try {
        chase::save::defaultSavePath();
    } catch (const std::runtime_error& error) {
        return error.what();
    }
    return "";
}
}

int main() {
    using namespace chase::save;
    try {
        setVariable("APPDATA", "C:\\Users\\u\\AppData\\Roaming");
        require(defaultSavePath() == "C:\\Users\\u\\AppData\\Roaming\\chase\\savegame.dat",
                "APPDATA path with backslashes, got " + defaultSavePath());
        setVariable("APPDATA", "C:/Users/u/AppData/Roaming");
        require(std::filesystem::path(defaultSavePath()) == std::filesystem::path("C:\\Users\\u\\AppData\\Roaming\\chase\\savegame.dat"),
                "APPDATA with slashes names the same file, got " + defaultSavePath());

        setVariable("APPDATA", "");
        require(failureOfDefaultSavePath() == "APPDATA is not set", "empty APPDATA: " + failureOfDefaultSavePath());
        _putenv("APPDATA=");
        require(failureOfDefaultSavePath() == "APPDATA is not set", "unset APPDATA: " + failureOfDefaultSavePath());

        const std::filesystem::path root =
            std::filesystem::temp_directory_path() / ("chase-save-default-path-" + std::to_string(std::random_device{}()));
        std::filesystem::create_directories(root);
        setVariable("APPDATA", root.string().c_str());
        SaveImage image;
        image.resetOptions(0);
        store(defaultSavePath(), image);
        require(std::filesystem::is_regular_file(root / "chase" / "savegame.dat"), "store creates the chase folder");
        require(load(defaultSavePath()).status == Status::Ok, "the saved file loads back");
        std::filesystem::remove_all(root);
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
    std::cout << "save default path tests passed\n";
    return 0;
}
