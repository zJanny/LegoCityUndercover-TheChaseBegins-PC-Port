// Build-its of lvl_01_cherrytreehills (src/runtime/buildit_set.cpp): the class parameters of 1-1_BI_DonutVanParts, its use zone and the
// run of mission 1-1 step 6: after 1-1_Found_Van enables it, holding the action button in the zone builds it and the finished build fires
// ObjectTriggered of 1-1_SO_VanFixed, which switches the broken van off and the driveable van on. Zone values were read from the record
// +0x28 of the object in the oracle state m11-4-van-parts (docs/re/buildit.md). Needs the extracted RomFS; exits with 77 without it.
#include "assets/assets.hpp"
#include "runtime/buildit_set.hpp"
#include "runtime/level_start.hpp"
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

bool near(float a, float b, float tolerance) { return std::fabs(a - b) <= tolerance; }
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: buildit-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "buildit-real-test: data not available, skipped\n";
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

        runtime::BuilditSet set;
        set.load(archive, level, classOf);
        const runtime::BuilditObject* parts = set.find("1-1_bi_donutvanparts");
        require(parts != nullptr, "1-1_BI_DonutVanParts is a build-it");
        require(parts->finalProp == "1-1_donut_van_body", "BuilditFinalProp " + parts->finalProp);
        require(parts->triggerObject == "1-1_so_vanfixed", "TriggerObject " + parts->triggerObject);
        require(near(parts->studValue, 25.0f, 0.0f), "StudValue 25");
        require(parts->build.progress.size() == 6, "six bricks, got " + std::to_string(parts->build.progress.size()));
        require(parts->hasZone, "use zone");
        const float centre[3] = {-0.4423f, 0.9966f, 0.9803f}, half[3] = {4.2706f, 2.5589f, 4.0072f};
        for (int a = 0; a < 3; ++a) {
            require(near(parts->zoneCentre[a], centre[a], 0.01f), "zone centre " + std::to_string(a) + ": " + std::to_string(parts->zoneCentre[a]));
            require(near(parts->zoneHalf[a], half[a], 0.01f), "zone half " + std::to_string(a) + ": " + std::to_string(parts->zoneHalf[a]));
        }
        const float standing[3] = {876.914f, 52.435f, 700.427f}, far[3] = {876.914f, 52.435f, 712.0f};
        require(runtime::BuilditSet::inZone(*parts, standing), "the oracle position m11-4-van-parts is in the zone");
        require(!runtime::BuilditSet::inZone(*parts, far), "12 m away is outside");

        assets::PlacedObjectState state = runtime::levelStartState(level, classOf);
        runtime::ScriptRunner scripts(level, state);
        require(scripts.start("1-1_Found_Van"), "1-1_Found_Van exists");
        for (int i = 0; i < 4000 && scripts.threadCount() > 0; ++i) scripts.update(0.034f);
        require(scripts.threadCount() == 0, "1-1_Found_Van ended");
        require(!state.isDisabled("1-1_BI_DonutVanParts"), "1-1_Found_Van enables the build-it");
        require(state.isDisabled("1-1_Donut_Van_Body"), "the final prop is hidden before the build");
        state.disabled.erase("1-1_brokenvan");
        require(state.isDisabled("1-1_DonutVan"), "driveable van off before the build");

        int builtAt = -1;
        for (int update = 1; update <= 120; ++update) {
            const auto actions = set.update(state, standing, true);
            runtime::applyBuilditActions(set, actions, state, scripts);
            scripts.update(0.034f);
            scripts.checkTriggers(standing);
            for (const runtime::BuilditAction& action : actions)
                if (action.kind == runtime::BuilditActionKind::Built) builtAt = update;
        }
        require(builtAt == 85, "built at update " + std::to_string(builtAt));
        require(state.isDisabled("1-1_BI_DonutVanParts"), "parts hidden after the build");
        require(state.isDisabled("1-1_BrokenVan"), "1-1_SO_VanFixed switched the broken van off");
        require(!state.isDisabled("1-1_DonutVan"), "1-1_SO_VanFixed switched the driveable van on");
        require(state.isDisabled("1-1_Donut_Van_Body"), "1-1_SO_VanFixed hid the final prop again");
        require(!state.isDisabled("1-1_Van_Waypoint_Enter"), "1-1_SO_VanFixed enabled the van waypoint");
    } catch (const std::exception& error) {
        std::cerr << "buildit-real-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "buildit-real-test: ok\n";
    return 0;
}
