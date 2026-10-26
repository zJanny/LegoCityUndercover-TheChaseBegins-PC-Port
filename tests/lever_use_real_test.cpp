// The player's lever use on the extracted RomFS: the clips of 0x200f90 (tablesanimations sheet 1 ids 185, 188, 191), their length and event time, the
// use point of 0x207be4 (node char_minifig_use) of the mission 1-3 switches and the length of their own "use" animation. Exits with 77 without the RomFS.
// Provenance: values read by 20261026-mission13-A on 2026-10-26 through chase_assets (`chase-tabledump tables/tablesanimations.bxls`, the clips under
// models/char_minifig/class_generic, the models prop_keyinhole and prop_switchpull, lvl_01_cherrytreehills_main.blvl); the rules are in docs/re/switches.md.
#include "assets/assets.hpp"
#include "assets/object_classes.hpp"
#include "runtime/carry_animation.hpp"
#include "runtime/lever_use.hpp"
#include "runtime/switch_set.hpp"

#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

bool near(float a, float b, float tolerance = 1e-3f) { return std::fabs(a - b) < tolerance; }

constexpr const char* kLevelFile = "levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl";
constexpr const char* kClipDirectory = "models/char_minifig/class_generic/";
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: lever-use-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "lever-use-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        const auto read = [&](const std::string& path) {
            const assets::FuseEntry* entry = archive.find(path);
            require(entry != nullptr, "not in archive: " + path);
            return archive.read(*entry);
        };
        const runtime::CarryAnimations names(assets::parseTable(read("tables/tablesanimations.bxls")));
        require(names.name(runtime::kLeverClipKey) == "key_interact", "185 is key_interact");
        require(names.name(runtime::kLeverClipTypeDefault) == "switch_lever_use", "188 is switch_lever_use");
        require(names.name(runtime::kLeverClipPull) == "pulllever", "191 is pulllever");

        const runtime::LeverClipTiming key = runtime::leverClipTiming(assets::parseAnimation(read(std::string(kClipDirectory) + "key_interact_minifig.bfnanm")));
        require(near(key.seconds, 2.4f) && near(key.eventSeconds, 0.766788f), "key_interact: 72 frames at 30 per second, event at 0.3195 of 2.4 s, got " +
                                                                                std::to_string(key.seconds) + " / " + std::to_string(key.eventSeconds));
        const runtime::LeverClipTiming pull = runtime::leverClipTiming(assets::parseAnimation(read(std::string(kClipDirectory) + "pulllever_minifig.bfnanm")));
        require(near(pull.seconds, 1.6f) && near(pull.eventSeconds, 0.004364f), "pulllever: 24 frames at 15 per second, event at its start, got " +
                                                                                  std::to_string(pull.seconds) + " / " + std::to_string(pull.eventSeconds));

        const assets::Level level = assets::parseLevel(read(kLevelFile));
        const assets::ClassLookup classOf = assets::archiveClassLookup(archive);
        runtime::SwitchSet switches;
        switches.load(level, classOf, {});
        std::map<std::string, std::string> meshes;
        for (const assets::Placement& placed : assets::objectPlacements(level, classOf)) meshes[placed.instance] = placed.mesh;
        const auto lever = [&](const std::string& instance) -> const runtime::SwitchObject& {
            const std::optional<std::size_t> index = switches.find(instance);
            require(index && switches.objects()[*index].lever, instance + " is a lever");
            return switches.objects()[*index];
        };
        const auto target = [&](const std::string& instance, const runtime::SwitchObject& sw) {
            const auto mesh = meshes.find(instance);
            require(mesh != meshes.end(), instance + " has a model");
            const std::optional<runtime::UseLerpTarget> found = runtime::leverUseTarget(assets::parseModel(read(assets::meshModelPath(mesh->second))), sw.matrix);
            require(found.has_value(), instance + ": its model has char_minifig_use");
            return *found;
        };
        const runtime::SwitchObject& keySwitch = lever("1-3_garage_key_switch");
        require(keySwitch.lever->use.clipId == 185, "the key switch (LeverType 6) plays key_interact");
        const runtime::UseLerpTarget keyPoint = target("1-3_Garage_Key_Switch", keySwitch);
        require(near(keyPoint.position[0], 985.39f, 0.01f) && near(keyPoint.position[2], 1141.25f, 0.01f) && std::abs(keyPoint.heading - 24493) <= 2,
                "use point of the key switch, got " + std::to_string(keyPoint.position[0]) + " " + std::to_string(keyPoint.position[2]) + " heading " +
                    std::to_string(keyPoint.heading));
        const runtime::SwitchObject& generator = lever("1-3_generator_pull_switch");
        require(generator.lever->use.clipId == 191, "the generator pull switch (LeverType 7) plays pulllever");
        const runtime::UseLerpTarget pullPoint = target("1-3_Generator_Pull_Switch", generator);
        require(near(pullPoint.position[0], 948.274f, 0.01f) && near(pullPoint.position[2], 1129.66f, 0.01f) && std::abs(pullPoint.heading - 19078) <= 2,
                "use point of the generator pull switch, got " + std::to_string(pullPoint.position[0]) + " " + std::to_string(pullPoint.position[2]) +
                    " heading " + std::to_string(pullPoint.heading));

        const assets::Animation keyUse = assets::parseAnimation(read("models/prop_keyinhole/use.bfnanm"));
        const assets::Animation pullUse = assets::parseAnimation(read("models/prop_switchpull/use.bfnanm"));
        require(assets::animationLength(keyUse) == 25 && assets::animationFramesPerSecond(keyUse) == 15.0f, "the key's use animation: 25 frames at 15");
        require(assets::animationLength(pullUse) == 24 && assets::animationFramesPerSecond(pullUse) == 15.0f, "the pull switch's use animation: 24 frames at 15");
    } catch (const std::exception& error) {
        std::cerr << "lever-use-real-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "lever-use-real-test: ok\n";
    return 0;
}
