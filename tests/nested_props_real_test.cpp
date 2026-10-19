// The models of objects nested in class files (runtime::nestedProps, stand-in LEVOBJ-S93) on lvl_01_cherrytreehills: the crane ladder's upper
// half `ladder_part_02` at (0, 6.088, 0) above `1-4_Ladder01`, so the ladder reaches y 30.70 (the roof where the oracle state m13-2-ladder
// shows the ladder above Chase at y 23.6). Reference values: class files through the C++ loaders (round 37 scratch census: 5 nested models drawn
// in lvl_01). Needs the extracted RomFS; exits 77 without it.
#include "assets/assets.hpp"
#include "runtime/nested_props.hpp"

#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: nested-props-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "nested-props-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        using namespace chase;
        assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        const assets::Level level = assets::parseLevel(archive.read(*archive.find("levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl")));
        const assets::ClassLookup classOf = assets::archiveClassLookup(archive);
        const assets::DrawableObjects drawable = assets::drawableObjects(archive, level, classOf);
        std::size_t total = 0;
        bool ladderChecked = false;
        for (const assets::Placement& placed : drawable.objects) {
            const assets::Level* classFile = classOf(placed.className);
            if (!classFile) continue;
            const std::vector<assets::Placement> children = runtime::nestedProps(archive, *classFile, classOf);
            total += children.size();
            if (placed.className.rfind("vehicles/", 0) == 0) require(children.empty(), placed.instance + ": vehicle wheels are left to the vehicle path");
            if (placed.instance != "1-4_Ladder01") continue;
            require(placed.className == "props/prop_crane_ladderlarge", "1-4_Ladder01 is a crane ladder");
            require(children.size() == 1 && children[0].instance == "ladder_part_02" && children[0].className == "props/prop_ladderlarge", "the crane ladder nests ladder_part_02");
            require(std::fabs(children[0].position[1] - 6.08812f) < 1e-4f && children[0].position[0] == 0.0f && children[0].position[2] == 0.0f, "ladder_part_02 sits 6.088 above");
            const assets::Matrix world = assets::multiply(assets::placementMatrix(children[0]), assets::placementMatrix(placed));
            const float top = world[13] + 6.096f;
            require(std::fabs(top - 30.700f) < 2e-3f, "the ladder reaches the roof: " + std::to_string(top));
            ladderChecked = true;
        }
        require(ladderChecked, "1-4_Ladder01 is drawn");
        require(total == 5, "nested models drawn in lvl_01: " + std::to_string(total));
        const assets::Level* locker = classOf("props/prop_classlocker_default");
        require(locker && !locker->objects.empty() && runtime::nestedProps(archive, *locker, classOf).empty(), "a class with a startup script keeps its nested objects uncreated");
    } catch (const std::exception& error) {
        std::cerr << "nested-props-real-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "nested-props-real-test: ok\n";
    return 0;
}
