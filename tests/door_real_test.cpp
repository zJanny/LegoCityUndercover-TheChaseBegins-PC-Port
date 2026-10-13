// Door of lvl_01_cherrytreehills (Police_Door_02) against the record and the pose matrices read from the original at gameplay-start
// (Azahar oracle, scenario with `poke` of the target state, docs/re/doors.md). Needs the extracted RomFS; exits with 77 when missing.
#include "assets/assets.hpp"
#include "assets/object_collision.hpp"
#include "runtime/door.hpp"
#include "runtime/level_start.hpp"
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace chase;
using namespace chase::runtime;

namespace {
constexpr const char* kLevelFile = "levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl";

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

void requirePose(const DoorPose& pose, const float expected[16], const std::string& what) {
    for (int k = 0; k < 16; ++k)
        require(std::fabs(pose.node[0][std::size_t(k)] - expected[k]) < 2e-6f,
                what + ": element " + std::to_string(k) + " is " + std::to_string(pose.node[0][std::size_t(k)]) + ", recorded " + std::to_string(expected[k]));
}
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: door-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "door-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        const assets::FuseEntry* entry = archive.find(kLevelFile);
        require(entry != nullptr, "level not in archive");
        const assets::Level level = assets::parseLevel(archive.read(*entry));
        const assets::ClassLookup classOf = assets::archiveClassLookup(archive);
        const assets::PlacedObjectState state = levelStartState(level, classOf);
        const assets::ObjectCollision* box = nullptr;
        const auto collisions = assets::objectCollisions(archive, level, state);
        for (const auto& item : collisions)
            if (item.instance == "Police_Door_02") box = &item;
        require(box != nullptr, "Police_Door_02 has no collision");
        const assets::LevelObject* object = nullptr;
        for (const auto& candidate : level.objects)
            if (candidate.instance == "Police_Door_02") object = &candidate;
        require(object != nullptr, "Police_Door_02 not in the level");
        const assets::FuseEntry* modelEntry = archive.find(box->model);
        require(modelEntry != nullptr, "door model not in the archive");
        const assets::Model model = assets::parseModel(archive.read(*modelEntry));
        const DoorParams params = doorParams(*object, *classOf(object->className), model, box->centre, box->half);

        // Record read at gameplay-start (entity 0x08dbcb90, record 0x08dbd220): +0xd = 22, +0xe = 0, +0xf = 0, +0x14 = -pi/2, +0x2c = 605, +0x2e = 601.
        require(params.openFrames == 22 && params.autoCloseFrames == 0 && !params.autoOpen && params.doorType == 0 && !params.twoLeaves, "door record changed");
        require(std::fabs(params.restAngle + 1.5707964f) < 1e-6f && params.sounds.opening == 605 && params.sounds.closing == 601, "door angle or sounds changed");
        require(model.nodePivot.size() == 1 && std::fabs(model.nodePivot[0][0] - 0.815166f) < 1e-6f, "pivot changed");
        const float pivots[2][3] = {{model.nodePivot[0][0], model.nodePivot[0][1], model.nodePivot[0][2]}, {0, 0, 0}};

        // 1-8_Building_Door: record read at gameplay-start (record 0x08be5ed0): +0xd = 22, +0xe = 0x2c, +0xf = 1. Its autoclosetime of 10 s is 300 frames,
        // which the byte cut to 44.
        for (const auto& item : collisions) {
            if (item.instance != "1-8_Building_Door") continue;
            const assets::LevelObject* building = nullptr;
            for (const auto& candidate : level.objects)
                if (candidate.instance == item.instance) building = &candidate;
            require(building != nullptr, "building door not in the level");
            const assets::Model buildingModel = assets::parseModel(archive.read(*archive.find(item.model)));
            const DoorParams buildingParams = doorParams(*building, *classOf(building->className), buildingModel, item.centre, item.half);
            require(buildingParams.openFrames == 22 && buildingParams.autoCloseFrames == 44 && buildingParams.twoLeaves, "building door record changed");
        }

        // Recorded node pose after each update: scenario `poke` target 2, `frames 1`, then 28 traced frames (door02open); the same for the
        // close request (door02close). Values are the first matrix of the entity's pose array (node+0x110).
        static const float opening1[16] = {0.9974521f, 0, 0.0713392f, 0, 0, 1, 0, 0, -0.0713392f, 0, 0.9974521f, 0, 0.0020769f, 0, -0.0581533f, 1};
        static const float opening11[16] = {0.7071067f, 0, 0.7071068f, 0, 0, 1, 0, 0, -0.7071068f, 0, 0.7071067f, 0, 0.2387567f, 0, -0.5764093f, 1};
        static const float opened[16] = {0, 0, 1, 0, 0, 1, 0, 0, -1, 0, 0, 0, 0.815166f, 0, -0.8151659f, 1};
        static const float closing1[16] = {0.0713391f, 0, 0.9974521f, 0, 0, 1, 0, 0, -0.9974521f, 0, 0.0713391f, 0, 0.7570127f, 0, -0.813089f, 1};
        static const float closed[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

        DoorState door;
        std::vector<DoorSoundEvent> sounds;
        doorMessage(door, params, DoorMessage::Open);
        doorUpdate(door, params, false, sounds);
        require(door.state == 2 && door.timer == 0, "first update after the request");
        doorUpdate(door, params, false, sounds);
        require(door.state == 2 && door.timer == 1, "opening timer 1");
        requirePose(doorPose(door, params, pivots), opening1, "opening 1");
        for (int i = 0; i < 10; ++i) doorUpdate(door, params, false, sounds);
        require(door.state == 2 && door.timer == 11, "opening timer 11");
        requirePose(doorPose(door, params, pivots), opening11, "opening 11");
        for (int i = 0; i < 11; ++i) doorUpdate(door, params, false, sounds);
        require(door.state == 2 && door.timer == 22, "opening timer 22");
        requirePose(doorPose(door, params, pivots), opened, "opening 22");
        doorUpdate(door, params, false, sounds);
        require(door.state == 3 && door.target == 3 && door.timer == 0, "open");
        requirePose(doorPose(door, params, pivots), opened, "open");

        doorMessage(door, params, DoorMessage::Close);
        doorUpdate(door, params, false, sounds);
        require(door.state == 4 && door.timer == 0, "closing starts");
        doorUpdate(door, params, false, sounds);
        require(door.state == 4 && door.timer == 1, "closing timer 1");
        requirePose(doorPose(door, params, pivots), closing1, "closing 1");
        for (int i = 0; i < 20; ++i) doorUpdate(door, params, false, sounds);
        require(door.state == 4 && door.timer == 21, "closing timer 21");
        doorUpdate(door, params, false, sounds);
        require(door.state == 4 && door.timer == 22, "closing timer 22");
        requirePose(doorPose(door, params, pivots), closed, "closing 22");
        doorUpdate(door, params, false, sounds);
        require(door.state == 1 && door.target == 1 && door.timer == 22 && !door.openFlag, "closed again with the timer at 22");
        requirePose(doorPose(door, params, pivots), closed, "closed");
    } catch (const std::exception& error) {
        std::cerr << "door-real-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "door-real-test: ok\n";
    return 0;
}
