// The level state refresh of a save request (0x192f60, 0x1c26dc; docs/re/save-format-state.md).
// Usage: level-state-test [<lego_city3ds.fib> <lego_city3ds.csv> <savegame.dat>]; without arguments only the synthetic
// part runs, with missing files it exits 77 (skip). The save is a copy of the user's save and is only read.
#include "save/level_state.hpp"
#include "save/save.hpp"

#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>

using namespace chase;
using namespace chase::save;

namespace {
void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

std::uint32_t bitsOf(float value) {
    std::uint32_t bits;
    std::memcpy(&bits, &value, 4);
    return bits;
}

assets::LevelParam param(const char* name, std::uint16_t flags, std::uint16_t type, std::uint32_t value) {
    assets::LevelParam p;
    p.hash = assets::nameHash(name);
    p.flags = flags;
    p.type = type;
    p.value = value;
    return p;
}

void testSynthetic() {
    // Provenance: 0x1c26dc (REA pseudo code, mode 2) and 0x192f60 / 0x2451d0 (Ghidra listing).
    assets::Level level;
    level.defaults.push_back(param("A", 2, 0, bitsOf(1.5f)));
    level.defaults.push_back(param("Skipped", 1, 0, bitsOf(9.0f)));
    level.defaults.push_back(param("StoryProgress", 3, 5, 7));
    assets::LevelParam text = param("Text", 2, 1, 0);
    text.blocks.push_back({'a', 'b', 0, 'x'});
    level.defaults.push_back(text);
    assets::LevelParam vector = param("Vec", 2, 2, 0);
    vector.blocks.push_back(std::vector<std::uint8_t>(12, 0x11));
    level.defaults.push_back(vector);

    LevelStateInput input;
    std::map<std::uint32_t, float> live{{assets::nameHash("A"), 2.5f}, {assets::nameHash("TASKS_COMPLETE"), 3.0f}};
    input.variable = [&](std::uint32_t hash) -> std::optional<float> {
        const auto found = live.find(hash);
        return found == live.end() ? std::nullopt : std::optional<float>(found->second);
    };
    input.keptRecords = {{0x1234, {1}}, {0x5678, {0, 0, 0, 0}}};
    input.endBitSetBytes = 3;
    const std::optional<std::vector<std::uint8_t>> data = levelStateData(level, input);
    require(data.has_value(), "data");
    const std::size_t levelRecord = 8 + 4 + 4 + 3 + 12;
    require(data->size() == levelRecord + 9 + 12 + 4 + 3, "size");
    require((*data)[0] == 0 && (*data)[4] == 23 && (*data)[5] == 0, "level-data record header");
    std::uint32_t first;
    std::memcpy(&first, data->data() + 8, 4);
    require(first == bitsOf(2.5f), "live float value");
    require((*data)[12] == 7 && (*data)[16] == 'a' && (*data)[18] == 0 && (*data)[19] == 0x11, "integer default, string, vector");
    require((*data)[levelRecord] == 0x34 && (*data)[levelRecord + 4] == 1 && (*data)[levelRecord + 8] == 1, "kept 1-byte record");
    std::uint32_t tasks;
    std::memcpy(&tasks, data->data() + levelRecord + 9 + 8, 4);
    require(tasks == bitsOf(3.0f), "a 4-byte record holds the live TASKS_COMPLETE");
    require((*data)[levelRecord + 21] == 0xff && (*data)[data->size() - 1] == 0, "end marker, zero end bit set");
    require(storyProgressOf(level, input) == 7, "StoryProgress");

    std::vector<std::uint8_t> slot(kSlotSize, 0xaa);
    require(writeLevelState(slot, 2, *data, 7), "write");
    const std::optional<LevelStateView> view = levelState({slot.data(), slot.size()}, 2);
    require(view && view->size == data->size() && view->storyProgress == 7, "the block reads back");
    require(slot[0x8 + 0x1f + data->size()] == 0 && slot[0x8 + 0x81f] == 0xaa, "rest of the data area cleared, collected byte kept");
    require(!writeLevelState(slot, 9, *data, std::nullopt), "no block for level 9");
}

assets::Level readLevel(assets::FuseArchive& archive, const std::string& path) {
    const assets::FuseEntry* entry = archive.find(path);
    require(entry != nullptr, "not in archive: " + path);
    return assets::parseLevel(archive.read(*entry));
}

void testRealSave(assets::FuseArchive& archive, const std::string& file) {
    // The user's save copy: re-serialising each saved level block from its own variables and records gives the same
    // bytes (level 1: 201, level 2: 823, read with python struct from the copy).
    const LoadResult loaded = load(file);
    require(loaded.found && loaded.status == Status::Ok && loaded.image, "the save copy is valid");
    const SlotView slot = loaded.image->slotView(0);
    const std::pair<unsigned, const char*> levels[] = {
        {1, "levels/levels/lvl_00_policestation/lvl_00_policestation_main.blvl"},
        {2, "levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl"},
    };
    for (const auto& [id, path] : levels) {
        const assets::Level level = readLevel(archive, path);
        const SlotStart start = slotStart(slot, level, id, 8);
        const std::optional<LevelStateView> state = levelState(slot, id);
        require(state && start.stateSaved, "level " + std::to_string(id) + " has a saved state");
        const std::optional<ObjectRecords> records = objectRecords(*state);
        require(records.has_value(), "records");
        std::map<std::uint32_t, float> values;
        for (const SavedVariable& variable : start.variables) values[variable.hash] = variable.value;
        LevelStateInput input;
        input.variable = [&](std::uint32_t hash) -> std::optional<float> {
            const auto found = values.find(hash);
            return found == values.end() ? std::nullopt : std::optional<float>(found->second);
        };
        input.keptRecords = records->records;
        input.endBitSetBytes = records->endBitSetBytes;
        const std::optional<std::vector<std::uint8_t>> data = levelStateData(level, input);
        require(data && data->size() == state->size, "level " + std::to_string(id) + " length");
        require(std::memcmp(data->data(), state->data, state->size) == 0, "level " + std::to_string(id) + " bytes");
        const std::optional<std::int32_t> story = storyProgressOf(level, input);
        require(!story || std::uint16_t(*story) == state->storyProgress, "level " + std::to_string(id) + " StoryProgress");
    }
}
}

int main(int argc, char** argv) {
    try {
        testSynthetic();
        if (argc == 1) {
            std::cout << "level-state-test passed (synthetic)\n";
            return 0;
        }
        if (argc != 4 || !std::ifstream(argv[1]) || !std::ifstream(argv[2]) || !std::ifstream(argv[3])) {
            std::cout << "level-state-test: data not available, skipped\n";
            return 77;
        }
        assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        testRealSave(archive, argv[3]);
    } catch (const std::exception& error) {
        std::cerr << "level-state-test failed: " << error.what() << '\n';
        return 1;
    }
    std::cout << "level-state-test passed\n";
    return 0;
}
