// The use spot 1-2_Dog_Cage_Use_Spot of lvl_01_cherrytreehills (src/runtime/use_spot.cpp, docs/re/use-spot.md). Needs the extracted RomFS; exits with
// 77 without it. Provenance: the object and its TriggerObject reference are level data (tools/convert_levels.py); the player position is the user's
// oracle save state at the dog cage trigger of mission 1-2 ("standing on the trigger in front of the dog cage", mission 1-2 in progress, position 916.702 64.431 663.724); the one-shot rule is the handler 0x1922e8
// with 0x236624.
#include "assets/assets.hpp"
#include "runtime/use_spot.hpp"

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

void cageSpot(const assets::Level& level, const assets::ClassLookup& classOf) {
    runtime::UseSpotSet set;
    set.load(level, classOf);
    require(set.spots().size() == 1, "one use spot in the level, found " + std::to_string(set.spots().size()));
    const runtime::UseSpot& spot = set.spots().front();
    require(spot.instance == "1-2_dog_cage_use_spot" && spot.triggerObject == "1-2_cage_key_target_script", "references " + spot.triggerObject);
    // Provenance: the class file levels/props/prop_simpleuse.blvl, bound "UseBound" (tools/convert_levels.py / assets::parseLevel).
    require(spot.zoneCentre[0] == 0.0f && spot.zoneCentre[1] == 0.0f && spot.zoneCentre[2] == 0.0f && spot.zoneHalf[0] == 2.0f && spot.zoneHalf[1] == 2.0f &&
                spot.zoneHalf[2] == 2.0f,
            "UseBound centre 0, half 2");
    require(!spot.sendPlayerObject && spot.active, "defaults of SendPlayerObject and UseStartActive");
    require(spot.matrix[12] == spot.position[0] && spot.matrix[13] == spot.position[1] && spot.matrix[14] == spot.position[2] && spot.matrix[5] == 1.0f,
            "the node matrix of the spot (the frame of its use marker) stands at its position, upright");

    const assets::PlacedObjectState state;
    const float oracle[3] = {916.702f, 64.431f, 663.724f};
    const float far[3] = {870.226f, 66.001f, 630.961f};
    require(set.update(state, oracle, false, true).empty(), "no press, no event");
    require(set.update(state, oracle, true, false).empty(), "the press is not delivered while the player is not free");
    require(set.update(state, far, true, true).empty(), "outside the zone");
    const auto fired = set.update(state, oracle, true, true);
    require(fired.size() == 1 && fired[0].target == "1-2_cage_key_target_script" && fired[0].spot == "1-2_dog_cage_use_spot", "fires once");
    require(set.update(state, oracle, true, true).empty(), "the zone is inactive after the use (0x236624)");
    set.reactivate("1-2_dog_cage_use_spot");
    require(set.update(state, oracle, true, true).size() == 1, "message 8 activates the zone again");

    assets::PlacedObjectState disabled;
    disabled.disabled.insert("1-2_dog_cage_use_spot");
    set.reactivate("1-2_dog_cage_use_spot");
    require(set.update(disabled, oracle, true, true).empty(), "a disabled object takes no press");
}
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: use-spot-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "use-spot-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        const assets::FuseEntry* entry = archive.find(kLevelFile);
        require(entry != nullptr, std::string("not in archive: ") + kLevelFile);
        const assets::Level level = assets::parseLevel(archive.read(*entry));
        cageSpot(level, assets::archiveClassLookup(archive));
    } catch (const std::exception& error) {
        std::cerr << "use-spot-real-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "use-spot-real-test: ok\n";
    return 0;
}
