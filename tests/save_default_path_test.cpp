// Default save path from XDG_DATA_HOME and HOME (POSIX environment).
#include "save/save.hpp"
#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
}

int main() {
    using chase::save::defaultSavePath;
    try {
        ::setenv("XDG_DATA_HOME", "/xdg", 1);
        require(defaultSavePath() == "/xdg/chase/savegame.dat", "XDG path");
        ::setenv("XDG_DATA_HOME", "relative", 1);
        ::setenv("HOME", "/home/u", 1);
        require(defaultSavePath() == "/home/u/.local/share/chase/savegame.dat", "relative XDG ignored");
        ::unsetenv("XDG_DATA_HOME");
        require(defaultSavePath() == "/home/u/.local/share/chase/savegame.dat", "HOME fallback");
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
    std::cout << "save default path tests passed\n";
    return 0;
}
