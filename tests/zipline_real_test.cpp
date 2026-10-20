// The zipline of mission 1-2 in lvl_01_cherrytreehills (src/runtime/zipline.cpp): the anchors 1-8_Zip_Line05 and 1-8_Zip_Line04 from the
// level data, then a replay of the ride recorded in the oracle from the state m12-9-zipline (docs/re/player-traversal.md, entry of
// 2026-10-18 on the zipline): scheduler step fixed to 1/30 (`dt 0.033333335 0.033333335`), B held for 3 updates, then `trace 200 zip-ride mem`
// (round 20 agent A, zip-ride, rows below are those frames). The grab happened in the 7th jump update (trap at 0x2a9f04:
// node position (870.22626, 67.5988, 630.96051) after the jump step, grab point (870.3043, 67.5988, 630.9428)). Needs the extracted RomFS;
// exits with 77 without it.
#include "assets/assets.hpp"
#include "runtime/zipline.hpp"
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
constexpr const char* kLevelFile = "levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl";
constexpr const char* kDismountClip = "models/char_minifig/class_police/zipline_dismount_minifig.bfnanm";

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

bool near(float a, float b, float tolerance) { return std::fabs(a - b) <= tolerance; }

struct Row {
    int frame;
    chase::runtime::ZiplinePhase phase;
    float t;  // ctrl+0x24c, negative when not checked
    float x, y, z;
};
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: zipline-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "zipline-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        using namespace chase;
        using runtime::ZiplinePhase;
        assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        const assets::FuseEntry* entry = archive.find(kLevelFile);
        require(entry != nullptr, std::string("not in archive: ") + kLevelFile);
        const assets::Level level = assets::parseLevel(archive.read(*entry));
        const std::vector<runtime::ZiplineAnchor> anchors = runtime::loadZiplineAnchors(level, assets::archiveClassLookup(archive));
        // docs/re/objects.md counts 24 zipline objects in lvl_01 (class props/parkour_zipline); the tightropes have GrappleLineType 0.
        require(anchors.size() == 24, "zipline anchors: " + std::to_string(anchors.size()));
        int high = -1, low = -1, fast = -1;
        for (std::size_t i = 0; i < anchors.size(); ++i) {
            if (anchors[i].instance == "1-8_zip_line05") high = int(i);
            if (anchors[i].instance == "1-8_zip_line04") low = int(i);
            if (anchors[i].instance == "1-8_zip_line08") fast = int(i);
        }
        require(high >= 0 && low >= 0 && fast >= 0, "1-8_Zip_Line04, 05 and 08 are anchors");
        require(anchors[std::size_t(high)].target == low && anchors[std::size_t(low)].target == high, "TargetTightropeAnchor pairs 04 and 05");
        // Oracle read: the line list 0x592490 (14 entries) holds 0x08c02a30 (1-8_Zip_Line05, name hash 0x753115c0 at +8), not its target.
        require(anchors[std::size_t(high)].listed && !anchors[std::size_t(low)].listed, "05 is listed, 04 is not");
        require(near(anchors[std::size_t(high)].speed, 7.0f, 0.0f), "ZipLineSpeed 7 (class default; read at +0x154 in m12-9)");
        require(near(anchors[std::size_t(fast)].speed, 30.0f, 0.0f), "1-8_Zip_Line08 sets ZipLineSpeed 30");

        const assets::FuseEntry* clipEntry = archive.find(kDismountClip);
        require(clipEntry != nullptr, std::string("not in archive: ") + kDismountClip);
        const assets::Animation clip = assets::parseAnimation(archive.read(*clipEntry));
        const std::optional<assets::RootMotion> motion = assets::rootMotion(clip);
        require(motion.has_value(), "zipline_dismount has a root track");

        runtime::ZiplineRider rider;
        const float point[3] = {870.3043f, 67.5988f, 630.9428f};
        require(runtime::ziplineGrabTest(anchors, point, {}) == high, "the grab test finds the line at the recorded grab point");
        runtime::ZiplineInput input;
        input.dt = 0.033333335f;
        input.position[0] = 870.22626f;
        input.position[1] = 67.5988f;
        input.position[2] = 630.96051f;
        input.facing = 18715;
        input.dismountMotion = &*motion;
        input.dismountFramesPerSecond = assets::animationFramesPerSecond(clip);
        runtime::ZiplineStep step = runtime::ziplineEnter(rider, anchors, high, input);
        // ctrl+0x250 = 0.273670 and ctrl+0xa = 15683 in every row of the ride.
        require(near(rider.rate, 0.273670f, 2e-6f), "rate " + std::to_string(rider.rate));
        require(rider.heading == 15683 && step.facing == 15683, "heading " + std::to_string(rider.heading));

        const std::vector<Row> rows = {
            {9, ZiplinePhase::GetOn, 0.14534f, 870.50049f, 67.91557f, 631.01099f},
            {10, ZiplinePhase::GetOn, 0.15446f, 870.76385f, 68.15879f, 631.05432f},
            {16, ZiplinePhase::GetOn, 0.20919f, 872.21173f, 68.72165f, 631.22729f},
            {46, ZiplinePhase::GetOn, 0.48286f, 878.85529f, 67.50455f, 631.70099f},
            {47, ZiplinePhase::Slide, 0.49199f, 879.07526f, 67.45379f, 631.71582f},
            {100, ZiplinePhase::Slide, 0.97547f, 890.73169f, 64.75984f, 632.49988f},
            {107, ZiplinePhase::Slide, 1.03933f, 892.27124f, 64.40400f, 632.60345f},
            {108, ZiplinePhase::Slide, 1.04845f, 892.27124f, 64.40400f, 632.60345f},
            {109, ZiplinePhase::Dismount, -1, 892.27124f, 64.40400f, 632.60345f},
            {110, ZiplinePhase::Dismount, -1, 892.34235f, 64.41275f, 632.60822f},
            {120, ZiplinePhase::Dismount, -1, 893.31036f, 65.38463f, 632.67334f},
            {138, ZiplinePhase::Dismount, -1, 895.25818f, 64.63119f, 632.79761f},
            {139, ZiplinePhase::None, -1, 895.25818f, 64.63119f, 632.79761f},
        };
        std::size_t next = 0;
        bool fell = false;
        for (int frame = 9; frame <= 139; ++frame) {
            if (frame > 9) {
                std::copy(step.position, step.position + 3, input.position);
                input.facing = step.facing;
                step = runtime::ziplineUpdate(rider, anchors, input);
                if (step.falling) {
                    fell = true;
                    // Row 139: state 5 with ctrl+0x2b8 = 0.22844 and ctrl+0x2b4 = 0.45688.
                    require(frame == 139 && near(step.fallSpeed, 0.22844f, 2e-4f) && near(step.fallDistance, 0.45688f, 2e-4f),
                            "fall at frame " + std::to_string(frame) + " speed " + std::to_string(step.fallSpeed) + " distance " + std::to_string(step.fallDistance));
                }
            }
            if (next < rows.size() && rows[next].frame == frame) {
                const Row& row = rows[next++];
                const std::string where = "frame " + std::to_string(frame);
                require(rider.phase == row.phase, where + ": phase");
                if (row.t >= 0) require(near(rider.t, row.t, 2e-5f), where + ": t " + std::to_string(rider.t));
                // The dismount root track is linear between keys here; the recorded position differs by up to 5e-4 at its end.
                const float tolerance = row.phase == ZiplinePhase::Dismount || row.phase == ZiplinePhase::None ? 1e-3f : 2e-4f;
                require(near(step.position[0], row.x, tolerance) && near(step.position[1], row.y, tolerance) && near(step.position[2], row.z, tolerance),
                        where + ": position " + std::to_string(step.position[0]) + " " + std::to_string(step.position[1]) + " " + std::to_string(step.position[2]));
            }
        }
        require(fell && next == rows.size(), "the ride ends in state 5 at frame 139");
    } catch (const std::exception& error) {
        std::cerr << "zipline-real-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "zipline-real-test: ok\n";
    return 0;
}
