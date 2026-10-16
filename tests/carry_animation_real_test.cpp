// The animations of carrying (src/runtime/carry_animation.cpp) on the extracted RomFS: the role record variants of 0x219614 (tablesanimations sheet 2),
// the clips of the pickup and put-down states, the event updates of those clips and the hand offset of the donut box. Exits with 77 without the RomFS.
// Provenance: variants and names from `chase-tabledump tables/tablesanimations.bxls` (rows 0, 1, 2, 12 of sheet 2; ids 155, 157, 551, 552, 556, 558 of sheet 1);
// states and hand joints from REA pseudo code of 0x29b170, 0x283118, 0x219614 and objdump of 0x2ce4a4 (docs/re/carrying.md); the put-down release at update 8
// is the oracle measurement of state 0xd6 (docs/re/carrying.md, 2026-10-15T00:17); the pickup attach at update 16 is derived from the clip, not measured.
#include "assets/assets.hpp"
#include "runtime/carry_animation.hpp"
#include "runtime/player_locomotion.hpp"
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

bool near(float a, float b) { return std::fabs(a - b) < 1e-4f; }
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: carry-animation-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "carry-animation-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        using namespace chase;
        assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        const auto read = [&](const std::string& path) {
            const assets::FuseEntry* entry = archive.find(path);
            require(entry != nullptr, "not in archive: " + path);
            return archive.read(*entry);
        };
        const runtime::CarryAnimations animations(assets::parseTable(read("tables/tablesanimations.bxls")));
        require(animations.name(runtime::kAnimationPickupBig) == "supercarry_pickup", "0x22c is supercarry_pickup");
        require(animations.name(runtime::kAnimationPutdownBig) == "supercarry_putdown", "0x22e is supercarry_putdown");
        require(animations.name(runtime::kAnimationPickupSmall) == "itemleft_pickup", "0x9b is itemleft_pickup");
        require(animations.name(runtime::kAnimationPutdownSmall) == "itemleft_putdown", "0x9d is itemleft_putdown");
        require(animations.name(animations.clip(0, 2)) == "supercarry_idle", "idle with a big object");
        require(animations.name(animations.clip(1, 2)) == "supercarry_walk", "walk with a big object");
        require(animations.name(animations.clip(2, 2)) == "supercarry_walk", "run with a big object");
        require(animations.clip(runtime::kAnimationSprint, 2) == runtime::kAnimationSprint, "the sprint role has no carried variant");
        require(animations.clip(1, 0) == 1, "nothing carried changes nothing");
        require(animations.clip(0, 1) == 149, "idle with a small object is record variant 3");

        const auto timing = runtime::carryTiming(assets::parseAnimation(read("models/char_minifig/class_generic/supercarry_pickup_minifig.bfnanm")),
                                                 assets::parseAnimation(read("models/char_minifig/class_generic/supercarry_putdown_minifig.bfnanm")));
        require(timing.placeRelease == 8, "put-down release at update " + std::to_string(timing.placeRelease));
        require(timing.pickupAttach == 16, "pickup attach at update " + std::to_string(timing.pickupAttach));
        require(timing.pickupEnd == 48, "the pickup state ends with its clip: 24 frames at 15 per second = 48 updates (oracle: slot replaced at update 49 with dt 0.033)");

        const assets::Model box = assets::parseModel(read("models/prop_box_o_donuts/prop_box_o_donuts_atlas_lvl_01_cherrytreehills_main.bfnmdl"));
        const std::array<float, 16> offset = runtime::carryHandOffset(box);
        require(near(offset[0], 1) && near(offset[5], 1) && near(offset[10], 1) && near(offset[12], 0) && near(offset[13], 0.1f) && near(offset[14], 0),
                "the donut box hangs 0.1 above the hand (carryhandle at y -0.1)");
    } catch (const std::exception& error) {
        std::cerr << "carry-animation-real-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "carry-animation-real-test: ok\n";
    return 0;
}
