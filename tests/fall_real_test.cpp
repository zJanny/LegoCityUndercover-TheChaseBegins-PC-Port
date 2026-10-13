// Differential test of falling and landing against traces recorded in the original game (Azahar oracle,
// tools/oracle/fall-*.txt; docs/re/player.md falling entry). Needs the extracted RomFS and the local trace
// directory; exits with 77 (skipped) when either is missing.
#include "assets/assets.hpp"
#include "player_trace.hpp"
#include "runtime/player_vertical.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace chase::runtime;
using player_trace::Row;
using player_trace::require;
namespace fs = std::filesystem;

namespace {
constexpr const char* kLevelModel = "models/lvl_01_cherrytreehills/lvl_01_cherrytreehills.bfnmdl";
constexpr std::uint16_t kStateIdle = 1, kStateRun = 2, kStateWalk = 3, kStateFall = 5, kStateSoftLanding = 6, kStateHardLanding = 7,
                        kStateStepOff = 8;

struct Totals {
    std::size_t traces = 0, updates = 0, starts = 0, landings = 0, hardLandings = 0, softStuns = 0, stunChecks = 0, stepOffs = 0, wrong = 0;
    float worstHeight = 0, worstSpeed = 0;
};

bool airborne(std::uint16_t state) { return state == kStateFall; }
bool grounded(std::uint16_t state) { return state == kStateIdle || state == kStateRun || state == kStateWalk; }

void checkUpdates(const CollisionWorld& world, const std::vector<Row>& rows, const std::string& name, Totals& totals) {
    for (std::size_t i = 1; i < rows.size(); ++i) {
        const Row& before = rows[i - 1];
        const Row& after = rows[i];
        if (after.state == kStateStepOff) {
            ++totals.stepOffs;
            break;
        }
        const bool wasAirborne = airborne(before.state);
        if (!wasAirborne && !grounded(before.state)) continue;
        VerticalState state;
        state.posture = wasAirborne ? Posture::Falling : Posture::Ground;
        state.y = before.y;
        state.fallSpeed = wasAirborne ? before.fallSpeed : 0;
        state.fallDistance = wasAirborne ? before.fallDistance : 0;
        VerticalInput input;
        input.x = after.x;
        input.z = after.z;
        input.forwardX = after.forwardX;
        input.forwardZ = after.forwardZ;
        input.rootDelta = std::hypot(after.x - before.x, after.z - before.z);
        input.dt = after.dt;
        input.moveRequested = (after.held & 1) != 0;
        const VerticalStep step = verticalUpdate(state, world, input);
        ++totals.updates;

        const bool expectFall = airborne(after.state);
        const bool expectLanding = after.state == kStateSoftLanding || after.state == kStateHardLanding;
        const bool landed = step.landed;
        const bool expectLanded = wasAirborne && !expectFall;
        const float heightError = std::fabs(state.y - after.y);
        const float speedError = expectFall ? std::fabs(state.fallSpeed - after.fallSpeed) : 0;
        totals.worstHeight = std::max(totals.worstHeight, heightError);
        totals.worstSpeed = std::max(totals.worstSpeed, speedError);
        if (!wasAirborne && expectFall) ++totals.starts;
        if (expectLanded) {
            ++totals.landings;
            if (after.state == kStateHardLanding) ++totals.hardLandings;
            if (after.state == kStateSoftLanding) ++totals.softStuns;
        }
        bool wrong = (state.posture == Posture::Falling) != expectFall || landed != expectLanded || heightError > 2e-4f || speedError > 1e-4f;
        if (expectFall && std::fabs(state.fallDistance - after.fallDistance) > 1e-3f) wrong = true;
        if (expectLanded && (step.hard != (after.state == kStateHardLanding) || (state.posture == Posture::Landing) != expectLanding)) wrong = true;
        if (!wrong) continue;
        ++totals.wrong;
        std::cerr << name << " row " << i << ": predicted y " << state.y << " speed " << state.fallSpeed << " posture "
                  << int(state.posture) << ", recorded y " << after.y << " speed " << after.fallSpeed << " state " << after.state << '\n';
    }
}

void checkStun(const std::vector<Row>& rows, const std::string& name, const LandingDurations& durations, Totals& totals) {
    for (std::size_t i = 1; i < rows.size(); ++i) {
        const bool soft = rows[i].state == kStateSoftLanding, hard = rows[i].state == kStateHardLanding;
        if (!((soft || hard) && airborne(rows[i - 1].state))) continue;
        std::size_t end = i;
        while (end < rows.size() && rows[end].state == rows[i].state) ++end;
        if (end >= rows.size() || rows[end].state != kStateIdle) continue;
        VerticalState state;
        state.posture = Posture::Landing;
        state.landingDuration = hard ? durations.hard : durations.soft;
        VerticalInput input;
        input.dt = rows[i].dt;
        std::size_t updates = 1;
        while (true) {
            const VerticalStep step = verticalUpdate(state, CollisionWorld(), input);
            if (step.stunEnded) break;
            ++updates;
        }
        ++totals.stunChecks;
        if (updates != end - i) {
            ++totals.wrong;
            std::cerr << name << " stun from row " << i << ": predicted " << updates << " updates, recorded " << end - i << '\n';
        }
    }
}
}

