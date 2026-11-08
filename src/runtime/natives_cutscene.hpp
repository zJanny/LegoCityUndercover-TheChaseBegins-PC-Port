#pragma once
// Cutscene natives of batch SCRIPT-29 (docs/re/cutscenes.md): DCamCutSceneLoad 0x2889a4, DCamCutScenePlay 0x26fa18 (-> 0x2889e4),
// DCamCutSceneWait 0x288a40, DCamCutSceneFree 0x28898c, DCamCutsceneFadeDisable 0x2c09ec, PlayCutscene 0x1bf9dc and
// PlayCutsceneNoFadeBackIn 0x2b9dac. The slot table of the original (8 slots of 0x24 bytes at 0x58976c, the object
// pointer at +0x20) and its busy test 0x2297b0 are kept; the host loads and draws the in-engine cutscene and plays the
// movie. No SDL dependency.
#include <array>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace chase::runtime {

constexpr std::size_t kCutsceneSlots = 8;
constexpr float kCutsceneFramesPerSecond = 30.0f;

enum class CutsceneNativeKind { Load, Play, Wait, Free, FadeDisable, Movie, MovieNoFadeBackIn };

std::optional<CutsceneNativeKind> cutsceneNativeKind(const std::string& lowerCaseName);
std::vector<std::string> cutsceneNativeNames();
unsigned cutsceneNativeArguments(CutsceneNativeKind kind);

struct CutsceneRequest {
    enum class Kind { Load, Play, Free } kind = Kind::Load;
    std::string name;
};

// What DCamCutScenePlay 0x1964dc stored: the object of the slot becomes the active one. The four flags are the
// script's a b c d; the original turns each into 0.5 (set) or 0 (clear), a and b going to the mode start 0x25d26c and
// c, d to the block at 0x58988c + 0x2c / 0x30. Their meaning is not recovered (SCRIPT-150).
// The fades of the cutscene mode are the screen fade (screen_fade.hpp) started with screen byte 1 (top screen): the update
// 0x2fae68 fades to black over 0.5 s once the cutscene has ended (0x2d19cc returns 0), the mode switch waits until the fade
// has ended, and the exit 0x2fac7c fades in over 0.5 s unless the cutscene is `7_9_A`.
constexpr float kCutsceneFadeSeconds = 0.5f;
// STAND-IN MOVIE-S34: between the end of the fade and the exit the mode switch (0x109708 .. 0x1097a0) sleeps once and waits
// for two ticks of the render counter at 0x529e30 + 0x3d4; the port takes the two ticks as 2 / 30 s (the 4 vblanks of black
// measured in the oracle) and the sleep as 0.
constexpr float kCutsceneExitWaitSeconds = 2.0f / kCutsceneFramesPerSecond;
// STAND-IN SCRIPT-S337: the free run of 1-4_ShowroomArrival (20261108-traffic-C-1) has DCamCutSceneLoad write the slot name 0.03 s after the script start and
// DCamCutScenePlay run 0.23 s after it, 12 vblanks of 59.8 per second later (6 updates); the port waits 0.19 s, which is 6 updates of 0.033 s, between a Load and the Play of the same
// cutscene, taken from this one cutscene for all. Likewise the mode switch needs 0.60 s from Play to the mode enter, the fade of 0.5 s plus 0.1 s.
constexpr float kCutsceneLoadSeconds = 0.19f;
constexpr float kCutsceneModeSwitchSeconds = 0.1f;
const char* const kCutsceneWithoutExitFade = "7_9_a";

enum class CutsceneFadeRequest { None, Out, In };

struct CutscenePlaying {
    std::string name;
    float elapsed = 0;
    float startFadeLeft = 0;
    float length = 0;
    std::array<bool, 4> flags{};
};

// PlayCutscene 0x1bf9dc: the movie plays on the top screen; the native returns 1 once the movie mode reached state 3.
struct MovieRequest {
    std::string name;
    bool fadeBackIn = true;
};

class CutsceneNativeState {
public:
    // Length in seconds of the cutscene `name` when its data can be loaded; nullopt otherwise (the slot stays empty).
    using LengthSource = std::function<std::optional<float>(const std::string& lowerCaseName)>;
    void setLengthSource(LengthSource source) { lengthSource_ = std::move(source); }

    void load(const std::string& name);
    // False when the cutscene has no data (nothing plays).
    bool play(const std::string& name, const std::array<bool, 4>& flags);
    void free(const std::string& name);
    // 0x2297b0: a loaded slot (named `name`; any slot for an empty name) holds the active object.
    bool busy(const std::string& name) const;
    // False while a slot named `name` was loaded less than kCutsceneLoadSeconds ago (SCRIPT-S337).
    bool loadFinished(const std::string& name) const;
    void update(float dt, bool screenFadeRunning);

    const CutscenePlaying* playing() const { return playing_ ? &*playing_ : nullptr; }
    // The picture is the cutscene: it is playing and the fade before its start has ended.
    bool shown() const { return playing_ && playing_->startFadeLeft <= 0; }
    // The fade the host starts on the screen fade (top screen) after an update: the end fade out of the update 0x2fae68 or
    // the fade in of the exit 0x2fac7c.
    CutsceneFadeRequest takeFadeRequest();
    // Once per cutscene: the cutscene mode was entered (0x2fadb8), at the end of the fade before the start.
    // STAND-IN SCRIPT-S243: the mode switch of 0x25d26c is not read; the port enters at the end of the delay of flag a.
    bool takeModeEntered();
    bool loaded(const std::string& name) const;
    bool fadeDisabled() const { return fadeDisabled_; }
    std::vector<CutsceneRequest> takeRequests();

    // Without a host that plays movies (the default, and the preroll of the level start) PlayCutscene ends at once.
    void setHostPlaysMovies(bool hosted) { hostPlaysMovies_ = hosted; }
    void startMovie(const std::string& name, bool fadeBackIn);
    bool movieOpen() const { return movie_.has_value(); }
    const MovieRequest* movie() const { return movie_ ? &*movie_ : nullptr; }
    void finishMovie();
    bool movieFinished() const { return movieFinished_; }
    void clearMovieFinished() { movieFinished_ = false; }

private:
    struct Slot {
        std::string name;
        float length = 0;
        float age = 0;
        bool used = false;
    };
    int find(const std::string& lowerCaseName) const;
    void end();

    std::array<Slot, kCutsceneSlots> slots_{};
    std::optional<CutscenePlaying> playing_;
    LengthSource lengthSource_;
    std::vector<CutsceneRequest> requests_;
    std::optional<MovieRequest> movie_;
    bool movieFinished_ = false;
    enum class EndPhase { None, Fading, Waiting } endPhase_ = EndPhase::None;
    float exitWaitLeft_ = 0;
    CutsceneFadeRequest fadeRequest_ = CutsceneFadeRequest::None;
    bool fadeDisabled_ = false;
    bool hostPlaysMovies_ = false;
    bool modeEntered_ = false;
};

}
