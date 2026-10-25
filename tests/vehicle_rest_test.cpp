#include "assets/assets.hpp"
#include "assets/object_collision.hpp"
#include "runtime/vehicle_drive.hpp"
#include "runtime/vehicle_world.hpp"

#include <cmath>
#include <cstdio>
#include <filesystem>

using namespace chase;
using namespace chase::runtime;

namespace {
int failures = 0;
void expect(bool ok, const char* what, double got) {
    if (!ok) {
        std::printf("FAIL: %s (got %.6f)\n", what, got);
        ++failures;
    }
}
void near(double got, double want, double tolerance, const char* what) { expect(std::fabs(got - want) <= tolerance, what, got); }
}

int main(int argc, char** argv) {
    if (argc != 3 || !std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) return 77;
    assets::FuseArchive archive(argv[1]);
    archive.loadNames(argv[2]);
    const assets::Model model = assets::parseModel(archive.read(*archive.find("models/lvl_01_cherrytreehills/lvl_01_cherrytreehills.bfnmdl")));
    const assets::Level level = assets::parseLevel(archive.read(*archive.find("levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl")));
    const assets::ClassLookup classOf = assets::archiveClassLookup(archive);
    VehicleSet set;
    set.load(level, classOf, parseVehicleTable(assets::parseTable(archive.read(*archive.find("tables/tablescharacters.bxls")))));
    const auto placed = assets::objectPlacements(level, classOf);
    CollisionWorld world;
    const assets::CollisionItem* floorItem = nullptr;
    for (const auto& item : model.collision) {
        world.add(item);
        if (item.floor) floorItem = &item;
    }
    const std::vector<BulletBox> boxes = levelVehicleBoxes(model);
    const WheelRay ray = [&world](const Vector3& from, const Vector3& to) { return castWheelRay(world, from, to); };
    // Rigid +0x34 of the sleeping bodies: agent A round 14, Azahar RPC heap reads two seconds after loadstate without input. FP_Vehicle_03, 04 and 05:
    // save state outside the police station after mission 1-1; the police car: the simulated body (flags 0x102, mode 2) of save state in front of the police car of mission 1-1. FP_Vehicle_03 also equals
    // trace hitP4 update 26 (agent VEH7).
    struct Rest {
        const char* record;
        const char* instance;
        Vector3 position;
    };
    const Rest rests[] = {{"fp_vehicle_03", "FP_Vehicle_03", {943.5024f, 17.3799f, 1116.9213f}},
                          {"fp_vehicle_04", "FP_Vehicle_04", {975.5808f, 17.1212f, 1136.9209f}},
                          {"fp_vehicle_05", "FP_Vehicle_05", {958.2173f, 17.7667f, 1157.5811f}},
                          {"1-1_police_car", "1-1_Police_Car", {887.3775f, 17.0743f, 1117.3153f}}};
    for (const Rest& rest : rests)
    for (const auto& p : placed) {
        if (p.instance != rest.instance) continue;
        const VehicleRecord* rec = set.find(rest.record);
        Vector3 centre, half;
        assets::modelCollisionBox(archive, assets::meshModelPath(p.mesh), centre.data(), half.data());
        const auto matrix = assets::placementMatrix(p);
        const float heading = std::atan2(matrix[8], matrix[10]);
        const VehicleRecord scaled = vehicleRecordAtMassScale(*rec, 1.0f);
        const VehicleShapeBoxes shape = vehicleShapeBoxes(*rec, centre.data(), half.data());
        VehicleRigidBody body = vehicleRigidBody(*rec, {matrix[12], matrix[13], matrix[14]}, heading, shape.shapeLow, shape.shapeHigh, 1.0f);
        DrivetrainState drive = vehicleDrivetrain(scaled);
        VehicleBoxContacts contacts = vehicleBoxContacts(shape.collisionLow, shape.collisionHigh, &boxes);
        VehicleFloor floor{FloorMesh(*floorItem), FloorTracker{}, vehicleCollisionBody(scaled, shape.collisionLow, shape.collisionHigh, 1.0f), &contacts};
        bool awake = true;
        BodyInputs inputs = bodyInputs(commandForState(rec->requestedState, DriveInput{}, false, 0.0f));
        inputs.driver = false;
        std::vector<VehicleWorldBody> bodies(1);
        bodies[0] = {0, &body, &drive, &awake, &scaled, inputs, &floor, nullptr};
        VehiclePairManifolds pairs;
        int steps = 0;
        while (awake && steps < 120) {
            stepVehicleBodies(bodies, pairs, ray, 1);
            ++steps;
        }
        expect(!awake, rest.instance, steps);
        for (std::size_t a = 0; a < 3; ++a) near(body.position[a], rest.position[a], 1e-3, rest.instance);
    }
    if (failures != 0) return 1;
    std::printf("vehicle rest test passed\n");
    return 0;
}
