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
    std::size_t traces = 0, updates = 0, starts = 0, landings = 0, hardLandings = 0, softStuns = 0, stunChecks = 0, stepOffs = 0, wrong = 0, normalChecks = 0, wrongNormals = 0, otherTriangle = 0;
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
        if (!expectFall && (wasAirborne || input.rootDelta > 0)) {
            ++totals.normalChecks;
            float normalError = 0;
            for (int a = 0; a < 3; ++a) normalError = std::max(normalError, std::fabs(state.groundNormal[a] - after.normal[a]));
            if (normalError > 1e-5f) ++totals.otherTriangle;
            if (normalError > 1e-5f) {
                ++totals.wrongNormals;
                std::cerr << name << " row " << i << ": ground normal " << state.groundNormal[0] << ' ' << state.groundNormal[1] << ' '
                          << state.groundNormal[2] << ", recorded " << after.normal[0] << ' ' << after.normal[1] << ' ' << after.normal[2] << '\n';
            }
        }
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

void checkStun(const std::vector<Row>& rows, const std::string& name, const LandingAnimations& animations, Totals& totals) {
    for (std::size_t i = 1; i < rows.size(); ++i) {
        const bool soft = rows[i].state == kStateSoftLanding, hard = rows[i].state == kStateHardLanding;
        if (!((soft || hard) && airborne(rows[i - 1].state))) continue;
        std::size_t end = i;
        while (end < rows.size() && rows[end].state == rows[i].state) ++end;
        if (end >= rows.size() || rows[end].state != kStateIdle) continue;
        VerticalState state;
        state.posture = Posture::Landing;
        state.landingAnimation = hard ? animations.hard : animations.soft;
        VerticalInput input;
        input.dt = rows[i].dt;
        input.ticks = chase::runtime::stepTicks(rows[i].dt);
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
        const LandingAnimations animations = landingAnimations(loadAnimation("land"), loadAnimation("fallland"));
        // land has 14 frames and fallland 19, both at 15 frames per second (chase_assets decode of class_police; the oracle
        // slot read of the landing entry in docs/re/player.md gives the same last frames and rate).
        require(animations.soft.lastFrame == 13.0f && animations.hard.lastFrame == 18.0f && animations.soft.framesPerSecond == 15.0f &&
                    animations.hard.framesPerSecond == 15.0f,
                "landing animations: last frames 13 and 18 at 15 frames per second");

        Totals totals;
        for (const player_trace::NamedTrace& trace : player_trace::loadDirectory(directory)) {
            checkUpdates(world, trace.rows, trace.name, totals);
            checkStun(trace.rows, trace.name, animations, totals);
            ++totals.traces;
        }
        std::cout << "falls: " << totals.traces << " traces, " << totals.updates << " updates, " << totals.starts << " fall starts, "
                  << totals.landings << " landings (" << totals.hardLandings << " hard, " << totals.softStuns << " soft with stun), "
                  << totals.stunChecks << " stun lengths, " << totals.stepOffs << " traces cut at the step-off state, " << totals.wrong << " wrong, worst height error " << totals.worstHeight
                  << ", worst fall speed error " << totals.worstSpeed << ", " << totals.normalChecks << " ground normals (" << totals.wrongNormals << " wrong; recorded normals compared to 1e-5, provenance docs/re/player-falling-jumping.md PLAYER-4 entry)\n";
        require(totals.traces >= 12 && totals.landings >= 8 && totals.hardLandings >= 3 && totals.starts >= 1,
                "too little recorded fall data");
        require(totals.wrong == 0 && totals.wrongNormals == 0, "fall or landing differs from the recorded traces");
    } catch (const std::exception& error) {
        std::cerr << "fall-real-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "fall-real-test: ok\n";
    return 0;
}
