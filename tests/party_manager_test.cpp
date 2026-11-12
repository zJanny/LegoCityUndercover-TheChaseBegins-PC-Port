// The party manager 0x51b248 (src/runtime/party_manager.*; docs/re/disguise.md). Rules transcribed with llvm-objdump of game.arm.elf by
// 20261112-enemy-B on 2026-11-12: list 0x20a4d4 / 0x1b1c90 / 0x171bb0, reset 0x1677b4, add 0x22dbcc, removal 0x22db24, lookup 0x2457bc,
// cycle 0x235110, swap 0x245258, SetPartyCharacter 0x27beb4, UnlockCharacter 0x263918, LockCharacter 0x1cda60.
// The level row of lvl_01_cherrytreehills (TablesLevels sheet 0 row 2: +0x43..+0x4a = 9 19 29 39 49 59 69 80, +0x4b = +0x4c = 0) was read
// with `build/chase-tabledump tables/tableslevels.bxls` by 20261112-enemy-B on 2026-11-12. A new game's slot has group bit 0 only (the slot
// callback 0x1a25cc, save/new_game.cpp).
#include "runtime/party_manager.hpp"
#include "runtime/script_runner.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase::runtime;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::array<std::uint8_t, kPartyLevelIds> kLevelOneRow = {9, 19, 29, 39, 49, 59, 69, 80};
constexpr std::array<std::uint8_t, kPartyLevelExtras> kNoExtras = {0, 0};

PartyManager newGameParty(std::uint8_t groupBits = 1) {
    PartyManager party;
    PartyFlags flags;
    flags.groupBytes = {groupBits, 0};
    party.setFlags(flags);
    party.setLevel(2);
    party.setUp(partyList(kLevelOneRow, kNoExtras, {}, false, false));
    return party;
}

void listOfTheLevelRow() {
    const PartyList row = partyList(kLevelOneRow, kNoExtras, {1, 2, 3, 0, 0, 0, 0, 0}, false, false);
    require(row.walkCount == 8 && row.ids[0] == 9 && row.ids[7] == 80 && row.ids[8] == 0, "without slot +7 bit 0 the row's eight ids, walk count 8");
    const PartyList extras = partyList({9, 19, 0, 0, 0, 0, 0, 0}, {5, 6}, {}, false, false);
    require(extras.walkCount == 4 && extras.ids[2] == 5 && extras.ids[3] == 6, "the extras +0x4b, +0x4c follow the listed ids");
    const PartyList firstExtra = partyList(kLevelOneRow, {5, 0}, {}, false, false);
    require(firstExtra.walkCount == 9 && firstExtra.ids[8] == 5 && firstExtra.ids[9] == 0, "extras up to the first 0");
    const PartyList own = partyList(kLevelOneRow, kNoExtras, {16, 19, 0, 0, 0, 0, 0, 0}, true, false);
    require(own.walkCount == 2 && own.ids[0] == 16 && own.ids[2] == 0, "slot +7 bit 0: the slot's ids +0x5201");
    const PartyList cheat = partyList({9, 0, 0, 0, 0, 0, 0, 0}, kNoExtras, {}, false, true);
    require(cheat.walkCount == 8 && cheat.ids[7] == 80, "cheat byte 0x375961 bit 0: 9 .. 69, 80");
    const PartyList ownCheat = partyList(kLevelOneRow, kNoExtras, {16, 0, 0, 0, 0, 0, 0, 0}, true, true);
    require(ownCheat.walkCount == 8 && ownCheat.ids[0] == 16 && ownCheat.ids[1] == 0, "0x171bb0 copies the slot's ids over the cheat list");
}

