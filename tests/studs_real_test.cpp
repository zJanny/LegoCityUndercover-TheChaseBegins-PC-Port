// Replays stud pickups recorded from the original game in Azahar (tools/oracle/studs-pickup.txt): the player is
// placed next to a stud with `teleport`, then every game update records the two pickup records, the player
// node's world matrix and the stud counter. Arguments: fib, names csv, trace directory. Exit code 77 when the
// game data or the traces are missing.
#include "assets/assets.hpp"
#include "runtime/studs.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
using namespace chase;

namespace {
constexpr std::size_t kRecordBytes = 0x4c, kMatrixBytes = 64;
constexpr std::size_t kFrameBytes = 8 + 2 * kRecordBytes + kMatrixBytes + 8;
constexpr std::size_t kStateOffset = 0x44, kKindOffset = 0x46, kSkipOffset = 0x30;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

struct Frame {
    std::uint32_t number = 0;
    float dt = 0;
    std::uint8_t records[2][kRecordBytes];
    float matrix[16];
    std::uint32_t counter = 0;
};

std::vector<Frame> readTrace(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    const std::vector<char> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    require(!bytes.empty() && bytes.size() % kFrameBytes == 0, path.string() + ": unexpected size");
    std::vector<Frame> frames(bytes.size() / kFrameBytes);
    for (std::size_t i = 0; i < frames.size(); ++i) {
        const char* at = bytes.data() + i * kFrameBytes;
        Frame& frame = frames[i];
        std::memcpy(&frame.number, at, 4);
        std::memcpy(&frame.dt, at + 4, 4);
        std::memcpy(frame.records[0], at + 8, kRecordBytes);
        std::memcpy(frame.records[1], at + 8 + kRecordBytes, kRecordBytes);
        std::memcpy(frame.matrix, at + 8 + 2 * kRecordBytes, kMatrixBytes);
        std::memcpy(&frame.counter, at + 8 + 2 * kRecordBytes + kMatrixBytes, 4);
    }
    return frames;
}

std::uint8_t originalState(runtime::StudState state) {
    switch (state) {
    case runtime::StudState::Idle: return 1;
    case runtime::StudState::Collecting: return 4;
    default: return 0;
    }
}

struct Outcome {
    int collectedFrame = -1;
    std::uint32_t finalCounter = 0;
};

// Rebuilds both studs from the first recorded frame (kind, position, skip counter before that update) and checks
// every later frame: state of both records, the skip counter of idle studs and the counter.
Outcome replay(const std::string& name, const std::vector<Frame>& frames) {
    std::vector<runtime::StudSpawn> spawns;
    for (int s = 0; s < 2; ++s) {
        runtime::StudSpawn spawn;
        spawn.kind = runtime::StudKind(frames[0].records[s][kKindOffset]);
        float centre[3];
        std::memcpy(centre, frames[0].records[s], 12);
        for (int a = 0; a < 3; ++a) spawn.position[a] = centre[a];
        spawn.position[1] -= runtime::kStudCentreHeight;
        spawns.push_back(spawn);
    }
    runtime::StudField field(spawns);
    for (int s = 0; s < 2; ++s) {
        std::int16_t skip;
        std::memcpy(&skip, frames[0].records[s] + kSkipOffset, 2);
        field.studs()[std::size_t(s)].skip = skip + 1;
    }
    Outcome outcome;
    for (const Frame& frame : frames) {
        const float* m = frame.matrix;
        std::array<float, 3> point;
        for (int a = 0; a < 3; ++a)
            point[std::size_t(a)] = m[12 + a] + runtime::kMeasuredPlayerPickupPoint[1] * m[4 + a] + runtime::kMeasuredPlayerPickupPoint[2] * m[8 + a];
        field.update(frame.dt, point);
        for (int s = 0; s < 2; ++s) {
            const runtime::Stud& stud = field.studs()[std::size_t(s)];
            const std::string where = name + " frame " + std::to_string(frame.number) + " stud " + std::to_string(s);
            require(originalState(stud.state) == frame.records[s][kStateOffset], where + ": state differs from the recording");
            if (stud.state == runtime::StudState::Idle) {
                std::int16_t skip;
                std::memcpy(&skip, frame.records[s] + kSkipOffset, 2);
                require(stud.skip == skip, where + ": skip counter differs from the recording");
            }
            if (s == 0 && stud.state == runtime::StudState::Collecting && outcome.collectedFrame < 0) outcome.collectedFrame = int(frame.number);
        }
        require(field.counter() == frame.counter, name + " frame " + std::to_string(frame.number) + ": counter differs from the recording");
        outcome.finalCounter = frame.counter;
    }
    return outcome;
}
}

int main(int argc, char** argv) {
    try {
        if (argc < 4) throw std::runtime_error("usage: studs-real-test FIB NAMES_CSV TRACE_DIR");
        const std::filesystem::path dir = argv[3];
        if (!std::filesystem::exists(argv[1]) || !std::filesystem::is_directory(dir)) {
            std::cout << "studs-real-test: game data or traces not found, skipping\n";
            return 77;
        }
        std::vector<std::filesystem::path> traces;
        for (const auto& entry : std::filesystem::directory_iterator(dir))
            if (entry.path().extension() == ".mem") traces.push_back(entry.path());
        std::sort(traces.begin(), traces.end());
        if (traces.empty()) {
            std::cout << "studs-real-test: no .mem traces in " << dir << ", skipping\n";
            return 77;
        }
        // Recorded 2026-10-13 from tools/oracle/studs-pickup.txt (state gameplay-start, Cherry Tree Hills): the player's
        // pickup point at (distance, height offset) from the silver stud FP_Studs005 (861.136, 16.792, 1060.531); the gold
        // stud FP_Studs106 lies 1.296 away in z. Stud A is collected 12 updates after the teleport (the skip counter of
        // the recording state) when its distance is below 1.5, and the silver value 10 is added 15 updates later.
        struct Expected {
            const char* trace;
            int collectedFrame;
            std::uint32_t counter;
        };
        const Expected expected[] = {
            {"rad_t0.mem", 42, 10},   // 1.40 away
            {"rad_t1.mem", 42, 10},   // 1.45
            {"rad_t2.mem", 42, 10},   // 1.49
            {"rad_t3.mem", -1, 0},    // 1.505
            {"rad_t4.mem", -1, 0},    // 1.52
            {"rad_t5.mem", 43, 10},   // 1.40 above (the player is in the air and falls, so one update later)
            {"rad_t6.mem", 48, 0},    // 1.49 above (falls: collected in update 48, the animation outlasts the 30 recorded updates)
            {"rad_t7.mem", -1, 0},    // 1.52 above
            {"rad_t8.mem", 42, 10},   // 1.0 along and 1.0 above: 1.414 away
            {"rad_t9.mem", 42, 10},   // 1.05 and 1.05: 1.485 away
        };
        std::size_t replayed = 0;
        for (const Expected& want : expected) {
            const std::filesystem::path path = dir / want.trace;
            if (!std::filesystem::exists(path)) continue;
            const Outcome outcome = replay(want.trace, readTrace(path));
            require(outcome.collectedFrame == want.collectedFrame, std::string(want.trace) + ": collected in a different update than recorded");
            require(outcome.finalCounter == want.counter, std::string(want.trace) + ": final counter differs from the recording");
            ++replayed;
        }
        require(replayed > 0, "none of the pinned traces was found");
        std::cout << "studs-real-test: " << replayed << " traces replayed, all frames match\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "studs-real-test: " << error.what() << '\n';
        return 1;
    }
}
