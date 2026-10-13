// Checks CameraSweep against the sweeps recorded in the original game (tools/oracle/camwall-*.txt memtraces, docs/re/camera.md).
// Per frame the trace holds the slab frame of the update (origin, axes, sizes at K+0x214..K+0x2ac, the camera constants at
// K+0x1b8..K+0x1cc) and the first 13 raw segments at K+0x324 with their count at K+0x320. The replica gathers the level
// collision into the same frame; every one of its segments must be one of the recorded ones. Recorded sweeps that
// contain more segments come from placed objects (lamp posts) that the replica does not have.
// Needs the extracted RomFS and the .mem files; exits with 77 (skipped) when one is missing.
#include "assets/assets.hpp"
#include "runtime/camera_sweep.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

using namespace chase::runtime;
namespace fs = std::filesystem;

namespace {
constexpr const char* kLevelModel = "models/lvl_01_cherrytreehills/lvl_01_cherrytreehills.bfnmdl";
constexpr std::size_t kRegions[] = {0x100, 0x200, 0x40, 0xb0, 0x60, 0x30, 0x200, 0x80, 0x60, 0x20, 0x10, 0x20};
constexpr std::size_t kRecordedSegments = 13;
constexpr float kTolerance = 2e-3f;

struct Trace {
    std::vector<char> data;
    std::size_t record = 8;
    std::size_t offset[13] = {8};
    Trace(const fs::path& path) {
        std::ifstream in(path, std::ios::binary);
        data.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
        for (int i = 0; i < 12; ++i) {
            offset[i + 1] = offset[i] + kRegions[i];
            record += kRegions[i];
        }
    }
    std::size_t frames() const { return data.size() / record; }
    float value(std::size_t frame, int region, std::size_t byte) const {
        float v;
        std::memcpy(&v, &data[frame * record + offset[region] + byte], sizeof v);
        return v;
    }
    unsigned char count(std::size_t frame, std::size_t byte) const {
        return static_cast<unsigned char>(data[frame * record + offset[6] + byte]);
    }
};

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
}

int main(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "usage: camera-sweep-real-test FIB NAMES CAMWALL_TRACE_DIR\n";
        return 2;
    }
    const fs::path fib = argv[1], names = argv[2], directory = argv[3];
    if (!fs::exists(fib) || !fs::exists(names) || !fs::exists(directory / "w_walk.mem")) {
        std::cout << "camera-sweep-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        chase::assets::FuseArchive archive(fib.string());
        archive.loadNames(names.string());
        const chase::assets::FuseEntry* entry = archive.find(kLevelModel);
        require(entry != nullptr, std::string("not in archive: ") + kLevelModel);
        CollisionWorld world;
        for (const auto& item : chase::assets::parseModel(archive.read(*entry)).collision) world.add(item);

        std::size_t frames = 0, equal = 0, withObstacles = 0;
        for (const char* name : {"w_walk", "w_rest", "w_L", "w_R", "w_east_R", "w_east_side"}) {
            const Trace trace(directory / (std::string(name) + ".mem"));
            std::size_t traceEqual = 0, traceFrames = trace.frames();
            for (std::size_t f = 0; f < traceFrames; ++f) {
                const float reach = trace.value(f, 1, 0x1b8), side = trace.value(f, 1, 0x1bc), up = trace.value(f, 1, 0x1c0);
                const float fullDistance = trace.value(f, 1, 0x1c4) - reach;
                const float nearLimit = trace.value(f, 1, 0x1c8);
                float depth[3], origin[3], look[3], camera[3];
                for (int i = 0; i < 3; ++i) {
                    depth[i] = trace.value(f, 6, 0x234 - 0x200 + 4 * std::size_t(i));
                    origin[i] = trace.value(f, 6, 0x244 - 0x200 + 4 * std::size_t(i));
                }
                const float distance = fullDistance - nearLimit + 0.1f;
                for (int i = 0; i < 3; ++i) {
                    look[i] = origin[i] + depth[i] * fullDistance;
                    camera[i] = look[i] - depth[i] * distance;
                }
                CameraSweepSettings settings;
                settings.reach = reach;
                settings.sideHalfWidth = side;
                settings.upHalfWidth = up;
                CameraSweep sweep;
                sweep.prepare(world, camera, look, fullDistance, settings);

                const std::size_t recorded = trace.count(f, 0x320 - 0x200);
                std::vector<CameraSweep::Segment> segments;
                for (std::size_t i = 0; i < std::min(recorded, kRecordedSegments); ++i) {
                    const std::size_t at = 0x324 - 0x200 + 16 * i;
                    segments.push_back({trace.value(f, 6, at), trace.value(f, 6, at + 4), trace.value(f, 6, at + 8), trace.value(f, 6, at + 12)});
                }
                const auto& mine = sweep.rawSegments(CameraSweep::Side);
                for (std::size_t i = 0; i < std::min(mine.size(), kRecordedSegments); ++i) {
                    const bool found = std::any_of(segments.begin(), segments.end(), [&](const CameraSweep::Segment& s) {
                        return std::fabs(s.x0 - mine[i].x0) < kTolerance && std::fabs(s.y0 - mine[i].y0) < kTolerance &&
                               std::fabs(s.x1 - mine[i].x1) < kTolerance && std::fabs(s.y1 - mine[i].y1) < kTolerance;
                    });
                    require(found, std::string(name) + " frame " + std::to_string(f) + ": replica segment " + std::to_string(i) + " not recorded");
                }
                if (mine.size() == recorded) {
                    ++traceEqual;
                    if (recorded > 0) ++withObstacles;
                }
            }
            std::cout << name << ": " << traceFrames << " frames, " << traceEqual << " with the recorded segment count\n";
            frames += traceFrames;
            equal += traceEqual;
        }
        std::cout << "total " << frames << " frames, " << equal << " with the recorded count, " << withObstacles << " of them with obstacles\n";
        // Recorded 2026-10-13 with tools/oracle/camwall-circle.txt and camwall-east.txt; the totals come from the first run of this
        // test (manual inspection: the unmatched recorded segments are the small lamp-post squares of the street east of the spawn).
        require(frames == 700 && equal == 510 && withObstacles == 323, "explained frame totals changed");
    } catch (const std::exception& error) {
        std::cerr << "camera-sweep-real-test: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
