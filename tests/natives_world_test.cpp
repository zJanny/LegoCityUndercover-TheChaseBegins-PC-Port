// Synthetic scripts for EffectFadeObject, EnableDistanceFade, AddToSemiTransNearCameraList, SetPartyCharacter and StreamRoomIn
// (src/runtime/natives_effects.cpp, natives_world.cpp, script_runner_natives.cpp; docs/re/script-natives-world.md).
// Provenance of the rules: EffectFadeObject 0x288a60 -> 0x229e54 -> tween registry 0x21b7cc and evaluation 0x1593c8 (REA pseudo code and
// llvm-objdump of game.arm.elf, 2026-10-18), EnableDistanceFade 0x2a05bc (REA), AddToSemiTransNearCameraList 0x2cb734 with 0x1f40d8 (REA and
// objdump), SetPartyCharacter 0x27beb4 with 0x2457bc and 0x245258 (REA; objdump 2026-11-12, 20261112-enemy-B), StreamRoomIn 0x1bfba4 with 0x20f194 and 0x2284bc (objdump and REA).
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

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

void add(Level& level, const std::string& name, std::vector<ScriptCall> calls) {
    LevelScript script;
    script.nameHash = chase::assets::nameHash(name);
    script.calls = std::move(calls);
    level.scripts.push_back(std::move(script));
}

