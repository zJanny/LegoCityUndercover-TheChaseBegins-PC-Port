#include "save/file_replace.hpp"
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <stdexcept>
#include <fcntl.h>
#include <unistd.h>

namespace chase::save {
namespace {
std::runtime_error systemError(const std::string& what, const std::string& path) {
    return std::runtime_error(what + " " + path + ": " + std::strerror(errno));
}

void writeAll(int fd, const std::uint8_t* data, std::size_t size, const std::string& path) {
    while (size > 0) {
        ssize_t written = ::write(fd, data, size);
        if (written < 0) {
            if (errno == EINTR) continue;
            throw systemError("cannot write", path);
        }
        data += written;
        size -= std::size_t(written);
    }
}

void syncDirectory(const std::filesystem::path& directory) {
    int fd = ::open(directory.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (fd < 0) throw systemError("cannot open directory", directory.string());
    int result = ::fsync(fd);
    ::close(fd);
    if (result != 0) throw systemError("cannot sync directory", directory.string());
}
}

void replaceFile(const std::string& path, const std::uint8_t* data, std::size_t size) {
    std::filesystem::path target(path);
    std::filesystem::path directory = target.has_parent_path() ? target.parent_path() : ".";
    std::string temporary = (directory / (target.filename().string() + ".tmp-XXXXXX")).string();
    int fd = ::mkstemp(temporary.data());
    if (fd < 0) throw systemError("cannot create", temporary);
    try {
        writeAll(fd, data, size, temporary);
        if (::fsync(fd) != 0) throw systemError("cannot sync", temporary);
        if (::close(fd) != 0) {
            fd = -1;
            throw systemError("cannot close", temporary);
        }
        fd = -1;
        if (::rename(temporary.c_str(), path.c_str()) != 0) throw systemError("cannot replace", path);
    } catch (...) {
        if (fd >= 0) ::close(fd);
        ::unlink(temporary.c_str());
        throw;
    }
    syncDirectory(directory);
}

}
