// The animals of lvl_01_cherrytreehills that are placed props (docs/re/objects.md): objects with an animal rig whose kind's create builds the
// model with the loader 0x2442b4 and no character setup (kinds 0, 64 and 67, assets::createsModelAsProp) are drawn like other placed models;
// ride_pig (kind 159) and the sharks (kind 239) have creates of their own and stay out. Position and class values: python3 tools/convert_levels.py
// lvl_01_cherrytreehills_main.blvl --param-names artifacts/game/code.bin (2026-10-19); kinds from the class files through the C++ loader. The
// release clip of the mission 1-2 dog: models/char_dog/dog_cage_open.bfnanm read with the C++ loaders on 2026-10-19 (45 frames stored,
// no loop), whose extra root curve the prop update applies to the node (runtime/prop_root_motion.hpp). Needs the extracted RomFS; exits with 77 (skipped) when it is missing.
#include "assets/assets.hpp"
#include "assets/object_classes.hpp"
#include "runtime/prop_animation.hpp"
#include <cmath>
#include <filesystem>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>

namespace {
using namespace chase;

constexpr const char* kLevelFile = "levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl";

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

struct Pin {
    const char* instance;
    const char* className;
    int kind;
    const char* mesh;
    float position[3];
    float yaw;
};

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
        std::cerr << "usage: animal-props-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "animal-props-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        require(assets::createsModelAsProp(0) && assets::createsModelAsProp(64) && assets::createsModelAsProp(67), "kinds 0, 64 and 67 build a prop model");
        require(!assets::createsModelAsProp(27) && !assets::createsModelAsProp(159) && !assets::createsModelAsProp(239), "characters, ride_pig and sharks do not");

        assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        const assets::FuseEntry* entry = archive.find(kLevelFile);
        require(entry != nullptr, std::string("not in archive: ") + kLevelFile);
        const assets::Level level = assets::parseLevel(archive.read(*entry));
        const assets::ClassLookup classOf = assets::archiveClassLookup(archive);
        const assets::DrawableObjects drawable = assets::drawableObjects(archive, level);

        std::map<std::string, const assets::Placement*> byInstance;
        for (const assets::Placement& placement : drawable.objects) byInstance[placement.instance] = &placement;
        const Pin pins[] = {
            {"1-2_Dog", "props/prop_prop", 67, "char_dog", {918.717f, 64.668f, 662.731f}, -54.984f},
            {"1-2_Dog_Prop", "props/prop_dogwaggles", 0, "char_dog", {918.664f, 64.763f, 663.005f}, -65.738f},
            {"FP_Task_FIREMAN_CTH_01", "props/ability_freeplay_fireman_cat", 0, "char_cat", {923.096f, 42.521f, 1125.745f}, 170.0f},
            {"1-3_Crow", "leprops/leprop_platform", 64, "char_Crow", {947.868f, 28.259f, 1145.544f}, 269.852f},
        };
        for (const Pin& pin : pins) {
            const auto found = byInstance.find(pin.instance);
            require(found != byInstance.end(), std::string(pin.instance) + " is a drawn placed object");
            const assets::Placement& placement = *found->second;
            const assets::LevelObject& object = level.objects[placement.objectIndex];
            require(placement.className == pin.className, std::string(pin.instance) + " class");
            require(assets::objectKind(object, *classOf(placement.className)) == pin.kind, std::string(pin.instance) + " kind");
            require(placement.mesh == pin.mesh, std::string(pin.instance) + " mesh");
            require(archive.find(assets::meshModelPath(placement.mesh)) != nullptr, std::string(pin.instance) + " model in the archive");
            for (int i = 0; i < 3; ++i)
                require(std::fabs(placement.position[i] - pin.position[i]) < 1e-3f, std::string(pin.instance) + " position");
            require(std::fabs(placement.orientation[1] - pin.yaw) < 1e-3f, std::string(pin.instance) + " facing");
        }
        std::size_t cats = 0;
        for (const assets::Placement& placement : drawable.objects) cats += placement.className == "props/ability_freeplay_fireman_cat";
        require(cats == 4, "the four fireman cats are drawn");
        require(drawable.skeletal == 9, "five ride_pig and four sharks stay out");
        require(assets::meshModelPath("char_dog") == "models/char_dog/char_dog.bfnmdl", "dog model path");

        require(runtime::animationTargets(level).count("1-2_dog") == 1, "the cage key script plays dog_cage_open on 1-2_Dog");
        const assets::Animation release = assets::parseAnimation(archive.read(*archive.find("models/char_dog/dog_cage_open.bfnanm")));
        require(!assets::animationLoops(release) && assets::animationLength(release) == 44, "dog_cage_open: 45 frames, played once");
        const assets::Matrix placed = assets::placementMatrix(*byInstance.at("1-2_Dog"));
        runtime::PropAnimations props(archive, {{"1-2_dog", "models/char_dog/char_dog.bfnmdl", "char_dog", placed}});
        require(props.poses().empty(), "no clip runs at the start (the create starts only an Animation parameter, which the dog has none of)");
        props.play(playOf("1-2_dog", "dog_cage_open"));
        require(props.started() == 1, "PlayAnimation starts the release clip");
        const auto first = props.poses();
        require(first.size() == 1 && first[0].key == "models/char_dog/char_dog.bfnmdl@1-2_dog" && !first[0].skin.empty(), "dog pose key and joints");
        require(!first[0].moved && first[0].node == placed, "the node stands at the placement before the first update");
        for (int i = 0; i < 30; ++i) props.update(1.0f / 30.0f);
        require(props.poses()[0].skin != first[0].skin, "the dog moves while the clip plays");
        for (int i = 0; i < 90; ++i) props.update(1.0f / 30.0f);
        const runtime::PropPose last = props.poses()[0];
        // Oracle: Azahar GDB trap at 0x16b358 from m12-13-cage-trigger after the A press (artifacts/scratch/r28-A/release_root.txt,
        // docs/re/objects.md 2026-10-19T10:10:34): the dog's node ends at 915.2977 64.3605 663.6295, yaw -87.64 degrees (cos 0.0411).
        require(last.moved && std::fabs(last.node[12] - 915.2977f) < 2e-3f && std::fabs(last.node[13] - 64.3605f) < 2e-3f &&
                    std::fabs(last.node[14] - 663.6295f) < 2e-3f && std::fabs(last.node[0] - 0.04113f) < 1e-3f,
                "the root motion of dog_cage_open carries the dog out of the cage");
        std::cout << "animal-props-real-test: passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "animal-props-real-test: " << error.what() << '\n';
        return 1;
    }
}
