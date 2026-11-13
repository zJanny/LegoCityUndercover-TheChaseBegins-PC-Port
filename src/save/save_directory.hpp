#pragma once
#include <string>

namespace chase::save {

// Folder that holds the save file on this platform (POSIX or Windows file of the build). Throws std::runtime_error
// naming the environment variables when the folder cannot be found.
std::string saveDirectory();

}
