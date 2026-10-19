// Synthetic scripts for the batch SCRIPT-35 natives (src/runtime/natives_party.cpp, script_runner_natives.cpp; docs/re/script-natives-party.md).
// Provenance of the rules (llvm-objdump --triple=armv6k of game.arm.elf, 2026-10-19; no emulator run):
//  SwapAllegiance 0x1da88c -> 0x2b6b5c (toggle of bit 2 of obj+0xc, move between the two 6-slot lists 0x58deb0 / 0x58dec8, then 0x230e60),
//  SetWeaponEnabled 0x26fea4 (value 0 -> 0x22ae28 / 0x22af80 with 0; 2.0 -> 0x22ae28(e, 1); other -> 0x22af80(e, 1); the player sets 0x375f04..06),
//  CharSetAllowVehicleEntry 0x29ea3c (bit 2 of byte ctrl+0xe0+0x131), SuperFreeplaySwap 0x27c0ac (record 0x5163e8+0xc8 / +0xcc, effects 7 and 0xf1),
//  RelockChasePlainClothes 0x2b4bac, Walnuts_Hit 0x1b2828 -> 0x16458c (state 5), 1-8_RejoinFight 0x26353c -> 0x25e0bc (6), 1-8_runfromarrest
//  0x27bb4c -> 0x1fcc40 (2), walnuts_startphase2 0x291730 -> 0x1c6914 (2), Rex_AxeSmash_Done 0x27bdb8 -> 0x1d4030 (6), Rex_GrapplePull_Done 0x29ea28
//  -> 0x275ac4 (8), getinmech 0x194e7c -> 0x1577c4, GetOutMech 0x1a2f04 -> 0x15a458, CharSetSkydiveMode 0x1da6e4, FireBeamWeapon 0x1d9cb4 -> 0x1b72f4,
//  HACK_PlayDowntownIntro 0x2ad87c (0x21d450(8, 0), level block 0x203464(8) + 0x1c, byte 0x5163e8 + 0xbd), UsePet 0x172184 (returns 1).
// The scripts are made up.
#include "runtime/script_runner.hpp"
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using chase::assets::Level;
using chase::assets::LevelScript;
using chase::assets::PlacedObjectState;
using chase::assets::ScriptCall;
using chase::runtime::ScriptRunner;
using namespace chase::runtime;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

void add(Level& level, const std::string& name, std::vector<ScriptCall> calls) {
    LevelScript script;
    script.nameHash = chase::assets::nameHash(name);
    script.calls = std::move(calls);
    level.scripts.push_back(std::move(script));
}

