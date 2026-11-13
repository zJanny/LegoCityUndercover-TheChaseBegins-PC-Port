#include "save/save_directory.hpp"
#include <cstdlib>
#include <filesystem>
#include <stdexcept>

namespace chase::save {

std::string saveDirectory() {
    const char* dataHome = std::getenv("XDG_DATA_HOME");
    std::filesystem::path base;
    if (dataHome && *dataHome && std::filesystem::path(dataHome).is_absolute()) {
        base = dataHome;
    } else {
        const char* home = std::getenv("HOME");
        if (!home || !*home) throw std::runtime_error("neither XDG_DATA_HOME nor HOME is set");
        base = std::filesystem::path(home) / ".local" / "share";
    }
    return (base / "chase").string();
}

}
