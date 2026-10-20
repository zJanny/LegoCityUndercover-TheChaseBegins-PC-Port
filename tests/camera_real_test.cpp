// Differential test of the follow camera against traces recorded in the original game (Azahar oracle, state
// gameplay-start of Cherry Tree Hills, 2026-10-13; scenarios tools/oracle/camera-trace.txt and camwall-*.txt, converted by
// tools/export_camera_trace.py; docs/re/camera.md). The rows of every trace are committed in tests/data/camera/follow and
// tests/data/camera/wall (all rows of each trace). The wall traces also need the extracted RomFS (the level collision); the
// test exits with 77 (skipped) when it is missing.
#include "assets/assets.hpp"
#include "runtime/follow_camera.hpp"
#include "runtime/level_start.hpp"
#include "runtime/pad.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace chase::runtime;
namespace fs = std::filesystem;

namespace {
struct Row {
    float dt;
    int stickX, stickY;
    unsigned held;
    float player[3], facing;
    float look[3], eye[3], wanted[3];
    float clearance = 1.0f, springHold = 0.0f;
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
        if (cells.size() != 19 && cells.size() != 21) throw std::runtime_error("bad row in " + path.string());
        Row row;
        row.dt = std::stof(cells[1]);
        row.stickX = std::stoi(cells[2]);
        row.stickY = std::stoi(cells[3]);
        row.held = static_cast<unsigned>(std::stoul(cells[4]));
        for (int i = 0; i < 3; ++i) {
            row.player[i] = std::stof(cells[5 + i]);
            row.look[i] = std::stof(cells[10 + i]);
            row.eye[i] = std::stof(cells[13 + i]);
            row.wanted[i] = std::stof(cells[16 + i]);
        }
        row.facing = std::atan2(std::stof(cells[8]), std::stof(cells[9]));
        if (cells.size() == 21) {
            row.clearance = std::stof(cells[19]);
            row.springHold = std::stof(cells[20]);
        }
        rows.push_back(row);
    }
    return rows;
}

constexpr std::uint16_t kPlayerType = 3;
constexpr const char* kLevelFile = "levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl";
constexpr const char* kLevelModel = "models/lvl_01_cherrytreehills/lvl_01_cherrytreehills.bfnmdl";

struct Result {
    float worstEye = 0, worstLook = 0, worstYaw = 0, worstClearance = 0;
    std::size_t compared = 0, worstRow = 0;
};

// Restore the recorded state at row `from`, run the camera over rows from+1..to with the recorded player
// position and the pad state of the trace, and compare every row to the recording.
Result run(const std::vector<Row>& rows, std::size_t from, std::size_t to, const CollisionWorld* world = nullptr) {
    FollowCamera camera;
    camera.setCollision(world);
    camera.restore(rows[from].wanted, rows[from].eye, rows[from].look, rows[from].player, rows[from].facing, rows[from].clearance,
                   rows[from].springHold);
    PadDevice pad;
    Result result;
    for (std::size_t i = from + 1; i < std::min(to + 1, rows.size()); ++i) {
        const Row& row = rows[i];
        PadState state;
        state.held = row.held;
        state.stickX = static_cast<std::int16_t>(row.stickX);
        state.stickY = static_cast<std::int16_t>(row.stickY);
        pad.poll(state);
        FollowCameraInput input;
        input.dt = row.dt;
        for (int k = 0; k < 3; ++k) input.target[k] = row.player[k];
        input.targetFacing = row.facing;
        input.stickTurn = stickTurnInput(pad.slot(14).value, kPlayerType);
        input.shoulderTurn = shoulderTurnInput(pad.slot(12).value, pad.slot(13).value, false);
        camera.update(input);
        for (int k = 0; k < 3; ++k) {
            const float eyeError = std::fabs(camera.eye()[k] - row.eye[k]);
            if (eyeError > result.worstEye) result.worstRow = i;
            result.worstEye = std::max(result.worstEye, eyeError);
            result.worstLook = std::max(result.worstLook, std::fabs(camera.lookAt()[k] - row.look[k]));
        }
        result.worstClearance = std::max(result.worstClearance, std::fabs(camera.clearance() - row.clearance));
        const float recordedYaw = std::atan2(row.look[0] - row.eye[0], row.look[2] - row.eye[2]);
        float difference = std::fabs(camera.yaw() - recordedYaw);
        if (difference > 3.14159265f) difference = 6.2831853f - difference;
        result.worstYaw = std::max(result.worstYaw, difference);
        ++result.compared;
    }
    return result;
}

