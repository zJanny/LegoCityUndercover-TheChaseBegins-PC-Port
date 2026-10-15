#pragma once
// Script music natives OverrideMusic 0x1cdb28, AutomaticMusic 0x1d9c70 and StopMusic 0x195054 (batch SCRIPT-33,
// docs/re/script-natives-audio.md). The state is the music block at 0x376b30 of the original; the stream itself is
// started by the owner of the audio device from the requests. No SDL dependency.
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace chase::runtime {

enum class MusicNativeKind { OverrideMusic, AutomaticMusic, StopMusic };

std::optional<MusicNativeKind> musicNativeKind(const std::string& lowerCaseName);
std::vector<std::string> musicNativeNames();
unsigned musicNativeArguments(MusicNativeKind kind);

constexpr float kOverrideMusicFade = 0.5f;           // literal 0x3f000000 at 0x1cdb50, the s0 argument of 0x240ec8
constexpr float kStopMusicFramesPerSecond = 30.0f;   // 0x195084: the per-update step is 1 / (30 * seconds)
constexpr float kStopMusicMinimumStep = 0.02f;       // 0x240f8c, used by 0x240f6c when the step is <= 0

struct MusicRequest {
    enum class Kind {
        Start,    // 0x24039c(name of the row, row, loop, crossfade, fade)
        StopNow,  // 0x21cc04(1)
        FadeOut,  // 0x240f6c(step) then 0x22b660(0.0): the volume ramps to 0 by `step` per update
        ClearOverride,  // 0x1c29ac: byte 0x376b31 cleared by AutomaticMusic
    };
    Kind kind = Kind::Start;
    unsigned track = 0;
    bool loop = false;
    bool crossfade = false;
    float fade = 0.0f;
    float step = 0.0f;
};

class MusicNativeState {
public:
    // The rows of tablessoundfx sheet 1; a larger track id would index past the name array of 0x376b40.
    void setTrackCount(std::size_t count) { trackCount_ = count; }

    void run(MusicNativeKind kind, const std::vector<float>& values);

    // Byte 0x376b31: OverrideMusic sets it (0x240ec8), AutomaticMusic clears it (0x1c29ac).
    bool overrideActive() const { return overrideActive_; }
    std::vector<MusicRequest> takeRequests();
    unsigned outOfRangeTracks() const { return outOfRange_; }

private:
    std::size_t trackCount_ = static_cast<std::size_t>(-1);
    bool overrideActive_ = false;
    unsigned outOfRange_ = 0;
    std::vector<MusicRequest> requests_;
};

}
