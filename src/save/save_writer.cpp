#include "save/save_writer.hpp"

#include "save/continue_start.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>

namespace chase::save {
namespace {
constexpr std::uint32_t kHoursReciprocal = 0x136b06e7;
constexpr std::uint32_t kHoursTicksNegated = 0xfffff2d1;
constexpr std::uint32_t kMinutesReciprocal = 0x91a2b3c5;
constexpr std::uint32_t kMinutesTicksNegated = 0xffffff1f;
constexpr std::uint32_t kSecondsReciprocal = 0x88888889;
constexpr std::uint32_t kHoursLimit = 1000;

std::uint32_t readU32(const std::uint8_t* data) {
    std::uint32_t value;
    std::memcpy(&value, data, 4);
    return value;
}

void writeU32(std::uint8_t* data, std::uint32_t value) { std::memcpy(data, &value, 4); }
}

void setSlotPermille(std::uint8_t* slot, std::uint16_t permille) {
    slot[kSlotPermilleOffset] = std::uint8_t(permille);
    slot[kSlotPermilleOffset + 1] = std::uint8_t(permille >> 8);
}

bool writeSlotSummary(std::uint8_t* slot, std::size_t size, const SlotSummaryInput& input, std::uint16_t permille) {
    if (size < kSlotSize) return false;
    for (int i = 0; i < 8; ++i) slot[kSlotStudsOffset + i] = std::uint8_t(input.studs >> (8 * i));
    const std::uint8_t shifted = std::uint8_t(input.studType << 2) & kSlotStudFlagsMask;
    slot[kSlotStudFlagsOffset] = std::uint8_t((slot[kSlotStudFlagsOffset] & ~kSlotStudFlagsMask) | shifted);
    writeU32(slot + kSlotSuperBrickCountOffset, input.superBricks);
    setSlotPermille(slot, permille);
    return true;
}

std::uint8_t slotStudType(SlotView slot) {
    return slot.size > kSlotStudFlagsOffset ? std::uint8_t((slot.data[kSlotStudFlagsOffset] & kSlotStudFlagsMask) >> 2) : 0;
}

void setSlotStudType(std::uint8_t* slot, unsigned type) {
    constexpr unsigned kKeepType = 4;
    if (type == kKeepType) return;
    slot[kSlotStudFlagsOffset] = std::uint8_t((slot[kSlotStudFlagsOffset] & ~kSlotStudFlagsMask) | ((type & 7) << 2));
}

std::uint32_t playTicks(SlotView slot) { return slot.size >= 4 ? readU32(slot.data + kSlotPlayTicksOffset) : 0; }

void addPlayTick(std::uint8_t* slot) { writeU32(slot + kSlotPlayTicksOffset, readU32(slot + kSlotPlayTicksOffset) + 1); }

bool commitSlot(SaveImage& image, int activeSlot, const Bytes& liveSlot) {
    if (activeSlot < 0 || std::size_t(activeSlot) >= kSlotCount || liveSlot.size() != kSlotSize) return false;
    std::copy(liveSlot.begin(), liveSlot.end(), image.slot(std::size_t(activeSlot)));
    image.options()[2] = std::uint8_t(activeSlot);
    image.seal();
    return true;
}

SaveRequestWriter::SaveRequestWriter(std::string path, SaveImage base, unsigned slot, std::optional<CompletionTotals> totals)
    : path_(std::move(path)), image_(std::move(base)), slot_(slot), totals_(std::move(totals)) {}

bool SaveRequestWriter::request(Bytes& liveSlot, const SlotSummaryInput& input) {
    if (liveSlot.size() != kSlotSize) return false;
    const std::uint16_t permille = std::uint16_t(liveSlot[kSlotPermilleOffset] | liveSlot[kSlotPermilleOffset + 1] << 8);
    writeSlotSummary(liveSlot.data(), liveSlot.size(), input, permille);
    if (totals_) {
        const std::optional<std::uint16_t> computed = completionPermille(SlotView{liveSlot.data(), liveSlot.size()}, *totals_);
        if (computed) setSlotPermille(liveSlot.data(), *computed);
    }
    if (path_.empty()) return false;
    SaveImage next = image_;
    if (!commitSlot(next, int(slot_), liveSlot)) return false;
    store(path_, next);
    image_ = std::move(next);
    ++written_;
    return true;
}

std::optional<SlotRow> slotRow(SlotView slot) {
    if (slot.size <= kSlotCurrentLevelOffset) return std::nullopt;
    SlotRow row;
    row.used = slotInUse(slot);
    row.playTicks = playTicks(slot);
    row.permille = std::uint16_t(slot.data[kSlotPermilleOffset] | slot.data[kSlotPermilleOffset + 1] << 8);
    row.levelByte = std::int8_t(slot.data[kSlotCurrentLevelOffset]);
    return row;
}

PlayTime playTime(std::uint32_t ticks) {
    PlayTime time;
    const std::uint64_t scaled = std::uint64_t(kHoursReciprocal) + std::uint64_t(ticks) * kHoursReciprocal;
    time.hours = std::uint32_t(scaled >> 32) >> 13;
    time.overflow = time.hours >= kHoursLimit;
    const std::uint32_t inHour = ticks + ((time.hours * kHoursTicksNegated) << 5);
    time.minutes = std::uint32_t((std::uint64_t(kMinutesReciprocal) * inHour) >> 32) >> 10;
    const std::uint32_t inMinute = inHour + ((time.minutes * kMinutesTicksNegated) << 3);
    time.seconds = std::uint32_t((std::uint64_t(kSecondsReciprocal) * inMinute) >> 32) >> 4;
    return time;
}

std::string formatPlayTime(std::uint32_t ticks) {
    const PlayTime time = playTime(ticks);
    if (time.overflow) return "999:59:59";
    char buffer[32];
    if (time.hours != 0) std::snprintf(buffer, sizeof buffer, "%u:%02u:%02u", time.hours, time.minutes, time.seconds);
    else std::snprintf(buffer, sizeof buffer, "%02u:%02u", time.minutes, time.seconds);
    return buffer;
}

std::string formatPercent(std::uint16_t permille, std::uint8_t language) {
    const unsigned value = permille == 1 ? 0 : permille;
    if (permille == 1000) return "100%";
    char buffer[32];
    std::snprintf(buffer, sizeof buffer, "%u%s%u%%", value / 10, language == 1 ? "." : ",", value % 10);
    return buffer;
}

int locationLevel(std::int8_t levelByte) { return levelByte == 0 ? 1 : levelByte; }

std::optional<std::uint32_t> locationTextHash(const assets::Table& levels, int level) {
    if (level < 0 || levels.sheets.empty()) return std::nullopt;
    const assets::TableSheet& sheet = levels.sheets[0];
    if (std::size_t(level) >= sheet.rows || sheet.columns.size() <= kLocationHashColumn) return std::nullopt;
    const assets::TableValue value = assets::tableValue(sheet, std::size_t(level), kLocationHashColumn);
    if (!value.present) return std::nullopt;
    return value.bits;
}

}
