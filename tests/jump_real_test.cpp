// Differential test of the jump (root-motion height profile, ground probe on the way down, landing and stun)
// against traces recorded in the original game (Azahar oracle, tools/oracle/jump-*.txt; docs/re/player.md jump
// entry). Needs the extracted RomFS and the local trace directory; exits with 77 (skipped) when either is missing.
#include "assets/assets.hpp"
#include "player_trace.hpp"
#include "runtime/player_vertical.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

using namespace chase::runtime;
using player_trace::Row;
using player_trace::require;
namespace fs = std::filesystem;

namespace {
constexpr const char* kLevelModel = "models/lvl_01_cherrytreehills/lvl_01_cherrytreehills.bfnmdl";
constexpr const char* kAnimationDirectory = "models/char_minifig/class_police/";
// The landing height is the bisected floor under the box (resolution 0.005, docs/re/collision.md); the box of a jump
// landing depends on the unsnapped height of that update, which is known to about 2e-4 only.
constexpr float kLandingHeightTolerance = 1e-2f;
constexpr std::uint16_t kStateIdle = 1, kStateRun = 2, kStateWalk = 3, kStateJump = 4, kStateSoftLanding = 6;

chase::assets::Animation loadAnimation(chase::assets::FuseArchive& archive, const std::string& name) {
    const chase::assets::FuseEntry* entry = archive.find(kAnimationDirectory + name + "_minifig.bfnanm");
    require(entry != nullptr, "animation not in archive: " + name);
    return chase::assets::parseAnimation(archive.read(*entry));
}

struct Totals {
    std::size_t traces = 0, jumps = 0, updates = 0, landings = 0, stunLandings = 0, directLandings = 0, stunChecks = 0, speedChecks = 0, wrong = 0;
    float worstHeight = 0, worstLanding = 0;
};

bool grounded(std::uint16_t state) { return state == kStateIdle || state == kStateRun || state == kStateWalk; }

void checkJump(const CollisionWorld& world, const JumpProfile& standing, const JumpProfile& moving, const LandingDurations& durations,
               const std::vector<Row>& rows, const std::string& name, Totals& totals) {
    for (std::size_t start = 1; start < rows.size(); ++start) {
        if (!(grounded(rows[start - 1].state) && rows[start].state == kStateJump)) continue;
        ++totals.jumps;
        VerticalState state;
        state.landingDurations = durations;
        state.y = rows[start - 1].y;
        VerticalInput input;
        input.dt = rows[start].dt;
        input.moveRequested = (rows[start].held & 1) != 0;
        input.startJump = input.moveRequested ? &moving : &standing;
        verticalUpdate(state, world, input);
        if (state.posture != Posture::Jumping || std::fabs(state.y - rows[start].y) > 5e-4f) {
            ++totals.wrong;
            std::cerr << name << ": jump did not start at row " << start << '\n';
            continue;
        }
        const Row& takeoffBefore = rows[start - 1];
        const Row& takeoffBeforeThat = rows[start >= 2 ? start - 2 : start - 1];
        const float groundSpeed = std::hypot(takeoffBefore.x - takeoffBeforeThat.x, takeoffBefore.z - takeoffBeforeThat.z);
        const bool sprintBeforeJump = groundSpeed > 1.2f * kCharacterSpeeds.run * rows[start].dt;
        std::size_t i = start + 1;
        for (; i < rows.size() && rows[i].state == kStateJump; ++i) {
            const Row& before = rows[i - 1];
            const Row& after = rows[i];
            input = VerticalInput();
            input.x = after.x;
            input.z = after.z;
            input.forwardX = after.forwardX;
            input.forwardZ = after.forwardZ;
            input.rootDelta = std::hypot(after.x - before.x, after.z - before.z);
            input.dt = after.dt;
            input.moveRequested = (after.held & 1) != 0;
            verticalUpdate(state, world, input);
            ++totals.updates;
            const float error = std::fabs(state.y - after.y);
            totals.worstHeight = std::max(totals.worstHeight, error);
            if (input.moveRequested) {
                const float walk = (after.held & 8) != 0;
                const RootMotionSpeeds speeds = jumpSpeeds(kCharacterSpeeds);
                float expected = (walk ? speeds.walk : speeds.run) * after.dt;
                if (sprintBeforeJump) expected *= speeds.sprintScale;
                ++totals.speedChecks;
                if (std::fabs(input.rootDelta - expected) > 3e-4f) {
                    ++totals.wrong;
                    std::cerr << name << " row " << i << ": horizontal movement " << input.rootDelta << ", expected " << expected << '\n';
                }
            }
            if (state.posture != Posture::Jumping || error > 5e-4f) {
                ++totals.wrong;
                std::cerr << name << " row " << i << ": predicted y " << state.y << " posture " << int(state.posture) << ", recorded y "
                          << after.y << '\n';
                break;
            }
        }
        if (i >= rows.size()) continue;
        const Row& before = rows[i - 1];
        const Row& after = rows[i];
        input = VerticalInput();
        input.x = after.x;
        input.z = after.z;
        input.forwardX = after.forwardX;
        input.forwardZ = after.forwardZ;
        input.rootDelta = std::hypot(after.x - before.x, after.z - before.z);
        input.dt = after.dt;
        input.moveRequested = (after.held & 1) != 0;
        const VerticalStep step = verticalUpdate(state, world, input);
        ++totals.landings;
        const bool expectStun = after.state == kStateSoftLanding;
        const float error = std::fabs(state.y - after.y);
        totals.worstLanding = std::max(totals.worstLanding, error);
        if (!step.landed || (state.posture == Posture::Landing) != expectStun || error > kLandingHeightTolerance ||
            (!expectStun && !grounded(after.state))) {
            ++totals.wrong;
            std::cerr << name << " landing row " << i << ": predicted y " << state.y << " posture " << int(state.posture) << ", recorded y "
                      << after.y << " state " << after.state << '\n';
            continue;
        }
        if (!expectStun) {
            ++totals.directLandings;
            continue;
        }
        ++totals.stunLandings;
        std::size_t end = i;
        while (end < rows.size() && rows[end].state == kStateSoftLanding) ++end;
        if (end >= rows.size() || rows[end].state != kStateIdle) continue;
        std::size_t updates = 1;
        input = VerticalInput();
        input.dt = after.dt;
        while (!verticalUpdate(state, world, input).stunEnded) ++updates;
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
        std::cerr << "usage: jump-real-test FIB NAMES JUMP_TRACE_DIR\n";
        return 2;
    }
    const fs::path fib = argv[1], names = argv[2], directory = argv[3];
    if (!fs::exists(fib) || !fs::exists(names) || !fs::exists(directory)) {
        std::cout << "jump-real-test: data not available, skipped\n";
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
        const JumpProfile standing = makeJumpProfile(loadAnimation(archive, "jump"));
        const JumpProfile moving = makeJumpProfile(loadAnimation(archive, "jump_run"));
        const LandingDurations durations = landingDurations(loadAnimation(archive, "land"), loadAnimation(archive, "fallland"));
        // Standing jump: slot+0x1c of the playing jump animation read in the oracle (trap 0x115ecc at the jump update, slot 0x0856a510,
        // word 0x3f5c6263 = 0.8608762622). Jump run: the scale fitted to the traces, 0.828197 (docs/re/player.md jump entry, fit error
        // below 5e-7 over 22 updates). Derivation: kJumpHeight / extent of the root joint curve (derivation entry, probe over the archive).
        require(std::fabs(standing.heightScale - 0.8608762622f) < 1e-6f && std::fabs(moving.heightScale - 0.828197f) < 1e-5f &&
                    standing.framesPerSecond == 15.0f,
                "jump height scales derived from the animation data match the oracle");

        Totals totals;
        for (const player_trace::NamedTrace& trace : player_trace::loadDirectory(directory)) {
            checkJump(world, standing, moving, durations, trace.rows, trace.name, totals);
            ++totals.traces;
        }
        std::cout << "jumps: " << totals.traces << " traces, " << totals.jumps << " jumps, " << totals.updates << " airborne updates, "
                  << totals.landings << " landings (" << totals.stunLandings << " with stun, " << totals.directLandings
                  << " straight into locomotion), " << totals.stunChecks << " stun lengths, " << totals.speedChecks << " horizontal speeds, " << totals.wrong
                  << " wrong, worst airborne height error " << totals.worstHeight << ", worst landing height error "
                  << totals.worstLanding << '\n';
        require(totals.traces >= 10 && totals.jumps >= 10 && totals.stunLandings >= 5 && totals.directLandings >= 3 && totals.speedChecks >= 50,
                "too little recorded jump data");
        require(totals.wrong == 0, "jump differs from the recorded traces");
    } catch (const std::exception& error) {
        std::cerr << "jump-real-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "jump-real-test: ok\n";
    return 0;
}
