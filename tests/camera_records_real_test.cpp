// Camera record selection and record pose on real level data against what the original game showed in the Azahar oracle
// (docs/re/camera.md). Needs the extracted RomFS; exits with 77 (skipped) when it is missing.
#include "assets/assets.hpp"
#include "runtime/camera_records.hpp"
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
using namespace chase;
using namespace chase::runtime;

namespace {
void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
bool near(float a, float b, float tolerance = 2e-4f) { return std::fabs(a - b) < tolerance; }

CameraRecords load(assets::FuseArchive& archive, const std::string& levelFile, assets::Level& level) {
    const assets::FuseEntry* entry = archive.find(levelFile);
    require(entry != nullptr, "not in archive: " + levelFile);
    level = assets::parseLevel(archive.read(*entry));
    std::string text;
    if (const assets::FuseEntry* cam = archive.find(levelFile.substr(0, levelFile.size() - 5) + ".cam")) {
        const assets::Bytes bytes = archive.read(*cam);
        text.assign(bytes.begin(), bytes.end());
    }
    return CameraRecords(level, text);
}
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: camera-records-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "camera-records-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);

        // Provenance: oracle state police-station (Azahar New 3DS), player node (-1.97040855884552, 1.4358727931976318,
        // 14.84306526184082): command tag 0x298cd8, one record (Camerabound_01), camera eye (-1.9704089, 6.9915738,
        // 23.1577549), look (-1.9704088, 3.4358723, 14.8430634), FOV 43, yaw pi (RPC read of C+0x4c, evidence in camera.md).
        assets::Level station;
        const CameraRecords stationRecords = load(archive, "levels/levels/lvl_00_policestation/lvl_00_policestation_main.blvl", station);
        require(stationRecords.records().size() == 94, "lvl_00 camera record count");
        const float lobby[3] = {-1.97040855884552f, 1.4358727931976318f, 14.84306526184082f};
        CameraRecordSelector selector(stationRecords);
        require(selector.update(lobby) == CameraCommand::RecordPose, "the station lobby selects the record pose");
        require(selector.blend().records.size() == 1 && stationRecords.records()[std::size_t(selector.blend().records[0])].name == "Camerabound_01",
                "record at the lobby start");
        const RecordPose pose = evaluateRecordPose(stationRecords, selector.blend(), lobby);
        require(near(pose.eye[0], -1.9704089f) && near(pose.eye[1], 6.9915738f) && near(pose.eye[2], 23.157755f), "pose eye");
        require(near(pose.look[0], -1.9704088f) && near(pose.look[1], 3.4358723f) && near(pose.look[2], 14.843063f), "pose look");
        require(near(pose.fovDegrees, 43.0f) && near(poseYaw(pose), 3.1415927f), "pose FOV and yaw");

        // Provenance: oracle states gameplay-start and street-after-1-1: command tag 0x2a8bfc with the level's Room record.
        assets::Level street;
        const CameraRecords streetRecords = load(archive, "levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl", street);
        const float outside[3] = {900.5491333007812f, 16.567216873168945f, 1110.280029296875f};
        CameraRecordSelector streetSelector(streetRecords);
        require(streetSelector.update(outside) == CameraCommand::Follow, "the street selects the follow controller");
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