void setUpOfANewGame() {
    const PartyManager party = newGameParty();
    require(party.walkCount() == 8 && party.usableCount() == 1, "eight slots walked, only slot 0 usable (group bits 1..7 clear)");
    require(party.slots()[0].id == 9 && !party.slots()[0].removed && party.slots()[0].entity == "Player1.Player01", "slot 0: Chase");
    for (unsigned i = 1; i < 8; ++i) require(party.slots()[i].removed && party.slots()[i].entity.empty(), "slots 1..7 removed, entity cleared");
    require(party.slots()[7].id == 80 && party.slots()[8].id == 0, "80 in slot 7, slots 8 and 9 empty");
    require(party.playerCharacter() == 9 && party.playerEntity() == "Player1.Player01" && party.characterByte() == 9 && party.secondCharacterByte() == 19,
            "the player is Player1.Player01 with character 9 (0x19f41c, 0x171bb0)");
    require(party.slotOf(19) == 1 && party.slotOf(0) == -1 && party.slotOf(16) == -1, "0x2457bc: -1 for id 0 and for no slot");

    const PartyManager robber = newGameParty(0x03);
    require(robber.usableCount() == 2 && !robber.slots()[1].removed, "group bit 1 keeps the robber (bit of the list position)");

    PartyManager extra;
    PartyFlags flags;
    flags.groupBytes = {1, 0x02};
    extra.setFlags(flags);
    extra.setUp(partyList(kLevelOneRow, {5, 6}, {}, false, false));
    require(extra.walkCount() == 10 && extra.slots()[8].id == 0 && extra.slots()[9].id == 0,
            "the add gives new ids only the first eight slots, so the extras get none");
}

void setPartyCharacter() {
    PartyManager party = newGameParty();
    std::optional<PartySwap> swap;
    require(party.setPartyCharacter(0, 9, true, swap) && !swap, "(Player1, 9, 1): the player's slot, 0x245258 returns 0");
    require(!party.setPartyCharacter(0, 16, false, swap), "(Player1, 16, 0): 16 has no slot");
    require(!party.setPartyCharacter(0, 9 + 0x100, false, swap), "the halfword 265 matches no id byte");
    party.setClock(100);
    require(party.setPartyCharacter(0, 19, false, swap) && swap, "(Player1, 19, 0): the removed robber still has its slot below the walk count");
    require(swap->swapped && swap->index == 1 && swap->member == "Player1.Player02" && swap->character == 19, "Player1.Player02 becomes the player");
    require(swap->effects && swap->scripts && swap->sceneScript, "not quiet: effects 7 and 241, the member's startup script");
    require(party.playerCharacter() == 19 && party.characterByte() == 19 && party.playerEntity() == "Player1.Player02", "0x375d45 is the new character");
    party.setClock(115);
    require(party.setPartyCharacter(0, 9, false, swap) && swap && !swap->effects && swap->character == 9, "the effect gate: update count + 15");
    party.setClock(116);
    require(party.setPartyCharacter(0, 19, true, swap) && swap && !swap->effects && !swap->scripts, "flag 1 (quiet): no effects, no scripts");
    require(party.takeSwaps().size() == 3 && party.takeSwaps().empty(), "three swaps, taken once");
}

void swapRules() {
    PartyManager party = newGameParty();
    require(!party.swap(0, 255, {}), "index 255: 0x245258 returns 0");
    require(!party.swap(0, 0, {}), "the player's slot: returns 0");
    const std::optional<PartySwap> beyond = party.swap(0, 12, {});
    require(beyond && !beyond->swapped && beyond->member == "Player1.Player13" && party.playerCharacter() == 9,
            "an index past the ten members finds no member: it returns 1, the player stays");
    PartyManager custom = newGameParty();
    custom.setLevel(9);
    custom.setClock(50);
    const std::optional<PartySwap> kept = custom.swap(0, 1, {});
    require(kept && !kept->swapped && !kept->effects && custom.playerCharacter() == 9, "session byte +0x25 = 9 (0x2254c4): no swap, no effects");
}

void standFor() {
    PartyManager party = newGameParty(0xff);
    require(party.usableCount() == 8, "all group bits");
    SwapOptions options;
    options.updateStandFor = true;
    party.swap(0, 2, options);
    require(party.slots()[2].standFor == 0, "usable count 8 > index 2: the stand-for id of the index is cleared");
    PartyManager few = newGameParty(0x01);
    few.swap(0, 3, options);
    require(few.slots()[0].standFor == 39, "usable count 1 <= index 3 and > slot 0 of the old player: slot 0 stands for 39");
}

