#include "save/slot_progress.hpp"
#include <algorithm>
#include <cctype>
#include <cstring>
#include <stdexcept>

namespace chase::save {
namespace {
constexpr std::size_t kRecordHeader = 8;
constexpr std::uint32_t kEndMarker = 0xffffffffu;
constexpr std::uint16_t kSavedParameterFlag = 2;

std::uint32_t readU32(const std::uint8_t* p) {
    return std::uint32_t(p[0]) | std::uint32_t(p[1]) << 8 | std::uint32_t(p[2]) << 16 | std::uint32_t(p[3]) << 24;
}

std::string lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return text;
}

float floatOf(std::uint32_t bits) {
    float value;
    std::memcpy(&value, &bits, sizeof value);
    return value;
}
}

std::optional<LevelStateView> levelState(SlotView slot, unsigned level) {
    const std::optional<std::size_t> offset = levelBlockOffset(level);
    if (!offset || !slot.data || *offset + kLevelBlockSize > slot.size) return std::nullopt;
    const std::uint8_t* block = slot.data + *offset;
    if (block[kLevelStateSavedOffset] == 0) return std::nullopt;
    const std::size_t length = block[kLevelStateLengthOffset] | std::size_t(block[kLevelStateLengthOffset + 1]) << 8;
    if (length > kLevelStateCapacity) return std::nullopt;
    LevelStateView view;
    view.data = block + kLevelStateDataOffset;
    view.size = length;
    view.storyProgress = std::uint16_t(block[kLevelStoryProgressOffset] | block[kLevelStoryProgressOffset + 1] << 8);
    return view;
}

std::optional<bool> levelUnlockBit(SlotView slot, unsigned level, unsigned which) {
    if (level >= kUnlockLevels || which > 1 || !slot.data) return std::nullopt;
    const std::size_t bit = 2 * std::size_t(level) + which;
    if (kLevelUnlockBitsOffset + bit / 8 >= slot.size) return std::nullopt;
    return (slot.data[kLevelUnlockBitsOffset + bit / 8] >> (bit % 8) & 1) != 0;
}

std::optional<bool> gameComplete(SlotView slot, std::uint8_t finalLevelTotal) {
    const std::optional<std::uint8_t> collected = levelCollected(slot, kGameCompleteLevel);
    if (!collected) return std::nullopt;
    return *collected >= finalLevelTotal;
}

std::optional<unsigned> levelIdForScene(const assets::Table& levels, const std::string& sceneName) {
    if (levels.sheets.empty()) return std::nullopt;
    const assets::TableSheet& sheet = levels.sheets[0];
    if (sheet.rows <= kLastCounterLevel || sheet.columns.size() <= kLevelNameColumn) return std::nullopt;
    for (unsigned row = 1; row <= kLastCounterLevel; ++row) {
        const assets::TableValue name = assets::tableValue(sheet, row, kLevelNameColumn);
        if (name.present && lower(name.text) == lower(sceneName)) return row;
    }
    return std::nullopt;
}

std::optional<std::vector<SavedVariable>> levelVariables(const assets::Level& level, const LevelStateView& state) {
    std::size_t at = 0;
    while (at + kRecordHeader <= state.size) {
        const std::uint32_t hash = readU32(state.data + at);
        if (hash == kEndMarker) return std::nullopt;
        const std::size_t size = state.data[at + 4];
        if (at + kRecordHeader + size > state.size) return std::nullopt;
        if (hash != 0) {
            at += kRecordHeader + size;
            continue;
        }
        const std::uint8_t* payload = state.data + at + kRecordHeader;
        std::size_t used = 0;
        std::vector<SavedVariable> variables;
        for (const assets::LevelParam& param : level.defaults) {
            if (!(param.flags & kSavedParameterFlag)) continue;
            std::size_t width = 0;
            if (param.type == 0 || param.type == 5 || param.type == 7) width = 4;
            else if (param.type == 1) {
                const std::uint8_t* end = std::find(payload + used, payload + size, std::uint8_t(0));
                if (end == payload + size) return std::nullopt;
                width = std::size_t(end - (payload + used)) + 1;
            } else if (param.type == 2) width = 12;
            if (used + width > size) return std::nullopt;
            if (param.type == 0) variables.push_back({param.hash, floatOf(readU32(payload + used))});
            else if (param.type == 5 || param.type == 7) variables.push_back({param.hash, float(readU32(payload + used))});
            used += width;
        }
        if (used != size) return std::nullopt;
        return variables;
    }
    return std::nullopt;
}

std::optional<ObjectRecords> objectRecords(const LevelStateView& state) {
    ObjectRecords out;
    std::size_t at = 0;
    while (true) {
        if (at + 4 > state.size) return std::nullopt;
        const std::uint32_t hash = readU32(state.data + at);
        if (hash == kEndMarker) {
            at += 4;
            break;
        }
        if (at + kObjectRecordHeader > state.size) return std::nullopt;
        const std::size_t size = state.data[at + 4];
        if (at + kObjectRecordHeader + size > state.size) return std::nullopt;
        if (hash != 0) {
            const std::uint8_t* payload = state.data + at + kObjectRecordHeader;
            out.records.push_back({hash, std::vector<std::uint8_t>(payload, payload + size)});
        }
        at += kObjectRecordHeader + size;
    }
    out.endBitSetBytes = state.size - at;
    out.endBitSetZero = std::all_of(state.data + at, state.data + state.size, [](std::uint8_t b) { return b == 0; });
    return out;
}

std::optional<unsigned> slotCurrentLevel(SlotView slot) {
    if (!slot.data || slot.size <= kSlotCurrentLevelOffset) return std::nullopt;
    return slot.data[kSlotCurrentLevelOffset];
}

std::vector<SavedVariable> tasksCompleteVariables(const ObjectRecords& records) {
    std::vector<SavedVariable> variables;
    const std::uint32_t hash = assets::nameHash("TASKS_COMPLETE");
    for (const ObjectRecord& record : records.records)
        if (record.payload.size() == kTasksCompleteRecordSize) variables.push_back({hash, floatOf(readU32(record.payload.data()))});
    return variables;
}

SlotStart slotStart(SlotView slot, const assets::Level& level, unsigned levelId, std::uint8_t finalLevelTotal) {
    SlotStart start;
    start.level = levelId;
    for (unsigned id = 0; id < kUnlockLevels; ++id) {
        const std::optional<bool> unlocked = levelUnlockBit(slot, id, 0);
        if (!unlocked) throw std::runtime_error("slot is too short for the level unlock bits");
        if (*unlocked) start.levelUnlocked |= 1u << id;
    }
    const std::optional<bool> complete = gameComplete(slot, finalLevelTotal);
    if (!complete) throw std::runtime_error("slot is too short for the level 8 block");
    start.gameComplete = *complete;
    if (const std::optional<LevelStateView> state = levelState(slot, levelId)) {
        const std::optional<std::vector<SavedVariable>> variables = levelVariables(level, *state);
        if (!variables) throw std::runtime_error("the saved state of the level does not match its parameter table");
        start.stateSaved = true;
        start.storyProgress = state->storyProgress;
        start.variables = *variables;
        const std::optional<ObjectRecords> objects = objectRecords(*state);
        if (!objects) throw std::runtime_error("the object records of the saved state are malformed");
        const std::vector<SavedVariable> tasks = tasksCompleteVariables(*objects);
        start.variables.insert(start.variables.end(), tasks.begin(), tasks.end());
        start.objectRecords = objects->records;
    }
    start.currentLevel = slotCurrentLevel(slot);
    return start;
}
}
