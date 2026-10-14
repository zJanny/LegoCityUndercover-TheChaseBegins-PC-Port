// Replay of camera traces recorded in the original game while the player walks across the police station lobby
// (Azahar oracle, New 3DS, state camera-lobby-walk; tools/oracle/camera-lobby-walk.txt: lobby_north = camera-lobby-north.mem (stick 0 150, 150 updates, one record), lobby_east = camera-lobby-east.mem (stick 150 0, 130 updates, walk west over five records), converted by
// tools/export_camera_blend_trace.py; docs/re/camera.md). The rig gets the recorded player position and dt of every row and
// must reproduce the recorded eye, look and FOV. Needs the extracted RomFS and the trace directory; exits with 77 (skipped)
// when either is missing.
#include "assets/assets.hpp"
#include "runtime/camera_rig.hpp"
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace chase;
using namespace chase::runtime;
namespace fs = std::filesystem;

namespace {
struct Row {
    float dt, player[3], look[3], eye[3], fov, topWeight;
    std::size_t depth;
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
        if (cells.size() < 17) throw std::runtime_error("short row in " + path.string());
        Row row;
        row.dt = std::stof(cells[1]);
        for (int a = 0; a < 3; ++a) {
            row.player[a] = std::stof(cells[std::size_t(2 + a)]);
            row.look[a] = std::stof(cells[std::size_t(5 + a)]);
            row.eye[a] = std::stof(cells[std::size_t(8 + a)]);
        }
        row.fov = std::stof(cells[11]);
        row.topWeight = std::stof(cells[15]);
        row.depth = std::size_t(std::stoul(cells[16]));
        rows.push_back(row);
    }
    return rows;
}

float distance(const float a[3], const float b[3]) {
    return std::sqrt((a[0] - b[0]) * (a[0] - b[0]) + (a[1] - b[1]) * (a[1] - b[1]) + (a[2] - b[2]) * (a[2] - b[2]));
}

struct Replay {
    const char* level;
    float start[3], yaw, facing;
    struct File {
        const char* name;
        std::size_t rows;
        std::size_t firstPoseRow;  // earlier rows still carry the follow camera's reaction to the teleport
    };
    std::vector<File> files;  // consecutive traces of one emulator run
    float eyeBound, lookBound;
};
}

int main(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "usage: camera-blend-real-test FIB NAMES TRACE_DIR\n";
        return 2;
    }
    if (!fs::exists(argv[1]) || !fs::exists(argv[2]) || !fs::exists(argv[3])) {
        std::cout << "camera-blend-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        // Provenance: state camera-lobby-walk (the user's police-station quick save, unmodified): player node
        // (-1.97040856, 1.43587279, 14.84306526), settled record pose, camera yaw pi. Traces: memtrace of camera C+0x4c / +0x70 /
        // +0x7c per update; lobby_north: stick (0, 150), lobby_east: stick (150, 0) over five records.
        // State street-after-1-1 (unmodified): player (900.549133, 16.567217, 1110.280029), follow camera, yaw -1.5655, facing -pi/2.
        // street_in / street_out: the player node was written to (933, 16.6, 1124) (inside CameraBound_PolicePK_03) and back to
        // (905, 16.6, 1110) with the oracle's teleport command between the traces (tools/oracle/camera-street-teleport.txt).
        const Replay replays[] = {
            {"levels/levels/lvl_00_policestation/lvl_00_policestation_main.blvl", {-1.97040855884552f, 1.4358727931976318f, 14.84306526184082f},
             3.14159265f, 3.14159265f, {{"lobby_north.csv", 150, 0}}, 5e-5f, 5e-5f},
            {"levels/levels/lvl_00_policestation/lvl_00_policestation_main.blvl", {-1.97040855884552f, 1.4358727931976318f, 14.84306526184082f},
             3.14159265f, 3.14159265f, {{"lobby_east.csv", 130, 0}}, 5e-5f, 5e-5f},
            {"levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl", {900.549133f, 16.567217f, 1110.280029f}, -1.5655f,
             -1.5707963f, {{"street_rest.csv", 20, 0},
              {"street_in.csv", 80, 60},
              {"street_out.csv", 80, 1000}}, 2e-1f, 2e-1f}};
        for (const Replay& replay : replays) {
            const assets::Level level = assets::parseLevel(archive.read(*archive.find(replay.level)));
            std::string camText;
            {
                const std::string base = std::string(replay.level).substr(0, std::string(replay.level).size() - 5);
                const assets::Bytes bytes = archive.read(*archive.find(base + ".cam"));
                camText.assign(bytes.begin(), bytes.end());
            }
            CameraRig rig(level, camText);
            rig.reset(replay.start, replay.yaw, replay.facing);
            for (const Replay::File& entry : replay.files) {
                const char* file = entry.name;
                const std::size_t expectedRows = entry.rows;
                const fs::path path = fs::path(argv[3]) / file;
                if (!fs::exists(path)) {
                    std::cout << "camera-blend-real-test: " << path << " missing, skipped\n";
                    return 77;
                }
                const std::vector<Row> rows = loadTrace(path);
                if (rows.size() != expectedRows) throw std::runtime_error(std::string("row count of ") + file);
                float worstEye = 0, worstLook = 0, worstFov = 0, worstWeight = 0;
                std::size_t worstRow = 0;
                for (std::size_t i = 0; i < rows.size(); ++i) {
                    FollowCameraInput input;
                    input.dt = rows[i].dt;
                    input.targetFacing = replay.facing;
                    for (int a = 0; a < 3; ++a) input.target[a] = rows[i].player[a];
                    rig.update(input);
                    worstWeight = std::max(worstWeight, std::fabs(rig.weightOfTop() - rows[i].topWeight));
                    if (rig.commandCount() != rows[i].depth) throw std::runtime_error(std::string("command count of ") + file + " row " + std::to_string(i));
                    if (i >= entry.firstPoseRow) {
                        const float eyeError = distance(rig.eye(), rows[i].eye), lookError = distance(rig.lookAt(), rows[i].look);
                        if (eyeError > worstEye) worstEye = eyeError, worstRow = i;
                        worstLook = std::max(worstLook, lookError);
                    }
                    worstFov = std::max(worstFov, std::fabs(rig.fieldOfViewDegrees() - rows[i].fov));
                }
                std::cout << file << ": " << rows.size() << " updates, worst eye error " << worstEye << " (row " << worstRow
                          << "), look " << worstLook << ", FOV " << worstFov << ", top weight " << worstWeight << '\n';
                if (worstEye > replay.eyeBound || worstLook > replay.lookBound || worstFov > 1e-2f || worstWeight > 2e-5f)
                    throw std::runtime_error(std::string("replay differs from the recorded pose: ") + file);
            }
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
