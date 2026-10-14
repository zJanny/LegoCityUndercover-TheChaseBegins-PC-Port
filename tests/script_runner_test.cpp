// Synthetic edge cases of the script runner (src/runtime/script_runner.cpp). The rules come from the disassembly of the
// executor 0x234204, Wait 0x174604, CallScript / SpawnScript 0x1c3730, thread creation 0x22ce9c (head insertion) and
// the runner 0x169254 (docs/re/scripts.md, 2026-10-14 entry); the scripts here are made up.
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

void waitCountsFromTheSecondCall() {
    Level level;
    add(level, "w", {{"Wait", {"0.1"}}, {"=", {"Done", "1"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    require(runner.start("w"), "start");
    require(runner.threadCount() == 1 && runner.variable("Done") == 0, "Wait blocks on its first call");
    runner.update(0.034f);
    runner.update(0.034f);
    require(runner.variable("Done") == 0, "0.1 - 2 * 0.034 > 0");
    runner.update(0.034f);
    require(runner.variable("Done") == 1 && runner.threadCount() == 0, "Wait ends when the timer reaches 0 or below");
}

void waitZeroYieldsOnce() {
    Level level;
    add(level, "w", {{"Wait", {"0"}}, {"=", {"Done", "1"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.start("w");
    require(runner.variable("Done") == 0, "Wait 0 still yields");
    runner.update(0.034f);
    require(runner.variable("Done") == 1, "Wait 0 ends in the next update");
}

void callScriptSuspendsTheParent() {
    Level level;
    add(level, "parent", {{"CallScript", {"child"}}, {"=", {"ParentDone", "1"}}});
    add(level, "child", {{"Wait", {"0.05"}}, {"=", {"ChildDone", "1"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.start("parent");
    require(runner.threadCount() == 2 && runner.variable("ParentDone") == 0, "parent waits for the child");
    require(runner.running("parent", false) && !runner.running("parent", true), "parent suspended");
    runner.update(0.034f);
    require(runner.variable("ChildDone") == 0 && runner.variable("ParentDone") == 0, "child still waiting");
    runner.update(0.034f);
    require(runner.variable("ChildDone") == 1, "child ended");
    require(runner.variable("ParentDone") == 1 && runner.threadCount() == 0, "the child is ahead of its parent: the parent resumes in the same update");
}

void callScriptOfAnEndingChildContinuesAtOnce() {
    Level level;
    add(level, "parent", {{"CallScript", {"child"}}, {"=", {"ParentDone", "1"}}});
    add(level, "child", {{"=", {"ChildDone", "1"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.start("parent");
    require(runner.variable("ParentDone") == 1 && runner.threadCount() == 0, "child without yield returns at once");
}

void spawnScriptDoesNotSuspend() {
    Level level;
    add(level, "parent", {{"SpawnScript", {"child"}}, {"=", {"ParentDone", "1"}}});
    add(level, "child", {{"=", {"ChildStarted", "1"}}, {"Wait", {"1"}}, {"=", {"ChildDone", "1"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.start("parent");
    require(runner.variable("ChildStarted") == 1, "child runs until it yields");
    require(runner.variable("ParentDone") == 1 && runner.threadCount() == 1, "parent continues and ends");
}

void newThreadsWaitForTheNextPass() {
    Level level;
    add(level, "spawner", {{"Wait", {"0"}}, {"SpawnScript", {"counter"}}});
    add(level, "counter", {{"=+", {"Count", "Count", "\"+\"", "1"}}, {"Wait", {"0"}}, {"=+", {"Count", "Count", "\"+\"", "1"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.start("spawner");
    runner.update(0.034f);
    require(runner.variable("Count") == 1, "spawned thread ran once at creation and not again in the same pass");
    runner.update(0.034f);
    require(runner.variable("Count") == 2 && runner.threadCount() == 0, "next pass");
}

void conditions() {
    Level level;
    add(level, "c", {{"=", {"A", "2"}},
                     {"?", {"A", "\"==\"", "1"}}, {"=", {"Branch", "1"}},
                     {"?!", {"A", "\"==\"", "2"}}, {"=", {"Branch", "2"}},
                     {"!?", {}}, {"=", {"Branch", "3"}},
                     {"??", {}},
                     {"?", {"A", "\">\"", "5"}}, {"=", {"Other", "1"}}, {"!?", {}}, {"=", {"Other", "2"}}, {"??", {}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.start("c");
    require(runner.variable("Branch") == 2, "else-if taken");
    require(runner.variable("Other") == 2, "else taken");
}

void unhandledFunctionsAreRecorded() {
    Level level;
    add(level, "u", {{"PlaySound", {"1"}}, {"PlaySound", {"2"}}, {"EnableObject", {"Crate", "0"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.start("u");
    require(runner.unhandledFunctions().size() == 1 && runner.unhandledCalls("playsound") == 2, "PlaySound recorded");
    require(state.isDisabled("Crate") && runner.threadCount() == 0, "continues after the unhandled call");
}

void triggerObjectSendsMessages() {
    Level level;
    add(level, "doors", {{"TriggerObject", {"Police_Door_01", "1"}}, {"Wait", {"3"}}, {"TriggerObject", {"Police_Door_01", "0"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.start("doors");
    std::vector<chase::runtime::ScriptObjectMessage> sent = runner.takeObjectMessages();
    require(sent.size() == 1 && sent[0].instance == "police_door_01" && sent[0].message == 0xff, "trigger sends 0xff");
    require(runner.takeObjectMessages().empty(), "messages are taken once");
    for (int update = 0; update < 89; ++update) runner.update(1.0f / 30.0f);
    require(runner.takeObjectMessages().empty(), "still waiting");
    runner.update(1.0f / 30.0f);
    runner.update(1.0f / 30.0f);
    sent = runner.takeObjectMessages();
    require(sent.size() == 1 && sent[0].message == 0xfe, "untrigger sends 0xfe after the wait");
}

chase::assets::LevelTrigger trigger(const std::string& name, const std::string& event, const std::string& object,
                                    const std::string& script, const std::string& bound) {
    chase::assets::LevelTrigger made;
    made.nameHash = chase::assets::nameHash(name);
    made.event = event;
    made.objectName = object;
    made.scriptName = script;
    made.firstArgument = bound;
    return made;
}

Level boundLevel() {
    Level level;
    chase::assets::LevelBound box;
    box.name = "Door_Bound";
    box.shape = 3;
    box.enabled = box.active = 1;
    box.centre[0] = 10;
    box.half[0] = box.half[1] = box.half[2] = 1;
    level.bounds.push_back(box);
    chase::assets::LevelBound cylinder = box;
    cylinder.name = "Room_Bound";
    cylinder.shape = 4;
    cylinder.room = "Room5";
    cylinder.centre[0] = 20;
    level.bounds.push_back(cylinder);
    level.triggers.push_back(trigger("Enter", "BoundEntered", "", "entered", "Door_Bound"));
    level.triggers.push_back(trigger("Leave", "BoundLeft", "", "left", "Door_Bound"));
    level.triggers.push_back(trigger("RoomEnter", "PlayerOnFootBoundEntered", "", "room", "Room_Bound"));
    level.triggers.push_back(trigger("Switch", "ObjectTriggered", "Lever", "switched", ""));
    add(level, "entered", {{"=+", {"Entered", "Entered", "\"+\"", "1"}}, {"Wait", {"1"}}});
    add(level, "left", {{"=+", {"Left", "Left", "\"+\"", "1"}}});
    add(level, "room", {{"=+", {"Room", "Room", "\"+\"", "1"}}});
    add(level, "switched", {{"=+", {"Switched", "Switched", "\"+\"", "1"}}});
    add(level, "pull", {{"TriggerObject", {"Lever", "1"}}});
    add(level, "off", {{"EnableTrigger", {"Enter", "0"}}, {"EnableBound", {"door_bound", "0"}}});
    return level;
}

void boundTriggersFireOnTheEdge() {
    const Level level = boundLevel();
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    const float inside[3] = {10.5f, 0, 0}, outside[3] = {0, 0, 0};
    runner.checkTriggers(inside);
    require(runner.threadCount() == 0, "the first check only records the state (skip byte)");
    runner.checkTriggers(outside);
    require(runner.threadCount() == 1, "BoundLeft queued and created");
    require(runner.variable("Left") == 0, "a queued thread runs from the next update");
    runner.update(0.034f);
    require(runner.variable("Left") == 1 && runner.threadCount() == 0, "BoundLeft script ran");
    runner.checkTriggers(inside);
    runner.update(0.034f);
    require(runner.variable("Entered") == 1, "BoundEntered fires when the player comes in");
    runner.checkTriggers(outside);
    runner.update(0.034f);
    runner.checkTriggers(inside);
    runner.update(0.034f);
    require(runner.variable("Entered") == 1, "no second thread while the script still runs");
    runner.checkTriggers(inside);
    require(runner.variable("Entered") == 1, "staying inside does not fire");
}

void roomBoundsCountOnlyWhileTheRoomIsLoaded() {
    const Level level = boundLevel();
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    const float outside[3] = {0, 0, 0}, inRoom[3] = {20.5f, 0.5f, 0.5f};
    runner.checkTriggers(outside);
    runner.checkTriggers(inRoom);
    runner.update(0.034f);
    require(runner.variable("Room") == 0 && !runner.boundActive(1), "room not loaded");
    runner.checkTriggers(outside);
    runner.setRoomLoaded("room5", true);
    runner.checkTriggers(inRoom);
    runner.update(0.034f);
    require(runner.variable("Room") == 1, "room bound fires once its room is loaded (case-insensitive name)");
}

void scriptSwitchesTriggersAndBounds() {
    const Level level = boundLevel();
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.start("off");
    require(!runner.triggers()[0].enabled && runner.triggers()[1].enabled, "EnableTrigger finds the trigger by name hash");
    require(runner.boundsEnabled()[0] == 0 && runner.boundsEnabled()[1] == 1, "EnableBound finds the bound by name without case");
    runner.start("pull");
    require(runner.threadCount() == 0, "the ObjectTriggered thread is created by the trigger pass");
    const float outside[3] = {0, 0, 0};
    runner.checkTriggers(outside);
    runner.update(0.034f);
    require(runner.variable("Switched") == 1, "TriggerObject starts the object's ObjectTriggered script");
    state.disabled.insert("lever");
    runner.start("pull");
    runner.checkTriggers(outside);
    runner.update(0.034f);
    require(runner.variable("Switched") == 1, "a disabled owner object does not start its trigger");
}
}

int main() {
    try {
        waitCountsFromTheSecondCall();
        waitZeroYieldsOnce();
        callScriptSuspendsTheParent();
        callScriptOfAnEndingChildContinuesAtOnce();
        spawnScriptDoesNotSuspend();
        newThreadsWaitForTheNextPass();
        conditions();
        unhandledFunctionsAreRecorded();
        triggerObjectSendsMessages();
        boundTriggersFireOnTheEdge();
        roomBoundsCountOnlyWhileTheRoomIsLoaded();
        scriptSwitchesTriggersAndBounds();
    } catch (const std::exception& error) {
        std::cerr << "script-runner: FAIL: " << error.what() << '\n';
        return 1;
    }
    std::cout << "script-runner: ok\n";
    return 0;
}
