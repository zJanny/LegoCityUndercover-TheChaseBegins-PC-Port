#include "save/level_state.hpp"

#include <cmath>
#include <cstring>
#include <limits>

namespace chase::save {
namespace {
constexpr std::uint16_t kSavedParameterFlag = 2;
constexpr std::uint32_t kEndMarker = 0xffffffffu;
constexpr std::size_t kVectorBytes = 12;
constexpr std::size_t kTasksCompleteBytes = 4;

void appendU32(std::vector<std::uint8_t>& out, std::uint32_t value) {
    for (unsigned i = 0; i < 4; ++i) out.push_back(std::uint8_t(value >> (8 * i)));
}

std::uint32_t bitsOf(float value) {
    std::uint32_t bits;
    std::memcpy(&bits, &value, sizeof bits);
    return bits;
}

float floatOf(std::uint32_t bits) {
    float value;
    std::memcpy(&value, &bits, sizeof value);
    return value;
}

// vcvt.s32.f32: truncation with saturation, NaN gives 0.
std::int32_t truncated(float value) {
    if (std::isnan(value)) return 0;
    if (value >= 2147483648.0f) return std::numeric_limits<std::int32_t>::max();
    if (value <= -2147483648.0f) return std::numeric_limits<std::int32_t>::min();
    return std::int32_t(value);
}

float liveValue(const assets::LevelParam& param, const LevelStateInput& input) {
    if (input.variable)
        if (const std::optional<float> value = input.variable(param.hash)) return *value;
    return param.type == 0 ? floatOf(param.value) : float(param.value);
}

void appendRecord(std::vector<std::uint8_t>& out, std::uint32_t hash, const std::vector<std::uint8_t>& payload) {
    appendU32(out, hash);
    out.push_back(std::uint8_t(payload.size()));
    out.insert(out.end(), 3, 0);
    out.insert(out.end(), payload.begin(), payload.end());
}
}

std::optional<std::vector<std::uint8_t>> levelStateData(const assets::Level& level, const LevelStateInput& input) {
    std::vector<std::uint8_t> payload;
    for (const assets::LevelParam& param : level.defaults) {
        if (!(param.flags & kSavedParameterFlag)) continue;
        if (param.type == 0) appendU32(payload, bitsOf(liveValue(param, input)));
        else if (param.type == 5 || param.type == 7) appendU32(payload, std::uint32_t(truncated(liveValue(param, input))));
        else if (param.type == 1) {
            const std::vector<std::uint8_t> empty;
            const std::vector<std::uint8_t>& text = param.blocks.empty() ? empty : param.blocks[0];
            for (std::uint8_t byte : text) {
                if (byte == 0) break;
                payload.push_back(byte);
            }
            payload.push_back(0);
        } else if (param.type == 2) {
            for (std::size_t i = 0; i < kVectorBytes; ++i)
                payload.push_back(!param.blocks.empty() && i < param.blocks[0].size() ? param.blocks[0][i] : 0);
        }
    }
    std::vector<std::uint8_t> data;
    if (!payload.empty()) appendRecord(data, 0, payload);
    const float tasksComplete = input.variable ? input.variable(assets::nameHash("TASKS_COMPLETE")).value_or(0.0f) : 0.0f;
    std::vector<bool> liveWritten(input.liveRecords.size(), false);
    for (const ObjectRecord& record : input.keptRecords) {
        bool live = false;
        for (std::size_t i = 0; i < input.liveRecords.size() && !live; ++i) {
            if (input.liveRecords[i].hash != record.hash) continue;
            appendRecord(data, record.hash, input.liveRecords[i].payload);
            liveWritten[i] = true;
            live = true;
        }
        if (live) continue;
        if (record.payload.size() != kTasksCompleteBytes) {
            appendRecord(data, record.hash, record.payload);
            continue;
        }
        std::vector<std::uint8_t> value;
        appendU32(value, bitsOf(tasksComplete));
        appendRecord(data, record.hash, value);
    }
    for (std::size_t i = 0; i < input.liveRecords.size(); ++i)
        if (!liveWritten[i]) appendRecord(data, input.liveRecords[i].hash, input.liveRecords[i].payload);
    appendU32(data, kEndMarker);
    data.insert(data.end(), input.endBitSetBytes, 0);
    if (data.size() > kLevelStateCapacity) return std::nullopt;
    return data;
}

std::optional<std::int32_t> storyProgressOf(const assets::Level& level, const LevelStateInput& input) {
    const std::uint32_t hash = assets::nameHash("StoryProgress");
    for (const assets::LevelParam& param : level.defaults) {
        if (param.hash != hash) continue;
        if (param.type == 0 || param.type == 5 || param.type == 7) return truncated(liveValue(param, input));
        return 0;
    }
    return std::nullopt;
}

bool writeLevelState(std::vector<std::uint8_t>& slot, unsigned levelId, const std::vector<std::uint8_t>& data,
                     std::optional<std::int32_t> storyProgress) {
    const std::optional<std::size_t> offset = levelBlockOffset(levelId);
    if (!offset || *offset + kLevelBlockSize > slot.size() || data.size() > kLevelStateCapacity) return false;
    std::uint8_t* block = slot.data() + *offset;
    std::memset(block + kLevelStateDataOffset, 0, kLevelStateCapacity);
    std::memcpy(block + kLevelStateDataOffset, data.data(), data.size());
    block[kLevelStateLengthOffset] = std::uint8_t(data.size());
    block[kLevelStateLengthOffset + 1] = std::uint8_t(data.size() >> 8);
    block[kLevelStateSavedOffset] = 1;
    if (storyProgress) {
        block[kLevelStoryProgressOffset] = std::uint8_t(*storyProgress);
        block[kLevelStoryProgressOffset + 1] = std::uint8_t(*storyProgress >> 8);
    }
    return true;
}
}
