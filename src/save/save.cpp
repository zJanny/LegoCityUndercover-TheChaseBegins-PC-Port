#include "save/save.hpp"
#include <array>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <fcntl.h>
#include <unistd.h>

namespace chase::save {
namespace {
constexpr std::size_t kMagicSize = 16;
constexpr std::size_t kMagicOffset = 8;

std::array<std::uint32_t, 256> crcTable() {
    std::array<std::uint32_t, 256> table{};
    for (std::uint32_t i = 0; i < 256; ++i) {
        std::uint32_t value = i;
        for (int bit = 0; bit < 8; ++bit) value = (value & 1) ? (value >> 1) ^ 0xedb88320u : value >> 1;
        table[i] = value;
    }
    return table;
}

std::uint16_t readU16(const Bytes& data, std::size_t at) { return std::uint16_t(data[at] | data[at + 1] << 8); }

void writeU16(Bytes& data, std::size_t at, std::uint16_t value) {
    data[at] = std::uint8_t(value);
    data[at + 1] = std::uint8_t(value >> 8);
}

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

std::uint32_t checksum(const std::uint8_t* data, std::size_t size) {
    static const std::array<std::uint32_t, 256> table = crcTable();
    std::uint32_t crc = 0xffffffffu;
    for (std::size_t i = 0; i < size; ++i) crc = table[(crc ^ data[i]) & 0xff] ^ (crc >> 8);
    return crc;
}

std::uint8_t languageForSystem(std::uint8_t systemLanguage) {
    switch (systemLanguage) {
    case 2: return 2;
    case 3: return 4;
    case 4: return 3;
    case 5: return 5;
    case 7: return 12;
    case 8: return 6;
    case 9: return 8;
    case 10: return 9;
    default: return 1;
    }
}

SaveImage::SaveImage() : bytes_(kFileSize, 0) { seal(); }

std::optional<SaveImage> SaveImage::fromBytes(const Bytes& data) {
    if (data.size() < kFileSize) return std::nullopt;
    SaveImage image;
    std::copy(data.begin(), data.begin() + kFileSize, image.bytes_.begin());
    return image;
}

Status SaveImage::validate() const {
    if (std::memcmp(bytes_.data() + kMagicOffset, kMagic, kMagicSize) != 0) return Status::Unreadable;
    if (storedChecksum() != checksum(bytes_.data() + kHeaderSize, kFileSize - kHeaderSize) ||
        storedSize() != std::uint16_t(kFileSize) || storedVersion() != kVersion)
        return Status::Corrupt;
    return Status::Ok;
}

void SaveImage::resetOptions(std::uint8_t language) {
    std::uint8_t* block = options();
    std::fill(block, block + kOptionsSize, 0);
    block[0] = 0x8a;
    block[1] = language;
    block[2] = 0xff;
    block[3] = 0x21;
    block[4] = block[5] = 10;
    block[6] = block[7] = 8;
}

void SaveImage::seal() {
    std::fill(bytes_.begin(), bytes_.begin() + kHeaderSize, 0);
    std::memcpy(bytes_.data() + kMagicOffset, kMagic, kMagicSize);
    writeU16(bytes_, 4, std::uint16_t(kFileSize));
    writeU16(bytes_, 6, kVersion);
    std::uint32_t crc = checksum(bytes_.data() + kHeaderSize, kFileSize - kHeaderSize);
    for (int i = 0; i < 4; ++i) bytes_[std::size_t(i)] = std::uint8_t(crc >> (8 * i));
}

std::uint32_t SaveImage::storedChecksum() const {
    return std::uint32_t(bytes_[0]) | std::uint32_t(bytes_[1]) << 8 | std::uint32_t(bytes_[2]) << 16 |
           std::uint32_t(bytes_[3]) << 24;
}

std::uint16_t SaveImage::storedSize() const { return readU16(bytes_, 4); }

std::uint16_t SaveImage::storedVersion() const { return readU16(bytes_, 6); }

std::uint8_t* SaveImage::slot(std::size_t index) {
    if (index >= kSlotCount) throw std::out_of_range("save slot index");
    return bytes_.data() + kOptionsOffset + kOptionsSize + index * kSlotSize;
}

const std::uint8_t* SaveImage::slot(std::size_t index) const {
    return const_cast<SaveImage*>(this)->slot(index);
}

std::string defaultSavePath() {
    const char* dataHome = std::getenv("XDG_DATA_HOME");
    std::filesystem::path base;
    if (dataHome && *dataHome && std::filesystem::path(dataHome).is_absolute()) {
        base = dataHome;
    } else {
        const char* home = std::getenv("HOME");
        if (!home || !*home) throw std::runtime_error("neither XDG_DATA_HOME nor HOME is set");
        base = std::filesystem::path(home) / ".local" / "share";
    }
    return (base / "chase" / "savegame.dat").string();
}

LoadResult load(const std::string& path) {
    LoadResult result;
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        if (std::filesystem::exists(path)) throw std::runtime_error("cannot open " + path);
        return result;
    }
    result.found = true;
    Bytes data(kFileSize);
    file.read(reinterpret_cast<char*>(data.data()), std::streamsize(data.size()));
    data.resize(std::size_t(file.gcount()));
    result.image = SaveImage::fromBytes(data);
    if (result.image) result.status = result.image->validate();
    return result;
}

void store(const std::string& path, SaveImage image) {
    image.seal();
    std::filesystem::path target(path);
    std::filesystem::path directory = target.has_parent_path() ? target.parent_path() : ".";
    std::filesystem::create_directories(directory);
    std::string temporary = (directory / (target.filename().string() + ".tmp-XXXXXX")).string();
    int fd = ::mkstemp(temporary.data());
    if (fd < 0) throw systemError("cannot create", temporary);
    try {
        writeAll(fd, image.bytes().data(), image.bytes().size(), temporary);
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
