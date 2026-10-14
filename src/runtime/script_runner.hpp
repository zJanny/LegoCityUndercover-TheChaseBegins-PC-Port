#pragma once
// Script threads of a level and their per-update runner (docs/re/scripts.md): thread creation 0x22ce9c, the runner
// 0x169254 that the gameplay update 0x2e5158 calls for every resident scene, and the executor 0x234204 with the
// script words, Wait 0x174604, CallScript / SpawnScript 0x1c3730 and the natives that have a handler. No SDL dependency.
#include "assets/object_collision.hpp"
#include "runtime/natives_animation.hpp"
#include "runtime/natives_objects.hpp"
#include "runtime/ai.hpp"
#include "runtime/dcam.hpp"
#include "runtime/natives_progress.hpp"
#include "runtime/objectives.hpp"
#include "runtime/script_text.hpp"

#include <cstdint>
#include <functional>
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

// WarningDialogBox 0x2700d4: text keys it hands to the dialog mode (0x20ffe8 stores them at +0x2c of the mode's data):
// gametext body, systemtext confirm button, systemtext cancel button. The result the mode stores at +0x28 is 2 for the
// confirm button (A) and 1 for the cancel button (B); 0 while no button was pressed.
struct ScriptDialog {
    std::uint32_t bodyKey = 0xfd849176;
    std::uint32_t confirmKey = 0xd12f028b;
    std::uint32_t cancelKey = 0x994b0cc9;
};
// InformationBox 0x1d9d4c opens the same mode with the script's gametext key, systemtext 0xd12f028b "OK" and systemtext 0xdf3cad3b (empty text);
// it takes no result variable. STAND-IN HUD-S14: the buttons the mode offers for an empty cancel text are not read.
constexpr std::uint32_t kInformationBoxCancelKey = 0xdf3cad3b;
constexpr int kDialogCancel = 1;
constexpr int kDialogConfirm = 2;

// PositionObject 0x1d9e6c: the node of `object` takes the transform of the node of `target` (names in lower case).
struct ScriptPlacement {
    std::string object;
    std::string target;
};

// FadeScreen 0x1a2e8c starts a fade of 0.5 s (immediate of its two start functions 0x252068 / 0x2520e8) and blocks
// until it is done (0x25207c). STAND-IN: the curve is linear and the end is the elapsed time (SCRIPT-S6).
constexpr float kFadeSeconds = 0.5f;

// What the natives IsLevelUnlocked 0x2636e4, GetGameComplete 0x2635d0 and IsRedBrickExtraActive 0x2a6d34 read from the
// loaded slot (docs/re/save-format.md). The defaults are those of a new game: only level 2 is unlocked (the slot
// defaults 0x1a25cc set bit 4 of slot +0x51e0).
struct ScriptProgress {
    std::uint32_t levelUnlocked = 1u << 2;
    bool gameComplete = false;
    std::uint32_t redBrickExtrasActive = 0;
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
    void setProgress(const ScriptProgress& progress) { progress_ = progress; }
    void setVariable(std::uint32_t nameHash, float value) { variables_[nameHash] = value; }
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
    // World position of a placed object by lower-case instance name (the node of an object bound check); false when unknown.
    using ObjectPositionSource = std::function<bool(const std::string&, float[3])>;
    void setObjectPositionSource(ObjectPositionSource source) { objectPosition_ = std::move(source); }
    void setTriggerEnabled(std::size_t index, bool enabled) { if (index < triggers_.size()) triggers_[index].enabled = enabled; }
    void setBoundEnabled(std::size_t index, bool enabled) { if (index < boundEnabled_.size()) boundEnabled_[index] = enabled; }
    // Bound byte +0xa (EnableBound 0x1d0ce8), one per Level::bounds entry.
    const std::vector<std::uint8_t>& boundsEnabled() const { return boundEnabled_; }
    // Bound byte +0xb: the room setup 0x173944 clears it for the bounds of a room, so such a bound counts only while
    // its room is loaded. Room names compare without case. No room is loaded until one is set.
    void setRoomLoaded(const std::string& room, bool loaded);
    void setAllRoomsLoaded();
    void clearAllRoomsLoaded() { allRoomsLoaded_ = false; }
    bool boundActive(std::size_t bound) const;
    // Trigger events whose check is not transcribed, by name, in the order they were first met.
    const std::vector<std::string>& unhandledTriggerEvents() const { return unhandledEvents_; }

