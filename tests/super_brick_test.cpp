// Tests of the placed super build bricks (src/runtime/super_brick.*), docs/re/super-bricks.md.
// Read in llvm-objdump of artifacts/game/game.arm.elf: the update 0x1acc10 (pickup test at 0x1acdd0..0x1acde8, collect 0x1acdf0..0x1ace44, spin
// 0x1aced0..0x1acf44, countdown 0x1acf80), the destroy 0x1d737c (type 103 at 0x1d7550) and 0x2144f8 / 0x262778.
// Oracle (Azahar, user state m13-1-roof-superbrick, New 3DS, port 45692, dt fixed 0.033, 2026-10-19): the player walked with stick (-82, 88) to the
// brick 1-3_Rainbow_02 at (904.9294, 30.6969, 1133.0990); `ctrace 0x1acdf8,0x1d7560` stopped at the collect in frame 6 and at the destroy in frame 37
// (r4 = 0xc8 = 200); the u32 at 0x593430 went from 96 to 296 (memory diff of the states and a watchpoint, write at 0x262774 called from 0x1d7560).
// The player rows of the same run (`trace`, frames 5 and 6) are below. Real data: the levels of the FIB, counted with the loaders of this port.
#include "assets/assets.hpp"
#include "assets/object_classes.hpp"
#include "runtime/super_brick.hpp"

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>

