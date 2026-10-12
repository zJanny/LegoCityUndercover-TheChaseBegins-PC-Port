#pragma once
// Save image "data:/savegame.dat" (plan step S4f); format in docs/re/save-format.md.
// The options and slot blocks stay opaque bytes. No SDL dependency.
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace chase::save {
using Bytes = std::vector<std::uint8_t>;

constexpr std::size_t kHeaderSize = 0x18;
constexpr std::size_t kOptionsSize = 0x21c;
constexpr std::size_t kSlotSize = 0x5230;
constexpr std::size_t kSlotCount = 2;
constexpr std::size_t kOptionsOffset = kHeaderSize;
constexpr std::size_t kFileSize = kHeaderSize + kOptionsSize + kSlotCount * kSlotSize;
constexpr std::uint16_t kVersion = 30;
constexpr char kMagic[] = "LEGO_ENGINE_SAVE";

// CRC routine 0x248308: reflected 0xedb88320, initial 0xffffffff, no final XOR.
std::uint32_t checksum(const std::uint8_t* data, std::size_t size);

// Values match the statuses of the game's load path (0x29ff4c, 0x1d0acc).
enum class Status {
    Ok = 0,
    Unreadable = 3,  // short read or bad magic
    Corrupt = 4,     // CRC, size or version mismatch
};

class SaveImage {
public:
    // Zero-filled blocks with a sealed header. The game's defaults (0x2081cc) are not reproduced.
    SaveImage();
    // Keeps the first kFileSize bytes, as the game's whole-buffer read does.
    static std::optional<SaveImage> fromBytes(const Bytes& data);

    Status validate() const;
    // Header writer 0x1fc604: magic, size, version, then the CRC over [0x18, end).
    void seal();

    std::uint32_t storedChecksum() const;
    std::uint16_t storedSize() const;
    std::uint16_t storedVersion() const;

    std::uint8_t* options() { return bytes_.data() + kOptionsOffset; }
    const std::uint8_t* options() const { return bytes_.data() + kOptionsOffset; }
    std::uint8_t* slot(std::size_t index);
    const std::uint8_t* slot(std::size_t index) const;
    const Bytes& bytes() const { return bytes_; }

private:
    Bytes bytes_;
};

// The file holds no valid save image unless status is Ok; image is set whenever the file existed.
struct LoadResult {
    bool found = false;
    Status status = Status::Unreadable;
    std::optional<SaveImage> image;
};

// $XDG_DATA_HOME/chase/savegame.dat, falling back to $HOME/.local/share. Throws when neither is set.
std::string defaultSavePath();
LoadResult load(const std::string& path);
// Seals a copy and replaces the file atomically (temporary file, fsync, rename, directory fsync).
// Creates missing parent directories. Throws std::runtime_error on failure.
void store(const std::string& path, SaveImage image);
}