void unlockAndLock() {
    PartyManager party = newGameParty();
    party.unlockCharacter(19);
    require(party.usableCount() == 2 && !party.slots()[1].removed && party.slots()[1].entity == "Player1.Player02",
            "UnlockCharacter 19: the add with the member of slot 1 makes the robber usable again");
    party.unlockCharacter(19);
    require(party.usableCount() == 2, "a usable slot is not counted twice");
    party.unlockCharacter(16);
    party.unlockCharacter(80);
    require(party.usableCount() == 2 && party.slotOf(16) == -1, "16 and ids outside the seven first rows are not added");

    const std::optional<PartySwap> next = party.cycle(false, false);
    require(next && next->index == 1 && party.playerCharacter() == 19, "the cycle forward from slot 0 takes the robber");
    party.lockCharacter(19);
    require(party.playerCharacter() == 9 && party.slots()[1].removed && party.usableCount() == 1,
            "LockCharacter of the player's character: the cycle forward first (back to slot 0), then the removal");
    require(!party.cycle(false, false) && !party.cycle(true, false), "no other usable slot: no swap");
    require(!party.remove(16), "0x22db24 without a slot returns 0");
}

void cycleRules() {
    PartyManager party = newGameParty(0x01 | 0x80);
    const std::optional<PartySwap> backward = party.cycle(true, false);
    require(backward && backward->index == 7 && party.playerCharacter() == 80, "backward from slot 0 wraps to slot 7, taken by group bit 7");
    PartyManager flagged = newGameParty();
    require(!flagged.cycle(false, true), "with the flag the walk reaches the start slot; 0x245258 returns 0 for it");
    PartyManager cheat = newGameParty();
    PartyFlags flags = cheat.flags();
    flags.cheatAllCharacters = true;
    cheat.setFlags(flags);
    const std::optional<PartySwap> any = cheat.cycle(false, false);
    require(any && any->index == 1, "cheat bit 0: removed slots are taken");
}

void addRules() {
    PartyManager party = newGameParty();
    party.add("", 16);
    require(party.slotOf(16) == -1, "no empty slot among the first eight");
    PartyManager custom;
    custom.setLevel(kCustomisationLevel);
    custom.setUp(partyList(kLevelOneRow, kNoExtras, {}, false, false));
    require(custom.usableCount() == 0 && custom.slotOf(9) == -1, "level 9: the add returns at once");
    PartyManager fresh;
    fresh.add("Player1.Player05", 16);
    require(fresh.slotOf(16) == 0 && fresh.usableCount() == 1 && fresh.slots()[0].entity == "Player1.Player05", "the first empty slot");
    fresh.reset();
    require(fresh.slotOf(16) == -1 && fresh.usableCount() == 0 && fresh.slots()[0].entity == "Player1.Player05",
            "0x1677b4 clears ids and counts, the entity words stay");
}

void scriptGlue() {
    chase::assets::Level level;
    chase::assets::LevelScript script;
    script.nameHash = chase::assets::nameHash("s");
    script.calls = {{"UnlockCharacter", {"19"}}, {"SetPartyCharacter", {"Player1", "19", "1"}}, {"LockCharacter", {"19"}}};
    level.scripts.push_back(script);
    chase::assets::PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.progressNatives().slotImage()[kCharacterGroupOffset] = 1;
    PartyFlags flags;
    flags.groupBytes = {1, 0};
    runner.partyNatives().party().setFlags(flags);
    runner.partyNatives().party().setUp(partyList(kLevelOneRow, kNoExtras, {}, false, false));
    runner.start("s");
    const std::vector<PartyRequest> requests = runner.takePartyRequests();
    require(requests.size() == 1 && requests[0].outcome == PartyOutcome::Swapped && requests[0].swap && requests[0].swap->character == 19,
            "UnlockCharacter(19) then SetPartyCharacter(Player1, 19, 1) swaps to the robber");
    const PartyManager& party = runner.partyNatives().party();
    require(party.playerCharacter() == 9 && party.slots()[1].removed, "LockCharacter(19) cycles back to Chase and removes the robber");
    require(party.flags().groupBytes[0] == 1, "the group bytes come from the slot after LockCharacter cleared bit 1");
}
}

int main() {
    try {
        listOfTheLevelRow();
        setUpOfANewGame();
        setPartyCharacter();
        swapRules();
        standFor();
        unlockAndLock();
        cycleRules();
        addRules();
        scriptGlue();
    } catch (const std::exception& error) {
        std::cerr << "party-manager: " << error.what() << '\n';
        return 1;
    }
    std::cout << "party-manager: ok\n";
    return 0;
}
