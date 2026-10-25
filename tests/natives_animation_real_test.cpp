// Scripts of lvl_01_cherrytreehills that use the animation and attach natives of batch SCRIPT-32, run by ScriptRunner. The
// expected values are the static call texts of the level file (dumped with the C++ level loader on 2026-10-14: every script of
// the level with its calls). Needs the extracted RomFS; exits with 77 (skipped) when it is missing.
#include "assets/assets.hpp"
#include "runtime/level_start.hpp"
#include "runtime/script_runner.hpp"
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <optional>
#include <string>
#include <vector>

namespace {
constexpr const char* kLevelFile = "levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl";

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: natives-animation-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "natives-animation-real-test: data not available, skipped\n";
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

        {
            // 1-2_Cage_Key_Target_Script: LoadAnimation(1-2_Dog_Cage, "open", "", 1), PlayAnimation(1-2_Dog_Cage, "open", 1, 0, 0),
            // PlayCharAnim(Player1, "dog_cage_open_minifig", "class_generic", 0), then Wait 3 and the cutscene natives.
            assets::PlacedObjectState state = runtime::levelInitialState(level, classOf);
            runtime::ScriptRunner runner(level, state);
            require(runner.start("1-2_Cage_Key_Target_Script"), "1-2_Cage_Key_Target_Script");
            const auto& animation = runner.animation();
            require(animation.loaded("1-2_dog_cage", "open") && animation.loaded("1-2_dog", "dog_cage_open"), "LoadAnimation");
            const auto* cage = animation.current("1-2_dog_cage");
            require(cage && cage->name == "open" && !cage->character && !cage->loop && cage->speed == 1.0f, "PlayAnimation of the cage");
            const auto* player = animation.current("player1");
            require(player && player->name == "dog_cage_open_minifig" && player->classFolder == "class_generic" && player->character &&
                        !player->loop,
                    "PlayCharAnim of the player");
            for (int i = 0; i < 200 && runner.threadCount(); ++i) runner.update(1.0f / 30.0f);
            require(animation.stopRequests("1-2_ai_dogman") == 1, "StopCharAnim(1-2_AI_Dogman)");
            require(!animation.loaded("1-2_dog_cage", "kick_open"), "UnloadAnimation(1-2_Dog_Cage, kick_open)");
        }
        {
            // 1-2_MissionStart ends with PlayCharAnim(1-2_AI_Dogman, "crying_idle_minifig", "CLASS_Generic", 1) after the title
            // DCam and two Wait 5 (the original reaches it too: docs/re/scripts-missions.md).
            assets::PlacedObjectState state = runtime::levelInitialState(level, classOf);
            runtime::ScriptRunner runner(level, state);
            require(runner.start("1-2_MissionStart"), "1-2_MissionStart");
            const auto noObject = [](const std::string&, float*) { return false; };
            for (int i = 0; i < 1200 && runner.threadCount(); ++i) {
                runner.update(1.0f / 30.0f);
                runner.director().update(1.0f / 30.0f, noObject);
            }
            const auto* dogman = runner.animation().current("1-2_ai_dogman");
            require(dogman && dogman->name == "crying_idle_minifig" && dogman->classFolder == "class_generic" && dogman->loop,
                    "MissionStart plays the crying animation of the dog man");
        }
        {
            // The level startup calls AttachObject(1-3_Lawnmower_01, 1-3_lawn_mower_particles_01 / _02, SFX_Lawnmower_Guy,
            // 1-3_Gardner_Prop): each second object goes under the mower (0x1bf68c, docs/re/script-natives-animation.md).
            assets::PlacedObjectState state = runtime::levelInitialState(level, classOf);
            runtime::ScriptRunner runner(level, state);
            runtime::runLevelStartup(runner, 0.034f, 600);
            const auto* gardener = runner.animation().attachment("1-3_gardner_prop");
            const auto* particles = runner.animation().attachment("1-3_lawn_mower_particles_01");
            require(gardener && gardener->kind == runtime::AttachKind::Object && gardener->parent == "1-3_lawnmower_01" && particles &&
                        particles->parent == "1-3_lawnmower_01",
                    "the gardener and the particles ride on 1-3_Lawnmower_01");
        }
        {
            // 1-1_VanBreakdown ends with PlayCharAnim(1-1_AI_DonutLady, "postboxstuck_idle_minifig", "Class_generic", 1): the driver gets
            // a looping play; the clip has 42 frames and joint table bit 29, the length and end frame of the original's slot
            // (oracle the save state in front of the broken van of mission 1-1, slot 0x08a6f460: end 42, mode 1 with bit 13; docs/re/script-natives-animation.md).
            struct Recorder : runtime::AnimationDriver {
                std::vector<runtime::AnimationPlay> plays;
                void play(const runtime::AnimationPlay& play) override { plays.push_back(play); }
                void stop(const std::string&) override {}
                std::optional<bool> currentNearEnd(const std::string&) const override { return std::nullopt; }
            } recorder;
            assets::PlacedObjectState state = runtime::levelInitialState(level, classOf);
            runtime::ScriptRunner runner(level, state);
            runner.setAnimationDriver(&recorder);
            require(runner.start("1-1_VanBreakdown"), "1-1_VanBreakdown");
            const auto noObject = [](const std::string&, float*) { return false; };
            for (int i = 0; i < 2400 && runner.threadCount(); ++i) {
                runner.update(1.0f / 30.0f);
                runner.director().update(1.0f / 30.0f, noObject);
                runner.cutscenes().update(1.0f / 30.0f, false);
            }
            require(recorder.plays.size() == 1 && recorder.plays[0].instance == "1-1_ai_donutlady", "one play for the van driver");
            const runtime::AnimationRequest& request = recorder.plays[0].request;
            require(request.character && request.name == "postboxstuck_idle_minifig" && request.classFolder == "class_generic" &&
                        request.loop && request.mode == runtime::kAnimationLoop && request.speed == 1.0f,
                    "PlayCharAnim of the stuck pose");
            const auto* entry = archive.find("models/char_minifig/class_generic/postboxstuck_idle_minifig.bfnanm");
            require(entry != nullptr, "postboxstuck_idle_minifig.bfnanm");
            const assets::Animation stuck = assets::parseAnimation(archive.read(*entry));
            require(assets::animationLength(stuck) == 42 && assets::animationLoops(stuck) && assets::animationFramesPerSecond(stuck) == 15.0f,
                    "length 42, loop bit, rate 15");
        }
    } catch (const std::exception& error) {
        std::cerr << "natives-animation-real-test: FAIL: " << error.what() << '\n';
        return 1;
    }
    std::cout << "natives-animation-real-test: ok\n";
    return 0;
}
