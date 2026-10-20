// Differential test of the slope slow-down of the run state update 0x1abe60 (docs/re/player-falling-jumping.md, slope entry) against
// runs recorded in the original game (Azahar oracle, tools/oracle/slope-run.txt; rows and provenance in data/slope_traces.inc):
// ctrl+0x94 of every run row equals the slope factor of the row's facing and ground normal times 1.5 once the sprint timer passed 1 s,
// and the next update moves by run speed * dt * that scale.
#include "memtrace.hpp"
#include "runtime/player_locomotion.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>

using namespace chase::runtime;

namespace {
const std::vector<memtrace::NamedTrace> kSegments = {
#include "data/slope_traces.inc"
};
}

int main() {
    try {
        std::set<std::string> recordings;
        std::size_t runRows = 0, slopeRows = 0, speedRows = 0, wrong = 0;
        float worstScale = 0, worstSpeed = 0;
        for (const memtrace::NamedTrace& segment : kSegments) {
            const std::vector<memtrace::Row>& rows = segment.rows;
            recordings.insert(segment.name.substr(0, segment.name.find('@')));
            for (std::size_t i = 0; i < rows.size(); ++i) {
                const memtrace::Row& row = rows[i];
                if (row.state != 2) continue;
                ++runRows;
                const float expected = slopeFactor(row.facing, row.normal) * (row.runTimer > kSprintTime ? kSprintSpeedScale : 1.0f);
                const float scaleError = std::fabs(expected - row.scale);
                worstScale = std::max(worstScale, scaleError);
                if (std::fabs(row.scale - 1.0f) > 1e-3f && std::fabs(row.scale - 1.5f) > 1e-3f) ++slopeRows;
                bool bad = scaleError > 1e-4f;
                if (i + 1 < rows.size() && rows[i + 1].state == 2 && rows[i + 1].dt > 0) {
                    const float moved = std::hypot(rows[i + 1].x - row.x, rows[i + 1].z - row.z);
                    const float predicted = kRunSpeedPerStep * kStepsPerSecond * rows[i + 1].dt * row.scale;
                    const float speedError = std::fabs(moved - predicted);
                    if (moved > 0.5f * predicted) {
                        ++speedRows;
                        worstSpeed = std::max(worstSpeed, speedError);
                        bad = bad || speedError > 2e-3f;
                    }
                }
                if (bad) {
                    ++wrong;
                    std::cerr << segment.name << " row " << i << ": scale " << expected << ", recorded " << row.scale << '\n';
                }
            }
        }
        std::cout << "slopes: " << recordings.size() << " traces, " << runRows << " run rows (" << slopeRows << " on a slope), " << speedRows
                  << " movements, " << wrong << " wrong, worst scale error " << worstScale << ", worst movement error " << worstSpeed << '\n';
        if (recordings.size() < 4 || slopeRows < 4) throw std::runtime_error("too little recorded slope data");
        if (wrong != 0) throw std::runtime_error("slope factor differs from the recorded traces");
    } catch (const std::exception& error) {
        std::cerr << "slope-real-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "slope-real-test: ok\n";
    return 0;
}
