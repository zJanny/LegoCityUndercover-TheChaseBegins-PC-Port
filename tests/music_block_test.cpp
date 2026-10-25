// The music block 0x376b30 (src/runtime/music_block.*). Recorded values: the ramp rows below come from the oracle trace
// of save state driving the donut van to the shop in mission 1-1 with the ramp byte 0x376b32 set to 1 and the target 0x586224 poked to 0
// (memtrace 14 frames of 0x376b30:48 and 0x586220:16, step 0x376b50 = 0.2, dt per row 0.033333, 0.07, 0.07, ...):
// volume 1.0 -> 0.8 -> 0.38 -> 0 with the stream closed and the track 0 in the same update, then the mood route restarted
// the track with the crossfade: volume 0.42 -> 0.84 -> 1.0, ramp byte cleared. The queue and mood cases follow the
// disassembly of 0x24039c, 0x206c40 and 0x240c3c (docs/re/script-natives-audio.md).
#include "runtime/music_block.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using chase::assets::MusicSet;
using chase::runtime::MusicBlock;
using chase::runtime::MusicStreams;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

bool near(float a, float b) { return std::fabs(a - b) < 1e-5f; }

struct FakeStreams : MusicStreams {
    struct Open {
        unsigned track;
        bool loop;
    };
    std::vector<Open> opened;
    unsigned closed = 0;
    float volume = -1.0f;
    bool ended = false;
    bool failOpen = false;
    bool open(unsigned track, bool loop) override {
        if (failOpen) return false;
        opened.push_back({track, loop});
        return true;
    }
    void close() override { ++closed; }
    void setVolume(float value) override { volume = value; }
    bool finished() override { return ended; }
};

void recordedRamp() {
    FakeStreams streams;
    MusicBlock block(streams);
    block.request(16, true, false, 1.0f);
    require(streams.opened.size() == 1 && streams.opened[0].track == 16 && streams.opened[0].loop && block.volume() == 1.0f, "start without crossfade at the fade volume");
    block.fadeOut(0.2f);
    require(block.rampActive() && block.target() == 0.0f && near(block.rampStep(), 0.2f), "fade out sets ramp and target");
    block.update(0.033333f);
    require(near(block.volume(), 0.8f) && streams.volume == block.volume() && block.streamOpen(), "row 2: 0.8");
    block.update(0.07f);
    require(near(block.volume(), 0.38f), "row 3: 0.38");
    block.update(0.07f);
    require(!block.streamOpen() && block.track() == 0 && !block.rampActive() && streams.closed == 1, "row 4: stream closed, track 0, ramp byte cleared by the clamp");

    block.setMusicSet(MusicSet{16, 5});
    block.moodRoute(0, true, true, 1.0f);
    require(streams.opened.size() == 2 && streams.opened[1].track == 16 && block.volume() == 0.0f && block.rampActive() && block.target() == 1.0f, "row 5 start: crossfade begins at volume 0");
    block.update(0.07f);
    require(near(block.volume(), 0.42f), "row 5: 0.42");
    block.update(0.07f);
    require(near(block.volume(), 0.84f), "row 6: 0.84");
    block.update(0.07f);
    require(block.volume() == 1.0f && !block.rampActive() && streams.volume == 1.0f, "row 7: clamped to the target, ramp cleared");
    block.update(0.07f);
    require(block.volume() == 1.0f && block.streamOpen(), "rows 8..14 stay at 1.0");
}

void crossfadeQueue() {
    FakeStreams streams;
    MusicBlock block(streams);
    block.request(3, true, false, 1.0f);
    block.request(5, true, true, 0.5f);
    require(block.track() == 3 && block.queued() && block.queuedTrack() == 5 && block.target() == 0.0f && block.rampActive() && streams.opened.size() == 1,
            "a crossfade over a playing track queues the new one and ramps the old to 0");
    const float dt = 1.0f / 30.0f;
    unsigned updates = 0;
    while (block.track() == 3 && updates < 200) {
        block.update(dt);
        ++updates;
    }
    require(updates >= 49 && updates <= 52, "the initial step 0.02 per update empties the volume in 50 updates");
    require(streams.opened.size() == 2 && streams.opened[1].track == 5 && block.track() == 5 && !block.queued() && block.volume() == 0.0f, "the queued track starts at volume 0");
    for (int i = 0; i < 30; ++i) block.update(dt);
    require(near(block.volume(), 0.5f) && !block.rampActive(), "it ramps up to the queued fade 0.5");
}

void moodRoutes() {
    FakeStreams streams;
    MusicBlock block(streams);
    block.moodRoute(0, true, true, 1.0f);
    require(streams.opened.empty() && block.mood() == 0, "no set: the route does nothing");
    block.setMusicSet(MusicSet{255, 5});
    block.moodRoute(0, true, true, 1.0f);
    require(streams.opened.empty() && !block.rampActive() && block.track() == 0, "set 2 ambient entry 255 with nothing playing starts nothing");
    block.request(7, true, false, 1.0f);
    block.moodRoute(0, true, true, 1.0f);
    require(block.track() == 0 && block.rampActive() && block.target() == 0.0f && block.streamOpen(), "entry 255 with a track playing fades it out and clears the track");
    for (int i = 0; i < 60 && block.streamOpen(); ++i) block.update(1.0f / 30.0f);
    require(!block.streamOpen(), "the fade ends the stream");

    FakeStreams second;
    MusicBlock overridden(second);
    overridden.setMusicSet(MusicSet{4, 5});
    overridden.startTrack(9, true, true, 0.5f);
    require(overridden.overrideActive() && second.opened.size() == 1 && second.opened[0].track == 9, "OverrideMusic route starts the track");
    overridden.moodRoute(0, true, true, 1.0f);
    require(overridden.track() == 9 && !overridden.queued(), "the override byte blocks the mood route");
    overridden.clearOverride();
    overridden.moodRoute(0, true, true, 1.0f);
    require(overridden.queued() && overridden.queuedTrack() == 4, "after AutomaticMusic the mood route queues the ambient track");

    FakeStreams third;
    MusicBlock same(third);
    same.setMusicSet(MusicSet{4, 5});
    same.moodRoute(0, true, false, 1.0f);
    same.moodRoute(0, true, false, 1.0f);
    require(third.opened.size() == 1 && third.closed == 0 && third.volume == 1.0f, "the track that already plays is not restarted; no crossfade starts at the fade volume");
}

void streamEnd() {
    FakeStreams streams;
    MusicBlock block(streams);
    block.request(2, false, false, 1.0f);
    streams.ended = true;
    block.update(1.0f / 30.0f);
    require(!block.streamOpen() && block.track() == 0 && streams.closed == 1, "a finished stream is closed and the track cleared");

    FakeStreams failing;
    failing.failOpen = true;
    MusicBlock failed(failing);
    failed.request(2, true, true, 1.0f);
    require(!failed.streamOpen() && failed.track() == 2, "a failed open keeps the track id as the original does");
}
}

int main() {
    try {
        recordedRamp();
        crossfadeQueue();
        moodRoutes();
        streamEnd();
    } catch (const std::exception& error) {
        std::cerr << "music-block: " << error.what() << '\n';
        return 1;
    }
    std::cout << "music-block: ok\n";
    return 0;
}