namespace {

using namespace chase;
using namespace chase::runtime;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

bool near(float a, float b) { return std::fabs(a - b) <= 1e-4f; }

constexpr std::array<float, 3> kBrick = {904.9293823f, 30.6968994f, 1133.0989990f};  // 1-3_Rainbow_02, level data
constexpr std::array<float, 3> kPlayerFrame5 = {905.40906f, 30.71917f, 1131.55151f};  // oracle trace row 5: distance 1.620
constexpr std::array<float, 3> kPlayerFrame6 = {905.33545f, 30.71917f, 1131.76697f};  // oracle trace row 6: distance 1.393
constexpr float kOracleDt = 0.033f;                                                    // the dt column of the trace

const auto kAlways = [](std::size_t) { return true; };

void pickupRule() {
    SuperBrickField field;
    field.add("1-3_Rainbow_02", kBrick, 200);
    require(field.update(kOracleDt, kPlayerFrame5, false, kAlways).collected.empty(), "at 1.62 the brick is not picked up (frame 5)");
    const SuperBrickStep step = field.update(kOracleDt, kPlayerFrame6, false, kAlways);
    require(step.collected.size() == 1 && step.collected[0].index == 0, "at 1.393 it is picked up (frame 6)");
    require(field.bricks()[0].state == SuperBrickState::Collected && near(field.bricks()[0].timer, kSuperBrickCollectTime), "state 3 with the timer 1.0");
    require(field.total() == 0, "nothing is added at the pickup");

    SuperBrickField blocked;
    blocked.add("b", kBrick, 200);
    require(blocked.update(kOracleDt, kPlayerFrame6, true, kAlways).collected.empty(), "the player's entity word +0xc bit 4 blocks the test (0x1acce4)");
    SuperBrickField disabled;
    disabled.add("d", kBrick, 200);
    require(disabled.update(kOracleDt, kPlayerFrame6, false, [](std::size_t) { return false; }).collected.empty(), "a disabled entity does not update");
    SuperBrickField edge;
    edge.add("e", {0, 0, 0}, 1);
    require(edge.update(kOracleDt, {1.5f, 0, 0}, false, kAlways).collected.empty(), "the compare with 0x3fc00000 is `at or above 1.5 skips`");
    require(edge.update(kOracleDt, {1.4999f, 0, 0}, false, kAlways).collected.size() == 1, "just below 1.5 picks up");
}

void countdownAndAdd() {
    SuperBrickField field;
    field.add("1-3_Rainbow_02", kBrick, 200);
    field.update(kOracleDt, kPlayerFrame6, false, kAlways);
    unsigned updates = 0;
    SuperBrickStep step;
    while (step.banked.empty() && updates < 100) {
        step = field.update(kOracleDt, kPlayerFrame6, false, kAlways);
        ++updates;
    }
    require(updates == 31, "the destroy runs 31 updates after the pickup (oracle frames 6 and 37), got " + std::to_string(updates));
    require(step.banked.size() == 1 && step.banked[0].value == 200 && step.banked[0].total == 200, "200 is added");
    require(step.banked[0].sound == kSuperBrickSoundBig, "value 200 plays index 14 (0x1d7580: 10 or more)");
    require(field.bricks()[0].state == SuperBrickState::Done && field.total() == 200, "state 0 afterwards");
    require(field.update(kOracleDt, kPlayerFrame6, false, kAlways).banked.empty(), "a done brick adds nothing again");

    SuperBrickField small;
    small.add("s", {0, 0, 0}, 9);
    small.add("t", {0, 0, 0}, 10);
    small.update(1.0f / 30.0f, {0, 0, 0}, false, kAlways);
    SuperBrickStep both;
    for (unsigned i = 0; i < 40; ++i) {
        const SuperBrickStep next = small.update(1.0f / 30.0f, {0, 0, 0}, false, kAlways);
        for (const SuperBrickBanked& banked : next.banked) both.banked.push_back(banked);
    }
    require(both.banked.size() == 2 && both.banked[0].sound == kSuperBrickSoundSmall && both.banked[1].sound == kSuperBrickSoundBig, "9 plays index 2, 10 plays index 14");
    require(small.total() == 19, "9 + 10");
}

void spinAndFade() {
    const std::array<float, 16> start = superBrickMatrix(kBrick, 0);
    require(near(start[0], 1) && near(start[2], 0) && near(start[8], 0) && near(start[10], 1), "angle 0 is the identity rotation");
    require(near(start[12], kBrick[0]) && near(start[13], kBrick[1]) && near(start[14], kBrick[2]) && near(start[15], 1), "the translation is kept");
    const std::array<float, 16> half = superBrickMatrix(kBrick, 1500);
    require(near(half[0], 0) && near(half[2], 1) && near(half[8], -1) && near(half[10], 0), "after 0.5 s (1500 ticks) the angle is -pi/2: row 0 (0, 0, 1)");
    const std::array<float, 16> turn = superBrickMatrix(kBrick, 3000);
    require(near(turn[0], -1) && near(turn[10], -1), "one second is half a turn (-pi)");
    require(near(superBrickAlpha(1.0f), 1.0f) && near(superBrickAlpha(0.5f), 127.0f / 255.0f) && near(superBrickAlpha(-0.2f), 0.0f), "alpha is int(timer * 255) / 255");
}

constexpr const char* kLevelFile = "levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl";

void realData(const char* fib, const char* names) {
    assets::FuseArchive archive(fib);
    archive.loadNames(names);
    const assets::Level level = assets::parseLevel(archive.read(*archive.find(kLevelFile)));
    const assets::DrawableObjects drawable = assets::drawableObjects(archive, level);
    require(drawable.superBricks.size() == 8, "lvl_01 places 8 super build bricks, got " + std::to_string(drawable.superBricks.size()));
    std::map<std::string, std::uint16_t> values;
    for (const assets::SuperBrickObject& brick : drawable.superBricks) {
        values[brick.instance] = brick.value;
        require(brick.placedIndex < drawable.objects.size() && drawable.objects[brick.placedIndex].instance == brick.instance, "a super brick is also a drawn model");
    }
    require(values.at("1-3_Rainbow_02") == 200, "1-3_Rainbow_02 has SuperBuildValue 200 (instance 0x43480000)");
    require(values.at("FP_Superbrick_01") == 1500 && values.at("FP_Superbrick_03") == 1500, "the 2x4 freeplay bricks 1500");
    require(values.at("FP_Superbrick_02") == 500 && values.at("FP_Superbrick_07") == 500, "the 2x2 freeplay bricks 500");
    for (const assets::SuperBrickObject& brick : drawable.superBricks)
        if (brick.instance == "1-3_Rainbow_02")
            require(near(brick.position[0], kBrick[0]) && near(brick.position[1], kBrick[1]) && near(brick.position[2], kBrick[2]), "the level position");
    const assets::ClassLookup classOf = assets::archiveClassLookup(archive);
    for (const assets::LevelObject& object : level.objects) {
        const bool brick = assets::isSuperBuildBrickObject(object, *classOf(object.className));
        require(brick == (object.className == "pickups/pickup_sbbrick2x2" || object.className == "pickups/pickup_sbbrick2x4"), "only the sbbrick classes of lvl_01 are super bricks: " + object.className);
    }
}

}

int main(int argc, char** argv) {
    try {
        pickupRule();
        countdownAndAdd();
        spinAndFade();
        if (argc == 3) {
            if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
                std::cout << "super-brick-test: data not available, skipped\n";
                return 77;
            }
            realData(argv[1], argv[2]);
        }
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
