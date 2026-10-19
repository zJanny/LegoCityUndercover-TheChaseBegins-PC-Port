// The cat rescue spots of lvl_01_cherrytreehills (src/runtime/cat_rescue.cpp, docs/re/objects.md 2026-10-19 cat rescue entry). Needs the extracted
// RomFS; exits with 77 (skipped) when it is missing. Provenance: the four spots, their `Cat` links and the cat clips are level and archive data read
// with tools/convert_levels.py and the C++ loaders on 2026-10-19; the clip per state and the transitions are the state change 0x1c6aa8 and the clip
// loader 0x182a5c (REA, Ghidra 12.1.2): state 1 plays idle_sat looping, 4 rescuecat_jump once, 6 rescuecat_jump_idle looping, 7 rescuecat_rescue once,
// 8 disables the cat and the spot.
#include "assets/assets.hpp"
#include "assets/object_classes.hpp"
#include "runtime/cat_rescue.hpp"
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
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: cat-rescue-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "cat-rescue-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        const assets::FuseEntry* entry = archive.find(kLevelFile);
        require(entry != nullptr, std::string("not in archive: ") + kLevelFile);
        const assets::Level level = assets::parseLevel(archive.read(*entry));
        const assets::ClassLookup classOf = assets::archiveClassLookup(archive);

        runtime::CatRescueSet set;
        set.load(level, classOf);
        require(set.spots().size() == 4, "four cat rescue spots, found " + std::to_string(set.spots().size()));
        for (std::size_t i = 0; i < 4; ++i) {
            const std::string number = "0" + std::to_string(i + 1);
            require(set.spots()[i].instance == "fp_task_fireman_cth_" + number + "_usespot", "spot " + number + ": " + set.spots()[i].instance);
            require(set.spots()[i].cat == "fp_task_fireman_cth_" + number, "the Cat link of spot " + number + ": " + set.spots()[i].cat);
            require(set.spots()[i].state == 0 && set.spots()[i].requested == runtime::kCatRescueIdle, "create: state 0, state 1 requested");
        }

        const auto starts = set.update(1.0f / 30.0f);
        require(starts.size() == 4, "the first update starts the idle clip on each cat");
        for (const runtime::AnimationPlay& start : starts)
            require(start.request.name == "idle_sat" && start.request.mode == runtime::kAnimationLoop && start.request.blend == 0.0f, "idle_sat looping");
        require(set.update(1.0f / 30.0f).empty(), "no change, no clip");
        require(set.spots()[0].timer > 0.03f && set.spots()[0].timer < 0.04f, "the timer counts the updates of the state");

        for (const char* name : {"idle_sat", "rescuecat_jump", "rescuecat_jump_idle", "rescuecat_rescue"})
            require(archive.find(runtime::PropAnimations::animationPath("char_cat", name)) != nullptr, std::string("cat clip in the archive: ") + name);

        const std::string spot = "fp_task_fireman_cth_01_usespot", cat = "fp_task_fireman_cth_01";
        set.request(spot, runtime::kCatRescueJump);
        auto jump = set.update(1.0f / 30.0f);
        require(jump.size() == 1 && jump[0].instance == cat && jump[0].request.name == "rescuecat_jump" && jump[0].request.mode == runtime::kAnimationOnce, "state 4");
        require(set.spots()[0].timer == 0.0f, "the timer restarts with the state");
        set.request(spot, runtime::kCatRescueJumpIdle);
        jump = set.update(1.0f / 30.0f);
        require(jump.size() == 1 && jump[0].request.name == "rescuecat_jump_idle" && jump[0].request.mode == runtime::kAnimationLoop, "state 6");
        set.request(spot, runtime::kCatRescueRescued);
        jump = set.update(1.0f / 30.0f);
        require(jump.size() == 1 && jump[0].request.name == "rescuecat_rescue" && jump[0].request.mode == runtime::kAnimationOnce, "state 7");
        require(set.takeDisabled().empty(), "nothing disabled before state 8");
        set.request(spot, runtime::kCatRescueGone);
        require(set.update(1.0f / 30.0f).empty(), "state 8 starts no clip");
        const std::vector<std::string> gone = set.takeDisabled();
        require(gone.size() == 2 && gone[0] == cat && gone[1] == spot, "state 8 disables the cat and the spot");

        const assets::DrawableObjects drawable = assets::drawableObjects(archive, level);
        std::vector<runtime::AnimatedProp> props;
        for (const assets::Placement& placement : drawable.objects)
            for (const std::string& name : set.cats()) {
                std::string lowered = placement.instance;
                for (char& c : lowered) c = char(std::tolower(static_cast<unsigned char>(c)));
                if (lowered == name) props.push_back({lowered, assets::meshModelPath(placement.mesh), placement.mesh, assets::placementMatrix(placement)});
            }
        require(props.size() == 4, "the four cats are drawn props");
        runtime::PropAnimations animations(archive, props);
        runtime::CatRescueSet playing;
        playing.load(level, classOf);
        for (const runtime::AnimationPlay& start : playing.update(1.0f / 30.0f)) animations.play(start);
        require(animations.started() == 4, "the idle clip runs on all four cats");
        const auto first = animations.poses();
        for (int i = 0; i < 30; ++i) animations.update(1.0f / 30.0f);
        const auto later = animations.poses();
        require(first.size() == 4 && later.size() == 4 && first[0].skin != later[0].skin, "the sitting cat moves");
    } catch (const std::exception& error) {
        std::cerr << "cat-rescue-real-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "cat-rescue-real-test: ok\n";
    return 0;
}
