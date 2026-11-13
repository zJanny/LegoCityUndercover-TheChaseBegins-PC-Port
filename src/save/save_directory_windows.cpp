#include "save/save_directory.hpp"
#include <cstdlib>
#include <filesystem>
#include <stdexcept>

namespace chase::save {

std::string saveDirectory() {
    const char* appData = std::getenv("APPDATA");
    if (!appData || !*appData) throw std::runtime_error("APPDATA is not set");
    return (std::filesystem::path(appData) / "chase").string();
}

}
