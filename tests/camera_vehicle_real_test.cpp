// Replay of the follow camera while the player drives the police car of 1-1_Police_Car (Azahar oracle, New 3DS, state
// drv-police-car-seated; tools/oracle/camera-vehicle-drive.txt, converted by tools/export_camera_vehicle_trace.py; docs/re/camera-vehicle.md).
// The camera starts from the recorded state of the first row and gets the recorded vehicle position and heading and the pad of every
// update: idle (A not held), A held, circle pad x 120, released. The eye tracker (K+0x34) and the look point (K+0x94 + K+0xf4) must
// equal the recorded ones. The rows of every trace are committed in tests/data/camera/vehicle (all rows of each trace; recorded
// 2026-10-15). Needs the extracted RomFS; exits with 77 (skipped) when it is missing.
#include "assets/assets.hpp"
#include "assets/object_collision.hpp"
#include "runtime/collision.hpp"
#include "runtime/follow_camera.hpp"
#include "runtime/level_start.hpp"
#include "runtime/pad.hpp"
#include "runtime/vehicle_camera.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace chase;
using namespace chase::runtime;
namespace fs = std::filesystem;

namespace {
// The sweep state the camera object keeps (previous near limit, timers) is not restored: the first updates differ and converge.
constexpr std::size_t kSettlingRows = 12;
// The recorded camera had just been pushed by a vertical push (S+0x24 = 0.5) and one moving update had passed (the value also seen in the street, 0.467).
constexpr float kLeaveSpringHold = 0.467f;
// After the release director camera of mission 1-1 (CAMERA-15): seed rows 297 to 304, the whole track from row 297 and the settled camera from row 340.
constexpr std::size_t kHandBackSeedFirstRow = 297, kHandBackSeedRows = 8, kHandBackSettledRow = 340;
// Limits pinned at the measured errors of the replica (CAMERA-15): the reset takes the ray test branch because StartDCam (0x194fd8) stores 0 in
// K+0x178, the wall of Collision_Player__35 13.24 m behind the car pulls the seed in, and in the first update the tracker jumps to the wanted
// position because that sees further before the first obstruction (near plane test 0x2e0e28). Measured: seed rows 2.4e-4, whole track eye 2.9e-3
// (the swing round the car from row 307), look 1.1e-4, settled horizontal 1.2e-4, height 1.6e-3.
// Provenance: this test, handback.csv from tools/oracle/camera-vehicle-handback.txt.
constexpr float kHandBackSeedLimit = 5e-4f, kHandBackTrackEyeLimit = 5e-3f, kHandBackTrackLookLimit = 3e-4f, kHandBackSettledHeightLimit = 3e-3f;
constexpr float kHandBackSettledHorizontalLimit = 2e-3f;
// Near plane of the render camera (*0x377544 + 0xc4, written by 0x23f168) after the hand-over, rows 297 to 309. Provenance: Azahar memtrace
// camq-near of tools/oracle/camera-vehicle-handback-near.txt (frames 503 to 515); the replica's error is 2.4e-4.
constexpr float kHandBackNearPlanes[] = {0.722414f, 0.563713f, 0.419943f, 0.303219f, 0.213484f, 0.2f, 0.2f, 0.2f, 0.2f, 0.2f, 1.0f, 1.0f, 1.0f};
constexpr float kHandBackNearLimit = 1e-3f;
constexpr const char* kPoliceCar = "1-1_police_car";
constexpr std::uint16_t kDrivingState = 0x11c;
constexpr std::uint8_t kVehicleObjectType = 183;

struct Row {
    float dt, vehicle[3], heading, wanted[3], eye[3], look[3], fov = 0;
    float player[3] = {0, 0, 0};
    unsigned state = 0;
    bool followOnTop = true;
};

std::vector<Row> loadTrace(const fs::path& path) {
    std::ifstream csv(path);
    if (!csv) throw std::runtime_error("cannot open " + path.string());
    std::string line;
    std::getline(csv, line);
    std::vector<Row> rows;
    while (std::getline(csv, line)) {
        std::vector<std::string> cells;
        std::stringstream stream(line);
        std::string cell;
        while (std::getline(stream, cell, ',')) cells.push_back(cell);
        if (cells.size() < 25) throw std::runtime_error("short row in " + path.string());
        Row row;
        row.dt = std::stof(cells[1]);
        for (int a = 0; a < 3; ++a) {
            row.vehicle[a] = std::stof(cells[std::size_t(2 + a)]);
            row.wanted[a] = std::stof(cells[std::size_t(6 + a)]);
            row.eye[a] = std::stof(cells[std::size_t(9 + a)]);
            row.look[a] = std::stof(cells[std::size_t(12 + a)]);
        }
        row.heading = std::stof(cells[5]);
        row.fov = std::stof(cells[21]);
        row.followOnTop = cells[22] == "002a8bfc";
        if (cells.size() >= 29) {
            for (int a = 0; a < 3; ++a) row.player[a] = std::stof(cells[std::size_t(25 + a)]);
            row.state = unsigned(std::stoul(cells[28]));
        }
        rows.push_back(row);
    }
    return rows;
}

struct Phase {
    const char* file;
    std::size_t rows;
    int stickX;
};

float distance(const float a[3], const float b[3]) {
    return std::sqrt((a[0] - b[0]) * (a[0] - b[0]) + (a[1] - b[1]) * (a[1] - b[1]) + (a[2] - b[2]) * (a[2] - b[2]));
}
}

