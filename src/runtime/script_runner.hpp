#pragma once
// Script threads of a level and their per-update runner (docs/re/scripts.md): thread creation 0x22ce9c, the runner
// 0x169254 that the gameplay update 0x2e5158 calls for every resident scene, and the executor 0x234204 with the
// script words, Wait 0x174604, CallScript / SpawnScript 0x1c3730 and the natives that have a handler. No SDL dependency.
#include "assets/object_collision.hpp"

#include <cstdint>
#include <list>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace chase::runtime {

// The script functions the executor handles: the script words, Wait and the natives with a handler. StandIn marks a
// handler whose result is not taken from the original (the unlock and completion natives write 0). A function not
// listed has no handler: ScriptRunner records the call and continues with the next call (stand-in SCRIPT-S4).
enum class ScriptFunctionSupport { None, Implemented, StandIn };
ScriptFunctionSupport scriptFunctionSupport(const std::string& name);
std::vector<std::string> handledScriptFunctions();

// Flags of a thread record (+0x14).
enum ScriptThreadFlag : std::uint16_t {
    kScriptSuspended = 1,
    kScriptElseIfPending = 2,
    kScriptBlocked = 4,
};

// Thread record (0x50 bytes in the original): +0x10 script, +0x14 flags, +0x16 program counter, +0x48 Wait timer,
// +0x4c parent (the thread a CallScript suspended).
struct ScriptThread {
    const assets::LevelScript* script = nullptr;
    std::uint16_t flags = 0;
    std::uint16_t pc = 0;
    float waitTimer = 0;
    ScriptThread* parent = nullptr;
};

// Object message a script sent (0x244fc8): TriggerObject 0x1cddf8 sends 0xff (trigger) for a non-zero value and
// 0xfe (untrigger) for 0. Doors answer them in their handler 0x1749b8.
constexpr std::uint8_t kMessageTrigger = 0xff;
constexpr std::uint8_t kMessageUntrigger = 0xfe;
struct ScriptObjectMessage {
    std::string instance;
    std::uint8_t message = 0;
};

// Runtime trigger record (0x20 bytes at owner+0x34, one per Level::triggers entry): +0 enabled (EnableTrigger 0x268164),
// +1 skip (the next check only records the state), +0x1c the player was inside at the last check.
struct ScriptTriggerState {
    bool enabled = true;
    bool skip = true;
    bool inside = false;
};

// The scripts of one level (owner): the threads of its scene list +0x1310, its variables (the level's default
// parameter table) and the placed-object state the natives change.
class ScriptRunner {
public:
    ScriptRunner(const assets::Level& level, assets::PlacedObjectState& state);
    ScriptRunner(const ScriptRunner&) = delete;
    ScriptRunner& operator=(const ScriptRunner&) = delete;

    // 0x22ce9c plus the immediate 0x234204 of its callers: a new thread at the head of the list runs until it yields
    // or ends. False when the level has no script of that name.
    bool start(const std::string& scriptName);
    // 0x169254: every thread once, from the head; a thread started during the pass waits for the next update.
    void update(float dt);
    // 0x224eb4: a thread of the script exists and has calls left (and, with requireActive, is not suspended).
    bool running(const std::string& scriptName, bool requireActive) const;

    std::size_t threadCount() const { return threads_.size(); }
    const std::list<ScriptThread>& threads() const { return threads_; }
    float variable(const std::string& name) const;
    // Calls of functions without a handler, by lower-case name, in the order they were first called.
    const std::vector<std::string>& unhandledFunctions() const { return unhandledOrder_; }
    std::uint32_t unhandledCalls(const std::string& lowerCaseName) const;
    // Messages sent since the last call, in order; the instance name is lower case.
    std::vector<ScriptObjectMessage> takeObjectMessages();

    // 0x1746c0, after the runner pass: the check of every enabled trigger whose owner object is enabled (bound events
    // of the player at `player`), then the queued triggers (0x21a560) become threads at the head of the list, the last
    // queued first; they run from the next update on.
    void checkTriggers(const float player[3]);
    const std::vector<ScriptTriggerState>& triggers() const { return triggers_; }
    // Bound byte +0xa (EnableBound 0x1d0ce8), one per Level::bounds entry.
    const std::vector<std::uint8_t>& boundsEnabled() const { return boundEnabled_; }
    // Bound byte +0xb: the room setup 0x173944 clears it for the bounds of a room, so such a bound counts only while
    // its room is loaded. Room names compare without case. No room is loaded until one is set.
    void setRoomLoaded(const std::string& room, bool loaded);
    void setAllRoomsLoaded();
    bool boundActive(std::size_t bound) const;
    // Trigger events whose check is not transcribed, by name, in the order they were first met.
    const std::vector<std::string>& unhandledTriggerEvents() const { return unhandledEvents_; }

private:
    enum class Result { Continue, Block };

    ScriptThread* create(const assets::LevelScript& script);
    void execute(ScriptThread& thread);
    Result call(ScriptThread& thread, const assets::ScriptCall& call);
    Result callScript(ScriptThread& thread, const assets::ScriptCall& call, bool suspendParent);
    Result wait(ScriptThread& thread, const assets::ScriptCall& call);
    void native(const assets::ScriptCall& call, int word);
    void finish(ScriptThread& thread);
    float read(const std::string& token) const;
    bool holds(const assets::ScriptCall& call) const;
    void assignArithmetic(const assets::ScriptCall& call);
    const assets::LevelScript* find(const std::string& name) const;
    bool scriptActive(const assets::LevelScript& script) const;
    void queueTrigger(std::size_t index);
    bool playerInBound(std::size_t bound, const float player[3]) const;
    int triggerIndex(const std::string& name) const;
    int boundIndex(const std::string& name) const;

    const assets::Level& level_;
    assets::PlacedObjectState& state_;
    std::list<ScriptThread> threads_;
    std::map<std::uint32_t, float> variables_;
    std::map<std::string, std::uint32_t> unhandled_;
    std::vector<std::string> unhandledOrder_;
    std::vector<ScriptObjectMessage> messages_;
    std::vector<ScriptTriggerState> triggers_;
    std::vector<std::uint8_t> boundEnabled_;
    std::vector<std::size_t> queued_;
    std::set<std::string> loadedRooms_;
    bool allRoomsLoaded_ = false;
    std::vector<std::string> unhandledEvents_;
    float dt_ = 0;
};

}
