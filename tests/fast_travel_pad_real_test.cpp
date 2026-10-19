// The fast travel pad of the helipad of lvl_01_cherrytreehills (src/runtime/fast_travel_pad.cpp): 1-3_Helipad (props/prop_sbhelipad_cherrytree)
// nests SBFastTravel (props/prop_fasttravelzone, kind 227) and the Helicopter (Veh_Police_Chopper). Provenance of the recorded values: the RPC
// reads of request r40-B-1 from the user's state m13-5-helipad-built-menu (docs/re/fast-travel.md): the player stands on the pad at 906.5608 31.4455
// 1138.9427 with ctrl+0x8 = ctrl+0xa = 24576, and the approach of state 0x180 moves and turns him by nothing. Zone: the class bound UseBound of
// levels/props/prop_fasttravelzone.blvl (centre 0 1 0.0253, half 1 1.5 1; C++ loader). Clip lengths: models/veh_police_chopper/*.bfnanm.
// Needs the extracted RomFS; exits with 77 without it.
#include "assets/assets.hpp"
#include "runtime/fast_travel_pad.hpp"
#include "runtime/level_start.hpp"
#include "runtime/super_buildit.hpp"
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

bool near(float a, float b, float tolerance) { return std::fabs(a - b) <= tolerance; }
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: fast-travel-pad-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "fast-travel-pad-real-test: data not available, skipped\n";
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

        runtime::FastTravelPads pads;
        pads.load(level, classOf);
        std::size_t index = pads.pads().size();
        for (std::size_t i = 0; i < pads.pads().size(); ++i)
            if (pads.pads()[i].instance == "1-3_helipad.sbfasttravel") index = i;
        require(index < pads.pads().size(), "1-3_Helipad.SBFastTravel is a fast travel pad (kind 227)");
        const runtime::FastTravelPad& pad = pads.pads()[index];
        require(pad.holder == "1-3_helipad", "holder " + pad.holder);
        require(near(pad.world[12], 906.5608f, 0.01f) && near(pad.world[13], 31.4455f, 0.01f) && near(pad.world[14], 1138.9427f, 0.01f),
                "the pad stands where the oracle's player stands in m13-5");
        require(runtime::FastTravelPads::heading(pad.world) == 24576, "pad heading " + std::to_string(runtime::FastTravelPads::heading(pad.world)) +
                                                                            ", the oracle's ctrl+0x8 and ctrl+0xa are 24576");
        require(pad.hasZone && near(pad.zoneCentre[1], 1.0f, 1e-4f) && near(pad.zoneCentre[2], 0.0253f, 1e-4f) && near(pad.zoneHalf[0], 1.0f, 1e-4f) &&
                    near(pad.zoneHalf[1], 1.5f, 1e-4f) && near(pad.zoneHalf[2], 1.0f, 1e-4f),
                "UseBound of the class file");
        const float player[3] = {906.5608f, 31.4455f, 1138.9427f};
        require(runtime::FastTravelPads::inZone(pad, player), "the m13-5 position is inside the zone");
        require(pad.helicopter == "1-3_helipad.helicopter" && pad.helicopterMesh == "Veh_Police_Chopper", "Helicopter " + pad.helicopter);
        require(near(pad.helicopterWorld[12], 902.318f, 0.01f) && near(pad.helicopterWorld[13], 30.6955f + 0.6592f, 0.01f) &&
                    near(pad.helicopterWorld[14], 1143.185f, 0.01f),
                "the Helicopter sits 0.6592 above the holder");

        const std::optional<runtime::FastTravelHelicopterModel> model = runtime::loadFastTravelHelicopter(archive, pad.helicopterMesh);
        require(model.has_value(), "skeleton and the clips land, idle, takeoff");
        require(model->clips[0].length == 42 && model->clips[1].length == 12 && model->clips[2].length == 48, "clip lengths");
        require(model->clips[0].rate == 15 && model->clips[1].rate == 15 && model->clips[2].rate == 15, "15 frames per second");
        pads.setHelicopterClips(index, model->clips);

        assets::PlacedObjectState state = runtime::levelInitialState(level, classOf);
        runtime::SuperBuilditSet superBuildits;
        superBuildits.load(archive, level, classOf);
        runtime::applySuperBuilditLink(superBuildits, state);
        runtime::FastTravelInput input;
        input.dt = 1.0f / 30.0f;
        std::copy_n(player, 3, input.player);
        input.facing = 24576;
        input.playerFree = true;
        input.actionPressed = true;
        runtime::FastTravelPlayer step;
        pads.update(state, input, step);
        input.actionPressed = false;
        pads.update(state, input, step);
        require(pads.playerState() == 0, "the pad is disabled until the helipad is built (the super build-it's UseObject)");
        state.disabled.erase("1-3_helipad.sbfasttravel");
        input.actionPressed = true;
        pads.update(state, input, step);
        input.actionPressed = false;
        pads.update(state, input, step);
        require(pads.playerState() == runtime::kPlayerStateFastTravelEnter, "A on the built pad enters 0x180");
        require(step.facing == 24576 && near(step.position[0], player[0], 1e-3f) && near(step.position[2], player[2], 1e-3f),
                "the approach leaves the player where he stands, as in the oracle");
        require(!runtime::fastTravelHelicopterPose(*model, pads.helicopter(index), pads.clock()).empty(), "the landing Helicopter has a pose");
        std::cout << "fast-travel-pad-real-test: ok\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "fast-travel-pad-real-test: " << error.what() << '\n';
        return 1;
    }
}