int main(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "usage: camera-vehicle-real-test FIB NAMES DATA_DIR\n";
        return 2;
    }
    if (!fs::exists(argv[1]) || !fs::exists(argv[2])) {
        std::cout << "camera-vehicle-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        const char* levelPath = "levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl";
        CollisionWorld world;
        for (const auto& item : assets::parseModel(archive.read(*archive.find("models/lvl_01_cherrytreehills/lvl_01_cherrytreehills.bfnmdl"))).collision)
            world.add(item);
        const assets::Level level = assets::parseLevel(archive.read(*archive.find(levelPath)));
        const assets::PlacedObjectState startState = levelStartState(level, assets::archiveClassLookup(archive));
        for (const auto& object : assets::objectCollisions(archive, level, startState)) world.addObject(object);

        // Provenance: record 0x08a884e0 of 1-1_Police_Car read in the oracle: +0x311 = 1 (ATTR_CameraOverride), +0x2ec = 22,
        // camera floats -0.2, 5.0, 0.4, 0.5 (docs/re/vehicles-system.md).
        VehicleRecord record;
        record.flags311 = kVehicleCameraOverride;
        record.vehicleType = 22;
        record.cameraPosY = -0.2f;
        record.cameraPosZ = 5.0f;
        record.cameraLookY = 0.4f;
        record.cameraLookZ = 0.5f;
        const assets::Model model =
            assets::parseModel(archive.read(*archive.find("models/veh_police_trooper/veh_police_trooper_atlas_lvl_01_cherrytreehills_main.bfnmdl")));
        const VehicleCameraSubject subject = vehicleCameraSubject(model, record);
        // Oracle: entity +0x64 = 0.0801 and +0x70 = 0.826, node +0xb0 = 0x3f7e012e.
        if (std::fabs(subject.bodyHeight - 0.9061f) > 1e-3f || std::fabs(subject.modelHalfHeight - 0.99219f) > 1e-3f)
            throw std::runtime_error("body height " + std::to_string(subject.bodyHeight) + " or model half height " +
                                     std::to_string(subject.modelHalfHeight));

        const Phase phases[] = {{"veh_idle.csv", 30, 0}, {"veh_accel.csv", 60, 0}, {"veh_turn.csv", 60, 120}, {"veh_coast.csv", 40, 0}};
        FollowCamera camera;
        camera.setCollision(&world);
        camera.setExcludedOwner(kPoliceCar);
        camera.setSubject(subject.preset, subject.bodyHeight, subject.modelHalfHeight);
        bool started = false;
        float worstEye = 0, worstLook = 0;
        std::string worstFile;
        std::size_t worstRow = 0;
        PadDevice pad;
        for (const Phase& phase : phases) {
            const std::vector<Row> rows = loadTrace(fs::path(argv[3]) / "vehicle" / phase.file);
            if (rows.size() != phase.rows) throw std::runtime_error(std::string("row count of ") + phase.file);
            for (std::size_t i = 0; i < rows.size(); ++i) {
                const Row& row = rows[i];
                if (!started) {
                    camera.restore(row.wanted, row.eye, row.look, row.vehicle, row.heading, 1.0f, 0.5f);
                    started = true;
                    continue;
                }
                PadState state;
                state.stickX = static_cast<std::int16_t>(phase.stickX);
                pad.poll(state);
                FollowCameraInput input;
                input.dt = row.dt;
                for (int a = 0; a < 3; ++a) input.target[a] = row.vehicle[a];
                input.targetFacing = row.heading;
                input.stickTurn = stickTurnInput(pad.slot(14).value, kDrivingState, kVehicleObjectType);
                input.shoulderTurn = shoulderTurnInput(pad.slot(12).value, pad.slot(13).value, false);
                camera.update(input);
                const float eyeError = distance(camera.eye(), row.eye), lookError = distance(camera.lookAt(), row.look);
                if (&phase == &phases[0] && i < kSettlingRows) continue;
                if (eyeError > worstEye) worstEye = eyeError, worstFile = phase.file, worstRow = i;
                worstLook = std::max(worstLook, lookError);
            }
        }
        {
            const VehicleCameraSubject onFoot = onFootCameraSubject();
            FollowCamera walking;
            walking.setCollision(&world);
            walking.setExcludedOwner("");
            walking.setSubject(onFoot.preset, onFoot.bodyHeight, onFoot.modelHalfHeight);
            std::vector<Row> rows;
            for (const char* file : {"enter_rest.csv", "enter_press.csv", "enter_jump.csv"}) {
                const std::vector<Row> part = loadTrace(fs::path(argv[3]) / "vehicle" / file);
                rows.insert(rows.end(), part.begin(), part.end());
            }
            walking.restore(rows[0].wanted, rows[0].eye, rows[0].look, rows[0].player, 0.0f);
            bool vehicleSubject = false;
            float worstEnterEye = 0, worstEnterLook = 0;
            std::size_t compared = 0;
            for (std::size_t i = 1; i < rows.size() && rows[i].followOnTop; ++i) {
                const Row& row = rows[i];
                const bool driving = row.state == kDrivingState;
                if (driving != vehicleSubject) {
                    vehicleSubject = driving;
                    walking.setExcludedOwner(driving ? kPoliceCar : "");
                    walking.setSubject(driving ? subject.preset : onFoot.preset, driving ? subject.bodyHeight : onFoot.bodyHeight,
                                       driving ? subject.modelHalfHeight : onFoot.modelHalfHeight);
                }
                FollowCameraInput input;
                input.dt = row.dt;
                for (int a = 0; a < 3; ++a) input.target[a] = driving ? row.vehicle[a] : row.player[a];
                input.targetFacing = driving ? row.heading : 0.0f;
                walking.update(input);
                worstEnterEye = std::max(worstEnterEye, distance(walking.eye(), row.eye));
                worstEnterLook = std::max(worstEnterLook, distance(walking.lookAt(), row.look));
                ++compared;
            }
            std::cout << "entering: " << compared << " updates, worst eye error " << worstEnterEye << ", look " << worstEnterLook << '\n';
            if (compared < 20 || worstEnterEye > 5e-3f || worstEnterLook > 5e-3f)
                throw std::runtime_error("entering differs from the recorded camera");
        }
        {
            const VehicleCameraSubject onFoot = onFootCameraSubject();
            FollowCamera leaving;
            leaving.setCollision(&world);
            leaving.setExcludedOwner(kPoliceCar);
            leaving.setSubject(subject.preset, subject.bodyHeight, subject.modelHalfHeight);
            std::vector<Row> rows;
            for (const char* file : {"leave_press.csv", "leave_out.csv"}) {
                const std::vector<Row> part = loadTrace(fs::path(argv[3]) / "vehicle" / file);
                rows.insert(rows.end(), part.begin(), part.end());
            }
            leaving.restore(rows[0].wanted, rows[0].eye, rows[0].look, rows[0].vehicle, rows[0].heading, 1.0f, kLeaveSpringHold);
            leaving.setExcludedOwner("");
            leaving.setSubject(onFoot.preset, onFoot.bodyHeight, onFoot.modelHalfHeight);
            float worstLeaveEye = 0, worstLeaveLook = 0;
            std::size_t worstLeaveRow = 0;
            for (std::size_t i = 1; i < rows.size(); ++i) {
                const Row& row = rows[i];
                FollowCameraInput input;
                input.dt = row.dt;
                for (int a = 0; a < 3; ++a) input.target[a] = row.player[a];
                leaving.update(input);
                const float eyeError = distance(leaving.eye(), row.eye);
                if (eyeError > worstLeaveEye) worstLeaveEye = eyeError, worstLeaveRow = i;
                worstLeaveLook = std::max(worstLeaveLook, distance(leaving.lookAt(), row.look));
            }
            std::cout << "getting out: worst eye error " << worstLeaveEye << " (row " << worstLeaveRow << "), look " << worstLeaveLook << '\n';
            if (worstLeaveEye > 5e-3f || worstLeaveLook > 5e-3f) throw std::runtime_error("getting out differs from the recorded camera");
        }
        {
            const VehicleCameraSubject onFoot = onFootCameraSubject();
            const std::vector<Row> rows = loadTrace(fs::path(argv[3]) / "vehicle" / "handback.csv");
            FollowCamera returning;
            returning.setCollision(&world);
            returning.setSubject(onFoot.preset, onFoot.bodyHeight, onFoot.modelHalfHeight);
            returning.restore(rows[0].wanted, rows[0].eye, rows[0].look, rows[0].player, 0.0f);
            returning.clearStoredHeading();  // StartDCam of the release script at frame 228 (oracle watchpoint and trap at 0x195038)
            bool vehicleSubject = false, wasOnTop = true;
            float seedError = 0, settledError = 0, trackEye = 0, trackLook = 0, settledHeight = 0, nearError = 0;
            std::size_t compared = 0;
            for (std::size_t i = 1; i < rows.size(); ++i) {
                const Row& row = rows[i];
                const bool driving = row.state == kDrivingState;
                if (driving != vehicleSubject) {
                    vehicleSubject = driving;
                    returning.setExcludedOwner(driving ? kPoliceCar : "");
                    returning.setSubject(driving ? subject.preset : onFoot.preset, driving ? subject.bodyHeight : onFoot.bodyHeight,
                                         driving ? subject.modelHalfHeight : onFoot.modelHalfHeight);
                }
                FollowCameraInput input;
                input.dt = row.dt;
                for (int a = 0; a < 3; ++a) input.target[a] = driving ? row.vehicle[a] : row.player[a];
                input.targetFacing = driving ? row.heading : 0.0f;
                // The near plane test reads the projection the previous update left (camera +0xbc / +0xc0 from the blended FOV).
                float xScale, yScale;
                FollowCamera::projectionScales(rows[i - 1].fov, xScale, yScale);
                returning.setProjectionScales(xScale, yScale);
                if (!row.followOnTop) {
                    wasOnTop = false;
                    continue;
                }
                if (!wasOnTop) {
                    // 0x1b4864 after the director camera: the top command is not a record pose, so no near rule (0x2a8c4c).
                    returning.takeOver(input.target, input.targetFacing, row.eye, false);
                    returning.update(input);
                    wasOnTop = true;
                }
                returning.update(input);
                if (i >= kHandBackSeedFirstRow) {
                    trackEye = std::max(trackEye, distance(returning.eye(), row.eye));
                    trackLook = std::max(trackLook, distance(returning.lookAt(), row.look));
                }
                const std::size_t nearRow = i - kHandBackSeedFirstRow;
                if (i >= kHandBackSeedFirstRow && nearRow < std::size(kHandBackNearPlanes))
                    nearError = std::max(nearError, std::fabs(returning.nearPlane() - kHandBackNearPlanes[nearRow]));
                if (i < kHandBackSeedFirstRow + kHandBackSeedRows) {
                    seedError = std::max(seedError, distance(returning.eye(), row.eye));
                } else if (i >= kHandBackSettledRow) {
                    const float dx = returning.eye()[0] - row.eye[0], dz = returning.eye()[2] - row.eye[2];
                    settledError = std::max(settledError, std::sqrt(dx * dx + dz * dz));
                    settledHeight = std::max(settledHeight, std::fabs(returning.eye()[1] - row.eye[1]));
                    ++compared;
                }
            }
            std::cout << "after the release director camera: seed " << seedError << ", whole track eye " << trackEye << " look " << trackLook
                      << ", horizontal error when settled " << settledError << ", height " << settledHeight << ", near plane " << nearError << '\n';
            if (compared < 30 || seedError > kHandBackSeedLimit || trackEye > kHandBackTrackEyeLimit || trackLook > kHandBackTrackLookLimit ||
                settledError > kHandBackSettledHorizontalLimit || settledHeight > kHandBackSettledHeightLimit || nearError > kHandBackNearLimit)
                throw std::runtime_error("hand-back differs from the recorded camera");
        }
        std::cout << "vehicle camera: worst eye error " << worstEye << " (" << worstFile << " row " << worstRow << "), look " << worstLook << '\n';
        if (worstEye > 5e-3f || worstLook > 5e-3f) throw std::runtime_error("replay differs from the recorded camera");
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