    // WarningDialogBox: true from the first call until the script has taken the answer. The host draws the dialog and
    // answers it with answerDialog (kDialogConfirm or kDialogCancel); the script reads the result at its next update.
    bool dialogOpen() const { return dialogOpen_ && !dialogAnswered_; }
    ScriptDialog dialog() const { return dialog_; }
    void answerDialog(int result);
    // Positions requested since the last call, in order (the host moves the nodes).
    std::vector<ScriptPlacement> takePlacements();
    // SceneChange 0x1b262c: the bound arguments of the calls since the last call, in order (the host resolves them
    // against the master level, see scene_change.hpp).
    std::vector<std::string> takeSceneChanges();
    // The scenechange update 0x1c47e8 tests the player against the enabled bound of the object (0x22f650).
    bool playerInNamedBound(const std::string& name, const float player[3]) const;
    // Flags and class messages written by the object natives (natives_objects.hpp).
    const ObjectNativeState& objectNatives() const { return objectNatives_; }
    std::vector<ObjectClassMessage> takeObjectClassMessages() { return objectNatives_.takeMessages(); }
    // Progress, stud and switch natives (natives_progress.hpp); the slot image is the one of a loaded save.
    ProgressNativeState& progressNatives() { return progressNatives_; }
    const ProgressNativeState& progressNatives() const { return progressNatives_; }
    // Brightness of the screen set by FadeScreen: 1 normal, 0 black.
    float fadeLevel() const { return fadeLevel_; }
    DirectorCamera& director() { return director_; }
    // Tip text, dialogue lines, dialogue background, mission title and summary (script_text.hpp).
    const ScriptTextState& text() const { return text_; }
    // Key of the level name in gametext (TablesLevels field +8 of the level's row); 0 shows no heading for ShowLevelText.
    void setLevelHeadingKey(std::uint32_t key) { levelHeadingKey_ = key; }
    // Waypoints and sub-objective groups switched on by the level scripts (objectives.hpp).
    const ObjectiveState& objectives() const { return objectives_; }
    // AI state the AI natives wrote and the move orders they started (ai.hpp).
    AiCore& ai() { return ai_; }
    const AiCore& ai() const { return ai_; }
    // Animation and attach requests of the batch SCRIPT-32 natives (natives_animation.hpp).
    const AnimationNativeState& animation() const { return animation_; }

private:
    enum class Result { Continue, Block };

    ScriptThread* create(const assets::LevelScript& script);
    void execute(ScriptThread& thread);
    Result call(ScriptThread& thread, const assets::ScriptCall& call);
    Result callScript(ScriptThread& thread, const assets::ScriptCall& call, bool suspendParent);
    Result wait(ScriptThread& thread, const assets::ScriptCall& call);
    Result native(ScriptThread& thread, const assets::ScriptCall& call, int word);
    void finish(ScriptThread& thread);
    void killScript(const ScriptThread& caller, const std::string& name);
    void progressNative(const assets::ScriptCall& call);
    std::uint32_t textKeyOf(const std::string& token);
    void textNative(const assets::ScriptCall& call);
    void objectiveNative(const assets::ScriptCall& call);
    void aiNative(const assets::ScriptCall& call);
    void animationNative(const assets::ScriptCall& call);
    void fireAiMoveEvents();
    float read(const std::string& token) const;
    bool holds(const assets::ScriptCall& call) const;
    void assignArithmetic(const assets::ScriptCall& call);
    const assets::LevelScript* find(const std::string& name) const;
    bool scriptActive(const assets::LevelScript& script) const;
    void queueTrigger(std::size_t index);
    bool pointInBound(std::size_t bound, const float point[3], bool requireRoom) const;
    bool playerInBound(std::size_t bound, const float player[3]) const { return pointInBound(bound, player, true); }
    int triggerIndex(const std::string& name) const;
    int boundIndex(const std::string& name) const;

    const assets::Level& level_;
    assets::PlacedObjectState& state_;
    std::list<ScriptThread> threads_;
    std::map<std::uint32_t, float> variables_;
    ScriptProgress progress_;
    std::map<std::string, std::uint32_t> unhandled_;
    std::vector<std::string> unhandledOrder_;
    std::vector<ScriptObjectMessage> messages_;
    std::vector<ScriptTriggerState> triggers_;
    std::vector<std::uint8_t> boundEnabled_;
    std::vector<std::size_t> queued_;
    std::set<std::string> loadedRooms_;
    bool allRoomsLoaded_ = false;
    std::vector<std::string> unhandledEvents_;
    ObjectPositionSource objectPosition_;
    float dt_ = 0;
    ScriptDialog dialog_;
    bool dialogOpen_ = false;
    bool dialogAnswered_ = false;
    int dialogResult_ = 0;
    std::vector<ScriptPlacement> placements_;
    std::vector<std::string> sceneChanges_;
    ObjectNativeState objectNatives_;
    ProgressNativeState progressNatives_;
    ScriptTextState text_;
    std::uint32_t levelHeadingKey_ = 0;
    ObjectiveState objectives_;
    AiCore ai_;
    AnimationNativeState animation_;
    float fadeLevel_ = 1.0f, fadeStart_ = 1.0f, fadeTarget_ = 1.0f, fadeElapsed_ = 0;
    DirectorCamera director_;
};

}
