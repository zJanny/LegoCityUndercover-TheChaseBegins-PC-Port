#pragma once
// The save request of the original (0x21cf38) and what a used slot row shows: the slot summary written before the
// request (0x17caf8), the commit of a live slot into the file image (task 0x2c8e2c, state 0 at 0x2c8e5c) and the row
// fields of the slot list (builder 0x2cc298). docs/re/save-format.md. No SDL.
#include "assets/assets.hpp"
#include "save/save.hpp"

#include <cstdint>
#include <optional>
#include <string>

namespace chase::save {
constexpr std::size_t kSlotPlayTicksOffset = 0;
constexpr std::size_t kSlotPermilleOffset = 4;
constexpr std::size_t kSlotStudsOffset = 0x5148;
constexpr std::size_t kSlotStudFlagsOffset = 0x520b;
constexpr std::size_t kSlotSuperBrickCountOffset = 0x5190;
constexpr std::uint8_t kSlotStudFlagsMask = 0x1c;
constexpr unsigned kPlayTicksPerSecond = 30;
constexpr unsigned kLocationHashColumn = 11;  // TablesLevels sheet 0, row +8 of the table at 0x375914

// The live values 0x17caf8 copies into the slot: the stud counter (8 bytes at 0x593418), the super brick count (its u32
// at +0x18) and the stud type of the HUD counter (byte 0x519760 + 0x40, set by 0x22ddf4).
struct SlotSummaryInput {
    std::uint64_t studs = 0;
    std::uint32_t superBricks = 0;
    std::uint8_t studType = 0;
};

// 0x17caf8: studs at +0x5148, bits 2..4 of +0x520b from (studType << 2), the u32 at +0x5190, then the completion
// per mille (0x1cd3dc) at +4. Returns false when the slot block is shorter than kSlotSize.
bool writeSlotSummary(std::uint8_t* slot, std::size_t size, const SlotSummaryInput& input, std::uint16_t permille);

// Bits 2..4 of slot +0x520b. 0x22ddf4 writes them from the stud type of the HUD counter (byte 0x519760 + 0x40) each time a
// stud is banked and at each level load (0x2e5fd8, which reads them back into the byte); type 4 leaves the type unchanged.
std::uint8_t slotStudType(SlotView slot);
void setSlotStudType(std::uint8_t* slot, unsigned type);

// The per-update play time counter at slot +0 (incremented once per update by the four update loops 0x2d8a30,
// 0x2e5338, 0x2f26e8 and 0x300444).
std::uint32_t playTicks(SlotView slot);
void addPlayTick(std::uint8_t* slot);

// Task 0x2c8e5c: `activeSlot` is the signed byte at +0x3c of the object at 0x376844 and must be below the slot count
// (0x2a0568), otherwise the request does nothing and false is returned. Copies `liveSlot` (kSlotSize bytes) over the
// slot, stores the slot index at options +2 and seals the image (header writer 0x1fc604). The options block stays the
// image's own (the original copies the live options, which hold the same bytes).
bool commitSlot(SaveImage& image, int activeSlot, const Bytes& liveSlot);

void setSlotPermille(std::uint8_t* slot, std::uint16_t permille);

// The save request 0x21cf38(0) on the live slot: the summary 0x17caf8 (with the completion per mille when `totals` is
// given) is written into `liveSlot`, then the commit of task 0x2c8e5c copies it into the image, which is stored at `path`
// (atomic replace). With an empty path only the summary is written. The level state refresh 0x192f60 is the caller's (level_state.hpp).
// STAND-IN SAVE-S8: the store happens in the update of the request; the I/O time of the save thread and the single
// pending request of the queue 0x2400a0 are not modelled.
class SaveRequestWriter {
public:
    SaveRequestWriter() = default;
    SaveRequestWriter(std::string path, SaveImage base, unsigned slot, std::optional<CompletionTotals> totals);
    bool enabled() const { return !path_.empty(); }
    // With `liveOptions` (kOptionsSize bytes, the live block 0x51b490) the task first stores the slot index at live options +2 (0x22801c)
    // and copies the live options over the image's (0x1d0b80).
    bool request(Bytes& liveSlot, const SlotSummaryInput& input, std::uint8_t* liveOptions = nullptr);
    // The options save of the pause Options page (request 0x2400a0(0x1c34a4, ...)): the live options over the image's, then stored.
    // STAND-IN UI-S10: the task's file reload, compare (0x2c32bc, 0x1c36ac), question and I/O time are not modelled.
    bool storeOptions(const std::uint8_t* liveOptions);
    unsigned written() const { return written_; }
    const std::string& path() const { return path_; }
    const SaveImage& image() const { return image_; }

private:
    std::string path_;
    SaveImage image_;
    unsigned slot_ = 0;
    std::optional<CompletionTotals> totals_;
    unsigned written_ = 0;
};

struct SlotRow {
    bool used = false;
    std::uint32_t playTicks = 0;
    std::uint16_t permille = 0;
    std::int8_t levelByte = 0;
};
// Builder 0x2cc298: fields of one slot block. Empty when the block is too short.
std::optional<SlotRow> slotRow(SlotView slot);

struct PlayTime {
    bool overflow = false;  // 1000 hours or more: the row shows "999:59:59"
    std::uint32_t hours = 0, minutes = 0, seconds = 0;
};
// The arithmetic of 0x2cc39c..0x2cc3e0 on the 32-bit tick counter.
PlayTime playTime(std::uint32_t ticks);
// "H:MM:SS" (format "%u:%02u:%02u") from one hour, "MM:SS" ("%02u:%02u") below, "999:59:59" from 1000 hours.
std::string formatPlayTime(std::uint32_t ticks);
// Text_Percent: "100%" at 1000, "0.0%" at 1 (the row clears a value of 1), else permille / 10, the decimal separator
// of the language ("." for language 1, "," otherwise, 0x358a50 / 0x358a54) and permille % 10, then "%".
std::string formatPercent(std::uint16_t permille, std::uint8_t language);
// Level index whose TablesLevels name the Location text shows: byte 0 shows row 1 (the signed byte may be negative).
int locationLevel(std::int8_t levelByte);
// Column 11 of that row (the text hash 0x2cc504 passes to the text lookup 0x235fb4); empty outside the table.
std::optional<std::uint32_t> locationTextHash(const assets::Table& levels, int level);
}
