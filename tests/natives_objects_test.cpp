// Synthetic scripts for the object natives of batch SCRIPT-26 (src/runtime/natives_objects.cpp, KillScript in
// script_runner.cpp). The rules come from the handlers' disassembly (docs/re/script-natives.md, 2026-10-14 batch 26
// entry): ToggleUseable 0x1cdcf8, EnableTargetable 0x26fb2c, SetIndestructible 0x27be40, activeduringdcam 0x26f978,
// ObjectUnshovable 0x26fd68, ToggleUpdateFlag 0x270094, ResetObject 0x1b25f8, SkipToFinalState 0x270078, PrintDebugString
// 0x26fde8, revealcollectable 0x27bdb0, KillScript 0x1c3834 with 0x223a14. The scripts are made up.
#include "runtime/script_runner.hpp"
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <tuple>

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

void valueNativesStoreTheirFlag() {
    Level level;
    add(level, "s", {{"ToggleUseable", {"Crate", "1"}},
                     {"ToggleUseable", {"Grab", "0"}},
                     {"EnableTargetable", {"Guy", "0"}},
                     {"EnableTargetable", {"Boss", "1"}},
                     {"EnableTargetable", {"Half", "0.5"}},
                     {"EnableTargetable", {"High", "256"}},
                     {"SetIndestructible", {"Jackhammer", "1"}},
                     {"SetIndestructible", {"Wheelbarrow", "0"}},
                     {"ActiveDuringDCam", {"Pig_Tractor", "1"}},
                     {"ObjectUnshovable", {"Monk", "1"}},
                     {"ObjectUnshovable", {"Douser", "0"}},
                     {"ToggleUpdateFlag", {"Fan", "0"}},
                     {"ToggleUpdateFlag", {"Door", "1"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    require(runner.start("s") && runner.threadCount() == 0, "runs to the end at once");
    const auto& natives = runner.objectNatives();
    require(natives.flags("crate")->usable == true && natives.flags("grab")->usable == false, "ToggleUseable: argument above 0");
    require(natives.flags("guy")->targetable == false && natives.flags("boss")->targetable == true, "EnableTargetable 0 / 1");
    require(natives.flags("half")->targetable == false, "EnableTargetable truncates 0.5 to 0 before the low byte test");
    require(natives.flags("high")->targetable == false, "EnableTargetable tests the low byte of the integer (256 -> 0)");
    require(natives.flags("jackhammer")->indestructible == true && natives.flags("wheelbarrow")->indestructible == false, "SetIndestructible");
    require(natives.flags("pig_tractor")->activeDuringDcam == true, "ActiveDuringDCam");
    require(natives.flags("monk")->unshovable == true && natives.flags("douser")->unshovable == false, "ObjectUnshovable");
    require(natives.flags("fan")->updateFlagBit == true && natives.flags("door")->updateFlagBit == false, "ToggleUpdateFlag sets its bit for 0");
    // 0x1a4094 skips the class update of an entity with mask 0x4 set; ToggleUpdateFlag(fan, 0) sets it, (door, 1) clears it (0x270094).
    require(natives.entityUpdateSkipped("fan") && !natives.entityUpdateSkipped("door") && !natives.entityUpdateSkipped("nobody"), "update skip");
    require(natives.flags("crate")->targetable == std::nullopt && natives.flags("nobody") == nullptr, "untouched flags stay unset");
    require(runner.unhandledFunctions().empty(), "no native of the batch is reported as unhandled");
}

void valueIsReadFromVariables() {
    Level level;
    add(level, "s", {{"=", {"Flag", "1"}}, {"ToggleUseable", {"Crate", "Flag"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.start("s");
    require(runner.objectNatives().flags("crate")->usable == true, "the value argument can be a variable");
}

void classMessagesAreQueued() {
    Level level;
    add(level, "s", {{"ResetObject", {"Platform"}}, {"SkipToFinalState", {"Door"}}, {"TriggerObject", {"Door", "1"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.start("s");
    const auto messages = runner.takeObjectClassMessages();
    require(messages.size() == 2, "two class messages");
    require(messages[0].instance == "platform" && messages[0].message == chase::runtime::kMessageReset && messages[0].message == 8, "ResetObject sends 8");
    require(messages[1].instance == "door" && messages[1].message == chase::runtime::kMessageSkipToFinalState && messages[1].message == 54, "SkipToFinalState sends 54");
    require(runner.takeObjectClassMessages().empty(), "messages are taken once");
    const auto triggers = runner.takeObjectMessages();
    require(triggers.size() == 1 && triggers[0].message == chase::runtime::kMessageTrigger, "the trigger queue only holds TriggerObject");
}

void noOpNativesContinue() {
    Level level;
    add(level, "s", {{"PrintDebugString", {"\"text\"", "5"}}, {"revealcollectable", {"Token"}}, {"=", {"Done", "1"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.start("s");
    require(runner.variable("Done") == 1 && runner.threadCount() == 0, "both return at once");
    require(runner.unhandledFunctions().empty() && runner.objectNatives().flags("token") == nullptr, "and change nothing");
}

void killScriptEndsOtherThreadsOfTheScript() {
    Level level;
    add(level, "victim", {{"Wait", {"1.5"}}, {"=", {"Victim", "1"}}});
    add(level, "killer", {{"Wait", {"1.5"}}, {"KillScript", {"victim"}}, {"=", {"Killer", "1"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.start("victim");
    runner.start("killer");
    require(runner.threadCount() == 2, "both wait");
    for (int i = 0; i < 100; ++i) runner.update(0.1f);
    require(runner.variable("Killer") == 1 && runner.variable("Victim") == 0 && runner.threadCount() == 0, "the victim ends without its last call");
}

void killScriptSparesItsOwnThread() {
    Level level;
    add(level, "self", {{"KillScript", {"self"}}, {"Wait", {"0.1"}}, {"=", {"Done", "1"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.start("self");
    runner.update(0.2f);
    require(runner.variable("Done") == 1, "the calling thread is not killed");
}

void killScriptWithoutThreadOrScriptIsHarmless() {
    Level level;
    add(level, "k", {{"KillScript", {"nobody"}}, {"KillScript", {"k2"}}, {"=", {"Done", "1"}}});
    add(level, "k2", {{"Wait", {"1"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.start("k");
    require(runner.variable("Done") == 1 && runner.threadCount() == 0, "no thread of an unknown or idle script");
}

void killScriptKillsEveryThreadOfTheScript() {
    Level level;
    add(level, "v", {{"Wait", {"5"}}, {"=", {"V", "1"}}});
    add(level, "k", {{"KillScript", {"v"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.start("v");
    runner.start("v");
    runner.start("k");
    runner.update(0.1f);
    require(runner.threadCount() == 0, "both threads of the script end in the next pass");
    runner.update(10.0f);
    require(runner.variable("V") == 0, "neither reached its last call");
}

void batchNativesAreListedAsHandled() {
    for (const char* name : {"PrintDebugString", "RevealCollectable", "KillScript"})
        require(chase::runtime::scriptFunctionSupport(name) == chase::runtime::ScriptFunctionSupport::Implemented, name);
    for (const char* name : {"ToggleUseable", "EnableTargetable", "SetIndestructible", "ActiveDuringDCam", "ObjectUnshovable", "ToggleUpdateFlag",
                             "ResetObject", "SkipToFinalState", "DestroyProp", "KillObject"})
        require(chase::runtime::scriptFunctionSupport(name) == chase::runtime::ScriptFunctionSupport::StandIn, name);
}

// 0x23cd80 (disassembly, docs/re/script-natives-world.md): DestroyProp and KillObject share 0x1a2fe4. An entity that is disabled
// (halfword +0x10 bit 0) or already destroyed (word +0xc bit 0x10) is left alone; otherwise message 1 goes to the class and an unhandled
// message destroys the entity.
void destroyPropRemovesAnEnabledEntity() {
    Level level;
    add(level, "d", {{"DestroyProp", {"Crate"}}, {"KillObject", {"Fence"}}, {"DestroyProp", {"Crate"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    require(runner.start("d") && runner.threadCount() == 0, "runs to the end at once");
    require(state.destroyed.count("crate") && state.destroyed.count("fence"), "both entities are destroyed");
    require(state.isDisabled("Crate"), "a destroyed entity counts as removed");
    const auto messages = runner.takeObjectClassMessages();
    require(messages.size() == 2 && messages[0].instance == "crate" && messages[0].message == 1 && messages[1].instance == "fence",
            "message 1 once per entity; the second DestroyProp finds the destroyed bit and sends nothing");
    require(runner.unhandledFunctions().empty(), "no native is reported as unhandled");
}

void destroyPropLeavesDisabledEntities() {
    Level level;
    add(level, "d", {{"EnableObject", {"Car", "0"}}, {"DestroyProp", {"Car"}}, {"EnableObject", {"Car", "1"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.start("d");
    require(state.destroyed.empty() && runner.takeObjectClassMessages().empty(), "0x23cd80 returns at once for a disabled entity");
    require(!state.isDisabled("car"), "the later EnableObject works as before");
}

chase::assets::LevelParam objectType(std::uint32_t value) {
    chase::assets::LevelParam param;
    param.hash = chase::assets::nameHash("ObjectType");
    param.flags = 5;
    param.type = 7;
    param.value = value;
    return param;
}

struct KindClasses {
    std::map<std::string, Level> classes;
    chase::assets::ClassLookup lookup() {
        return [this](const std::string& name) -> const Level* {
            const auto found = classes.find(name);
            return found == classes.end() ? nullptr : &found->second;
        };
    }
};

chase::assets::LevelParam triggerObjectParam(std::uint32_t flags, const std::string& target) {
    chase::assets::LevelParam param;
    param.hash = chase::assets::nameHash("TriggerObject");
    param.flags = std::uint16_t(flags);
    param.type = 4;
    chase::assets::Bytes block(8, 0);
    const std::uint32_t hash = target.empty() ? 0 : chase::assets::nameHash(target);
    for (int i = 0; i < 4; ++i) block[4 + i] = std::uint8_t(hash >> (8 * i));
    param.blocks.push_back(block);
    return param;
}

// Level with one object per (instance, class, kind). Kinds and the reaction of message 1 per kind: docs/re/script-natives-world.md, entry
// 2026-10-19T08:53:30. The class names are the real classes of the kinds (kind 85 leprops/leprop_simpledestructible, 64 props/prop_platform,
// 58 props/ability_phil_m_test, 42 leprops/leprop_door, 183 vehicles/vehicle_police_trooper, 27 chars/char_ai_prisoner01, 67 props/prop_prop).
struct DestroyFixture {
    KindClasses kinds;
    Level level;
    void object(const std::string& instance, const std::string& className, int kind, const std::string& triggerTarget = std::string()) {
        Level classFile;
        classFile.defaults.push_back(objectType(std::uint32_t(kind)));
        if (!triggerTarget.empty()) classFile.defaults.push_back(triggerObjectParam(2, std::string()));
        kinds.classes[className] = classFile;
        chase::assets::LevelObject placed;
        placed.instance = instance;
        placed.className = className;
        if (!triggerTarget.empty()) placed.params.push_back(triggerObjectParam(2, triggerTarget));
        level.objects.push_back(placed);
    }
};

void destroyPropActsPerKind() {
    DestroyFixture fixture;
    for (const auto& [instance, className, kind] : std::vector<std::tuple<std::string, std::string, int>>{
             {"Crate", "leprops/leprop_simpledestructible", 85}, {"Platform", "props/prop_platform", 64}, {"Switch", "props/ability_phil_m_test", 58},
             {"Door", "leprops/leprop_door", 42}, {"Truck", "vehicles/vehicle_police_trooper", 183}, {"Prisoner", "chars/char_ai_prisoner01", 27},
             {"Sign", "props/prop_prop", 67}})
        fixture.object(instance, className, kind);
    add(fixture.level, "d", {{"DestroyProp", {"Crate"}}, {"DestroyProp", {"Platform"}}, {"DestroyProp", {"Switch"}}, {"DestroyProp", {"Door"}},
                             {"DestroyProp", {"Truck"}}, {"KillObject", {"Prisoner"}}, {"KillObject", {"Sign"}}, {"DestroyProp", {"Crate"}},
                             {"DestroyProp", {"Nobody"}}});
    PlacedObjectState state;
    ScriptRunner runner(fixture.level, state);
    runner.loadObjectKinds(fixture.kinds.lookup());
    std::vector<std::string> told;
    runner.setClassMessageHandler([&](const std::string& instance, std::uint8_t message) {
        require(message == 1, "only message 1 reaches the class");
        told.push_back(instance);
    });
    require(runner.start("d") && runner.threadCount() == 0, "runs to the end");
    for (const char* name : {"crate", "platform", "switch", "door", "truck", "prisoner", "sign"})
        require(state.destroyed.count(name) == 1, std::string("no handler returns non-zero for message 1: ") + name + " is destroyed");
    for (const char* hidden : {"crate", "platform", "prisoner", "sign"})
        require(state.isHidden(hidden), std::string("message 1 reaches the disable 0x2428c0 for ") + hidden);
    for (const char* shown : {"switch", "door", "truck"})
        require(!state.isHidden(shown) && state.isDisabled(shown), std::string(shown) + " is destroyed but not disabled (stays visible)");
    require(told.size() == 8 && told[0] == "crate" && told[7] == "nobody", "the class is told once per entity that gets past the destroyed test");
    require(state.destroyed.count("nobody") == 1 && !state.isHidden("nobody"), "an instance without a kind has no reaction");
}

void destroyedEntityComesBackWithEnableObjectButWithoutCollision() {
    DestroyFixture fixture;
    fixture.object("Crate", "leprops/leprop_simpledestructible", 85);
    add(fixture.level, "d", {{"DestroyProp", {"Crate"}}, {"EnableObject", {"Crate", "1"}}, {"DestroyProp", {"Crate"}}});
    PlacedObjectState state;
    ScriptRunner runner(fixture.level, state);
    runner.loadObjectKinds(fixture.kinds.lookup());
    runner.start("d");
    require(!state.isHidden("crate"), "EnableObject clears the disabled bit and shows the node (0x242cf0 does not test the destroyed bit)");
    require(state.isDisabled("crate") && state.destroyed.count("crate") == 1, "the destroyed bit stays (the port counts it as disabled for collision, SCRIPT-304)");
    require(runner.takeObjectClassMessages().size() == 1, "a destroyed entity gets no second message 1 even when it is shown again");
}

void resetObjectAnswersMessage8PerKind() {
    DestroyFixture fixture;
    fixture.object("Platform", "props/prop_platform", 64);
    fixture.object("Pickup", "pickups/pickup_redbrick", 132);
    fixture.object("Crate", "leprops/leprop_simpledestructible", 85);
    add(fixture.level, "d", {{"DestroyProp", {"Platform"}}, {"DestroyProp", {"Pickup"}}, {"DestroyProp", {"Crate"}},
                             {"ResetObject", {"Platform"}}, {"ResetObject", {"Pickup"}}, {"ResetObject", {"Crate"}}});
    PlacedObjectState state;
    ScriptRunner runner(fixture.level, state);
    runner.loadObjectKinds(fixture.kinds.lookup());
    runner.start("d");
    require(!state.isHidden("platform") && state.destroyed.count("platform") == 1,
            "kind 64 answers message 8 itself (0x1a90b8): it enables the entity (0x242cf0) and never clears the destroyed bit");
    require(runner.objectNatives().restoredPlacement("platform") == nullptr, "the platform's placement comes from its record (SCRIPT-317), not initialposition");
    require(!state.isHidden("pickup") && state.destroyed.count("pickup") == 0 && runner.objectNatives().restoredPlacement("pickup") != nullptr,
            "kind 132 answers message 8 itself (0x1ba268): placement restored, enabled, destroyed bit (0x1ba320) cleared");
    require(state.isDisabled("platform") && !state.isDisabled("pickup"), "a reset pickup collides again; the destroyed platform still counts as disabled (SCRIPT-304)");
    require(state.destroyed.count("crate") && state.isHidden("crate") && runner.objectNatives().restoredPlacement("crate") == nullptr,
            "kind 85 has another handler for message 8 that is not ported");
    DestroyFixture again;
    again.object("Pickup", "pickups/pickup_redbrick", 132);
    add(again.level, "d", {{"DestroyProp", {"Pickup"}}, {"ResetObject", {"Pickup"}}, {"DestroyProp", {"Pickup"}}});
    PlacedObjectState secondState;
    ScriptRunner second(again.level, secondState);
    second.loadObjectKinds(again.kinds.lookup());
    second.start("d");
    require(second.takeObjectClassMessages().size() == 3 && secondState.destroyed.count("pickup") && secondState.isHidden("pickup"),
            "a reset pickup gets message 1 again: the destroyed test passes after the reset");
}

void destroyRaisesTheDeadEventAndTheTriggerObjectOfThePropHandler() {
    DestroyFixture fixture;
    fixture.object("Crate", "leprops/leprop_simpledestructible", 85, "Gate");
    fixture.object("Gate", "props/prop_plain", 86);
    fixture.object("Sign", "props/prop_prop", 67, "Gate");
    chase::assets::LevelTrigger dead;
    dead.nameHash = chase::assets::nameHash("CrateDead");
    dead.event = "Dead";
    dead.objectName = "Crate";
    dead.scriptName = "crate_dead";
    fixture.level.triggers.push_back(dead);
    chase::assets::LevelTrigger triggered;
    triggered.nameHash = chase::assets::nameHash("GateOn");
    triggered.event = "ObjectTriggered";
    triggered.objectName = "Gate";
    triggered.scriptName = "gate_on";
    fixture.level.triggers.push_back(triggered);
    add(fixture.level, "sign", {{"EnableObject", {"Gate", "0"}}, {"DestroyProp", {"Sign"}}});
    add(fixture.level, "crate", {{"DestroyProp", {"Crate"}}});
    add(fixture.level, "crate_dead", {{"=", {"CrateDead", "1"}}});
    add(fixture.level, "gate_on", {{"=", {"GateOn", "1"}}});
    PlacedObjectState state;
    ScriptRunner runner(fixture.level, state);
    runner.loadObjectKinds(fixture.kinds.lookup());
    runner.start("sign");
    require(state.destroyed.count("sign") == 1 && state.isHidden("sign"), "kind 67 hides");
    require(state.isHidden("gate"), "kind 67 does not run the TriggerObject of the base prop handler");
    runner.start("crate");
    const float player[3] = {0, 0, 0};
    runner.checkTriggers(player);
    runner.update(0.034f);
    runner.checkTriggers(player);
    runner.update(0.034f);
    require(runner.variable("CrateDead") == 1, "the Dead event is raised before the entity is disabled, so its trigger finds it enabled");
    require(runner.variable("GateOn") == 1 && !state.isDisabled("gate"), "kind 85 runs the base prop handler: TriggerObject enables the Gate and raises ObjectTriggered");
    bool triggerMessage = false;
    for (const auto& message : runner.takeObjectMessages()) triggerMessage |= message.instance == "gate" && message.message == chase::runtime::kMessageTrigger;
    require(triggerMessage, "0x244f40 sends message 0xff to the triggered object");
}

void destroyReactionTableKeepsRealClassKinds() {
    using chase::runtime::DestroyReaction;
    using chase::runtime::destroyReaction;
    for (const int hides : {27, 64, 67, 71, 85, 102, 119, 134, 204, 214, 242}) require(destroyReaction(std::uint8_t(hides)) == DestroyReaction::HidesEntity, "hides " + std::to_string(hides));
    for (const int none : {42, 58, 38, 139, 141, 185, 186, 6, 255}) require(destroyReaction(std::uint8_t(none)) == DestroyReaction::NoReaction, "no reaction " + std::to_string(none));
    require(destroyReaction(183) == DestroyReaction::BreaksVehicle, "the vehicle class breaks");
    for (const int unread : {31, 45, 100, 125, 159, 208, 222, 243}) require(!chase::runtime::destroyReactionRead(std::uint8_t(unread)), "unread " + std::to_string(unread));
    require(chase::runtime::destroyReactionRead(85) && chase::runtime::destroyReactionRead(58), "read kinds");
}

void kindGatesDecideTheRegistryAndShadowNatives() {
    // Kinds from docs/re/object-kinds.md: 58 (prop_switchpull) and 20 (parkour_brickgrab_new) register a use zone with 0x23e130, 42
    // (prop_door), 86 and 71 do not; 27 is a character, 159 (chars/ride_pig) is in the shadow kind list through its creator 0x23e394.
    KindClasses kinds;
    for (const auto& [name, kind] : std::vector<std::pair<std::string, int>>{{"pull", 58}, {"grab", 20}, {"door", 42}, {"plain", 86}, {"char", 27}, {"pig", 159}, {"box", 67}}) {
        Level classFile;
        classFile.defaults.push_back(objectType(std::uint32_t(kind)));
        kinds.classes[name] = classFile;
    }
    Level level;
    for (const auto& [instance, className] : std::vector<std::pair<std::string, std::string>>{
             {"Pull", "pull"}, {"Grab", "grab"}, {"Door", "door"}, {"Plain", "plain"}, {"Guy", "char"}, {"Pig", "pig"}, {"Box", "box"}}) {
        chase::assets::LevelObject object;
        object.instance = instance;
        object.className = className;
        level.objects.push_back(object);
    }
    chase::runtime::ObjectNativeState natives;
    natives.loadKinds(level, kinds.lookup());
    using chase::runtime::ObjectNativeKind;
    for (const char* name : {"pull", "grab", "door", "plain"}) natives.apply(ObjectNativeKind::ToggleUseable, name, 1.0f);
    require(natives.flags("pull")->usable == true && natives.flags("grab")->usable == true, "kinds 58 and 20 register a use zone");
    require(natives.flags("door") == nullptr && natives.flags("plain") == nullptr, "kinds 42 and 86 register none");
    natives.apply(ObjectNativeKind::EnableSwitch, "door", 0.0f);
    natives.apply(ObjectNativeKind::EnableSwitch, "pull", 0.0f);
    require(natives.flags("door") == nullptr || !natives.flags("door")->usable, "EnableSwitch writes no flag for an unregistered kind");
    require(natives.flags("pull")->usable == false && natives.flags("pull")->switchState == 3, "EnableSwitch off on kind 58: flag and state 3");
    natives.apply(ObjectNativeKind::EnableShadow, "guy", 0.0f);
    natives.apply(ObjectNativeKind::EnableShadow, "pig", 1.0f);
    natives.apply(ObjectNativeKind::EnableShadow, "box", 1.0f);
    require(natives.flags("guy")->shadow == false && natives.flags("pig")->shadow == true && natives.flags("box") == nullptr,
            "EnableShadow: kinds 27 and the creator-0x23e394 kinds act, kind 67 does not");
}

void enableObjectFadeEndsLikeEnableObjectUnlessTheKindAnswers() {
    // Kinds 27 and 102 are the handlers that compare the message with 24 (0x18e588, 0x18c104); kind 102 is leprops/leprop_topple.
    KindClasses kinds;
    for (const auto& [name, kind] : std::vector<std::pair<std::string, int>>{{"plain", 0}, {"topple", 102}, {"fan", 47}}) {
        Level classFile;
        classFile.defaults.push_back(objectType(std::uint32_t(kind)));
        kinds.classes[name] = classFile;
    }
    Level level;
    for (const auto& [instance, className] : std::vector<std::pair<std::string, std::string>>{{"Beam", "plain"}, {"Tree", "topple"}, {"Blower", "fan"}}) {
        chase::assets::LevelObject object;
        object.instance = instance;
        object.className = className;
        level.objects.push_back(object);
    }
    add(level, "s", {{"enableobjectfade", {"Beam", "0"}}, {"enableobjectfade", {"Tree", "0"}}, {"enableobjectfade", {"Blower", "0"}},
                     {"enableobjectfade", {"Blower", "1"}}, {"enableobjectfade", {"Beam", "0.5"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.loadObjectKinds(kinds.lookup());
    require(runner.start("s") && runner.threadCount() == 0, "runs to the end");
    require(state.isDisabled("beam") == false, "0.5 is not 0: enabled again");
    require(!state.isDisabled("blower"), "0 then 1 enables");
    require(!state.isDisabled("tree"), "a kind with a message 24 handler is not ported (SCRIPT-S241)");
    add(level, "t", {{"enableobjectfade", {"Beam", "0"}}});
    PlacedObjectState second;
    ScriptRunner other(level, second);
    other.loadObjectKinds(kinds.lookup());
    other.start("t");
    require(second.isDisabled("beam"), "argument 0 disables like EnableObject");
    require(chase::runtime::scriptFunctionSupport("enableobjectfade") == chase::runtime::ScriptFunctionSupport::StandIn, "listed as a stand-in");
}
}

int main() {
    try {
        valueNativesStoreTheirFlag();
        valueIsReadFromVariables();
        classMessagesAreQueued();
        noOpNativesContinue();
        killScriptEndsOtherThreadsOfTheScript();
        killScriptSparesItsOwnThread();
        killScriptWithoutThreadOrScriptIsHarmless();
        killScriptKillsEveryThreadOfTheScript();
        batchNativesAreListedAsHandled();
        destroyPropRemovesAnEnabledEntity();
        destroyPropLeavesDisabledEntities();
        destroyPropActsPerKind();
        destroyedEntityComesBackWithEnableObjectButWithoutCollision();
        resetObjectAnswersMessage8PerKind();
        destroyRaisesTheDeadEventAndTheTriggerObjectOfThePropHandler();
        destroyReactionTableKeepsRealClassKinds();
        kindGatesDecideTheRegistryAndShadowNatives();
        enableObjectFadeEndsLikeEnableObjectUnlessTheKindAnswers();
    } catch (const std::exception& error) {
        std::cerr << "natives-objects-test: FAIL: " << error.what() << '\n';
        return 1;
    }
    std::cout << "natives-objects-test: ok\n";
    return 0;
}
