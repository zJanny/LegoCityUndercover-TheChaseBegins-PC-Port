// Mission 1-1 scripts of lvl_01_cherrytreehills with the cutscene natives, run by ScriptRunner. The cast, camera frames and
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
        require(runner.cutscenes().playing() && runner.cutscenes().playing()->name == "1_1_b", "the script starts 1_1_b");
        const auto noObject = [](const std::string&, float*) { return false; };
        int freedAt = -1;
        for (int i = 1; i <= 600 && freedAt < 0; ++i) {
            runner.update(1.0f / 30.0f);
            runner.director().update(1.0f / 30.0f, noObject);
            if (!runner.cutscenes().loaded("1_1_b")) freedAt = i;
        }
        require(freedAt > 0, "DCamCutSceneFree follows the Wait");
        const int expected = int(std::ceil((lengthB + chase::runtime::kCutsceneFadeSeconds) * 30.0f));
        require(freedAt >= expected - 1 && freedAt <= expected + 1,
                "Wait blocks for the fade before the start (flag a of 1-1_VanBreakdown is 1) and the cutscene length: freed at update " + std::to_string(freedAt) + ", expected about " + std::to_string(expected));
        require(runner.cutscenes().playing() == nullptr, "the cutscene has ended when Free runs");
    } catch (const std::exception& error) {
        std::cerr << "natives-cutscene-real-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "natives-cutscene-real-test: ok\n";
    return 0;
}
