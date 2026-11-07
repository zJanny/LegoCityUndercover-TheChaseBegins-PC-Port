// The four natives of mission 1-4 that had no handler: BulletTime 0x1a2df0 -> BulletTimeSystem 0x239330 / update 0x2ec070, VehicleSetAI 0x1bfb50 ->
// 0x1796e8, VehicleSetChased 0x26fe54 -> 0x1afd38, SpawnDebris2Colour 0x2860f0 -> 0x23aebc / 0x20c938. The rules were read from the disassembly of
// artifacts/game/game.arm.elf by 20261102-mission14-C (2026-11-02, llvm-objdump); the scripts and numbers are made up except the argument lists,
// which are those of the shipped scripts 1-4_SO_Phase2, 1-4_Car_Chase and 1-4_Hatch_Open (dumped from lvl_01 with the C++ level loader).
// 20261102-mission14b-C (2026-11-02, REA Ghidra decompile of 0x239330, 0x2ec070, 0x23ae84, 0x23ae50, 0x23878c): 97, 98 and 99 are sounds of the
// owner, not messages, and the blended value is the scheduler's tick scale C+0x1c; the step values are hand-computed from the rule of 0x100f44.
#include "runtime/bullet_time.hpp"
#include "runtime/scheduler.hpp"
#include "runtime/script_runner.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase;
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
    script.nameHash = assets::nameHash(name);
    script.calls = std::move(calls);
    level.scripts.push_back(std::move(script));
}

void bulletTimeBlendsTheTimeScale() {
    runtime::BulletTimeSystem system;
    require(!system.set(1.0f, 0.1f), "0x239330 does nothing for scale 1.0 while no blend runs (0x239380..0x239388)");
    require(!system.active() && system.sounds().empty(), "nothing was stored or played");
    require(system.set(0.25f, 0.125f), "a scale below 1.0 starts a blend");
    require(system.active() && system.baseScale() == 1.0f && system.targetScale() == 0.25f && system.duration() == 0.125f, "+0x58 base, +0x5c scale, +0x64 duration");
    require(system.mode() == runtime::kBulletTimeNoMode, "+0x68 is 255 for a scale below 1.0 with the flag clear (0x2393c8)");
    require(system.sounds() == std::vector<runtime::BulletTimeSound>({{false, 0, 0.0f, runtime::BulletTimeVoices::Pause}, {false, runtime::kBulletTimeSoundStart, 0.0f}}),
            "every voice paused (0x1eecec(1) -> 0x1cef9c(1)), then sound 97 at the owner (0x23ae84(0x61, owner))");
    require(!system.cameraShotRequested(), "the flag is clear: no camera shot (0x2393c4)");
    system.takeSounds();
    system.update(0.0625f, false);
    require(system.timeScale() == 0.625f, "halfway: 1.0 + 0.5 x (0.25 - 1.0) (0x2ec100..0x2ec114)");
    require(system.sounds().empty(), "no loop sound while 97 still runs (0x237acc(0x61, owner) not 0)");
    system.update(0.0625f, true);
    require(system.timeScale() == 0.25f && system.active(), "at the end the scale holds the target, the system stays on");
    require(system.sounds() == std::vector<runtime::BulletTimeSound>({{false, runtime::kBulletTimeSoundLoop, 0.0f}}), "98 once 97 has ended (0x2ec09c)");
    system.update(1.0f, false);
    require(system.timeScale() == 0.25f && system.active(), "progress above 1.0 is clamped to 1.0");
    system.takeSounds();
    require(system.set(1.0f, 0.0625f), "scale 1.0 while a blend holds 0.25 restarts the blend");
    require(system.baseScale() == 0.25f && system.targetScale() == 1.0f, "the base is the time scale at the call");
    const auto expected = std::vector<runtime::BulletTimeSound>(
        {{true, runtime::kBulletTimeSoundStart, 0.0625f}, {true, runtime::kBulletTimeSoundLoop, 0.0625f},
         {false, 0, 0.0f, runtime::BulletTimeVoices::Resume}, {false, runtime::kBulletTimeSoundEnd, 0.0f}});
    require(system.sounds() == expected, "restoring stops 97 and 98 with the duration as the fade, resumes the voices (0x1cef9c(0)), then plays 99 (0x239454..0x239488)");
    system.takeSounds();
    system.update(0.03125f, true);
    require(system.timeScale() == 0.625f, "0.25 + 0.5 x 0.75");
    require(system.sounds().empty(), "no loop sound while the target is 1.0");
    system.update(0.0625f, true);
    require(system.timeScale() == 1.0f && !system.active(), "a blend that ends at scale 1.0 switches the system off (0x2ec0f8)");
    require(!system.set(1.0f, 0.1f), "scale 1.0 again does nothing");
    system.set(0.5f, 1.0f);
    system.update(0.25f, false);
    system.stop();
    require(system.timeScale() == 1.0f && !system.active(), "0x2ec044: scale 1.0 and off");
}

