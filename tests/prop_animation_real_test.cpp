// Skeletal animations of placed props (src/runtime/prop_animation.cpp) on the shipped data of lvl_01_cherrytreehills. Clip values come from
// the archive: models/prop_garage_shutter/open.bfnanm has 60 frames at 15 fps and does not loop; models/prop_dog_cage/open.bfnanm has 44
// frames at 15 fps and loops (read with the C++ loaders on 2026-10-15, listing of animationLength / animationFramesPerSecond / animationLoops).
// The targets are the first arguments of the PlayAnimation / LoadAnimation calls of the level's scripts. Needs the extracted RomFS; exits with 77
// (skipped) when it is missing.
#include "assets/assets.hpp"
#include "runtime/prop_animation.hpp"
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase;

constexpr const char* kLevelFile = "levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl";

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

runtime::AnimationPlay playOf(const std::string& instance, const std::string& name) {
    runtime::AnimationPlay play;
    play.instance = instance;
    play.request.name = name;
    play.request.mode = runtime::kAnimationOnce;
    play.request.speed = 1.0f;
    play.request.blend = 0.0f;
    return play;
}
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: prop-animation-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "prop-animation-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        const assets::FuseEntry* entry = archive.find(kLevelFile);
        require(entry != nullptr, std::string("not in archive: ") + kLevelFile);
        const assets::Level level = assets::parseLevel(archive.read(*entry));

        const std::set<std::string> targets = runtime::animationTargets(level);
        for (const char* name : {"police_garage_door_01", "1-2_dog_cage", "1-2_dumpster_open", "1-3_crane", "1-3_lawnmower_01", "fp_lombardstreet_garagedoor01"})
            require(targets.count(name) == 1, std::string("animation target ") + name);
        require(targets.count("player1") == 0 && targets.count("1-1_ai_donutlady") == 0, "PlayCharAnim targets are not collected");

        require(runtime::PropAnimations::animationPath("Prop_Garage_shutter", "open") == "models/prop_garage_shutter/open.bfnanm", "animation path");
        runtime::PropAnimations props(archive, {{"police_garage_door_01", "models/prop_garage_shutter/prop_garage_shutter.bfnmdl", "Prop_Garage_shutter"},
                                                {"1-2_dog_cage", "models/prop_dog_cage/prop_dog_cage.bfnmdl", "Prop_Dog_Cage"}});
        require(props.poses().empty() && !props.currentNearEnd("1-3_crane"), "nothing plays at the start; an unknown instance is not animated");

        props.play(playOf("police_garage_door_01", "open"));
        require(props.started() == 1, "PlayAnimation starts the shutter");
        const auto firstPose = props.poses();
        require(firstPose.size() == 1 && firstPose[0].key == "models/prop_garage_shutter/prop_garage_shutter.bfnmdl@police_garage_door_01" &&
                    !firstPose[0].skin.empty(),
                "the pose of the shutter has the instance's scene key and one matrix per joint");
        require(props.currentNearEnd("police_garage_door_01") == std::optional<bool>(false), "WaitForCurrentAnim blocks at frame 0");
        for (int i = 0; i < 100; ++i) props.update(1.0f / 30.0f);
        require(props.currentNearEnd("police_garage_door_01") == std::optional<bool>(false), "still opening after 3.3 s (59 frames at 15 fps take 3.93 s)");
        for (int i = 0; i < 20; ++i) props.update(1.0f / 30.0f);
        require(props.currentNearEnd("police_garage_door_01") == std::optional<bool>(true), "WaitForCurrentAnim ends within one frame of the last frame");
        const auto lastPose = props.poses();
        require(lastPose.size() == 1 && lastPose[0].skin != firstPose[0].skin, "the pose moved while the door opened");

        props.stop("police_garage_door_01");
        const auto stopped = props.poses();
        for (int i = 0; i < 10; ++i) props.update(1.0f / 30.0f);
        require(props.poses()[0].skin == stopped[0].skin, "StopAnim freezes the pose");

        props.play(playOf("1-2_dog_cage", "open"));
        require(props.started() == 2, "a second prop plays on its own slots");
        props.play(playOf("1-2_dog_cage", "no_such_clip"));
        require(props.started() == 2, "a clip that is not in the archive is ignored");
        std::cout << "prop-animation-real-test: passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "prop-animation-real-test: " << error.what() << '\n';
        return 1;
    }
}
