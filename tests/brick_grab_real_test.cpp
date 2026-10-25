// The climbing bricks of mission 1-2 in lvl_01_cherrytreehills (src/runtime/brick_grab.cpp): the bricks 1-2_Brick_Grab_08 and _07 from
// the level data, then a replay of the climb recorded in the oracle from the save state climbing the bricks of mission 1-2 (docs/re/player-traversal.md,
// entry of 2026-10-18 on the climbing bricks): scheduler step fixed to 1/30 (`dt 0.033333335 0.033333335`), the player hanging on 08 in
// state 145; run `up`: stick (0, 120) from update 4 (state 146 in that update), B in update 16 (state 149 to 07), `trace` rows below; run
// `drop`: A in update 3 (state 5), landing at y 64.4281, B in update 49, state 141 on 08 in update 55. Needs the extracted RomFS; exits
// with 77 without it.
#include "assets/assets.hpp"
#include "runtime/brick_grab.hpp"
#include <cmath>
#include <filesystem>
#include <iostream>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
constexpr const char* kLevelFile = "levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl";
constexpr const char* kClipDirectory = "models/char_minifig/class_generic/";

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

bool near(float a, float b, float tolerance) { return std::fabs(a - b) <= tolerance; }

int find(const std::vector<chase::runtime::GrabBrick>& bricks, const std::string& name) {
    for (std::size_t i = 0; i < bricks.size(); ++i)
        if (bricks[i].instance == name) return int(i);
    return -1;
}
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: brick-grab-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "brick-grab-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        using namespace chase;
        using runtime::BrickState;
        assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        const assets::FuseEntry* entry = archive.find(kLevelFile);
        require(entry != nullptr, std::string("not in archive: ") + kLevelFile);
        std::vector<runtime::GrabBrick> bricks = runtime::loadGrabBricks(assets::parseLevel(archive.read(*entry)), assets::archiveClassLookup(archive));
        // Oracle read in the save state climbing the bricks of mission 1-2: the brick list 0x37718c holds 35 entries.
        require(bricks.size() == 35, "bricks: " + std::to_string(bricks.size()));
        const int low = find(bricks, "1-2_brick_grab_08"), high = find(bricks, "1-2_brick_grab_07");
        require(low >= 0 && high >= 0, "1-2_Brick_Grab_07 and 08 are bricks");
        // Oracle read of 08 (0x08a92010): +0x98 = 07 (0x08a91d00), +0xc2 = 0; 07: no adjacent brick, +0xc2 = 0.
        require(bricks[std::size_t(low)].adjacent[0] == high, "AdjacentBrick01 of 08 is 07");
        for (const int adjacent : bricks[std::size_t(high)].adjacent) require(adjacent < 0, "07 has no adjacent brick");
        require(!bricks[std::size_t(low)].topBrick && !bricks[std::size_t(high)].topBrick, "neither is a top brick");

        std::map<int, runtime::BrickClip> clips;
        std::map<int, assets::RootMotion> motions;
        for (const int role : {runtime::kAnimationBrickGrab, runtime::kAnimationBrickGrabUp, runtime::kAnimationBrickJumpUp}) {
            const char* name = role == runtime::kAnimationBrickGrab ? "brickgrab_grab" : role == runtime::kAnimationBrickGrabUp ? "brickgrab_grabup" : "brickgrab_up";
            const assets::FuseEntry* clipEntry = archive.find(std::string(kClipDirectory) + name + "_minifig.bfnanm");
            require(clipEntry != nullptr, std::string("clip missing: ") + name);
            const assets::Animation clip = assets::parseAnimation(archive.read(*clipEntry));
            const float rate = assets::animationFramesPerSecond(clip);
            if (const std::optional<assets::RootMotion> motion = assets::rootMotion(clip)) motions[role] = *motion;
            clips[role] = {unsigned(std::ceil(float(assets::animationLength(clip)) * 30.0f / rate - 1e-3f)), nullptr, rate};
        }
        clips[runtime::kAnimationBrickJumpUp].motion = &motions.at(runtime::kAnimationBrickJumpUp);
        require(clips[runtime::kAnimationBrickGrab].updates == 22, "brickgrab_grab: 11 frames = 22 updates (state 141 in updates 55..76)");
        require(clips[runtime::kAnimationBrickGrabUp].updates == 14, "brickgrab_grabup: 7 frames = 14 updates (state 142 in updates 32..45)");
        require(clips[runtime::kAnimationBrickJumpUp].updates == 16, "brickgrab_up: 8 frames = 16 updates (state 149 in updates 16..31)");

        // Run `drop`: standing on the ground at y 64.4281 the box is far below 08; at the jump height of update 54 it overlaps.
        const float ground[3] = {908.8964f, 64.4281f, 673.1982f};
        require(runtime::brickGrabTest(bricks, ground, 22209, {}) == -1, "no brick from the ground");
        const float jumping[3] = {908.8964f, 65.7799f, 673.1982f};
        require(runtime::brickGrabTest(bricks, jumping, 22209, {}) == low, "the jump height of update 54 reaches 08");

        runtime::BrickClimber climber;
        runtime::BrickInput input;
        input.clips = clips;
        runtime::BrickStep step = runtime::brickGrabEnter(climber, bricks, low);
        // State 141 in update 55 of run `drop`: node (908.9421, 65.6976, 673.1696), ctrl+8 = 0x56c1.
        require(near(step.position[0], 908.9421f, 2e-4f) && near(step.position[1], 65.6976f, 2e-4f) && near(step.position[2], 673.1696f, 2e-4f),
                "hanging position under 08");
        require(step.facing == 22209, "hanging heading 22209, got " + std::to_string(step.facing));
        unsigned updates = 0;
        do {
            std::copy(step.position, step.position + 3, input.position);
            input.facing = step.facing;
            step = runtime::brickGrabUpdate(climber, bricks, input);
            ++updates;
        } while (climber.state == BrickState::Grab);
        require(climber.state == BrickState::Idle && updates == 22, "state 145 22 updates after the grab (update 77 of run `drop`)");

        // Run `up`: ctrl+6 = 0x57ff while the stick is held.
        input.stickHeld = true;
        input.stickHeading = 0x57ff;
        step = runtime::brickGrabUpdate(climber, bricks, input);
        require(climber.state == BrickState::ReachUp, "state 146 in the first stick update");
        input.jumpPressed = true;
        step = runtime::brickGrabUpdate(climber, bricks, input);
        require(climber.state == BrickState::JumpUp && climber.target == high, "B: state 149 towards 07 in the same update");
        input.jumpPressed = false;
        // Node y of updates 17..31 of run `up`.
        const float heights[] = {65.6976f, 65.6976f, 65.8505f, 66.0035f, 66.1571f, 66.3107f, 66.4636f, 66.6165f,
                                 66.7702f, 66.9238f, 67.0767f, 67.2296f, 67.3832f, 67.5368f, 67.6898f};
        for (const float y : heights) {
            step = runtime::brickGrabUpdate(climber, bricks, input);
            require(climber.state == BrickState::JumpUp, "still jumping");
            require(near(step.position[1], y, 2e-4f) && near(step.position[0], 908.9421f, 2e-4f) && near(step.position[2], 673.1696f, 2e-4f),
                    "jump height " + std::to_string(step.position[1]) + " expected " + std::to_string(y));
        }
        step = runtime::brickGrabUpdate(climber, bricks, input);
        require(climber.state == BrickState::GrabUp && climber.brick == high && near(step.position[1], 67.8427f, 2e-4f), "update 32: state 142 under 07");
        updates = 0;
        do {
            step = runtime::brickGrabUpdate(climber, bricks, input);
            ++updates;
        } while (climber.state == BrickState::GrabUp);
        require(climber.state == BrickState::Idle && updates == 14, "update 46: state 145");
        step = runtime::brickGrabUpdate(climber, bricks, input);
        require(climber.state == BrickState::ReachUp, "update 47: state 146");
        input.jumpPressed = true;
        step = runtime::brickGrabUpdate(climber, bricks, input);
        require(!climber.active() && step.exit == runtime::BrickExit::Jump, "B on 07 (no adjacent brick): state 4 (run `top`, update 37)");
    } catch (const std::exception& error) {
        std::cerr << "brick-grab-real-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "brick-grab-real-test: ok\n";
    return 0;
}