int failures = 0;
void check(const std::string& name, const Result& result, float eyeLimit, float yawLimit) {
    const bool ok = result.compared > 0 && result.worstEye <= eyeLimit && result.worstLook <= eyeLimit && result.worstYaw <= yawLimit && result.worstClearance <= 1e-3f;
    std::cout << (ok ? "ok   " : "FAIL ") << name << ": " << result.compared << " rows, eye " << result.worstEye << ", look "
              << result.worstLook << ", yaw " << result.worstYaw << ", clearance " << result.worstClearance << " (worst eye row " << result.worstRow << ")\n";
    if (!ok) ++failures;
}
}

int main(int argc, char** argv) {
    try {
        if (argc < 2 || !fs::exists(fs::path(argv[1]) / "follow" / "k_still.csv")) {
            std::cerr << "usage: camera-real-test DATA_DIR [FIB NAMES]\n";
            return 2;
        }
        const fs::path directory = fs::path(argv[1]) / "follow";
        // Tolerances: positions are float32 near 900 (one ulp 6e-5), the recorded L/R turn is exact to 1e-4.
        for (const char* name : {"k_still", "k_R", "k_Rrel", "k_L", "k_Lrel"}) {
            const auto rows = loadTrace(directory / (std::string(name) + ".csv"));
            check(name, run(rows, 0, rows.size()), 1e-3f, 1e-4f);
        }
        // The player walks towards the camera: the look point lags, the wanted position stays on the rope.
        {
            const auto rows = loadTrace(directory / "k_back.csv");
            check("k_back (before the player reaches the camera)", run(rows, 0, 28), 1e-3f, 1e-4f);
        }
        // Sideways run: exact once the push-out from nearby walls has ended (rows 17..59); the rows before
        // are shifted by up to 0.2 units by that push-out, which is not modelled.
        {
            const auto rows = loadTrace(directory / "k_left.csv");
            check("k_left (after the wall push-out)", run(rows, 17, rows.size()), 1e-3f, 1e-4f);
        }
        {
            const auto rows = loadTrace(directory / "k_rest.csv");
            check("k_rest", run(rows, 0, rows.size()), 1e-3f, 1e-4f);
        }
        {
            const auto rows = loadTrace(directory / "k_dt16.csv");
            check("k_dt16", run(rows, 0, rows.size()), 2e-3f, 2e-4f);
        }
        if (argc >= 4 && fs::exists(argv[2]) && fs::exists(argv[3])) {
            chase::assets::FuseArchive archive(argv[2]);
            archive.loadNames(argv[3]);
            const chase::assets::FuseEntry* entry = archive.find(kLevelModel);
            if (!entry) throw std::runtime_error(std::string("not in archive: ") + kLevelModel);
            CollisionWorld world;
            for (const auto& item : chase::assets::parseModel(archive.read(*entry)).collision) world.add(item);
            const chase::assets::FuseEntry* levelEntry = archive.find(kLevelFile);
            if (!levelEntry) throw std::runtime_error(std::string("not in archive: ") + kLevelFile);
            const chase::assets::Level level = chase::assets::parseLevel(archive.read(*levelEntry));
            const chase::assets::PlacedObjectState startState = chase::runtime::levelStartState(level, chase::assets::archiveClassLookup(archive));
            for (const auto& object : chase::assets::objectCollisions(archive, level, startState)) world.addObject(object);
            // Wall traces (tools/oracle/camwall-circle.txt, camwall-east.txt; CSV from tools/export_camera_trace.py --extra 0x330, recorded 2026-10-13).
            // Every row is replayed: level collision and the placed objects (lamp posts, doors, plaques).
            struct WallTrace {
                const char* name;
                std::size_t from, to;
            };
            const WallTrace walls[] = {
                {"w_walk", 0, 1000},      // stick back towards the camera with a wall behind it: vertical push, pull-in, side push
                {"w_rest", 0, 1000},    // the player stands still, the camera against the wall
                {"w_L", 0, 1000},         // L held, the camera turns round the player: pull-in by the deepest obstacle (turning rule)
                {"w_R", 0, 1000},       // R held in the open street: no obstruction
                {"w_east_R", 0, 1000},   // R held near the wall line at x 911
                {"w_east_side", 0, 1000},  // stick pushed sideways: the stick turn path, large vertical pushes (the camera rises to y 29)
            };
            for (const WallTrace& wall : walls) {
                const auto rows = loadTrace(fs::path(argv[1]) / "wall" / (std::string(wall.name) + ".csv"));
                check(std::string(wall.name) + " (with level collision)", run(rows, wall.from, std::min(wall.to, rows.size()), &world), 1e-3f,
                      1e-4f);
            }
        }
    } catch (const std::exception& error) {
        std::cerr << "camera-real-test: " << error.what() << '\n';
        return 1;
    }
    return failures == 0 ? 0 : 1;
}