void swapAllegianceCountsToggles() {
    Level level;
    add(level, "s", {{"SwapAllegiance", {"Guard"}}, {"SwapAllegiance", {"Guard"}}, {"SwapAllegiance", {"Thief"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.start("s");
    require(runner.partyNatives().allegianceSwaps("guard") == 2 && runner.partyNatives().allegianceSwaps("thief") == 1, "one toggle of bit 2 per call");
    require(runner.unhandledFunctions().empty(), "handled");
}

void setWeaponEnabledPicksTheHand() {
    Level level;
    add(level, "s", {{"SetWeaponEnabled", {"Guard", "2"}}, {"SetWeaponEnabled", {"Guard", "1"}}, {"SetWeaponEnabled", {"Guard", "0"}},
                     {"SetWeaponEnabled", {"Player1", "3"}}, {"SetWeaponEnabled", {"Player1", "1"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.start("s");
    const auto& requests = runner.partyNatives().weaponRequests();
    require(requests.size() == 5, "five requests");
    require(requests[0].hand == WeaponHand::A && requests[1].hand == WeaponHand::B && requests[2].hand == WeaponHand::None, "2.0 hand A, other hand B, 0 off");
    require(requests[3].hand == WeaponHand::None && requests[4].hand == WeaponHand::None, "the player takes the global bytes, not the hands");
    const auto bytes = runner.partyNatives().playerWeaponBytes();
    require(bytes && bytes->byte4 && bytes->byte5 && !bytes->byte6, "the last player call (1.0): 0x375f06 is 2.0 < value");
    require(weaponHandOf(2.5f) == WeaponHand::B && weaponHandOf(2.0f) == WeaponHand::A, "only exactly 2.0 selects hand A");
}

void playerWeaponByteSixIsSetAboveTwo() {
    Level level;
    add(level, "s", {{"SetWeaponEnabled", {"Player1", "3"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.start("s");
    require(runner.partyNatives().playerWeaponBytes() && runner.partyNatives().playerWeaponBytes()->byte6, "value 3 sets 0x375f06");
}

void vehicleEntryFlagAndSwapRecords() {
    Level level;
    add(level, "s", {{"CharSetAllowVehicleEntry", {"Player1", "0"}}, {"CharSetAllowVehicleEntry", {"Guard", "1"}}, {"SuperFreeplaySwap", {"Player1", "65545"}},
                     {"SuperFreeplaySwap", {"Guard", "16"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.start("s");
    const auto& party = runner.partyNatives();
    require(party.vehicleEntryAllowed("player1") == std::optional<bool>(false) && party.vehicleEntryAllowed("guard") == std::optional<bool>(true), "bit 2 = flag != 0");
    require(!party.vehicleEntryAllowed("other"), "unset until written");
    require(party.freeplaySwap() && party.freeplaySwap()->object == "guard" && party.freeplaySwap()->character == 16, "the record holds the last call; the id is a halfword");
    require(party.freeplaySwapEffects() == 1, "effects 7 and 0xf1 only for the player entity (the first call)");
}

void relockPlainClothesSwapsBackOnlyForCharacterSixteen() {
    Level level;
    add(level, "s", {{"RelockChasePlainClothes", {}}});
    PlacedObjectState state;
    ScriptRunner a(level, state);
    a.start("s");
    require(a.partyNatives().plainClothesRemovals() == 1 && !a.partyNatives().freeplaySwap(), "character 9 is current: the party loses 16, no swap");
    ScriptRunner b(level, state);
    b.partyNatives().setCurrentCharacter(kCharacterPlainClothes);
    b.start("s");
    require(b.partyNatives().freeplaySwap() && b.partyNatives().freeplaySwap()->character == 9, "character 16 is current: the swap record gets character 9");
}

void bossHooksStoreTheirRequestedState() {
    Level level;
    add(level, "s", {{"Walnuts_Hit", {"Walnuts"}}, {"walnuts_startphase2", {"Walnuts"}}, {"1-8_RejoinFight", {"Boss"}}, {"1-8_runfromarrest", {"Boss"}},
                     {"Rex_AxeSmash_Done", {"Rex"}}, {"Rex_GrapplePull_Done", {"Rex"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.start("s");
    const auto& requests = runner.partyNatives().bossRequests();
    const std::uint16_t expected[] = {5, 2, 6, 2, 6, 8};
    require(requests.size() == 6, "six hooks");
    for (std::size_t i = 0; i < 6; ++i) require(requests[i].state == expected[i], "requested state " + std::to_string(i));
    require(runner.partyNatives().runFromArrest().size() == 1 && runner.partyNatives().runFromArrest()[0] == "boss", "runfromarrest also marks the target");
}

void mechPilotingPairsAndClears() {
    Level level;
    add(level, "s", {{"getinmech", {"Mech", "Player1"}}});
    add(level, "t", {{"GetOutMech", {"Mech"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.start("s");
    require(runner.partyNatives().pilotOf("mech") == std::optional<std::string>("player1"), "the pilot is stored in the mech");
    runner.start("t");
    require(!runner.partyNatives().pilotOf("mech"), "GetOutMech clears it");
}

void skydiveAndBeamRecord() {
    Level level;
    add(level, "s", {{"CharSetSkydiveMode", {"Player1", "1"}}, {"FireBeamWeapon", {"Boss", "261", "1.5", "2.5"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.start("s");
    const auto& party = runner.partyNatives();
    require(party.skydiveRequests().size() == 1 && party.skydiveRequests()[0].state == 129 && party.skydiveRequests()[0].on, "state 129");
    require(party.floorNormalMin() && *party.floorNormalMin() == kSkydiveFloorNormalMin && kSkydiveFloorNormalMin > 0.2019f && kSkydiveFloorNormalMin < 0.2021f, "0x376910 = 0x3e4ed917");
    require(party.beamShots().size() == 1 && party.beamShots()[0].beam == 5 && party.beamShots()[0].first == 1.5f && party.beamShots()[0].second == 2.5f, "beam id is `& 0xff`");
    Level other;
    add(other, "s", {{"CharSetSkydiveMode", {"Player1", "0"}}});
    ScriptRunner off(other, state);
    off.start("s");
    require(off.partyNatives().skydiveRequests()[0].state == 1 && *off.partyNatives().floorNormalMin() == kNormalFloorNormalMin, "state 1 and 0x3f350108");
}

void downtownIntroPlaysOnceWhenTheLevelIsUnlockedEarly() {
    Level level;
    add(level, "s", {{"HACK_PlayDowntownIntro", {"Play"}}, {"HACK_PlayDowntownIntro", {"Again"}}});
    PlacedObjectState state;
    {
        ScriptRunner runner(level, state);
        runner.start("s");
        require(runner.variable("Play") == 0.0f, "level 8 is locked in a new slot");
    }
    ScriptRunner runner(level, state);
    std::vector<std::uint8_t> slot(kSlotImageSize, 0);
    slot[kLevelUnlockOffset + 2] = 1;  // bit 2 * 8 + 0
    runner.progressNatives().setSlotImage(slot);
    runner.start("s");
    require(runner.variable("Play") == 1.0f && runner.variable("Again") == 0.0f, "once: byte 0x5163e8 + 0xbd is set by the first call");
    std::vector<std::uint8_t> late = slot;
    late[kStoryProgressOffset] = 2;
    ScriptRunner done(level, state);
    done.progressNatives().setSlotImage(late);
    done.start("s");
    require(done.variable("Play") == 0.0f, "StoryProgress above 1 skips the intro (bhi)");
}

void usePetDoesNothingAndAllAreListed() {
    Level level;
    add(level, "s", {{"UsePet", {"Pet", "1"}}, {"=", {"Done", "1"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.start("s");
    require(runner.variable("Done") == 1.0f && runner.unhandledFunctions().empty(), "0x172184 only returns 1");
    require(scriptFunctionSupport("UsePet") == ScriptFunctionSupport::Implemented && scriptFunctionSupport("HACK_PlayDowntownIntro") == ScriptFunctionSupport::Implemented, "transcribed");
    require(partyNativeNames().size() == 17, "17 natives");
    for (const std::string& name : partyNativeNames())
        if (name != "usepet" && name != "hack_playdowntownintro") require(scriptFunctionSupport(name) == ScriptFunctionSupport::StandIn, name);
}
}

int main() {
    try {
        swapAllegianceCountsToggles();
        setWeaponEnabledPicksTheHand();
        playerWeaponByteSixIsSetAboveTwo();
        vehicleEntryFlagAndSwapRecords();
        relockPlainClothesSwapsBackOnlyForCharacterSixteen();
        bossHooksStoreTheirRequestedState();
        mechPilotingPairsAndClears();
        skydiveAndBeamRecord();
        downtownIntroPlaysOnceWhenTheLevelIsUnlockedEarly();
        usePetDoesNothingAndAllAreListed();
    } catch (const std::exception& error) {
        std::cerr << "natives-party-test: FAIL: " << error.what() << '\n';
        return 1;
    }
    std::cout << "natives-party-test: ok\n";
    return 0;
}
