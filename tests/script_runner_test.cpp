// Synthetic edge cases of the script runner (src/runtime/script_runner.cpp). The rules come from the disassembly of the
// executor 0x234204, Wait 0x174604, CallScript / SpawnScript 0x1c3730, thread creation 0x22ce9c (head insertion) and
// the runner 0x169254 (docs/re/scripts.md, 2026-10-14 entry); the scripts here are made up. waitThreeMatchesTheOracleTrace pins
// the Wait timer recorded in the oracle from the unmodified state street-at-entrance (docs/re/scripts.md, script thread entry).
#include "runtime/script_placement.hpp"
#include "runtime/script_runner.hpp"
#include "runtime/script_thread_log.hpp"
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

void waitThreeMatchesTheOracleTrace() {
    Level level;
    add(level, "police", {{"Wait", {"3"}}, {"=", {"Done", "1"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.start("police");
    require(runner.threads().front().waitTimer == 3.0f, "the first call stores the argument");
    const struct {
        int updates;
        float timer;
    } recorded[] = {{1, 2.93f}, {20, 1.60f}, {42, 0.06f}};
    int done = 0;
    for (const auto& sample : recorded) {
        for (; done < sample.updates; ++done) runner.update(0.07f);
        const float timer = runner.threads().front().waitTimer;
        require(timer > sample.timer - 0.0005f && timer < sample.timer + 0.0005f, "Wait timer after " + std::to_string(sample.updates) + " updates");
    }
    require(runner.variable("Done") == 0, "the Wait is still pending after 42 repeat calls");
    runner.update(0.07f);
    require(runner.variable("Done") == 1 && runner.threadCount() == 0, "the 43rd repeat call ends Wait 3 at dt 0.07");
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

// 0x1c3730 with a null script: 0x22ce9c stores it in the new thread without a check, then the handler reads the script's
// halfword +4 (ldrh at 0x1c3764) and faults; the port stops with an error naming the call instead of continuing.
void callOfAMissingScriptStops() {
    for (const char* function : {"CallScript", "SpawnScript"}) {
        Level level;
        add(level, "parent", {{function, {"nothere"}}, {"=", {"ParentDone", "1"}}});
        PlacedObjectState state;
        ScriptRunner runner(level, state);
        bool stopped = false;
        try {
            runner.start("parent");
        } catch (const std::runtime_error& error) {
            stopped = std::string(error.what()).find("nothere") != std::string::npos;
        }
        require(stopped, std::string(function) + " of a missing script stops with an error");
        require(runner.variable("ParentDone") == 0, "the caller does not continue");
    }
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

void cameraFollowNeedsNoHandler() {
    Level level;
    add(level, "f", {{"CameraFollow", {}}, {"DuckMusicFor", {"1"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.start("f");
    require(runner.unhandledCalls("camerafollow") == 0 && runner.unhandledCalls("duckmusicfor") == 1 && runner.threadCount() == 0, "CameraFollow handled");
}

void unhandledFunctionsAreRecorded() {
    Level level;
    add(level, "u", {{"DuckMusicFor", {"1"}}, {"DuckMusicFor", {"2"}}, {"EnableObject", {"Crate", "0"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.start("u");
    require(runner.unhandledFunctions().size() == 1 && runner.unhandledCalls("duckmusicfor") == 2, "DuckMusicFor recorded");
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

// Provenance: 0x244f40 (llvm-objdump): 0x242cf0 (clears the disabled bit), 0x244fc8 message 0xff, 0x23cee8 with the ObjectTriggered type; the script
// native TriggerObject 0x1cddf8 does not enable (docs/re/carrying.md, entry of the trigger paths).
void objectTriggerFromCodeEnablesTheObject() {
    const Level level = boundLevel();
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    state.disabled.insert("lever");
    runner.triggerObjectEnabling("lever");
    require(!state.isDisabled("lever"), "0x244f40 enables the object first");
    const std::vector<chase::runtime::ScriptObjectMessage> sent = runner.takeObjectMessages();
    require(sent.size() == 1 && sent[0].instance == "lever" && sent[0].message == chase::runtime::kMessageTrigger, "message 0xff to the object");
    const float outside[3] = {0, 0, 0};
    runner.checkTriggers(outside);
    runner.update(0.034f);
    require(runner.variable("Switched") == 1, "the ObjectTriggered script of the enabled object starts");
}

void warningDialogBlocksUntilAnswered() {
    Level level;
    add(level, "d", {{"WarningDialogBox", {"DIALOG_RETURNCODE"}},
                     {"?", {"DIALOG_RETURNCODE", "\"==\"", "2"}}, {"=", {"Entered", "1"}}, {"!?", {}}, {"=", {"Cancelled", "1"}}, {"??", {}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.start("d");
    require(runner.dialogOpen() && runner.threadCount() == 1, "the first call opens the dialog and blocks");
    require(runner.dialog().bodyKey == 0xfd849176 && runner.dialog().confirmKey == 0xd12f028b && runner.dialog().cancelKey == 0x994b0cc9,
            "text keys of 0x20ffe8");
    for (int i = 0; i < 5; ++i) runner.update(0.034f);
    require(runner.threadCount() == 1 && runner.variable("Entered") == 0, "blocked while no button was pressed");
    runner.answerDialog(chase::runtime::kDialogConfirm);
    require(!runner.dialogOpen(), "answered");
    require(runner.variable("DIALOG_RETURNCODE") == 0, "the result is stored by the script's next call");
    runner.update(0.034f);
    require(runner.variable("DIALOG_RETURNCODE") == 2 && runner.variable("Entered") == 1 && runner.variable("Cancelled") == 0 &&
                runner.threadCount() == 0,
            "confirm gives 2 as a float variable and the script continues");
}

void warningDialogCancelGivesOne() {
    Level level;
    add(level, "d", {{"WarningDialogBox", {"R"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.start("d");
    runner.answerDialog(chase::runtime::kDialogCancel);
    runner.answerDialog(chase::runtime::kDialogConfirm);
    runner.update(0.034f);
    require(runner.variable("R") == 1, "the first answer counts");
}

void fadeScreenBlocksForHalfASecond() {
    Level level;
    add(level, "f", {{"FadeScreen", {"0"}}, {"=", {"Faded", "1"}}, {"FadeScreen", {"1"}}, {"=", {"Back", "1"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.start("f");
    require(runner.fadeLevel() == 1.0f && runner.variable("Faded") == 0, "the first call starts the fade and blocks");
    for (int i = 0; i < 7; ++i) runner.update(0.07f);
    require(runner.variable("Faded") == 0 && runner.fadeLevel() < 1.0f && runner.fadeLevel() > 0.0f, "still fading after 0.49 s");
    runner.update(0.07f);
    require(runner.variable("Faded") == 1 && runner.fadeLevel() == 0.0f, "black after 0.5 s");
    for (int i = 0; i < 9; ++i) runner.update(0.07f);
    require(runner.variable("Back") == 1 && runner.fadeLevel() == 1.0f, "FadeScreen 1 brings the screen back");
}

void positionObjectIsRecorded() {
    Level level;
    add(level, "p", {{"PositionObject", {"Player1", "Marker_A"}}, {"=", {"After", "1"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.start("p");
    const auto placements = runner.takePlacements();
    require(placements.size() == 1 && placements[0].object == "player1" && placements[0].target == "marker_a", "request");
    require(runner.variable("After") == 1 && runner.takePlacements().empty(), "PositionObject does not block");
}

void positionPlayerIsRecordedLikePositionObject() {
    Level level;
    add(level, "p", {{"PositionPlayer", {"Player1", "Marker_A"}}, {"=", {"After", "1"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.start("p");
    const auto placements = runner.takePlacements();
    require(placements.size() == 1 && placements[0].object == "player1" && placements[0].target == "marker_a", "request");
    require(runner.variable("After") == 1, "PositionPlayer does not block");
    require(chase::runtime::scriptFunctionSupport("PositionPlayer") == chase::runtime::ScriptFunctionSupport::StandIn, "ground probe missing");
}

void placementControllerCondition() {
    const std::vector<std::uint8_t>& list = chase::runtime::measuredControllerKindList();
    // Oracle, state street-after-1-1: player entity 0x085526f0 kind byte (+0x12) 0x1b, flags (+0xc) 0x200; list count byte
    // at 0x3743ec = 1, kind at 0x515a94 = 0x9f (read 2026-10-14, docs/re/script-natives-flags.md).
    require(chase::runtime::entityHasController(0x1b, 0x200, list), "the player entity (kind 27) gets the controller reset");
    require(chase::runtime::entityHasController(149, 0, {}), "kind 149");
    require(chase::runtime::entityHasController(0x9f, 0, list), "kind in the run-time list");
    require(!chase::runtime::entityHasController(0x9f, 0, {}), "kind 159 only through the list");
    require(chase::runtime::entityHasController(3, 0x4000, {}), "flag 0x4000");
    require(!chase::runtime::entityHasController(3, 0x200, list), "no other kind");
    chase::runtime::VerticalState vertical;
    vertical.fallDistance = 1.0f;
    vertical.fallSpeed = 2.0f;
    chase::runtime::resetControllerAfterPlacement(vertical);
    require(vertical.fallDistance == 0.0f && vertical.fallSpeed == 0.0f, "fall distance and speed are cleared");
}

// Disassembly of 0x1d9f64 / 0x1da3e0 (bic 0x14 on ctrl+0x315), 0x20d5e0 (orr 0x14 at the end of the locomotion step 0x20cd30),
// 0x1da3fc (message 8 skipped when 0x24255c found a vehicle) and the oracle read of 2026-10-14 (ctrl+0x315 0xff -> 0xfe after
// the update, docs/re/script-natives-flags.md): the bits are clear only between the placement and the next locomotion step.
void placementEffectsOfThePlayer() {
    chase::runtime::VerticalState vertical;
    require(!vertical.placement.settleStepPending && !vertical.placement.defaultStateRequested, "nothing pending at the start");
    chase::runtime::resetControllerAfterPlacement(vertical);
    require(vertical.placement.settleStepPending && vertical.placement.defaultStateRequested && vertical.placement.nodeShown &&
                vertical.placement.entityEnabled,
            "on foot: bits cleared and message 8 effects");
    chase::runtime::locomotionStepEnded(vertical.placement);
    require(!vertical.placement.settleStepPending && vertical.placement.defaultStateRequested, "the step sets the bits again");
    chase::runtime::VerticalState seated;
    chase::runtime::resetControllerAfterPlacement(seated, true);
    require(seated.placement.settleStepPending && !seated.placement.defaultStateRequested, "in a vehicle: bits cleared, no message 8");
    require(!chase::runtime::takeNodeShow(seated.placement), "in a vehicle: 0x23f734(node, 1) is not called");
    require(chase::runtime::takeNodeShow(vertical.placement) && !chase::runtime::takeNodeShow(vertical.placement),
            "message 8 shows the player's node once (0x18ed9c: 0x23f734(node, 1) clears mask 0x60)");
}

void placementProbeSegment() {
    // Oracle: 0x56d424 = (0, 1, 0) (words 00000000 3f800000 00000000), length 5.0 from the literal pool 0x1da550.
    const float start[3] = {1.0f, 2.0f, 3.0f};
    float end[3];
    chase::runtime::placementProbeEnd(start, end);
    require(end[0] == 1.0f && end[1] == 7.0f && end[2] == 3.0f, "segment end");
    require(chase::runtime::isProbeCharacterKind(179) && chase::runtime::isProbeCharacterKind(183) && chase::runtime::isProbeCharacterKind(243) &&
                !chase::runtime::isProbeCharacterKind(27),
            "character kinds of the gather");
}

// Oracle trace street-at-entrance (docs/re/streaming.md 2026-10-14T11:13): SceneChange does not block the thread (it ended in the update that ran the call).
void sceneChangeIsRecorded() {
    Level level;
    add(level, "p", {{"SceneChange", {"Police_ExtToInt"}}, {"=", {"After", "1"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.start("p");
    const auto requests = runner.takeSceneChanges();
    require(requests.size() == 1 && requests[0] == "Police_ExtToInt", "request");
    require(runner.variable("After") == 1 && runner.takeSceneChanges().empty(), "SceneChange does not block");
}
}

void objectBoundTriggersUseTheObjectPosition() {
    Level level = boundLevel();
    level.triggers.push_back(trigger("ObjEnter", "ObjectBoundEntered", "Lever", "objentered", "Door_Bound"));
    add(level, "objentered", {{"=+", {"ObjEntered", "ObjEntered", "\"+\"", "1"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    float lever[3] = {0, 0, 0};
    runner.setObjectPositionSource([&lever](const std::string& name, float out[3]) {
        if (name != "lever") return false;
        std::copy(lever, lever + 3, out);
        return true;
    });
    const float far[3] = {0, 0, 100};
    runner.checkTriggers(far);
    lever[0] = 10.5f;
    runner.checkTriggers(far);
    runner.update(0.034f);
    require(runner.variable("ObjEntered") == 1, "the object entering the bound starts the script (player elsewhere)");
    runner.setBoundEnabled(0, false);
    lever[0] = 0;
    runner.checkTriggers(far);
    lever[0] = 10.5f;
    runner.checkTriggers(far);
    runner.update(0.034f);
    require(runner.variable("ObjEntered") == 1, "a switched-off bound counts for no object");
}

void threadEventsAreLoggedWithNames() {
    Level level;
    add(level, "parent", {{"CallScript", {"child"}}, {"=", {"Done", "1"}}});
    add(level, "child", {{"Wait", {"0.1"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    chase::runtime::ScriptThreadLog log(level);
    log.learn("parent");
    require(runner.start("parent"), "start");
    for (int i = 0; i < 6; ++i) runner.update(0.034f);
    const std::vector<std::string> lines = log.lines(runner.takeThreadEvents(), 7);
    require(lines.size() == 4 && lines[0] == "script parent started at update 7" && lines[1] == "script child started at update 7" &&
                lines[2] == "script child ended at update 7" && lines[3] == "script parent ended at update 7",
            "thread start and end lines");
    require(runner.takeThreadEvents().empty(), "events are taken once");
    require(log.lines({{12345u, true}}, 1)[0] == "script #12345 started at update 1", "unknown names print their hash");
}

int main() {
    try {
        threadEventsAreLoggedWithNames();
        waitCountsFromTheSecondCall();
        waitThreeMatchesTheOracleTrace();
        waitZeroYieldsOnce();
        callScriptSuspendsTheParent();
        callScriptOfAnEndingChildContinuesAtOnce();
        callOfAMissingScriptStops();
        spawnScriptDoesNotSuspend();
        newThreadsWaitForTheNextPass();
        conditions();
        unhandledFunctionsAreRecorded();
        cameraFollowNeedsNoHandler();
        triggerObjectSendsMessages();
        boundTriggersFireOnTheEdge();
        roomBoundsCountOnlyWhileTheRoomIsLoaded();
        scriptSwitchesTriggersAndBounds();
        objectTriggerFromCodeEnablesTheObject();
        objectBoundTriggersUseTheObjectPosition();
        warningDialogBlocksUntilAnswered();
        warningDialogCancelGivesOne();
        fadeScreenBlocksForHalfASecond();
        positionObjectIsRecorded();
        positionPlayerIsRecordedLikePositionObject();
        placementControllerCondition();
        placementEffectsOfThePlayer();
        placementProbeSegment();
        sceneChangeIsRecorded();
    } catch (const std::exception& error) {
        std::cerr << "script-runner: FAIL: " << error.what() << '\n';
        return 1;
    }
    std::cout << "script-runner: ok\n";
    return 0;
}
