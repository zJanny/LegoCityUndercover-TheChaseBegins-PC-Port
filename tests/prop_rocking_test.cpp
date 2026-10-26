// SetPropRocking 0x1da6a8, the rocking 0x28a758 of the ObjectType 67 props (record fields of the create 0x16a268) and WaitForPlatform 0x263b84,
// with made-up objects and scripts (docs/re/props-rocking.md). Expected rotations are hand evaluations of 0x28a758 (20261025-garage-C, 2026-10-25):
// pitch = 0.0349066 * sin(2 pi f1), roll = 0.0436332 * cos(2 pi f2), rows of 0x237c48(roll, 0, pitch) times the rows of the base angles.
#include "runtime/prop_rocking.hpp"
#include "runtime/platform_wait.hpp"
#include "runtime/script_runner.hpp"

#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>

namespace {
using chase::assets::Level;
using chase::assets::LevelObject;
using chase::assets::LevelParam;
using chase::assets::LevelScript;
using chase::assets::ScriptCall;
using chase::runtime::PlatformRecord;
using chase::runtime::PropRocking;
using chase::runtime::ScriptRunner;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

LevelParam intParam(const char* name, std::uint32_t value) {
    LevelParam param;
    param.hash = chase::assets::nameHash(name);
    param.flags = 5;
    param.type = 7;
    param.value = value;
    return param;
}

void add(Level& level, const std::string& name, std::vector<ScriptCall> calls) {
    LevelScript script;
    script.nameHash = chase::assets::nameHash(name);
    script.calls = std::move(calls);
    level.scripts.push_back(std::move(script));
}

struct Classes {
    std::map<std::string, Level> files;
    chase::assets::ClassLookup lookup() {
        return [this](const std::string& name) -> const Level* {
            const auto found = files.find(name);
            return found == files.end() ? nullptr : &found->second;
        };
    }
};

LevelObject placed(const std::string& instance, const std::string& className) {
    LevelObject object;
    object.instance = instance;
    object.className = className;
    return object;
}

void recordFieldsComeFromTheCreate() {
    Level girder;
    girder.defaults = {intParam("ObjectType", 67), intParam("Rocking", 1), intParam("WaveBob1", 4), intParam("WaveBob2", 5), intParam("WaveBobOffset", 7)};
    Level plain;
    plain.defaults = {intParam("ObjectType", 67), intParam("Rocking", 1)};
    Level even;
    even.defaults = {intParam("ObjectType", 67), intParam("Rocking", 2)};
    Level platform;
    platform.defaults = {intParam("ObjectType", 64), intParam("Rocking", 1)};
    PropRocking prop;
    require(chase::runtime::propRockingOf(placed("Girder", "g"), girder, prop), "kind 67 with Rocking 1 has the bit");
    require(prop.instance == "girder" && prop.rocking && prop.period1 == 120 && prop.period2 == 150 && prop.phase == 7,
            "+0xb4 = WaveBob1 * 30, +0xb8 = WaveBob2 * 30, +0xbc = WaveBobOffset (0x16a860..0x16a8c8, 0x23ca4c = 30)");
    require(chase::runtime::propRockingOf(placed("Plain", "p"), plain, prop) && prop.period1 == 60 && prop.period2 == 90 && prop.phase == 0,
            "defaults 2, 3 and 0 (the r2 of the three 0x2451d0 calls)");
    require(chase::runtime::propRockingOf(placed("Even", "e"), even, prop) && !prop.rocking,
            "Rocking 2 keeps a record (the float test 0x16a3dc) and bit 6 takes bit 0 of the int (`and r0, r1, r2, lsl #6` at 0x16a528)");
    require(!chase::runtime::propRockingOf(placed("Lift", "l"), platform, prop), "only the kind 67 create 0x16a268 reads Rocking");
}

LevelParam floatParam(const char* name, float value) {
    LevelParam param;
    param.hash = chase::assets::nameHash(name);
    param.flags = 5;
    param.type = 0;
    std::memcpy(&param.value, &value, 4);
    return param;
}

LevelParam referenceParam(const char* name, const std::string& target) {
    LevelParam param;
    param.hash = chase::assets::nameHash(name);
    param.flags = 5;
    param.type = 4;
    chase::assets::Bytes block(8, 0);
    const std::uint32_t hash = target.empty() ? 0u : chase::assets::nameHash(target);
    std::memcpy(block.data() + 4, &hash, 4);
    param.blocks.push_back(block);
    return param;
}

void recordRuleOfTheCreate() {
    using chase::runtime::kindPropHasRecord;
    const auto has = [](std::vector<LevelParam> defaults) {
        Level cls;
        defaults.insert(defaults.begin(), intParam("ObjectType", 67));
        cls.defaults = std::move(defaults);
        return kindPropHasRecord(placed("Prop", "p"), cls);
    };
    require(!has({}) && !has({intParam("Rocking", 0), floatParam("Rotating", 0.0f)}), "mode 0 without the listed parameters: no record (0x16a40c)");
    require(has({intParam("Destructable", 1)}) && has({intParam("Destructable", 6)}), "a mode other than 0 keeps a record (0x16a334..0x16a35c)");
    require(has({referenceParam("ControlObject", "Lift")}) && has({referenceParam("TriggerObject", "Door")}), "ControlObject or TriggerObject naming an object");
    require(!has({referenceParam("TriggerObject", "")}), "an unset reference (STAND-IN SCRIPT-S297: name hash 0)");
    require(has({intParam("Navnode", 0)}), "Navnode listed by the class (the pointer test at 0x16a39c)");
    require(has({intParam("ATTR_SFX", 3)}) && has({intParam("ATTR_SFX_DESTROYED", 4)}), "the ints ATTR_SFX and ATTR_SFX_DESTROYED (0x16a3a0, 0x16a3a4)");
    require(has({floatParam("Rotating", 0.5f)}) && has({floatParam("Rocking", 0.5f)}) && has({floatParam("FaceTargetSpeed", 2.0f)}),
            "the floats Rotating, Rocking and FaceTargetSpeed compared with 0.0 (0x16a3c0, 0x16a3e0, 0x16a400)");
    Level rotating;
    rotating.defaults = {intParam("ObjectType", 67), floatParam("Rotating", 1.0f)};
    Classes classes;
    classes.files["props/prop_prop"] = rotating;
    Level level;
    level.objects = {placed("Spinner", "props/prop_prop")};
    add(level, "start", {{"SetPropRocking", {"Spinner", "1"}}});
    chase::assets::PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.loadObjectKinds(classes.lookup());
    require(runner.propRocking().find("spinner") && !runner.propRocking().find("spinner")->rocking, "a record without the Rocking bit");
    runner.start("start");
    require(runner.propRocking().find("spinner")->rocking, "SetPropRocking(x, 1) sets the bit of any kind 67 record");
}

void rotationFollowsTheTwoWaves() {
    PropRocking prop;
    prop.period1 = 60;
    prop.period2 = 90;
    const std::array<float, 9> rock = chase::runtime::rockingRotation(prop, 15);
    const std::array<float, 9> expected = {0.9993908f, 0.0348912f, 0.0007613f, -0.0348995f, 0.9991530f, 0.0218016f, 0.0f, -0.0218149f, 0.9997620f};
    for (int i = 0; i < 9; ++i) require(std::fabs(rock[i] - expected[i]) < 2e-6f, "clock 15: pitch 2 degrees (15 / 60 of a sine), roll cos(60 degrees) * 2.5 degrees, element " + std::to_string(i));
    prop.baseAngles = {0.0f, 0.5f, 0.0f};
    const std::array<float, 9> turned = chase::runtime::rockingRotation(prop, 15);
    const std::array<float, 9> turnedExpected = {0.8774130f, 0.0348912f, -0.4784654f, -0.0201750f, 0.9991530f, 0.0358644f, 0.4793114f, -0.0218149f, 0.8773737f};
    for (int i = 0; i < 9; ++i) require(std::fabs(turned[i] - turnedExpected[i]) < 2e-6f, "0x25898c: rock rows times base rows, element " + std::to_string(i));
    prop.phase = 45;
    const std::array<float, 9> wrapped = chase::runtime::rockingRotation(prop, 60 * 90 + 15 - 45);
    for (int i = 0; i < 9; ++i) require(std::fabs(wrapped[i] - turned[i]) < 2e-6f, "the phase is added to the clock before the unsigned remainder");
    prop.placement[12] = 947.88f;
    prop.placement[13] = 27.48f;
    prop.placement[15] = 1.0f;
    const std::array<float, 16> node = chase::runtime::rockingMatrix(prop, 60 * 90 + 15 - 45);
    require(node[12] == 947.88f && node[13] == 27.48f && node[15] == 1.0f && node[3] == 0.0f && std::fabs(node[0] - turned[0]) < 1e-7f,
            "0x232788 copies the three rotation rows; the translation stays");
}

void setPropRockingWritesTheBit() {
    Classes classes;
    classes.files["leprops/leprop_prop"].defaults = {intParam("ObjectType", 67), intParam("Rocking", 1)};
    Level level;
    level.objects = {placed("Crane_Girder", "leprops/leprop_prop")};
    add(level, "stop", {{"SetPropRocking", {"Crane_Girder", "0"}}});
    add(level, "start", {{"SetPropRocking", {"Crane_Girder", "1.9"}}});
    add(level, "two", {{"SetPropRocking", {"Crane_Girder", "2"}}, {"SetPropRocking", {"Nobody", "1"}}});
    chase::assets::PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.loadObjectKinds(classes.lookup());
    require(runner.propRocking().find("crane_girder") && runner.propRocking().find("crane_girder")->rocking, "Rocking 1 from the create");
    runner.start("stop");
    require(!runner.propRocking().find("crane_girder")->rocking, "SetPropRocking(x, 0) clears bit 6 of +0xde");
    runner.start("start");
    require(runner.propRocking().find("crane_girder")->rocking, "the value is truncated (vcvt.s32.f32): 1.9 is 1 and sets the bit");
    runner.start("two");
    require(!runner.propRocking().find("crane_girder")->rocking, "any value other than 1 clears it");
    require(runner.threadCount() == 0 && runner.unhandledCalls("setproprocking") == 0, "the native returns 1 and is handled");
}

void waitForPlatformComparesValueBitsAndState() {
    using chase::runtime::platformReached;
    require(platformReached({1.0f, 0}), "value 1.0 (0x263b98)");
    require(platformReached({2.0f, 0}), "a value above 1.0 passes the signed compare of the bits (0x263bb0 bge)");
    require(!platformReached({0.99999994f, 0}), "a value below 1.0 waits");
    require(!platformReached({-1.0f, 0}) && !platformReached({-2.0f, 0}), "negative values have the sign bit set and compare below");
    require(platformReached({std::numeric_limits<float>::infinity(), 0}), "+inf has bits above 0x3f800000");
    require(platformReached({0.0f, 5}) && !platformReached({0.0f, 4}), "state 5 at R+2 ends the wait (0x263bbc)");
}

void waitForPlatformBlocksTheThread() {
    Classes classes;
    classes.files["props/prop_platform"].defaults = {intParam("ObjectType", 64), intParam("type", 1)};
    Level level;
    level.objects = {placed("Crane_Platform_Dummy", "props/prop_platform")};
    add(level, "wait", {{"WaitForPlatform", {"Crane_Platform_Dummy"}}, {"SetPropRocking", {"Girder", "0"}}});
    add(level, "go", {{"Wait", {"0.01"}}, {"TriggerObject", {"Crane_Platform_Dummy", "1"}}, {"WaitForPlatform", {"Crane_Platform_Dummy"}}});
    add(level, "other", {{"WaitForPlatform", {"NotAPlatform"}}});
    chase::assets::PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.loadObjectKinds(classes.lookup());
    runner.start("wait");
    require(runner.threadCount() == 1, "the native returns 0 while the record is not there: the thread blocks");
    runner.update(1.0f / 30.0f);
    require(runner.threadCount() == 1, "it asks again every update");
    runner.platforms().find("crane_platform_dummy")->state = 5;
    runner.update(1.0f / 30.0f);
    require(runner.threadCount() == 0, "state 5 lets the thread go on");
    chase::assets::PlacedObjectState second;
    ScriptRunner standIn(level, second);
    standIn.loadObjectKinds(classes.lookup());
    standIn.start("go");
    // RPC 20261025-garage-C-1 (recorded from a save state on the generator trigger spot of mission 1-3, stepped dt 1/30 where the game's dt
    // 0x23602c read 0.033): TriggerObject(1-3_Crane_Platform_Dummy, 1) at u90 gives R+2 = R+4 = 2 and the value 0 at u90, then +0.0099 per
    // update (0.594 at u150, 0.9999 at u191), 1.0 and state 5 at u192; WaitForPlatform stops at every update u90..u193 and the script goes on at u193.
    constexpr float kTraceDt = 0.033f;
    const PlatformRecord* crane = standIn.platforms().find("crane_platform_dummy");
    int updates = 0;
    while (standIn.threadCount() > 0 && updates < 400) {
        standIn.update(kTraceDt);
        ++updates;
        if (updates == 1) require(crane->state == 2 && crane->value == 0.0f, "u90: the trigger requests state 2 (0x2150bc), no step yet");
        if (updates == 61) require(std::fabs(crane->value - 0.594f) < 1e-4f, "u150: 60 steps of dt * 30 * speed (0x280980, speed 0.01)");
        if (updates == 102) require(std::fabs(crane->value - 0.9999f) < 1e-4f && crane->state == 2, "u191: 0.9999, still moving");
        if (updates == 103) require(crane->value == 1.0f && crane->state == 5, "u192: 1.0 and state 5");
    }
    require(updates == 104, "the script goes on at u193, 103 updates after the trigger, got " + std::to_string(updates));
    standIn.start("other");
    require(standIn.threadCount() == 0, "an object without a platform record does not block (the original reads another record; no shipped call does this)");
}

}

int main() {
    try {
        recordFieldsComeFromTheCreate();
        recordRuleOfTheCreate();
        rotationFollowsTheTwoWaves();
        setPropRockingWritesTheBit();
        waitForPlatformComparesValueBitsAndState();
        waitForPlatformBlocksTheThread();
    } catch (const std::exception& error) {
        std::cerr << "prop-rocking: " << error.what() << '\n';
        return 1;
    }
    std::cout << "prop-rocking: ok\n";
    return 0;
}
