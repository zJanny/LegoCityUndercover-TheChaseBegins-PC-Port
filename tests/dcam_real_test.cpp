// Director camera against the real data of lvl_01_cherrytreehills and the Azahar oracle (docs/re/dcam.md): the level paths
// that the script Police_ExtToInt names, their pose in the oracle, and the update on which the script's WaitForDCam ends.
// Needs the extracted RomFS; exits with 77 (skipped) when it is missing.
#include "assets/assets.hpp"
#include "runtime/dcam.hpp"
#include "runtime/script_runner.hpp"
#include <cmath>
#include <filesystem>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>

namespace {
constexpr const char* kLevelFile = "levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl";
using namespace chase;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

void pathCensus(const assets::Level& level) {
    // Provenance: tools-less census with the same parser (C++ loader of section +0x20, replica of 0x22a3b0 lookup):
    // 457 path records in lvl_01, all of type 0; the 257 camera and look arguments of its StartDCam / StartDCamTrack
    // calls resolve (0 missing), all open (flag 0), with 1 to 5 points.
    require(level.paths.size() == 457, "path count changed: " + std::to_string(level.paths.size()));
    std::size_t total = 0, missing = 0, closed = 0, otherType = 0;
    std::map<std::size_t, std::size_t> counts;
    for (const assets::LevelScript& script : level.scripts)
        for (const assets::ScriptCall& call : script.calls) {
            if (call.function != "StartDCam" && call.function != "StartDCamTrack") continue;
            const std::size_t paths = call.function == "StartDCam" ? 2 : 1;
            for (std::size_t i = 0; i < paths; ++i) {
                ++total;
                const assets::LevelPath* path = runtime::findLevelPath(level, call.arguments[i]);
                if (!path) {
                    ++missing;
                    continue;
                }
                closed += path->closed;
                otherType += path->type != 0;
                ++counts[path->points.size()];
            }
        }
    require(total == 257 && missing == 0 && closed == 0 && otherType == 0, "DCam path references changed");
    require(counts[1] == 106 && counts[2] == 108 && counts[3] == 38 && counts[4] == 4 && counts[5] == 1, "DCam path point counts changed");
}

void policeEntryPoseMatchesTheOracle(const assets::Level& level) {
    // Provenance: Azahar oracle, New 3DS state dcam-police-dialog, tools/oracle/dcam-police-entry.txt, update 4 of the press trace
    // (the update on which Police_ExtToInt starts the director camera): the top camera stack entry's pose, floats exact.
    const assets::LevelPath* eye = runtime::findLevelPath(level, "Dcam_Enter_Station_Ground");
    const assets::LevelPath* look = runtime::findLevelPath(level, "Dcam_Enter_Station_Ground_Target");
    require(eye && look, "paths missing");
    runtime::DirectorCamera director;
    director.startPaths(eye, look, 4.0f, 3);
    director.update(0.033f, nullptr);
    const float expectedEye[3] = {904.7454223632812f, 22.484100341796875f, 1106.5201416015625f};
    const float expectedLook[3] = {906.4791259765625f, 18.109600067138672f, 1115.2774658203125f};
    for (int a = 0; a < 3; ++a)
        require(director.pose().eye[a] == expectedEye[a] && director.pose().look[a] == expectedLook[a], "pose differs from the oracle");
    require(director.pose().fovDegrees == 43.0f && director.pose().offset3d == 8.0f, "pose fov / +0x38");
}

void twoPointPathMatchesTheOracle(const assets::Level& level) {
    // Provenance: Azahar oracle, state dcam-police-dialog with the script's path names overwritten by Dcam_Police_Door and
    // Dcam_Police_Door_Target (tools/oracle/dcam-police-poked.txt, a measuring aid): eye of the stack entry at the updates
    // after the start (update 4 = elapsed 33 ms); 122 updates agree with the replica to 1.6e-4 (the pinned ones are exact floats).
    const assets::LevelPath* eye = runtime::findLevelPath(level, "Dcam_Police_Door");
    const assets::LevelPath* look = runtime::findLevelPath(level, "Dcam_Police_Door_Target");
    require(eye && look && eye->points.size() == 2 && look->points.size() == 1, "paths");
    runtime::DirectorCamera director;
    director.startPaths(eye, look, 4.0f, 3);
    const struct {
        int update;
        float eye[3];
    } expected[] = {{4, {900.7855834960938f, 23.3604736328125f, 1102.9415283203125f}},
                    {10, {900.7894287109375f, 23.359182357788086f, 1102.949462890625f}},
                    {40, {900.9224853515625f, 23.314027786254883f, 1103.2265625f}},
                    {70, {901.2542114257812f, 23.201358795166016f, 1103.918212890625f}},
                    {100, {901.5167236328125f, 23.112205505371094f, 1104.46533203125f}},
                    {125, {901.5692138671875f, 23.09440040588379f, 1104.57470703125f}}};
    std::size_t next = 0;
    for (int update = 4; update <= 125 && next < 6; ++update) {
        director.update(0.033f, nullptr);
        if (update != expected[next].update) continue;
        for (int a = 0; a < 3; ++a)
            require(std::fabs(director.pose().eye[a] - expected[next].eye[a]) < 5e-4f, "eye at update " + std::to_string(update));
        ++next;
    }
    require(next == 6, "all pinned updates checked");
}

void policeEntryTiming(const assets::Level& level) {
    // Provenance: same trace. The director camera starts in update 4 (script runs StartDCam 4 s, snap 3), the done flag
    // (camera object +0xdd bit 3) is set in update 125 = 121 updates later, the end test clears the active flag (D+0x24 bit 0)
    // and switches the current camera in update 126, the stack top is the follow command with weight 1 in update 127.
    const assets::LevelPath* eye = runtime::findLevelPath(level, "Dcam_Enter_Station_Ground");
    const assets::LevelPath* look = runtime::findLevelPath(level, "Dcam_Enter_Station_Ground_Target");
    runtime::DirectorCamera director;
    director.startPaths(eye, look, 4.0f, 3);
    int update = 4;
    std::vector<runtime::DirectorRequest> requests = director.takeRequests();
    require(requests.size() == 1 && requests[0].instant, "snap start: instant command");
    int endUpdate = -1;
    for (; update < 200 && endUpdate < 0; ++update) {
        director.update(0.033f, nullptr);
        for (const runtime::DirectorRequest& request : director.takeRequests())
            if (request.kind == runtime::DirectorRequest::Kind::End) {
                endUpdate = update;
                require(request.instant || request.fadeSeconds == 0.0f, "snap end: instant hand-back");
            }
    }
    require(endUpdate == 126, "end test update: " + std::to_string(endUpdate));
    require(!director.busy(), "not busy after the end test");
}
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: dcam-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "dcam-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        const assets::FuseEntry* entry = archive.find(kLevelFile);
        require(entry != nullptr, std::string("not in archive: ") + kLevelFile);
        const assets::Level level = assets::parseLevel(archive.read(*entry));
        pathCensus(level);
        policeEntryPoseMatchesTheOracle(level);
        twoPointPathMatchesTheOracle(level);
        policeEntryTiming(level);
    } catch (const std::exception& error) {
        std::cerr << "dcam-real-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "dcam-real-test: ok\n";
    return 0;
}