int main(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "usage: fall-real-test FIB NAMES FALL_TRACE_DIR\n";
        return 2;
    }
    const fs::path fib = argv[1], names = argv[2], directory = argv[3];
    if (!fs::exists(fib) || !fs::exists(names) || !fs::exists(directory)) {
        std::cout << "fall-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        chase::assets::FuseArchive archive(fib.string());
        archive.loadNames(names.string());
        const chase::assets::FuseEntry* entry = archive.find(kLevelModel);
        require(entry != nullptr, std::string("not in archive: ") + kLevelModel);
        const chase::assets::Model model = chase::assets::parseModel(archive.read(*entry));
        CollisionWorld world;
        for (const auto& item : model.collision) world.add(item);

        const auto loadAnimation = [&](const std::string& name) {
            const chase::assets::FuseEntry* animationEntry = archive.find("models/char_minifig/class_police/" + name + "_minifig.bfnanm");
            require(animationEntry != nullptr, "animation not in archive: " + name);
            return chase::assets::parseAnimation(archive.read(*animationEntry));
        };
        const LandingDurations durations = landingDurations(loadAnimation("land"), loadAnimation("fallland"));
        // Landing animation lengths: land 14 frames, fallland 19 frames at 15 frames per second (probe at the derivation entry of
        // docs/re/player.md); the recorded stuns are 26/30 s and 36/30 s.
        require(std::fabs(durations.soft - 26.0f / 30.0f) < 1e-5f && std::fabs(durations.hard - 36.0f / 30.0f) < 1e-5f,
                "landing stun lengths derived from the landing animations");

        Totals totals;
        for (const player_trace::NamedTrace& trace : player_trace::loadDirectory(directory)) {
            checkUpdates(world, trace.rows, trace.name, totals);
            checkStun(trace.rows, trace.name, durations, totals);
            ++totals.traces;
        }
        std::cout << "falls: " << totals.traces << " traces, " << totals.updates << " updates, " << totals.starts << " fall starts, "
                  << totals.landings << " landings (" << totals.hardLandings << " hard, " << totals.softStuns << " soft with stun), "
                  << totals.stunChecks << " stun lengths, " << totals.stepOffs << " traces cut at the step-off state, " << totals.wrong << " wrong, worst height error " << totals.worstHeight
                  << ", worst fall speed error " << totals.worstSpeed << '\n';
        require(totals.traces >= 12 && totals.landings >= 8 && totals.hardLandings >= 3 && totals.starts >= 1,
                "too little recorded fall data");
        require(totals.wrong == 0, "fall or landing differs from the recorded traces");
    } catch (const std::exception& error) {
        std::cerr << "fall-real-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "fall-real-test: ok\n";
    return 0;
}
