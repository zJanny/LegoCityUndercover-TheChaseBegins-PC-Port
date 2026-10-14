// Synthetic scripts for the object natives of batch SCRIPT-26 (src/runtime/natives_objects.cpp, KillScript in
// script_runner.cpp). The rules come from the handlers' disassembly (docs/re/script-natives.md, 2026-10-14 batch 26
// entry): ToggleUseable 0x1cdcf8, EnableTargetable 0x26fb2c, SetIndestructible 0x27be40, activeduringdcam 0x26f978,
// ObjectUnshovable 0x26fd68, ToggleUpdateFlag 0x270094, ResetObject 0x1b25f8, SkipToFinalState 0x270078, PrintDebugString
// 0x26fde8, revealcollectable 0x27bdb0, KillScript 0x1c3834 with 0x223a14. The scripts are made up.
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
                             "ResetObject", "SkipToFinalState"})
        require(chase::runtime::scriptFunctionSupport(name) == chase::runtime::ScriptFunctionSupport::StandIn, name);
    require(chase::runtime::scriptFunctionSupport("DestroyProp") == chase::runtime::ScriptFunctionSupport::None, "DestroyProp stays unhandled");
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
    } catch (const std::exception& error) {
        std::cerr << "natives-objects-test: FAIL: " << error.what() << '\n';
        return 1;
    }
    std::cout << "natives-objects-test: ok\n";
    return 0;
}
