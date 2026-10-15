#pragma once
// The music block 0x376b30 with its current-track record 0x5861e0 and queued-track record 0x586230: the start routine
// 0x24039c, the stop 0x21cc04, the fade-out setter 0x240f6c + 0x22b660, the mood route 0x206c40 and the per-update
// ramp and queue updater 0x240c3c. Evidence: docs/re/audio-music-block.md. The stream itself is owned by the audio
// device through MusicStreams. No SDL dependency.
#include "assets/assets.hpp"
#include <cstdint>
#include <optional>

namespace chase::runtime {

class MusicStreams {
public:
    virtual ~MusicStreams() = default;
    virtual bool open(unsigned track, bool loop) = 0;
    virtual void close() = 0;
    virtual void setVolume(float volume) = 0;
    virtual bool finished() = 0;
};

constexpr float kMusicUpdatesPerSecond = 30.0f;   // 0x23ca4c returns 30 without a rate object; the oracle ramp matches (docs/re/audio-music-block.md)
constexpr float kMusicInitialRampStep = 0.02f;    // literal 0x3ca3d70a at 0x17eb84, stored at +0x20 by the block init 0x17eb5c

class MusicBlock {
public:
    explicit MusicBlock(MusicStreams& streams) : streams_(streams) {}

    void setMusicSet(std::optional<assets::MusicSet> set) { set_ = set; }

    // 0x240ec8: the script start (OverrideMusic): skipped while the lock hword +6 is nonzero, sets the override byte +1.
    void startTrack(unsigned track, bool loop, bool crossfade, float fade);
    // 0x24039c
    void request(unsigned track, bool loop, bool crossfade, float fade);
    // 0x21cc04
    void stopNow();
    // 0x240f6c(step) then 0x22b660(0.0)
    void fadeOut(float step);
    // 0x1c29ac
    void clearOverride() { override_ = false; }
    // 0x206c40(fade, mood, loop, crossfade)
    void moodRoute(unsigned mood, bool loop, bool crossfade, float fade);
    // 0x240c3c(dt) with the rate of 0x23ca4c
    void update(float dt, float updatesPerSecond = kMusicUpdatesPerSecond);

    bool streamOpen() const { return streamOpen_; }
    unsigned track() const { return currentTrack_; }
    float volume() const { return volume_; }
    float target() const { return target_; }
    bool rampActive() const { return ramp_; }
    bool overrideActive() const { return override_; }
    unsigned mood() const { return mood_; }
    bool queued() const { return queuedPresent_; }
    unsigned queuedTrack() const { return queuedTrack_; }
    float rampStep() const { return step_; }

private:
    MusicStreams& streams_;
    std::optional<assets::MusicSet> set_;
    bool streamOpen_ = false;
    bool queuedFlag_ = false;
    bool override_ = false;
    bool ramp_ = false;
    unsigned mood_ = 0;
    unsigned lock_ = 0;
    float step_ = kMusicInitialRampStep;
    unsigned currentTrack_ = 0;
    bool currentLoop_ = false;
    float target_ = 0.0f;
    float volume_ = 0.0f;
    bool queuedPresent_ = false;
    unsigned queuedTrack_ = 0;
    bool queuedLoop_ = false;
    float queuedFade_ = 0.0f;
};

}
