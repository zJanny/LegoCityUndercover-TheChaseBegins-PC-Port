// Differential test of the follow camera against traces recorded in the original game (Azahar oracle,
// tools/oracle/camera-trace.txt, converted by tools/export_camera_trace.py; docs/re/camera.md). Needs the
// local trace directory; exits with 77 (skipped) when it is missing.
#include "runtime/follow_camera.hpp"
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
        if (cells.size() != 19) throw std::runtime_error("bad row in " + path.string());
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
        rows.push_back(row);
    }
    return rows;
}

constexpr std::uint16_t kPlayerType = 3;

struct Result {
    float worstEye = 0, worstLook = 0, worstYaw = 0;
    std::size_t compared = 0;
};

// Restore the recorded state at row `from`, run the camera over rows from+1..to with the recorded player
// position and the pad state of the trace, and compare every row to the recording.
Result run(const std::vector<Row>& rows, std::size_t from, std::size_t to) {
    FollowCamera camera;
    camera.restore(rows[from].wanted, rows[from].eye, rows[from].look, rows[from].player, rows[from].facing);
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
            result.worstEye = std::max(result.worstEye, std::fabs(camera.eye()[k] - row.eye[k]));
            result.worstLook = std::max(result.worstLook, std::fabs(camera.lookAt()[k] - row.look[k]));
        }
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
    const bool ok = result.compared > 0 && result.worstEye <= eyeLimit && result.worstLook <= eyeLimit && result.worstYaw <= yawLimit;
    std::cout << (ok ? "ok   " : "FAIL ") << name << ": " << result.compared << " rows, eye " << result.worstEye << ", look "
              << result.worstLook << ", yaw " << result.worstYaw << '\n';
    if (!ok) ++failures;
}
}

int main(int argc, char** argv) {
    try {
        if (argc < 2 || !fs::exists(fs::path(argv[1]) / "k_still.csv")) {
            std::cout << "camera traces not found, skipping\n";
            return 77;
        }
        const fs::path directory = argv[1];
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
    } catch (const std::exception& error) {
        std::cerr << "camera-real-test: " << error.what() << '\n';
        return 1;
    }
    return failures == 0 ? 0 : 1;
}
