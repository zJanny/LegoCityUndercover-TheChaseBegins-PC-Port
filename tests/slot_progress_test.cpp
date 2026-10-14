// Synthetic slots: level state block, saved variables, unlock bits and the game-complete test (docs/re/save-format.md).
#include "save/slot_progress.hpp"
#include <cstring>
#include <iostream>
#include <stdexcept>
using namespace chase::save;
using chase::assets::Level;
using chase::assets::LevelParam;

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

LevelParam param(const char* name, std::uint16_t flags, std::uint16_t type) {
    LevelParam p;
    p.hash = chase::assets::nameHash(name);
    p.flags = flags;
    p.type = type;
    return p;
}

void putFloat(Bytes& out, float value) {
    std::uint8_t raw[4];
    std::memcpy(raw, &value, 4);
    out.insert(out.end(), raw, raw + 4);
}

void putRecord(Bytes& out, std::uint32_t hash, const Bytes& payload) {
    for (int i = 0; i < 4; ++i) out.push_back(std::uint8_t(hash >> (8 * i)));
    out.push_back(std::uint8_t(payload.size()));
    out.insert(out.end(), 3, 0);
    out.insert(out.end(), payload.begin(), payload.end());
}

void writeLevelState(SaveImage& image, unsigned level, std::uint16_t storyProgress, const Bytes& data) {
    std::uint8_t* block = image.slot(0) + *levelBlockOffset(level);
    block[kLevelStateLengthOffset] = std::uint8_t(data.size());
    block[kLevelStateLengthOffset + 1] = std::uint8_t(data.size() >> 8);
    block[kLevelStoryProgressOffset] = std::uint8_t(storyProgress);
    block[kLevelStateSavedOffset] = 1;
    std::memcpy(block + kLevelStateDataOffset, data.data(), data.size());
}
}

int main() {
    try {
        Level level;
        level.defaults = {param("StoryProgress", 2, 0), param("ClassOnly", 1, 0), param("Name", 2, 1),
                          param("Spot", 2, 2), param("Flag", 2, 5), param("Reference", 2, 4), param("Counter", 2, 7)};

        Bytes payload;
        putFloat(payload, 3.0f);
        for (char c : std::string("lvl_x")) payload.push_back(std::uint8_t(c));
        payload.push_back(0);
        payload.insert(payload.end(), 12, 0);
        payload.insert(payload.end(), {1, 0, 0, 0});
        payload.insert(payload.end(), {4, 0, 0, 0});
        Bytes data;
        putRecord(data, 0x1234abcd, {7});
        putRecord(data, 0, payload);

        SaveImage image;
        writeLevelState(image, 2, 3, data);
        image.slot(0)[kLevelUnlockBitsOffset] = 0x10 | 0x20;
        image.slot(0)[kLevelUnlockBitsOffset + 1] = 0x01;

        const SlotView slot = image.slotView(0);
        const std::optional<LevelStateView> state = levelState(slot, 2);
        require(state && state->size == data.size() && state->storyProgress == 3, "level state view");
        require(!levelState(slot, 3), "level without saved state");
        require(!levelState(slot, 0) && !levelState(slot, 9), "level numbers outside 1..8");

        const auto variables = levelVariables(level, *state);
        require(variables && variables->size() == 3, "variable count");
        require((*variables)[0].hash == chase::assets::nameHash("StoryProgress") && (*variables)[0].value == 3.0f, "StoryProgress");
        require((*variables)[1].hash == chase::assets::nameHash("Flag") && (*variables)[1].value == 1.0f, "type 5 value");
        require((*variables)[2].hash == chase::assets::nameHash("Counter") && (*variables)[2].value == 4.0f, "type 7 value");

        Level shorter = level;
        shorter.defaults.pop_back();
        require(!levelVariables(shorter, *state), "record size must match the parameter table");

        require(levelUnlockBit(slot, 2, 0) == std::optional<bool>(true) && levelUnlockBit(slot, 2, 1) == std::optional<bool>(true), "level 2 bits");
        require(levelUnlockBit(slot, 3, 0) == std::optional<bool>(false), "level 3 locked");
        require(levelUnlockBit(slot, 4, 0) == std::optional<bool>(true), "level 4 bit 0 in byte 1");
        require(!levelUnlockBit(slot, 9, 0) && !levelUnlockBit(slot, 2, 2), "unlock bit range");

        const SlotStart start = slotStart(slot, level, 2, 8);
        require(start.stateSaved && start.storyProgress == 3 && start.variables.size() == 3, "slot start state");
        require(start.levelUnlocked == (1u << 2 | 1u << 4) && !start.gameComplete, "slot start unlocks");

        image.slot(0)[*levelBlockOffset(8) + 0x81f] = 7;
        require(gameComplete(image.slotView(0), 8) == std::optional<bool>(false), "7 of 8 is not complete");
        image.slot(0)[*levelBlockOffset(8) + 0x81f] = 8;
        require(gameComplete(image.slotView(0), 8) == std::optional<bool>(true), "8 of 8 is complete");

        SaveImage empty;
        const SlotStart fresh = slotStart(empty.slotView(1), level, 2, 8);
        require(!fresh.stateSaved && fresh.variables.empty() && fresh.levelUnlocked == 0, "empty slot");

        writeLevelState(image, 2, 3, Bytes(10, 1));
        bool threw = false;
        try { slotStart(image.slotView(0), level, 2, 8); } catch (const std::runtime_error&) { threw = true; }
        require(threw, "a state without a level-data record is rejected");
    } catch (const std::exception& error) {
        std::cerr << "slot-progress: FAIL: " << error.what() << '\n';
        return 1;
    }
    std::cout << "slot-progress: ok\n";
    return 0;
}
