#pragma once
// Thread starters of a scene besides the level's own "startup" script and the trigger records (docs/re/scene-start.md):
// the "startup" scripts of the class files of placed objects (scene start 0x197c3c), the camera bound switches of the
// scene systems (0x198014 -> 0x2ff35c), the "OnSceneEnter<room>" event of the start room (0x1ecc50) and the request
// ScriptCheckExtras (0x2e5638). No SDL dependency.
#include "assets/assets.hpp"
#include "runtime/script_runner.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace chase::runtime {

// Object parameters that name a bound the camera bound system switches off at scene start (strings at 0x2ff738 and 0x2ff744).
constexpr const char* kCameraBoundParams[] = {"CameraBound", "UseableCameraBound"};

// Bounds named by those parameters of placed objects (instance value, else class value), in bound order. The system
// writes +0xa = 0 for each bound it finds; a name without a bound is skipped.
std::vector<std::size_t> cameraBoundsSwitchedOffAtStart(const assets::Level& level, const assets::ClassLookup& classOf);

// STAND-IN SCRIPT-S15: triggers whose object name is set but names no object of the level start disabled. Data fit
// (lvl_01: triggers 79-82 and 141 of 241 and no other; lvl_00: none of 53); the builder of the runtime records was not read.
std::vector<std::size_t> triggersWithoutObject(const assets::Level& level);

// "Room7" and "room7" give 7; other names give -1.
int roomNumber(const std::string& room);
// The event script of the owner level when a room is entered (0x1ecc50 formats "OnSceneEnter%d" with the room number).
std::string sceneEnterScriptName(int room);

constexpr const char* kExtrasCheckScript = "ScriptCheckExtras";

// One placed object whose class file has a "startup" script, with the scripts of its class run for it. The runner has
// the class file as its level and private object state (names of the class's own sub-objects).
struct ClassOwner {
    std::size_t objectIndex = 0;
    std::string instance;
    std::string className;
    assets::PlacedObjectState state;
    std::unique_ptr<ScriptRunner> runner;
};

struct SceneStartOptions {
    int startRoom = -1;
    float dt = 0.034f;
    std::uint32_t maxUpdates = 3000;
    bool untilAllEnded = true;
    std::function<bool()> yieldWhen;
};

class SceneStart {
public:
    // `levelRunner` runs the scripts of `level` and its trigger records are switched by triggersWithoutObject at once; the
    // class files behind `classOf` must outlive this object (the lookup of archiveClassLookup owns them).
    SceneStart(const assets::Level& level, const assets::ClassLookup& classOf, ScriptRunner& levelRunner);

    // The order of the original at level load: the class startups from the last object to the first (0x197c3c walks the
    // owner array backwards; the level itself is owner 0 and runs last), the level's "startup", the camera bound switches,
    // "OnSceneEnter<startRoom>", then the threads are stepped with `dt` until the startup ended (or every thread ended).
    // Returns the number of updates stepped.
    std::uint32_t run(const SceneStartOptions& options);

    // One update of the class owners' threads; the level's own runner is updated by its caller.
    void update(float dt);
    std::size_t classThreadCount() const;
    const std::vector<std::unique_ptr<ClassOwner>>& classOwners() const { return owners_; }

    // ScriptCheckExtras 0x2e5638: the gameplay update starts it at once in the level when the Extras screen was left.
    void requestExtrasCheck() { extrasRequested_ = true; }
    bool startRequested();

private:
    ScriptRunner& levelRunner_;
    std::vector<std::unique_ptr<ClassOwner>> owners_;
    std::vector<std::size_t> cameraBounds_;
    bool extrasRequested_ = false;
};

}