// 0x21b7cc registers the tween and applies `from` at once (from == 0 hides the node, 0x23f734); 0x1593c8 moves the value linearly and clamps it
// to 0..1, applies int(value x 255) as the node alpha for flag bit 0 and shows the node while the value is above 0.
void fadeInAndOut() {
    Level level;
    add(level, "s", {{"EffectFadeObject", {"Car", "0", "1", "1", "1"}}, {"EffectFadeObject", {"Crow", "1", "0", "1", "3"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.start("s");
    require(runner.effects().fade("car") && runner.effects().fade("car")->hidden && runner.effects().fade("car")->multiplier == 0.0f, "from 0: hidden at registration");
    require(!runner.effects().fade("crow")->hidden && runner.effects().fade("crow")->multiplier == 1.0f, "from 1: shown and opaque at registration");
    require(runner.effects().activeFades() == 2, "two tweens");
    runner.update(0.5f);
    require(!runner.effects().fade("car")->hidden && runner.effects().fade("car")->multiplier == 127.0f / 255.0f, "half way: int(0.5 x 255) = 127");
    require(runner.effects().fade("crow")->multiplier == 127.0f / 255.0f, "flags 3 (alpha and colour) fade the same");
    runner.update(0.5f);
    require(runner.effects().activeFades() == 0, "both tweens end after their duration");
    require(runner.effects().fade("car")->multiplier == 1.0f && !runner.effects().fade("car")->hidden, "fade in ends opaque");
    require(runner.effects().fade("crow")->hidden, "a tween that ends at 0 leaves the node hidden");
    require(runner.effects().fade("crow")->multiplier == 1.0f, "0x159494 sets the alpha of a node at value 0 from `from` (255), not 0");
    require(!runner.effects().fade("nobody"), "an untouched object has no fade");
    require(runner.unhandledFunctions().empty(), "EffectFadeObject is handled");
}

void zeroDurationAppliesFromThenFinishes() {
    Level level;
    add(level, "s", {{"EffectFadeObject", {"Wall", "1", "0", "0", "1"}}, {"EffectFadeObject", {"Door", "0", "1", "0", "1"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.start("s");
    runner.update(0.1f);
    require(runner.effects().activeFades() == 0, "0x159414 skips the value of a zero duration and 0x159618 finishes the entry");
    require(runner.effects().fade("wall")->hidden, "to 0 hides the node when it finishes");
    require(runner.effects().fade("door")->hidden && runner.effects().fade("door")->multiplier == 0.0f, "from 0 stays hidden: the value is never evaluated");
}

void flagsSelectWhatIsApplied() {
    Level level;
    add(level, "s", {{"EffectFadeObject", {"Third", "0", "1", "1", "4"}}, {"EffectFadeObject", {"Alpha", "0", "1", "1", "256"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.start("s");
    runner.update(0.5f);
    require(runner.effects().fade("third")->multiplier == 1.0f && !runner.effects().fade("third")->hidden, "bit 2 alone changes no alpha or colour (the third property is not ported)");
    require(runner.effects().fade("alpha")->multiplier == 1.0f, "the flag byte is the low byte of the integer: 256 has no bit set");
}

void aSecondFadeOfTheSameNodeReusesItsEntry() {
    Level level;
    add(level, "s", {{"EffectFadeObject", {"Car", "0", "1", "4", "1"}}, {"Wait", {"1"}}, {"EffectFadeObject", {"Car", "1", "0", "1", "1"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.start("s");
    runner.update(1.0f);
    require(runner.effects().activeFades() == 1, "one entry per node (0x21b7cc looks the node up first)");
    runner.update(1.0f);
    require(runner.effects().activeFades() == 0 && runner.effects().fade("car")->hidden, "the second tween runs from its own start");
}

void fullTableCompletesTheOldestEntry() {
    Level level;
    std::vector<ScriptCall> calls;
    calls.push_back({"EffectFadeObject", {"E0", "0", "1", "100", "1"}});
    calls.push_back({"Wait", {"0.5"}});
    for (int i = 1; i < 21; ++i) calls.push_back({"EffectFadeObject", {"E" + std::to_string(i), "0", "1", "100", "1"}});
    add(level, "s", calls);
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.start("s");
    runner.update(1.0f);
    require(runner.effects().activeFades() == 20, "20 entries, the 21st added made room: 0x21b840 completes the oldest first");
    require(!runner.effects().fade("e0")->hidden && runner.effects().fade("e0")->multiplier == 1.0f, "the completed entry ends at its target value");
    require(runner.effects().fade("e20") && runner.effects().fade("e20")->hidden, "the new entry started at 0");
}

void distanceFadeAndSemiTransListRecordTheirArguments() {
    Level level;
    add(level, "s", {{"EnableDistanceFade", {"Cage", "0"}},
                     {"EnableDistanceFade", {"Salesman", "0"}},
                     {"EnableDistanceFade", {"Salesman", "1"}},
                     {"AddToSemiTransNearCameraList", {"\"Prop_Waypoint_Small\"", "\"\""}},
                     {"AddToSemiTransNearCameraList", {"\"Prop_Waypoint_Small\"", "\"\""}},
                     {"AddToSemiTransNearCameraList", {"\"Prop_Waypoint_Large\"", "\"\""}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.start("s");
    require(runner.effects().distanceFade("cage") == false && runner.effects().distanceFade("salesman") == true, "the last value wins (0x2a05bc: bit 4 of +0xde)");
    require(!runner.effects().distanceFade("other"), "untouched: the mesh node default (bit 4 set at creation 0x10d2d4) is the host's");
    const auto& list = runner.effects().semiTransNearCamera();
    require(list.size() == 2 && list[0].model == "prop_waypoint_small" && list[1].model == "prop_waypoint_large" && list[0].tag.empty(),
            "one entry per model and tag (0x1f40d8 finds an existing entry first), quotes removed");
    require(runner.unhandledFunctions().empty(), "all three are handled");
}

void setPartyCharacterRecordsTheRequest() {
    Level level;
    add(level, "s", {{"SetPartyCharacter", {"Player1", "9", "1"}},
                     {"SetPartyCharacter", {"Player1", "16", "0"}},
                     {"SetPartyCharacter", {"Player2", "65537", "0"}},
                     {"SetPartyCharacter", {"Someone", "9", "1"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    chase::runtime::PartyFlags flags;
    flags.groupBytes = {1, 0};
    runner.partyNatives().party().setFlags(flags);
    runner.partyNatives().party().setUp(chase::runtime::partyList({9, 19, 29, 39, 49, 59, 69, 80}, {0, 0}, {}, false, false));
    runner.start("s");
    const auto requests = runner.takePartyRequests();
    require(requests.size() == 3, "an object other than Player1 and Player2 does nothing (0x27beb4 returns at once)");
    require(requests[0].slot == 0 && requests[0].character == 9 && requests[0].flag, "Player1: slot 0, character 9, flag 1");
    require(requests[1].character == 16 && !requests[1].flag, "(Player1, 16, 0)");
    require(requests[2].slot == 1 && requests[2].character == 1, "Player2: slot 1; the character is `& 0xffff` (65537 -> 1)");
    require(runner.takePartyRequests().empty(), "taken");
    using chase::runtime::PartyOutcome;
    require(requests[0].outcome == PartyOutcome::AlreadyCurrent && !requests[0].swap, "0x245258 returns at once for the slot of the player's character");
    require(requests[1].outcome == PartyOutcome::NotInParty, "character 16 has no slot in the party of the level row");
    require(requests[2].outcome == PartyOutcome::NotInParty, "character 1 has no slot either");
}

void streamRoomInRequestsAndWaitsForTheRoom() {
    Level level;
    add(level, "s", {{"StreamRoomIn", {"\"Room40\""}}, {"=", {"Done", "1"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.setRoomLoaded("Room40", false);
    runner.setRoomLoaded("Room8", true);
    runner.start("s");
    require(runner.threadCount() == 1 && runner.variable("Done") == 0, "0x1bfba4 returns 0 while the room is not loaded");
    require(runner.takeRoomRequests() == std::vector<std::string>{"room40"}, "the room goes onto the request list 0x20f194");
    runner.update(0.03f);
    require(runner.takeRoomRequests() == std::vector<std::string>{"room40"}, "and again at every pass while the script waits");
    runner.setRoomLoaded("Room40", true);
    runner.update(0.03f);
    require(runner.threadCount() == 0 && runner.variable("Done") == 1, "state 2 with a resource (0x2284bc) lets the script go on");
    require(runner.takeRoomRequests() == std::vector<std::string>{"room40"}, "the request is made before the loaded test");
}

void streamRoomInIgnoresUnknownRoomsAndUnstreamedLevels() {
    Level level;
    add(level, "s", {{"StreamRoomIn", {"\"Room99\""}}, {"=", {"A", "1"}}});
    add(level, "t", {{"StreamRoomIn", {"\"Room40\""}}, {"=", {"B", "1"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.setRoomLoaded("Room40", false);
    runner.start("s");
    require(runner.variable("A") == 1 && runner.takeRoomRequests().empty(), "a room the level does not have: the handler returns 1 without a request");
    runner.setAllRoomsLoaded();
    runner.start("t");
    require(runner.variable("B") == 1, "a level whose rooms are all resident does not wait");
}

void batchNativesAreListedAsHandled() {
    using chase::runtime::ScriptFunctionSupport;
    using chase::runtime::scriptFunctionSupport;
    for (const char* name : {"EffectFadeObject", "EnableDistanceFade", "AddToSemiTransNearCameraList", "SetPartyCharacter"})
        require(scriptFunctionSupport(name) == ScriptFunctionSupport::StandIn, name);
    require(scriptFunctionSupport("StreamRoomIn") == ScriptFunctionSupport::Implemented, "StreamRoomIn");
}
}

int main() {
    try {
        fadeInAndOut();
        zeroDurationAppliesFromThenFinishes();
        flagsSelectWhatIsApplied();
        aSecondFadeOfTheSameNodeReusesItsEntry();
        fullTableCompletesTheOldestEntry();
        distanceFadeAndSemiTransListRecordTheirArguments();
        setPartyCharacterRecordsTheRequest();
        streamRoomInRequestsAndWaitsForTheRoom();
        streamRoomInIgnoresUnknownRoomsAndUnstreamedLevels();
        batchNativesAreListedAsHandled();
    } catch (const std::exception& error) {
        std::cerr << "natives-world-test: FAIL: " << error.what() << '\n';
        return 1;
    }
    std::cout << "natives-world-test: ok\n";
    return 0;
}
