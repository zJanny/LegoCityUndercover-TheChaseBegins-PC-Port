#pragma once
// Save image "data:/savegame.dat" (plan step S4f); format in docs/re/save-format.md.
// The options and slot blocks stay opaque bytes. No SDL dependency.
#include <array>
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

// Language index order from the sprite switch 0x31dc84 and the LANG_* text keys at 0x175d34.
enum class Language : std::uint8_t {
    English = 1,
    French,
    Italian,
    German,
    Spanish,
    Dutch,
    Danish,
    Portuguese,
    Russian,
};

// 0x21b650: 3DS system language (CFG block 0xA0002) to the game's language index.
std::uint8_t languageForSystem(std::uint8_t systemLanguage);

constexpr std::uint8_t kMaxVolume = 10;

// Values match the statuses of the game's load path (0x29ff4c, 0x1d0acc).
enum class Status {
    Ok = 0,
    Unreadable = 3,  // short read or bad magic
    Corrupt = 4,     // CRC, size or version mismatch
};

// Read-only view of one opaque slot block; every accessor bounds-checks against size.
struct SlotView {
    const std::uint8_t* data = nullptr;
    std::size_t size = 0;
};

class SaveImage {
public:
    // Zero-filled blocks with a sealed header. The game's defaults (0x2081cc) are not reproduced.
    SaveImage();
    // Keeps the first kFileSize bytes, as the game's whole-buffer read does.
    static std::optional<SaveImage> fromBytes(const Bytes& data);

    Status validate() const;
    // Options defaults from 0x2081cc and its callback 0x1a25a0; the rest of the block is zeroed.
    // `language` is the game's language index (see languageForSystem).
    void resetOptions(std::uint8_t language);
    // Header writer 0x1fc604: magic, size, version, then the CRC over [0x18, end).
    void seal();

    std::uint32_t storedChecksum() const;
    std::uint16_t storedSize() const;
    std::uint16_t storedVersion() const;

    std::uint8_t language() const { return options()[1]; }
    void setLanguage(std::uint8_t language) { options()[1] = language; }
    // Options +2; empty when no slot has been used (0xff).
    std::optional<std::size_t> lastSlot() const;
    bool subtitles() const { return options()[3] & 1; }
    void setSubtitles(bool on) { setOptionBit(0, on); }
    bool invertCamera() const { return options()[3] & 2; }
    void setInvertCamera(bool on) { setOptionBit(1, on); }
    // Per-slot volumes 0..kMaxVolume; the game divides them by 10.
    std::uint8_t soundVolume(std::size_t slot) const;
    void setSoundVolume(std::size_t slot, std::uint8_t volume);
    std::uint8_t musicVolume(std::size_t slot) const;
    void setMusicVolume(std::size_t slot, std::uint8_t volume);

    std::uint8_t* options() { return bytes_.data() + kOptionsOffset; }
    const std::uint8_t* options() const { return bytes_.data() + kOptionsOffset; }
    std::uint8_t* slot(std::size_t index);
    const std::uint8_t* slot(std::size_t index) const;
    const Bytes& bytes() const { return bytes_; }
    SlotView slotView(std::size_t index) const;

private:
    void setOptionBit(int bit, bool on);
    std::uint8_t& volume(std::size_t base, std::size_t slot);

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

// Slot progress layout, docs/re/save-format.md (2026-10-13T09:13 and 09:25).
constexpr std::size_t kLevelBlockSize = 0x820;
constexpr std::size_t kLevelCategories = 13;
constexpr unsigned kFirstCounterLevel = 2;
constexpr unsigned kLastCounterLevel = 8;
constexpr unsigned kCounterLevels = kLastCounterLevel - kFirstCounterLevel + 1;
constexpr unsigned kCharacterIdLimit = 141;
constexpr unsigned kFirstCompletionCharacter = 9;
constexpr unsigned kExtraSetBits = 21;
constexpr unsigned kIdSetFirst = 1;
constexpr unsigned kIdSetLast = 41;
constexpr unsigned kFlagSetBits = 28;

// 0x203464: block offset of a level, 1..8; empty otherwise.
std::optional<std::size_t> levelBlockOffset(unsigned level);
// Byte +0x81f of the level block (read by the completion function 0x1cd3dc).
std::optional<std::uint8_t> levelCollected(SlotView slot, unsigned level);
// 0x20a630 without the cheat flag: category 0..12, level 2..8. Category 7 sums four flagBit values.
std::optional<std::uint16_t> categoryCounter(SlotView slot, unsigned category, unsigned level);
// Set at +0x51fa, bit 0..27 (0x1ffa74 without the cheat flag).
std::optional<bool> flagBit(SlotView slot, unsigned index);
// Bits at +0x51bc, 2 per character id below 141: which 0 = unlocked, 1 = second state.
std::optional<bool> characterBit(SlotView slot, unsigned id, unsigned which);
// Two 21-bit sets at +0x51b5 (set 0) and +0x51b8 (set 1); index 0..20.
std::optional<bool> extraSetBit(SlotView slot, unsigned set, unsigned index);
// Bits at +0x51e3, 2 per id 1..41: which 0 or 1, without cheat flags.
std::optional<bool> idSetBit(SlotView slot, unsigned id, unsigned which);

// Table-derived totals the caller supplies; chase_save loads no tables.
struct CompletionTotals {
    std::array<std::uint8_t, kCounterLevels> levelTotals{};  // TablesLevels sheet 0, byte +0x40 of rows 2..8
    std::array<std::array<std::uint8_t, kCounterLevels>, kLevelCategories> categoryTotals{};  // TablesSuperCounters
    std::vector<std::uint8_t> excludedCharacters;  // ids 9..140 whose TablesCharacters byte +0xc is 1
};

struct CompletionCounts {
    std::uint32_t collected = 0;
    std::uint32_t total = 0;
};

// 0x1cd3dc without cheat flags. Empty when the slot is too short or an excluded id is outside 9..140.
// Arithmetic wraps like the game's 32-bit registers.
std::optional<CompletionCounts> completionCounts(SlotView slot, const CompletionTotals& totals);
// 0x1cd5e8: min(1000, collected * 1000 / total) as an unsigned truncating u16; 0 when total is 0.
std::uint16_t completionPermille(std::uint32_t collected, std::uint32_t total);
std::optional<std::uint16_t> completionPermille(SlotView slot, const CompletionTotals& totals);
}
