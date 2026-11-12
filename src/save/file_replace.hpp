#pragma once
#include <cstddef>
#include <cstdint>
#include <string>

namespace chase::save {

// Writes the bytes to a new file in the folder of path, flushes it to the disk and then replaces path with it in one
// step, so path holds either the old or the new bytes. The folder must exist. Errors throw std::runtime_error
// "<what> <file>: <reason>"; the new file is removed then.
void replaceFile(const std::string& path, const std::uint8_t* data, std::size_t size);

}
