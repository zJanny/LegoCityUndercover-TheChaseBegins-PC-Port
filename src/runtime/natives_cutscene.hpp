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
// STAND-IN MOVIE-S20: the fade times are measured in the oracle (docs/re/cutscenes.md), not read from the code. A linear fade
// to black of 0.5 s (30 vblanks) before the start when flag a is set, the same fade over the last 0.5 s of the cutscene, then
// black for 4 vblanks and a fade in of the game picture over 43 vblanks. The fades cover the top screen only.
constexpr float kCutsceneFadeSeconds = 0.5f;
constexpr float kCutsceneBlackSeconds = 4.0f / 60.0f;
constexpr float kCutsceneFadeInSeconds = 43.0f / 60.0f;

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
    void update(float dt);

    const CutscenePlaying* playing() const { return playing_ ? &*playing_ : nullptr; }
    // The picture is the cutscene: it is playing and the fade before its start has ended.
    bool shown() const { return playing_ && playing_->startFadeLeft <= 0; }
    // Opacity of the black veil of the cutscene end on the top screen, 0 clear to 1 black (the fade before the start is
    // the screen fade of the script runner).
    float veil() const;
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
    float afterEnd_ = -1;
    bool fadeDisabled_ = false;
    bool hostPlaysMovies_ = false;
    bool modeEntered_ = false;
};

}
