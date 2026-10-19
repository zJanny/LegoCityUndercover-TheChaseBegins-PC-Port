// The new-game slot (0x228274, 0x1a25cc), the level entry 0x20a4d4 and a save round trip (docs/re/save-format-state.md).
// Usage: new-game-test [<lego_city3ds.fib> <lego_city3ds.csv> [<savegame.dat>]]; without arguments only the synthetic
// part runs, with missing files it exits 77 (skip). The save is a copy of the user's save and is only read.
#include "save/completion_tables.hpp"
#include "save/continue_start.hpp"
#include "save/new_game.hpp"
#include "save/save_writer.hpp"

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unistd.h>

using namespace chase::save;

namespace {
void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

bool bit(const Bytes& slot, std::size_t offset, unsigned index) { return slot[offset + (index >> 3)] >> (index & 7) & 1; }

NewGameTables syntheticTables() {
    NewGameTables tables;
    tables.levelOneGroup = {0x21, 0x22};
    tables.characters.resize(kCharacterIdLimit);
    for (NewGameCharacter& character : tables.characters) character.startGroup = 3;
    tables.characters[9].startGroup = 1;
    tables.characters[9].linked = 12;
    tables.characters[9].next = 15;
    tables.characters[12].secondBit = true;
    tables.characters[15].startGroup = 2;
    tables.characters[140].startGroup = 0;
    tables.characters[kEmptyGroupCharacter].ownCustomisation = true;
    tables.abilityLocked[0] = tables.abilityLocked[17] = tables.abilityLocked[63] = true;
    for (unsigned row = 0; row < 10; ++row) {
        std::array<std::uint8_t, 8> party{};
        for (unsigned i = 0; i < 8; ++i) party[i] = std::uint8_t(row * 10 + i);
        tables.levelParty.push_back(party);
    }
    return tables;
}

void testSyntheticSlot() {
    // Provenance: 0x1a25cc, 0x231fc0 and 0x232028 (REA pseudo code and Ghidra listing of artifacts/game/game.arm.elf).
    const NewGameTables tables = syntheticTables();
    const Bytes slot = newGameSlot(tables);
    require(slot.size() == kSlotSize && slot[6] == 2 && slot[kSlotFlagsOffset] == kSlotInUseBit, "level 2, used bit");
    require(slot[kSlotLevelOneGroupOffset] == 0x21 && slot[kSlotLevelOneGroupOffset + 1] == 0x22, "TablesLevels row 1 +0x43/+0x44");
    require(slot[kSlotGroupBitsOffset] == 1 && slot[0x51e0] == 0x10 && slot[kSlotStudFlagsOffset] == 0x20, "group bit, level 2 unlock, +0x520b");
    require(bit(slot, kSlotCharacterBitsOffset, 18) && !bit(slot, kSlotCharacterBitsOffset, 19), "id 9 unlocked, no second bit");
    require(bit(slot, kSlotCharacterBitsOffset, 24) && bit(slot, kSlotCharacterBitsOffset, 25), "linked id 12 with its second bit");
    require(bit(slot, kSlotCharacterBitsOffset, 30), "next id 15 is unlocked through the chain");
    require(bit(slot, kSlotCharacterBitsOffset, 280) && !bit(slot, kSlotCharacterBitsOffset, 20), "id 140 by its group, id 10 not");
    require(!bit(slot, kSlotCharacterNewOffset, 9) && bit(slot, kSlotCharacterNewOffset, 12) && bit(slot, kSlotCharacterNewOffset, 140),
            "the new bit for every id but 9");
    require(slot[kSlotAbilityOffset] == 0xfe && slot[kSlotAbilityOffset + 2] == 0xfd && slot[kSlotAbilityOffset + 7] == 0x7f &&
                slot[kSlotAbilityOffset + 1] == 0xff,
            "abilities: all set but the locked rows");
    for (unsigned record = 0; record < kCustomisationRecords; ++record) {
        const std::size_t at = kSlotCustomisationOffset + record * kCustomisationRecordSize;
        require(slot[at + 5] == kGroupCharacters[record], "record id");
        require(slot[at + 4] == (record == 7 ? 0 : 2), "record flags from TablesCharacters +0x21");
    }
}

void testEnterLevel() {
    // Provenance: 0x20a4d4 and 0x1947a0 (REA pseudo code).
    const NewGameTables tables = syntheticTables();
    Bytes slot = newGameSlot(tables);
    for (unsigned i = 0; i < 8; ++i) slot[0x5148 + i] = std::uint8_t(i + 1);
    slot[0x5190] = 0x28;
    slot[0x5191] = 0x01;
    LevelEntry entry = enterLevel(slot, 0, tables);
    require(entry.level == 2 && entry.studs == 0x0807060504030201ull && entry.superBricks == 296, "level 0 becomes 2, counters");
    require(slot[kSlotPartyOffset] == 20 && slot[kSlotPartyOffset + 7] == 27 && slot[6] == 2, "party of row 2");
    entry = enterLevel(slot, 5, tables);
    require(slot[6] == 5 && bit(slot, 0x51e0, 10) && slot[kSlotPartyOffset] == 50, "level 5: unlock bit 10, +6, party");
    entry = enterLevel(slot, 1, tables);
    require(entry.level == 1 && slot[6] == 5 && !bit(slot, 0x51e0, 2) && slot[kSlotPartyOffset] == 10, "level 1: no +6, no unlock bit");
    slot[kSlotFlagsOffset] |= kSlotOwnPartyBit;
    enterLevel(slot, 3, tables);
    require(slot[kSlotPartyOffset] == 10, "+7 bit 0 keeps the party");
    Bytes shortSlot(kSlotSize - 1, 0);
    bool threw = false;
    try {
        enterLevel(shortSlot, 2, tables);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    require(threw, "a short slot is refused");
}

void testOptions() {
    SaveImage image;
    image.options()[4] = image.options()[6] = 3;
    image.options()[0x1ec + 0x18] = 7;
    resetSlotOptions(image, 1);
    require(image.options()[5] == 10 && image.options()[7] == 8 && image.options()[0x1ec + 0x18] == 0, "slot 1 options");
    require(image.options()[4] == 3 && image.options()[6] == 3, "slot 0 options untouched");
}

// The live slot 0x51b6b0 in the oracle state gameplay-start (new game, Old 3DS), read with `memtrace 1 slotdump
// 0x51b490:0x5450` (tools/azahar_oracle.py, port 24711, no RPC, 2026-10-19). Every byte the slot callback and the level
// entry write; the rest of that slot is the play time (+0), the per mille (+4), the saved state of level 2 (+0x8..+0x33c,
// written by the level state refresh of a save request) and +0x5180 = 1 (writer not read).
struct ReferenceBytes {
    std::size_t offset;
    std::vector<std::uint8_t> bytes;
};
const ReferenceBytes kGameplayStart[] = {
    {6, {0x02, 0x04}},
    {0x5150, {0x00, 0x00, 0x00, 0x00, 0x02, 0x09, 0x09, 0x09, 0x0a, 0x01, 0x02, 0x13, 0x12, 0x00, 0x14, 0x0f, 0x02, 0x1d,
              0x1b, 0x00, 0x1e, 0x14, 0x02, 0x27, 0x22, 0x00, 0x28, 0x1d, 0x02, 0x31, 0x2c, 0x00, 0x30, 0x24, 0x02, 0x3b,
              0x31, 0x00, 0x3a, 0x01, 0x02, 0x45, 0x51, 0x07, 0x5d, 0x44, 0x00, 0x00}},
    {0x51b3, {0x09, 0x13}},
    {0x51bb, {0x01, 0x00, 0x00, 0x0c, 0x00}},
    {0x51e0, {0x10}},
    {0x51ee, {0xfb, 0xff, 0xff, 0xff, 0xbf, 0x3f, 0x3c, 0xe0}},
    {0x5201, {0x09, 0x13, 0x1d, 0x27, 0x31, 0x3b, 0x45, 0x50}},
    {0x520b, {0x20}},
};

void testRealTables(chase::assets::FuseArchive& archive) {
    const NewGameTables tables = newGameTables(archive);
    Bytes slot = newGameSlot(tables);
    const LevelEntry entry = enterLevel(slot, 0, tables);
    require(entry.level == 2 && entry.studs == 0 && entry.superBricks == 0, "a new game starts with no studs and no super bricks");
    for (const ReferenceBytes& reference : kGameplayStart)
        for (std::size_t i = 0; i < reference.bytes.size(); ++i)
            require(slot[reference.offset + i] == reference.bytes[i], "byte +0x" + [&] {
                char text[16];
                std::snprintf(text, sizeof text, "%zx", reference.offset + i);
                return std::string(text);
            }() + " differs from the oracle's gameplay-start slot");
    std::size_t referenceNonZero = 0;
    for (const ReferenceBytes& reference : kGameplayStart)
        if (reference.offset >= 8) referenceNonZero += std::size_t(std::count_if(reference.bytes.begin(), reference.bytes.end(), [](std::uint8_t b) { return b != 0; }));
    const std::size_t nonZero = std::size_t(std::count_if(slot.begin() + 8, slot.end(), [](std::uint8_t b) { return b != 0; }));
    require(nonZero == referenceNonZero, "no other byte from +8 on is set (" + std::to_string(nonZero) + ")");
}

void testRoundTrip(chase::assets::FuseArchive& archive) {
    // New game -> save request with live counters -> file -> load -> same slot bytes and counters.
    const NewGameTables tables = newGameTables(archive);
    Bytes live = newGameSlot(tables);
    enterLevel(live, 0, tables);
    for (int i = 0; i < 95; ++i) addPlayTick(live.data());
    const std::filesystem::path dir = std::filesystem::temp_directory_path() / ("chase-new-game-" + std::to_string(getpid()));
    const std::string path = (dir / "chase" / "savegame.dat").string();
    SaveImage base;
    base.resetOptions(1);
    resetSlotOptions(base, 0);
    SaveRequestWriter writer(path, base, 0, completionTotals(archive));
    SlotSummaryInput input;
    input.studs = 4321;
    input.superBricks = 1500;
    input.studType = 1;
    require(writer.request(live, input), "the first save request is written");
    const LoadResult loaded = load(path);
    require(loaded.found && loaded.status == Status::Ok && loaded.image, "a valid savegame.dat (header, CRC)");
    require(loaded.image->lastSlot() == std::size_t(0) && loaded.image->options()[4] == 10, "options +2 and the slot volume");
    const SlotView stored = loaded.image->slotView(0);
    require(std::equal(live.begin(), live.end(), stored.data), "the stored slot is the live slot");
    require(slotInUse(stored) && playTicks(stored) == 95 && slotStudType(stored) == 1, "used, play time, stud type");
    Bytes reloaded(stored.data, stored.data + stored.size);
    const LevelEntry entry = enterLevel(reloaded, reloaded[6], tables);
    require(entry.level == 2 && entry.studs == 4321 && entry.superBricks == 1500, "studs and super bricks survive the restart");
    require(bit(reloaded, 0x51e0, 4) && bit(reloaded, kSlotCharacterBitsOffset, 18), "level 2 unlocked, character 9 unlocked");
    std::filesystem::remove_all(dir);
}

void testRealSave(chase::assets::FuseArchive& archive, const std::string& file) {
    // Provenance: the user's save copy, slot 0 read with python struct: studs u64 +0x5148 = 1130, u32 +0x5190 = 52, +6 = 1.
    const LoadResult loaded = load(file);
    require(loaded.found && loaded.status == Status::Ok && loaded.image, "the save copy is valid");
    const SlotView view = loaded.image->slotView(0);
    Bytes slot(view.data, view.data + view.size);
    const LevelEntry entry = enterLevel(slot, slot[6], newGameTables(archive));
    require(entry.level == 1 && entry.studs == 1130 && entry.superBricks == 52, "the counters of the user's slot 0");
    require(slot[6] == 1 && slot[0x51e0] == 0x10, "level 1 changes neither +6 nor the unlock bits");
}
}

int main(int argc, char** argv) {
    try {
        testSyntheticSlot();
        testEnterLevel();
        testOptions();
        if (argc == 1) {
            std::cout << "new-game-test passed (synthetic)\n";
            return 0;
        }
        if (argc < 3 || !std::ifstream(argv[1]) || !std::ifstream(argv[2])) {
            std::cout << "new-game-test: data not available, skipped\n";
            return 77;
        }
        chase::assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        testRealTables(archive);
        testRoundTrip(archive);
        if (argc > 3 && std::ifstream(argv[3])) testRealSave(archive, argv[3]);
        else std::cout << "new-game-test: no real save, the slot load of the user's save is skipped\n";
    } catch (const std::exception& error) {
        std::cerr << "new-game-test failed: " << error.what() << '\n';
        return 1;
    }
    std::cout << "new-game-test passed\n";
    return 0;
}
