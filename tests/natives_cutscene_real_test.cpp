// Mission 1-1 and 1-4 scripts of lvl_01_cherrytreehills with the cutscene natives, run by ScriptRunner. The cast, camera frames and
// soundtrack lengths are read from the archive with the C++ cutscene loader; the expected values were dumped with it on
// 2026-10-14 (1_1_a 405 frames, sound 14.5 s; 1_1_b 391 frames, sound 14.0333 s; 1_1_d 198 frames, 7.6 s; 1_1_e 231 frames,
// 9.5333 s; rate 30). The length of a cutscene is max(frames / 30, soundtrack) as in the viewer, a stand-in until the
// oracle measures the original (MOVIE-S10). Needs the extracted RomFS; exits with 77 (skipped) when it is missing.
#include "assets/assets.hpp"
#include "runtime/level_start.hpp"
#include "runtime/script_runner.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
constexpr const char* kLevelFile = "levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl";

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

// 1-4_ShowroomArrival plays 1_4_a with `DCamCutScenePlay "1_4_a" 1 1 1 1` (flag a set) and waits for it. Oracle values: round
// 20261103-large, emulator servant, 2026-11-03, recorded from a save state in front of the mission 1-4 waypoint, RPC stepped updates
// with the fixed step 0.033 s (docs/re/scripts-missions-1-4-spawners.md, entry of 2026-11-07T10:36:43): the first cut in update
// 23 and Chase at 1-4_P1_GoTo in update 529, 507 updates; the file is 16.7 s (200400 samples at 12 kHz). In a stepped run the game
// clock moves about 0.77 s per update, so the screen fades of 0x26fa18, 0x2fae68 and 0x2fac7c and the wait of the mode switch,
// which run on the clock, take about one update each there; the 507 updates are the play alone. Free run (round
// 20261102-mission14, emulator servant, 2026-11-02, same entry's correction of scripts-missions-1-4.md, 2026-11-02T15:54:42): 17.28
// to 17.32 s from the first cut to the placement, the play plus the end fade and the wait. Port counts of 20261107-traffic-F.
// Free run recorded from a save state before the waypoint (20261108-traffic-C-1, servant, 2026-11-08, RPC polls of the game clock, 7 loads
// agreeing within one update), seconds after Chase comes within 0.47 m of the waypoint: the slot name 0.03, DCamCutScenePlay 0.23, the
// mode enter with the control lost 0.84 to 0.87, the first camera cut 0.90 to 0.94.
void showroomArrivalUpdates(const chase::assets::Level& level, const chase::assets::ClassLookup& classOf,
                            const chase::runtime::CutsceneNativeState::LengthSource& lengthOf) {
    using namespace chase;
    constexpr float kStep = 0.033f;
    const float length = *lengthOf("1_4_a");
    require(std::fabs(length - 16.7f) < 0.001f, "1_4_a is 16.7 s long (camera frames / 30 and the soundtrack agree)");
    assets::PlacedObjectState state = runtime::levelInitialState(level, classOf);
    runtime::ScriptRunner runner(level, state);
    runner.cutscenes().setLengthSource(lengthOf);
    require(runner.start("1-4_ShowroomArrival"), "1-4_ShowroomArrival");
    const auto noObject = [](const std::string&, float*) { return false; };
    int played = -1, shown = -1, playEnd = -1, ended = -1;
    for (int i = 1; i <= 800 && ended < 0; ++i) {
        runner.update(kStep);
        runner.director().update(kStep, noObject);
        const runtime::CutscenePlaying* playing = runner.cutscenes().playing();
        if (playing && played < 0) played = i;
        if (playing && runner.cutscenes().shown() && shown < 0) shown = i;
        if (playing && playing->elapsed >= playing->length && playEnd < 0) playEnd = i;
        if (!playing && played > 0) ended = i;
    }
    require(played > 0 && ended > 0, "1_4_a plays and ends");
    require(!runner.cutscenes().loaded("1_4_a"), "DCamCutSceneFree follows the Wait in the update of the end");
    require(played == 7, "DCamCutScenePlay runs 0.23 s after the script start (7 updates of 0.033 s, free run 20261108-traffic-C-1), got " + std::to_string(played));
    require(shown - played == 19, "flag a: the fade to black of 0.5 s and the mode switch of 0.1 s take 19 updates of 0.033 s, got " + std::to_string(shown - played));
    require(float(shown) * kStep >= 0.84f && float(shown) * kStep <= 0.87f + kStep,
            "the mode enter comes 0.84 to 0.87 s after the script start (free run 20261108-traffic-C-1), got " + std::to_string(float(shown) * kStep));
    require(playEnd - shown == 507, "the play lasts 507 updates of 0.033 s as in the stepped oracle run, got " + std::to_string(playEnd - shown));
    require(ended - played == 545, "Play to the end of the Wait: 19 + 507 + the end fade 16 + the exit wait 3 updates, got " + std::to_string(ended - played));
    const float cutToPlacementSeconds = float(ended - shown) * kStep;
    require(cutToPlacementSeconds >= 17.28f && cutToPlacementSeconds <= 17.32f + 2 * kStep,
            "first cut to the placement within two updates of the free-run 17.28 to 17.32 s, got " + std::to_string(cutToPlacementSeconds));
}
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: natives-cutscene-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "natives-cutscene-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        using namespace chase;
        assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        const assets::FuseEntry* entry = archive.find(kLevelFile);
        require(entry != nullptr, std::string("not in archive: ") + kLevelFile);
        const assets::Level level = assets::parseLevel(archive.read(*entry));
        const assets::ClassLookup classOf = assets::archiveClassLookup(archive);

        const auto lengthOf = [&](const std::string& name) -> std::optional<float> {
            const assets::CutsceneContent content = assets::loadCutscene(archive, name);
            double sound = 0;
            if (!content.soundPath.empty()) {
                const assets::Sound decoded = assets::decodeSound(archive.read(*archive.find(content.soundPath)));
                sound = double(decoded.frames) / decoded.rate;
            }
            return float(std::max(content.camera.frames / double(assets::animationFramesPerSecond(content.camera)), sound));
        };
        const float lengthB = *lengthOf("1_1_b");
        require(std::fabs(lengthB - 14.0333f) < 0.001f, "1_1_b is 14.0333 s long");

        assets::PlacedObjectState state = runtime::levelInitialState(level, classOf);
        runtime::ScriptRunner runner(level, state);
        runner.cutscenes().setLengthSource(lengthOf);
        require(runner.start("1-1_VanBreakdown"), "1-1_VanBreakdown");
        const auto noObject = [](const std::string&, float*) { return false; };
        int freedAt = -1;
        for (int i = 1; i <= 600 && freedAt < 0; ++i) {
            runner.update(1.0f / 30.0f);
            runner.director().update(1.0f / 30.0f, noObject);
            if (i == 1) require(runner.cutscenes().loaded("1_1_b") && !runner.cutscenes().playing(), "1_1_b is loaded and waits for its load time");
            if (!runner.cutscenes().loaded("1_1_b")) freedAt = i;
        }
        require(freedAt > 0, "DCamCutSceneFree follows the Wait");
        const int expected = int(std::ceil((lengthB + 2 * chase::runtime::kCutsceneFadeSeconds + chase::runtime::kCutsceneModeSwitchSeconds + chase::runtime::kCutsceneLoadSeconds + chase::runtime::kCutsceneExitWaitSeconds) * 30.0f));
        require(freedAt >= expected - 1 && freedAt <= expected + 3,
                "Wait blocks for the fade before the start (flag a of 1-1_VanBreakdown is 1), the cutscene length, the end fade and the exit wait: freed at update " + std::to_string(freedAt) + ", expected about " + std::to_string(expected));
        require(runner.cutscenes().playing() == nullptr, "the cutscene has ended when Free runs");
        showroomArrivalUpdates(level, classOf, lengthOf);
    } catch (const std::exception& error) {
        std::cerr << "natives-cutscene-real-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "natives-cutscene-real-test: ok\n";
    return 0;
}
