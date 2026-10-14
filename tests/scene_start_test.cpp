// Synthetic edge cases of the scene start (src/runtime/scene_start.cpp). The rules come from the disassembly of the scene
// start 0x197c3c, the camera bound system 0x2ff35c, the event 0x1ecc50 and the gameplay update 0x2e5638 (docs/re/scene-start.md);
// the levels here are made up. The real levels are checked by level-start-real-data.
#include "runtime/scene_start.hpp"
#include <functional>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>

namespace {
using chase::assets::Level;
using chase::assets::LevelBound;
using chase::assets::LevelObject;
using chase::assets::LevelParam;
using chase::assets::LevelScript;
using chase::assets::LevelTrigger;
using chase::assets::PlacedObjectState;
using chase::assets::ScriptCall;
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

LevelObject object(const std::string& instance, const std::string& className, const std::string& room = "Room1") {
    LevelObject result;
    result.instance = instance;
    result.className = className;
    result.room = room;
    return result;
}

LevelParam stringParam(const std::string& paramName, const std::string& value, std::uint16_t flags) {
    LevelParam param;
    param.hash = chase::assets::nameHash(paramName);
    param.flags = flags;
    param.type = 1;
    param.blocks.emplace_back(value.begin(), value.end());
    param.blocks[0].push_back(0);
    return param;
}

LevelBound bound(const std::string& name, const std::string& room = {}) {
    LevelBound result;
    result.name = name;
    result.enabled = 1;
    result.room = room;
    return result;
}

LevelTrigger trigger(const std::string& objectName, const std::string& script) {
    LevelTrigger result;
    result.objectName = objectName;
    result.scriptName = script;
    result.event = "ObjectTriggered";
    return result;
}

chase::assets::ClassLookup lookupOf(std::map<std::string, Level>& classes) {
    return [&classes](const std::string& name) -> const Level* {
        const auto found = classes.find(name);
        return found == classes.end() ? nullptr : &found->second;
    };
}

void classStartupsRunFromTheLastObjectToTheFirst() {
    std::map<std::string, Level> classes;
    add(classes["props/a"], "startup", {{"=", {"Seen", "1"}}});
    add(classes["props/b"], "startup", {{"=", {"Seen", "2"}}});
    classes["props/plain"];
    Level level;
    level.objects = {object("A1", "props/a"), object("P", "props/plain"), object("B1", "props/b"), object("A2", "props/a")};
    add(level, "startup", {{"=", {"LevelSaw", "1"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    const auto classOf = lookupOf(classes);
    SceneStart start(level, classOf, runner);
    const auto& owners = start.classOwners();
    require(owners.size() == 3, "one owner per object whose class has a startup script");
    require(owners[0]->instance == "A2" && owners[1]->instance == "B1" && owners[2]->instance == "A1", "owners are started from the last object to the first");
    require(start.run({}) == 0, "scripts without waits end in the first pass");
    require(owners[0]->runner->variable("Seen") == 1 && owners[1]->runner->variable("Seen") == 2, "each class script ran in its own runner");
    require(runner.variable("LevelSaw") == 1, "the level's startup ran");
}

void classOwnersKeepTheirOwnObjectState() {
    std::map<std::string, Level> classes;
    add(classes["props/gem"], "startup", {{"EnableObject", {"Gem", "0"}}});
    Level level;
    level.objects = {object("Task_01", "props/gem")};
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    const auto classOf = lookupOf(classes);
    SceneStart start(level, classOf, runner);
    start.run({});
    require(start.classOwners()[0]->state.isDisabled("Gem"), "the class's own sub-object is disabled in the owner's state");
    require(!state.isDisabled("Gem") && !state.isDisabled("Task_01"), "the level's object state is untouched");
}

void levelStartupRunsAfterTheClassStartups() {
    std::map<std::string, Level> classes;
    add(classes["props/a"], "startup", {{"Wait", {"0.05"}}, {"=", {"ClassDone", "1"}}});
    Level level;
    level.objects = {object("A1", "props/a")};
    add(level, "startup", {{"Wait", {"0.05"}}, {"=", {"LevelDone", "1"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    const auto classOf = lookupOf(classes);
    SceneStart start(level, classOf, runner);
    SceneStartOptions options;
    options.dt = 0.034f;
    const std::uint32_t updates = start.run(options);
    require(updates == 2, "both Waits end in the second update after their first call");
    require(start.classOwners()[0]->runner->variable("ClassDone") == 1 && runner.variable("LevelDone") == 1, "both threads ended");
    require(start.classThreadCount() == 0 && runner.threadCount() == 0, "no thread left");
}

void untilStartupEndedStopsWhenTheLevelStartupEnded() {
    Level level;
    add(level, "startup", {{"Wait", {"0.01"}}, {"=", {"Done", "1"}}});
    add(level, "later", {{"Wait", {"10"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    std::map<std::string, Level> classes;
    const auto classOf = lookupOf(classes);
    SceneStart start(level, classOf, runner);
    runner.start("later");
    SceneStartOptions options;
    options.untilAllEnded = false;
    require(start.run(options) == 1, "stops when startup ended");
    require(runner.threadCount() == 1, "the other thread keeps waiting");
}

void cameraBoundsAreSwitchedOffAtStart() {
    std::map<std::string, Level> classes;
    classes["props/helipad"].defaults.push_back(stringParam("CameraBound", "", 2));
    classes["props/useable"].defaults.push_back(stringParam("UseableCameraBound", "", 2));
    classes["props/plain"];
    Level level;
    level.bounds = {bound("CameraBound_A"), bound("Other"), bound("CameraBound_B"), bound("camerabound_c")};
    LevelObject a = object("H1", "props/helipad");
    a.params.push_back(stringParam("CameraBound", "CameraBound_A", 2));
    LevelObject b = object("U1", "props/useable");
    b.params.push_back(stringParam("UseableCameraBound", "CameraBound_B", 2));
    LevelObject missing = object("H2", "props/helipad");
    missing.params.push_back(stringParam("CameraBound", "NoSuchBound", 2));
    LevelObject noParam = object("P1", "props/plain");
    noParam.params.push_back(stringParam("CameraBound", "Other", 2));
    LevelObject caseless = object("H3", "props/helipad");
    caseless.params.push_back(stringParam("CameraBound", "CameraBound_C", 2));
    level.objects = {a, b, missing, noParam, caseless};
    const auto classOf = lookupOf(classes);
    const std::vector<std::size_t> switched = cameraBoundsSwitchedOffAtStart(level, classOf);
    require((switched == std::vector<std::size_t>{0, 2, 3}), "bounds named by CameraBound and UseableCameraBound, found without case, not found skipped, class without the parameter ignored");

    add(level, "startup", {{"EnableBound", {"CameraBound_A", "1"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    SceneStart start(level, classOf, runner);
    start.run({});
    require(!runner.boundsEnabled()[0] && runner.boundsEnabled()[1] && !runner.boundsEnabled()[2], "the switch comes after the startup script");
}

void triggersWithoutAnObjectStartDisabled() {
    Level level;
    level.objects = {object("Door", "props/door")};
    level.triggers = {trigger("Door", "s"), trigger("", "s"), trigger("Ghost", "s"), trigger("DOOR", "s")};
    require((triggersWithoutObject(level) == std::vector<std::size_t>{2}), "only the named object that does not exist");
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    std::map<std::string, Level> classes;
    const auto classOf = lookupOf(classes);
    SceneStart start(level, classOf, runner);
    require(runner.triggers()[0].enabled && runner.triggers()[1].enabled && !runner.triggers()[2].enabled && runner.triggers()[3].enabled, "records are switched when the scene start is built");
}

void sceneEnterStartsTheScriptOfTheStartRoom() {
    Level level;
    add(level, "OnSceneEnter1", {{"=", {"One", "1"}}});
    add(level, "OnSceneEnter2", {{"=", {"Two", "1"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    std::map<std::string, Level> classes;
    const auto classOf = lookupOf(classes);
    SceneStart start(level, classOf, runner);
    SceneStartOptions options;
    options.startRoom = 2;
    start.run(options);
    require(runner.variable("Two") == 1 && runner.variable("One") == 0, "only the script of the start room runs");
    require(roomNumber("Room12") == 12 && roomNumber("room7") == 7 && roomNumber("Roomy") == -1 && roomNumber("") == -1 && roomNumber("Room") == -1, "room numbers");
    require(sceneEnterScriptName(3) == "OnSceneEnter3", "event name");
}

void extrasCheckStartsOnceWhenRequested() {
    Level level;
    add(level, kExtrasCheckScript, {{"=", {"Checked", "1"}}});
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    std::map<std::string, Level> classes;
    const auto classOf = lookupOf(classes);
    SceneStart start(level, classOf, runner);
    require(!start.startRequested() && runner.variable("Checked") == 0, "nothing without a request");
    start.requestExtrasCheck();
    require(start.startRequested() && runner.variable("Checked") == 1, "the request starts the script at once");
    require(!start.startRequested(), "the request is consumed");
}
}

int main() {
    try {
        classStartupsRunFromTheLastObjectToTheFirst();
        classOwnersKeepTheirOwnObjectState();
        levelStartupRunsAfterTheClassStartups();
        untilStartupEndedStopsWhenTheLevelStartupEnded();
        cameraBoundsAreSwitchedOffAtStart();
        triggersWithoutAnObjectStartDisabled();
        sceneEnterStartsTheScriptOfTheStartRoom();
        extrasCheckStartsOnceWhenRequested();
    } catch (const std::exception& error) {
        std::cerr << "scene-start-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "scene-start-test: ok\n";
    return 0;
}
