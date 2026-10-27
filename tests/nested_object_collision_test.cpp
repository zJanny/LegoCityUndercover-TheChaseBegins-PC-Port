// Collision of the objects a class file nests, and of the super build-it plate (kind 98): the helipad of mission 1-3 of lvl_01_cherrytreehills
// (`1-3_Helipad`, kind 253, with the nested plate `SBPlate` kind 98, the built prop `SBHelipadProp` kind 67 and the fast travel pad `SBFastTravel`
// kind 227). Rules: docs/re/collision.md (link 0x1af224 -> 0x23aff8 with automatic box and no margin, then 0x2337e0; the holder's create 0x29c964 and
// the pad register nothing). Pinned values: the oracle's grid entries of the plate (kind 0, mask 6, box centre (0, 0.106, 0), half (6.096, 0.106, 6.096),
// owner flags 0x780 before the build and 0x781 after) and of the built prop (kind 1, mask 6, owner box centre (0, 1.906, 0), half (7.622, 1.906, 7.622),
// flags 0x1709 before and 0x1708 after), request 20261027-graphics-G-1, recorded from two save states of mission 1-3 on 2026-10-27 (docs/re/collision.md,
// entry 2026-10-27T19:14:39); the same boxes and the node placement (holder at 902.318 30.6955 1143.185, yaw 45) read with the C++ loaders
// (20261027-graphics-G, 2026-10-27). Needs the extracted RomFS; exits with 77 (skipped) when it is missing.
#include "assets/assets.hpp"
#include "assets/object_collision.hpp"
#include "runtime/collision.hpp"
#include "runtime/collision_enable.hpp"
#include "runtime/vehicle_wheels.hpp"

#include <cmath>
#include <filesystem>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>

namespace {
using namespace chase;

constexpr const char* kLevelFile = "levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl";

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

bool near(float a, float b, float tolerance = 2e-3f) { return std::fabs(a - b) <= tolerance; }

const assets::ObjectCollision* find(const std::vector<assets::ObjectCollision>& list, const std::string& instance) {
    for (const assets::ObjectCollision& entry : list)
        if (entry.instance == instance) return &entry;
    return nullptr;
}

std::size_t hits(const runtime::CollisionWorld& world, const float low[3], const float high[3]) {
    std::size_t count = 0;
    world.query(low, high, runtime::kPlayerLayer, [&](const runtime::CollisionWorld::Triangle&) { ++count; });
    return count;
}
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: nested-object-collision-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "nested-object-collision-test: data not available, skipped\n";
        return 77;
    }
    try {
        assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        const assets::Level level = assets::parseLevel(archive.read(*archive.find(kLevelFile)));
        const std::vector<assets::ObjectCollision> placed = assets::objectCollisionCandidates(archive, level);
        const std::vector<assets::ObjectCollision> nested = assets::nestedObjectCollisionCandidates(archive, level);

        require(find(placed, "1-3_Helipad") == nullptr, "the holder (kind 253) registers nothing");
        for (const char* child : {"SBFastTravel", "Helicopter", "SBHelipadNewPlayerPos", "Veh_Exclusion", "SBBuildCompleteScript"})
            require(find(nested, std::string("1-3_Helipad.") + child) == nullptr, std::string(child) + " registers nothing");

        const assets::ObjectCollision* plate = find(nested, "1-3_Helipad.SBPlate");
        require(plate != nullptr, "the plate registers");
        require(plate->mask == 6 && !plate->mesh && plate->collisionDefault, "the plate is a dynamic box with collision on");
        require(near(plate->centre[0], 0.0f) && near(plate->centre[1], 0.105528f) && near(plate->centre[2], 0.0f), "plate box centre");
        require(near(plate->half[0], 6.096f) && near(plate->half[1], 0.105528f) && near(plate->half[2], 6.096f), "plate box half extents");
        require(near(plate->matrix[12], 902.318f) && near(plate->matrix[13], 30.6955f) && near(plate->matrix[14], 1143.185f, 6e-3f), "plate node position");
        require(near(plate->matrix[0], 0.70711f) && near(plate->matrix[2], -0.70711f), "the plate turns with the holder (yaw 45)");

        const assets::ObjectCollision* prop = find(nested, "1-3_Helipad.SBHelipadProp");
        require(prop != nullptr, "the built prop registers");
        require(prop->mask == 6 && prop->mesh, "the built prop registers the model's collision item");
        require(near(prop->centre[1], 1.90648f) && near(prop->half[0], 7.6217f) && near(prop->half[1], 1.90648f) && near(prop->half[2], 7.6217f),
                "the root node of the item is the entity box");

        runtime::CollisionWorld world;
        for (const assets::ObjectCollision& entry : nested)
            if (entry.instance.rfind("1-3_Helipad.", 0) == 0) world.addObject(entry);

        assets::PlacedObjectState state;
        state.disabled.insert("1-3_helipad.sbhelipadprop");
        state.disabled.insert("1-3_helipad.sbfasttravel");
        runtime::applyObjectCollisionState(world, state);
        const float plateLow[3] = {902.2f, 30.75f, 1143.1f}, plateHigh[3] = {902.4f, 30.95f, 1143.3f};
        const float propLow[3] = {894.0f, 30.0f, 1135.0f}, propHigh[3] = {911.0f, 35.0f, 1152.0f};
        require(hits(world, plateLow, plateHigh) > 0, "before the build the plate stops the player");
        for (const runtime::CollisionWorld::Entry& entry : world.entries())
            require(entry.owner != "1-3_helipad.sbhelipadprop" || !entry.active, "before the build the hidden prop does not collide");

        const std::optional<runtime::WheelHit> wheel = runtime::castWheelRay(world, {902.3f, 31.5f, 1143.2f}, {902.3f, 30.5f, 1143.2f});
        require(wheel && near(wheel->point[1], 30.6955f + 2 * 0.105528f, 5e-3f), "a wheel ray lands on the top of the plate (the stand-in wheel ray queries the player layer)");

        state.disabled.insert("1-3_helipad.sbplate");
        state.disabled.erase("1-3_helipad.sbhelipadprop");
        runtime::applyObjectCollisionState(world, state);
        require(hits(world, propLow, propHigh) > 0, "after the build the prop stops the player");
        std::size_t plateEntries = 0;
        for (const runtime::CollisionWorld::Entry& entry : world.entries()) plateEntries += entry.owner == "1-3_helipad.sbplate" && entry.active;
        require(plateEntries == 0, "after the build the plate no longer collides");
    } catch (const std::exception& error) {
        std::cerr << "nested-object-collision: " << error.what() << '\n';
        return 1;
    }
    std::cout << "nested-object-collision: ok\n";
    return 0;
}