void bulletTimeScriptCalls() {
    Level level;
    add(level, "s", {{"BulletTime", {"0.01", "0.1", "0"}}});
    add(level, "t", {{"BulletTime", {"1", "0.01", "0"}}});
    add(level, "short", {{"BulletTime", {"0.01", "0.1"}}});
    add(level, "shot", {{"BulletTime", {"0.2", "0.5", "1"}}});
    add(level, "negative", {{"BulletTime", {"0.2", "0.5", "-1"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    require(runner.start("short") && !runner.bulletTime().active(), "a call with fewer than 3 arguments does nothing");
    require(runner.start("negative") && runner.bulletTime().active() && !runner.bulletTime().cameraShotRequested(), "the flag is the third argument above 0 (0x1a2e0c..0x1a2e28)");
    runner.bulletTime().stop();
    require(runner.start("shot") && runner.bulletTime().cameraShotRequested(), "a third argument above 0 asks for the camera shot (0x2393d8..0x2393f0)");
    runner.bulletTime().stop();
    runner.bulletTime().clearCameraShotRequest();
    runner.bulletTime().takeSounds();
    require(runner.start("s") && runner.bulletTime().active(), "BulletTime 0.01 0.1 0 of 1-4_SO_Phase2");
    require(runner.bulletTime().targetScale() == 0.01f && runner.bulletTime().duration() == 0.1f, "arguments 0 and 1");
    runner.update(0.1f);
    require(runner.bulletTime().timeScale() == 1.0f, "the runner's update does not step the system: its clock is real time, stepped by the level");
    runner.bulletTime().update(0.1f, false);
    require(runner.bulletTime().timeScale() < 0.0101f && runner.bulletTime().timeScale() > 0.0099f, "the scale reaches 0.01 after the duration");
    require(runner.start("t") && runner.bulletTime().targetScale() == 1.0f, "BulletTime 1 0.01 0 of 1-4_SO_Phase2 restores");
    runner.bulletTime().update(0.02f, false);
    require(runner.bulletTime().timeScale() > 0.99999f && runner.bulletTime().timeScale() < 1.00001f && !runner.bulletTime().active(), "back at 1.0 after 0.01 s");
    require(runner.unhandledFunctions().empty(), "BulletTime has a handler");
    require(runtime::scriptFunctionSupport("BulletTime") == runtime::ScriptFunctionSupport::StandIn, "BulletTime is a stand-in (SCRIPT-S323)");
}

void bulletTimeScalesTheSchedulerStep() {
    require(runtime::scaledStepDt(0.033f, 1.0f) == 0.033f, "scale 1.0: 99 ticks of 3000, the step unchanged");
    require(runtime::scaledStepDt(0.033f, 0.5f) == 49.0f / 3000.0f, "ticks = (int)(scale x 99) (0x100f44, arm-executable.md)");
    require(runtime::scaledStepDt(0.033f, 0.01f) == 1.0f / 3000.0f, "at least one tick");
    runtime::StepConfig config;
    runtime::Scheduler scheduler(config, 0);
    float seen = 0;
    scheduler.advance(99, [&](float dt) { seen = dt; });
    require(seen == 99.0f / 3000.0f, "an unscaled step of 99 ticks");
    require(scheduler.lastRealDt() == 99.0f / 3000.0f, "the real step is the period");
    scheduler.setScale(0.01f);
    scheduler.advance(198, [&](float dt) { seen = dt; });
    require(seen == 1.0f / 3000.0f && scheduler.lastRealDt() == 99.0f / 3000.0f, "C+0x1c scales the ticks, not the period");
}

void vehicleAiNatives() {
    Level level;
    add(level, "ai", {{"VehicleSetAI", {"1-4_Stolen_Vehicle"}}});
    add(level, "chase", {{"VehicleSetChased", {"1-4_Stolen_Vehicle", "1-4_Vehicle_Path", "29"}}});
    add(level, "chase_only", {{"VehicleSetChased", {"Other_Car", "1-4_Vehicle_Path", "7"}}});
    add(level, "short_ai", {{"VehicleSetAI", {}}});
    add(level, "short_chase", {{"VehicleSetChased", {"Car", "Path"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.setObjectPositionSource([](const std::string& name, float out[3]) {
        if (name != "1-4_stolen_vehicle") return false;
        out[0] = 10.0f;
        out[1] = 2.0f;
        out[2] = -3.0f;
        return true;
    });
    runner.start("short_ai");
    runner.start("short_chase");
    require(runner.vehicleAi().size() == 0, "calls with too few arguments do nothing");
    runner.start("ai");
    const runtime::VehicleAiRecord* record = runner.vehicleAi().find("1-4_stolen_vehicle");
    require(record != nullptr, "VehicleSetAI makes the record (0x1796e8)");
    require(record->driver.flags == runtime::kAiDriverFlagAi && record->driver.heightOffset == 1.0f, "flags |= 1 and the attribute default 1.0 (0x179744, 0x179710)");
    require(record->driver.path[3] == runtime::AiVec3({10.0f, 2.0f, -3.0f}), "the node matrix row 3 is the placement (rotation stand-in SCRIPT-S323)");
    require(record->kinematic && record->driver.state == 0 && record->driver.requestedState == 0 && !record->chased, "kinematic on, zero-filled");
    runner.start("chase");
    record = runner.vehicleAi().find("1-4_stolen_vehicle");
    require(runner.vehicleAi().size() == 1, "VehicleSetChased uses the record of the vehicle");
    require(record->chased && !record->kinematic, "0x1afd38 turns the kinematic mode off (0x22a270(entity, 0))");
    require(record->requestedVehicleState == runtime::kVehicleChasedRequestedState && record->driver.requestedState == 5, "vehicle +4 = 2 (0x1afd98), driver +0x79 = 5 (0x1afdc0)");
    require(record->driver.cruiseSpeed == 29.0f && record->driver.targetSpeed == 29.0f && record->driver.speed == 0.0f, "+0x6c and +0x70 = the speed argument, +0x68 = 0 (0x1afdc4..0x1afdcc)");
    require(record->driver.route == assets::nameHash("1-4_Vehicle_Path") && record->routeHash == record->driver.route, "+4 = the path");
    require(record->driver.stateUpdates == 0, "+0xa4 = 0");
    runner.start("chase_only");
    const runtime::VehicleAiRecord* fresh = runner.vehicleAi().find("other_car");
    require(fresh != nullptr && fresh->chased && fresh->driver.flags == runtime::kAiDriverFlagAi, "a vehicle without a record gets one first (0x1afe3c)");
    require(runner.unhandledFunctions().empty(), "both natives have handlers");
    require(runtime::scriptFunctionSupport("VehicleSetAI") == runtime::ScriptFunctionSupport::StandIn &&
                runtime::scriptFunctionSupport("VehicleSetChased") == runtime::ScriptFunctionSupport::StandIn,
            "both are stand-ins (SCRIPT-S323)");
}

void chasedCallsOnARecord() {
    runtime::VehicleAiNatives natives;
    runtime::AiMatrix node{};
    natives.setAi("car", node);
    natives.mutableRecord("car")->driver.state = runtime::kAiStateSteer;
    natives.setChased("car", node, 1u, 5.0f);
    require(natives.find("car")->driver.state == runtime::kAiStateRoute, "a driver in state 5 is put back to state 2 (0x1afdb4..0x1afdbc)");
    natives.mutableRecord("car")->driver.state = runtime::kAiStateStopping;
    natives.setChased("car", node, 1u, 5.0f);
    require(natives.find("car")->driver.state == runtime::kAiStateStopping, "any other state stays");
    natives.setChased("car", node, 1u, 5.0f);
    natives.setChased("car", node, 2u, 6.0f);
    require(natives.find("car")->driver.route == 2u && natives.find("car")->driver.cruiseSpeed == 6.0f, "a second call replaces route and speeds");
    natives.setAi("car", node);
    require(!natives.find("car")->chased && natives.find("car")->driver.route == 0, "VehicleSetAI allocates a new zero-filled record each call (0x1796fc)");
}

void debrisBytes() {
    require(runtime::scriptDebrisByte(68.0f) == 68 && runtime::scriptDebrisByte(255.9f) == 255 && runtime::scriptDebrisByte(256.0f) == 0, "truncated, low byte kept");
    require(runtime::scriptDebrisByte(-1.0f) == 255, "-1 is 0xffffffff, low byte 255");
    require(runtime::scriptDebrisByte(1.0e20f) == 255 && runtime::scriptDebrisByte(-1.0e20f) == 0, "vcvt.s32.f32 saturates to 0x7fffffff / 0x80000000");
    require(runtime::scriptDebrisColour(68.0f, 68.0f, 68.0f) == 0xff444444u, "alpha 255 in the top byte (0x286138)");
    require(runtime::scriptDebrisColour(1.0f, 2.0f, 3.0f) == 0xff030201u, "r in the low byte");
}

void debrisClassColoursWin() {
    runtime::PropDestroyEffects none;
    runtime::DebrisBurst burst = runtime::scriptDebrisBurst(none, {1, 2, 3}, 0xff111111u, 0xff222222u);
    require(burst.colour == 0xff111111u && burst.secondColour == 0xff222222u && burst.count == 6 && burst.position == runtime::DebrisVec3({1, 2, 3}), "the call's colours and 6 bricks (0x20ca1c)");
    runtime::PropDestroyEffects both;
    both.hasColour = both.hasSecondColour = true;
    both.colour = 0xff0000ffu;
    both.secondColour = 0xff00ff00u;
    both.debrisAmount = 3;
    burst = runtime::scriptDebrisBurst(both, {0, 0, 0}, 0xff111111u, 0xff222222u);
    require(burst.colour == 0xff0000ffu && burst.secondColour == 0xff00ff00u && burst.count == 3, "DebrisColour, DebrisColour2 and DebrisAmount of the class win (0x20c978, 0x20ca3c, 0x20cb1c)");
    runtime::PropDestroyEffects first;
    first.hasColour = true;
    first.colour = 0xff0000ffu;
    burst = runtime::scriptDebrisBurst(first, {0, 0, 0}, 0xff111111u, std::nullopt);
    require(burst.colour == 0xff0000ffu && burst.secondColour == runtime::kDebrisNoSecondColour, "no second colour at all gives the default 0xffff00ff (0x20ca8c)");
}

void spawnDebrisNative() {
    Level level;
    assets::LevelObject locator;
    locator.className = "dummy/dummy";
    locator.instance = "BoosterSkylightDebris";
    level.objects.push_back(locator);
    add(level, "s", {{"SpawnDebris2Colour", {"BoosterSkylightDebris", "68", "68", "68", "68", "68", "68"}}});
    add(level, "short", {{"SpawnDebris2Colour", {"BoosterSkylightDebris", "68", "68", "68"}}});
    add(level, "unknown", {{"SpawnDebris2Colour", {"Nowhere", "1", "2", "3", "4", "5", "6"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    Level emptyClass;
    runner.loadBulletTimeVehicleDebrisNatives([&emptyClass](const std::string&) -> const Level* { return &emptyClass; });
    runner.setObjectPositionSource([](const std::string& name, float out[3]) {
        if (name != "boosterskylightdebris") return false;
        out[0] = 5.0f;
        out[1] = 6.0f;
        out[2] = 7.0f;
        return true;
    });
    runner.start("short");
    runner.start("unknown");
    require(runner.takeDebrisRequests().empty(), "too few arguments or an unplaced object start no burst");
    require(runner.start("s") && runner.unhandledFunctions().empty(), "SpawnDebris2Colour has a handler");
    const auto requests = runner.takeDebrisRequests();
    require(requests.size() == 1 && requests[0].object == "boosterskylightdebris", "one request");
    require(requests[0].burst.colour == 0xff444444u && requests[0].burst.secondColour == 0xff444444u && requests[0].burst.count == 6, "68 68 68 / 68 68 68 of 1-4_Hatch_Open, 6 bricks");
    require(requests[0].burst.position == runtime::DebrisVec3({5, 6, 7}), "at the object's position (box centre stand-in SCRIPT-S323)");
    require(runner.takeDebrisRequests().empty(), "taken once");
    require(runtime::scriptFunctionSupport("SpawnDebris2Colour") == runtime::ScriptFunctionSupport::StandIn, "stand-in (SCRIPT-S323)");
}
}

int main() {
    try {
        bulletTimeBlendsTheTimeScale();
        bulletTimeScalesTheSchedulerStep();
        bulletTimeScriptCalls();
        vehicleAiNatives();
        chasedCallsOnARecord();
        debrisBytes();
        debrisClassColoursWin();
        spawnDebrisNative();
    } catch (const std::exception& error) {
        std::cerr << "bullet-time-vehicle-debris-natives-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "bullet-time-vehicle-debris-natives-test: ok\n";
    return 0;
}
