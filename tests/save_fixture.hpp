#pragma once
// The save the save tests and the chase-game save tests start from, built at test time with the port's own save code from
// named values (no save file is read or committed). The values are the progress of slot 0 of the user's save as read on
// 2026-10-20 (mission 1-1 done, Continue starts in the police station, 1130 studs, 52 super bricks); the field offsets are those
// of docs/re/save-format.md and docs/re/save-format-state.md. Fields the values do not name stay as a new game leaves them
// (newGameSlot). The 48 object records of level 2 and the record of level 1 keep the sizes and the 4-byte TASKS_COMPLETE
// payload of the user's save but not the object name hashes (STAND-IN SAVE-S5 keeps records the port does not produce).
#include "assets/assets.hpp"
#include "save/level_state.hpp"
#include "save/new_game.hpp"
#include "save/save.hpp"
#include "save/save_writer.hpp"

#include <cstdint>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace chase::testing {
constexpr std::uint32_t kUserPlayTicks = 10462;
constexpr std::uint16_t kUserPermille = 2;
constexpr std::uint8_t kUserCurrentLevel = 1;
constexpr std::uint64_t kUserStuds = 1130;
constexpr std::uint32_t kUserSuperBricks = 52;
constexpr std::uint8_t kUserUnlockBits = 0x10;
constexpr std::uint8_t kUserLevelTwoCollected = 1;

struct UserVariable {
    const char* name;
    std::uint32_t hash;
    float value;
};

inline const UserVariable kLevelOneVariables[] = {
    {"Lobby2Basement_Lift_Leave", 0x52783976u, 1.0f},
    {"Lobby_Dcam_Intro", 0x5fea3906u, 1.0f},
    {"(name not recovered)", 0x3952d0cbu, 2.0f},
};

inline const UserVariable kLevelTwoVariables[] = {
    {"StoryProgress", 0x05b583b9u, 2.0f},
    {"1-1_IntroPlayed", 0x4b6d4a5bu, 1.0f},
    {"Entered_Police_Station", 0xde721e9bu, 1.0f},
    {"Enter_Police_Car", 0xeb9ff755u, 1.0f},
    {"DIALOG_RETURNCODE", 0xaa615aeeu, 2.0f},
    {"(name not recovered)", 0x3952d0cbu, 2.0f},
};

constexpr const char* kLevelOneFile = "levels/levels/lvl_00_policestation/lvl_00_policestation_main.blvl";
constexpr const char* kLevelTwoFile = "levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl";

inline std::vector<save::ObjectRecord> userRecords(std::size_t oneByte, std::size_t fourByte, std::uint32_t firstHash) {
    std::vector<save::ObjectRecord> records;
    for (std::size_t i = 0; i < oneByte; ++i) records.push_back({firstHash++, {0}});
    for (std::size_t i = 0; i < fourByte; ++i) records.push_back({firstHash++, {0, 0, 0, 0}});
    return records;
}

inline void writeUserLevel(save::Bytes& slot, assets::FuseArchive& archive, unsigned id, const char* file, const UserVariable* variables,
                           std::size_t count, std::vector<save::ObjectRecord> records, std::size_t endBitSetBytes) {
    const assets::FuseEntry* entry = archive.find(file);
    if (!entry) throw std::runtime_error(std::string("not in archive: ") + file);
    const assets::Level level = assets::parseLevel(archive.read(*entry));
    std::map<std::uint32_t, float> values;
    for (std::size_t i = 0; i < count; ++i) values[variables[i].hash] = variables[i].value;
    values[assets::nameHash("TASKS_COMPLETE")] = 0.0f;
    save::LevelStateInput input;
    input.variable = [&](std::uint32_t hash) -> std::optional<float> {
        const auto found = values.find(hash);
        return found == values.end() ? std::nullopt : std::optional<float>(found->second);
    };
    input.keptRecords = std::move(records);
    input.endBitSetBytes = endBitSetBytes;
    const std::optional<std::vector<std::uint8_t>> data = save::levelStateData(level, input);
    if (!data || !save::writeLevelState(slot, id, *data, save::storyProgressOf(level, input))) throw std::runtime_error("level state does not fit");
}

inline save::SaveImage userSaveImage(assets::FuseArchive& archive) {
    const save::NewGameTables tables = save::newGameTables(archive);
    save::Bytes slot = save::newGameSlot(tables);
    slot[save::kSlotCurrentLevelOffset] = kUserCurrentLevel;
    slot[save::kLevelUnlockBitsOffset] = kUserUnlockBits;
    for (std::uint32_t i = 0; i < kUserPlayTicks; ++i) save::addPlayTick(slot.data());
    writeUserLevel(slot, archive, 1, kLevelOneFile, kLevelOneVariables, sizeof kLevelOneVariables / sizeof *kLevelOneVariables,
                   userRecords(1, 0, 0x1000), 7);
    writeUserLevel(slot, archive, 2, kLevelTwoFile, kLevelTwoVariables, sizeof kLevelTwoVariables / sizeof *kLevelTwoVariables,
                   userRecords(6, 42, 0x2000), 34);
    slot[*save::levelBlockOffset(2) + 0x81f] = kUserLevelTwoCollected;
    save::SlotSummaryInput summary;
    summary.studs = kUserStuds;
    summary.superBricks = kUserSuperBricks;
    save::writeSlotSummary(slot.data(), slot.size(), summary, kUserPermille);
    save::SaveImage image;
    image.resetOptions(1);
    save::resetSlotOptions(image, 0);
    if (!save::commitSlot(image, 0, slot)) throw std::runtime_error("commit failed");
    return image;
}

}
