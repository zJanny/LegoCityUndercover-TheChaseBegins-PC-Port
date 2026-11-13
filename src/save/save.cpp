#include "save/save.hpp"
#include "save/file_replace.hpp"
#include "save/save_directory.hpp"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <stdexcept>

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

std::optional<std::size_t> SaveImage::lastSlot() const {
    std::uint8_t value = options()[2];
    if (value >= kSlotCount) return std::nullopt;
    return value;
}

void SaveImage::setOptionBit(int bit, bool on) {
    std::uint8_t mask = std::uint8_t(1u << bit);
    options()[3] = std::uint8_t(on ? options()[3] | mask : options()[3] & ~mask);
}

std::uint8_t& SaveImage::volume(std::size_t base, std::size_t slot) {
    if (slot >= kSlotCount) throw std::out_of_range("save slot index");
    return options()[base + slot];
}

std::uint8_t SaveImage::soundVolume(std::size_t slot) const { return const_cast<SaveImage*>(this)->volume(4, slot); }

void SaveImage::setSoundVolume(std::size_t slot, std::uint8_t value) {
    volume(4, slot) = std::min(value, kMaxVolume);
}

std::uint8_t SaveImage::musicVolume(std::size_t slot) const { return const_cast<SaveImage*>(this)->volume(6, slot); }

void SaveImage::setMusicVolume(std::size_t slot, std::uint8_t value) {
    volume(6, slot) = std::min(value, kMaxVolume);
}

std::string defaultSavePath() { return (std::filesystem::path(saveDirectory()) / "savegame.dat").string(); }

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
    replaceFile(path, image.bytes().data(), image.bytes().size());
}

SlotView SaveImage::slotView(std::size_t index) const { return {slot(index), kSlotSize}; }

namespace {
constexpr std::size_t kLevelOneOffset = 0x4928;
constexpr std::size_t kLevelTwoOffset = 0x8;
constexpr std::size_t kCollectedByteInBlock = 0x81f;
constexpr std::size_t kExtraSetOffsets[2] = {0x51b5, 0x51b8};
constexpr std::size_t kCharacterBitsOffset = 0x51bc;
constexpr std::size_t kIdSetOffset = 0x51e3;
constexpr std::size_t kFlagSetOffset = 0x51fa;
constexpr std::uint32_t kExtraSetItems = 2 * kExtraSetBits;

std::optional<bool> bitAt(SlotView slot, std::size_t base, std::size_t bit) {
    std::size_t byte = base + bit / 8;
    if (byte >= slot.size) return std::nullopt;
    return (slot.data[byte] >> (bit % 8) & 1) != 0;
}

bool validLevel(unsigned level) { return level >= kFirstCounterLevel && level <= kLastCounterLevel; }
}

std::optional<std::size_t> levelBlockOffset(unsigned level) {
    if (level == 1) return kLevelOneOffset;
    if (!validLevel(level)) return std::nullopt;
    return kLevelTwoOffset + (level - kFirstCounterLevel) * kLevelBlockSize;
}

std::optional<std::uint8_t> levelCollected(SlotView slot, unsigned level) {
    std::optional<std::size_t> block = levelBlockOffset(level);
    if (!block || *block + kCollectedByteInBlock >= slot.size) return std::nullopt;
    return slot.data[*block + kCollectedByteInBlock];
}

std::optional<bool> flagBit(SlotView slot, unsigned index) {
    if (index >= kFlagSetBits) return std::nullopt;
    return bitAt(slot, kFlagSetOffset, index);
}

std::optional<std::uint16_t> categoryCounter(SlotView slot, unsigned category, unsigned level) {
    if (category >= kLevelCategories || !validLevel(level)) return std::nullopt;
    if (category == 7) {
        std::uint16_t sum = 0;
        for (unsigned k = 0; k < 4; ++k) {
            std::optional<bool> flag = flagBit(slot, 4 * (level - kFirstCounterLevel) + k);
            if (!flag) return std::nullopt;
            sum = std::uint16_t(sum + *flag);
        }
        return sum;
    }
    std::size_t offset = *levelBlockOffset(level) + category * 2;
    if (offset + 2 > slot.size) return std::nullopt;
    return std::uint16_t(slot.data[offset] | slot.data[offset + 1] << 8);
}

std::optional<bool> characterBit(SlotView slot, unsigned id, unsigned which) {
    if (id >= kCharacterIdLimit || which > 1) return std::nullopt;
    return bitAt(slot, kCharacterBitsOffset, 2 * id + which);
}

std::optional<bool> extraSetBit(SlotView slot, unsigned set, unsigned index) {
    if (set > 1 || index >= kExtraSetBits) return std::nullopt;
    return bitAt(slot, kExtraSetOffsets[set], index);
}

std::optional<bool> idSetBit(SlotView slot, unsigned id, unsigned which) {
    if (id < kIdSetFirst || id > kIdSetLast || which > 1) return std::nullopt;
    return bitAt(slot, kIdSetOffset, 2 * (id - kIdSetFirst) + which);
}

std::optional<CompletionCounts> completionCounts(SlotView slot, const CompletionTotals& totals) {
    std::uint32_t collected = 0;
    std::uint32_t total = 0;
    auto count = [&](std::optional<bool> bit) {
        if (!bit) return false;
        collected += *bit;
        return true;
    };
    for (unsigned level = kFirstCounterLevel; level <= kLastCounterLevel; ++level) {
        std::optional<std::uint8_t> levelCount = levelCollected(slot, level);
        if (!levelCount) return std::nullopt;
        collected += *levelCount;
        total += totals.levelTotals[level - kFirstCounterLevel];
    }
    for (unsigned set = 0; set < 2; ++set)
        for (unsigned index = 0; index < kExtraSetBits; ++index)
            if (!count(extraSetBit(slot, set, index))) return std::nullopt;
    total += kExtraSetItems;
    for (unsigned id = kFirstCompletionCharacter; id < kCharacterIdLimit; ++id)
        for (unsigned which = 0; which < 2; ++which) {
            if (!count(characterBit(slot, id, which))) return std::nullopt;
            ++total;
        }
    for (unsigned id = kIdSetFirst; id <= kIdSetLast; ++id)
        for (unsigned which = 0; which < 2; ++which) {
            if (!count(idSetBit(slot, id, which))) return std::nullopt;
            ++total;
        }
    for (unsigned category = 0; category < kLevelCategories; ++category)
        for (unsigned level = kFirstCounterLevel; level <= kLastCounterLevel; ++level) {
            std::optional<std::uint16_t> counter = categoryCounter(slot, category, level);
            if (!counter) return std::nullopt;
            collected += *counter;
            total += totals.categoryTotals[category][level - kFirstCounterLevel];
        }
    for (std::uint8_t id : totals.excludedCharacters) {
        if (id < kFirstCompletionCharacter || id >= kCharacterIdLimit) return std::nullopt;
        --collected;
        --total;
    }
    return CompletionCounts{collected, total};
}

std::uint16_t completionPermille(std::uint32_t collected, std::uint32_t total) {
    if (total == 0) return 0;
    std::uint32_t permille = collected * 1000u / total;
    return std::uint16_t(std::min<std::uint32_t>(permille, 1000));
}

std::optional<std::uint16_t> completionPermille(SlotView slot, const CompletionTotals& totals) {
    std::optional<CompletionCounts> counts = completionCounts(slot, totals);
    if (!counts) return std::nullopt;
    return completionPermille(counts->collected, counts->total);
}
}
