// Replays stud pickups recorded from the original game in Azahar (tools/oracle/studs-pickup.txt): the player is
// placed next to a stud with `teleport`, then every game update records the two pickup records, the player
// node's world matrix and the stud counter. The 30 frames of each of the ten traces are committed in
// tests/data/studs/pickups.csv (state gameplay-start, recorded 2026-10-13). Argument: the data directory.
// The test needs no game data and never skips.
#include "runtime/studs.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
using namespace chase;

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

struct Init {
    float position[3];
    int kind, skip;
};

struct Frame {
    std::uint32_t number = 0;
    float dt = 0;
    int state[2], skip[2];
    float m[16] = {};
    std::uint32_t counter = 0;
};

struct Trace {
    Init init[2];
    std::vector<Frame> frames;
};

std::map<std::string, Trace> readTraces(const std::filesystem::path& path) {
    std::ifstream in(path);
    require(bool(in), "cannot open " + path.string());
    std::map<std::string, Trace> traces;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::vector<std::string> cells;
        std::stringstream stream(line);
        std::string cell;
        while (std::getline(stream, cell, ',')) cells.push_back(cell);
        if (cells[0] == "init") {
            require(cells.size() == 8, "bad init row in " + path.string());
            Init& init = traces[cells[1]].init[std::stoi(cells[2])];
            for (int a = 0; a < 3; ++a) init.position[a] = std::stof(cells[3 + std::size_t(a)]);
            init.kind = std::stoi(cells[6]);
            init.skip = std::stoi(cells[7]);
        } else {
            require(cells[0] == "frame" && cells.size() == 18, "bad frame row in " + path.string());
            Frame frame;
            frame.number = std::uint32_t(std::stoul(cells[2]));
            frame.dt = std::stof(cells[3]);
            for (std::size_t s = 0; s < 2; ++s) {
                frame.state[s] = std::stoi(cells[4 + 2 * s]);
                frame.skip[s] = std::stoi(cells[5 + 2 * s]);
            }
            frame.counter = std::uint32_t(std::stoul(cells[8]));
            const int slots[9] = {4, 5, 6, 8, 9, 10, 12, 13, 14};
            for (std::size_t i = 0; i < 9; ++i) frame.m[slots[i]] = std::stof(cells[9 + i]);
            traces[cells[1]].frames.push_back(frame);
        }
    }
    return traces;
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
Outcome replay(const std::string& name, const Trace& trace) {
    const std::vector<Frame>& frames = trace.frames;
    std::vector<runtime::StudSpawn> spawns;
    for (int s = 0; s < 2; ++s) {
        runtime::StudSpawn spawn;
        spawn.kind = runtime::StudKind(trace.init[s].kind);
        for (int a = 0; a < 3; ++a) spawn.position[a] = trace.init[s].position[a];
        spawn.position[1] -= runtime::kStudCentreHeight;
        spawns.push_back(spawn);
    }
    runtime::StudField field(spawns);
    for (int s = 0; s < 2; ++s) {
        field.studs()[std::size_t(s)].skip = trace.init[s].skip + 1;
    }
    Outcome outcome;
    for (const Frame& frame : frames) {
        const float* m = frame.m;
        std::array<float, 3> point;
        for (int a = 0; a < 3; ++a)
            point[std::size_t(a)] = m[12 + a] + runtime::kMeasuredPlayerPickupPoint[1] * m[4 + a] + runtime::kMeasuredPlayerPickupPoint[2] * m[8 + a];
        field.update(frame.dt, point);
        for (int s = 0; s < 2; ++s) {
            const runtime::Stud& stud = field.studs()[std::size_t(s)];
            const std::string where = name + " frame " + std::to_string(frame.number) + " stud " + std::to_string(s);
            require(originalState(stud.state) == frame.state[s], where + ": state differs from the recording");
            if (stud.state == runtime::StudState::Idle) {
                require(stud.skip == frame.skip[s], where + ": skip counter differs from the recording");
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
        if (argc != 2) throw std::runtime_error("usage: studs-real-test DATA_DIR");
        const std::map<std::string, Trace> traces = readTraces(std::filesystem::path(argv[1]) / "studs" / "pickups.csv");
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
            {"rad_t0", 42, 10},   // 1.40 away
            {"rad_t1", 42, 10},   // 1.45
            {"rad_t2", 42, 10},   // 1.49
            {"rad_t3", -1, 0},    // 1.505
            {"rad_t4", -1, 0},    // 1.52
            {"rad_t5", 43, 10},   // 1.40 above (the player is in the air and falls, so one update later)
            {"rad_t6", 48, 0},    // 1.49 above (falls: collected in update 48, the animation outlasts the 30 recorded updates)
            {"rad_t7", -1, 0},    // 1.52 above
            {"rad_t8", 42, 10},   // 1.0 along and 1.0 above: 1.414 away
            {"rad_t9", 42, 10},   // 1.05 and 1.05: 1.485 away
        };
        std::size_t replayed = 0;
        for (const Expected& want : expected) {
            const Outcome outcome = replay(want.trace, traces.at(want.trace));
            require(outcome.collectedFrame == want.collectedFrame, std::string(want.trace) + ": collected in a different update than recorded");
            require(outcome.finalCounter == want.counter, std::string(want.trace) + ": final counter differs from the recording");
            ++replayed;
        }
        require(replayed == 10, "not all pinned traces replayed");
        std::cout << "studs-real-test: " << replayed << " traces replayed, all frames match\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "studs-real-test: " << error.what() << '\n';
        return 1;
    }
}
