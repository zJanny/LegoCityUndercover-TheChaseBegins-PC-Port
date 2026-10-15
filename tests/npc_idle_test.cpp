// The game's random generator 0x25b2b8 and the idle state of level characters (docs/re/npc-animation.md).
// Provenance of the recorded values: oracle state m11-1-donut-shop, `ctrace 0x25b304 30 rng2` (the word returned by every call of
// 0x25b2b8 in 30 stepped updates, 134 calls; the first 22 are pinned); the clip lengths and rates are the slot fields of the idle
// animations of 1-1_AI_DonutShopMan read in the same state (frames 0x13 and 0x45, rate 15.0); the 42 frames of the looping
// animation are the end frame of 1-1_AI_DonutLady's slot in m11-3-repair-van-marker (docs/re/npc-animation.md).
#include "runtime/game_random.hpp"
#include "runtime/npc_idle.hpp"

#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace chase::runtime;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

const std::array<std::uint32_t, 22> kRecorded = {0xc8f8ff7d, 0x967fbe3a, 0xce6a648a, 0xafb35d2a, 0xf2497c73, 0xe45d28c0, 0xb95e42b4, 0xbc9abc67,
                                                 0x2876521b, 0x11950351, 0x4f1990a6, 0xda13df85, 0xd6e737d3, 0x3e4539f4, 0x6270cc8b, 0xd5fe7470,
                                                 0xaf91608f, 0x43be9ad0, 0xb574df64, 0xfaec4f77, 0x7ed1ea2a, 0x8c52e98d};

constexpr AnimationClipInfo kIdle{0, 19, 15, true};
constexpr AnimationClipInfo kIdle2{1, 69, 15, true};
constexpr std::int64_t kTicks = kAnimationClockTicksPerSecond;

GameRandom recordedGenerator() {
    std::array<std::uint32_t, kGameRandomTableSize> table{};
    for (unsigned k = 0; k < kGameRandomTableSize; ++k) table[10 - k] = kRecorded[k];
    return GameRandom(table, 10, 6);
}

void testGeneratorContinuesRecordedSequence() {
    GameRandom random = recordedGenerator();
    for (unsigned k = 11; k < kRecorded.size(); ++k) require(random.next() == kRecorded[k], "recorded word " + std::to_string(k));
}

void testSeedExpansion() {
    GameRandom random(1);
    require(random.table()[0] == 270369u, "xorshift32 of 1");
    require(random.firstCursor() == 0 && random.secondCursor() == kGameRandomSecondCursorStart, "cursors");
    GameRandom zero(0);
    GameRandom allOnes(0xffffffffu);
    require(zero.table() == allOnes.table(), "seed 0 is 0xffffffff");
}

void testCursorsWrapFromZeroToTen() {
    GameRandom random(7);
    for (unsigned step = 0; step < kGameRandomTableSize; ++step) random.next();
    require(random.firstCursor() == 0 && random.secondCursor() == kGameRandomSecondCursorStart, "eleven steps return the cursors");
}

void testFirstUpdateStartsAnAnimation() {
    GameRandom random = recordedGenerator();
    AnimationSlots slots;
    NpcIdle idle;
    require(!idle.started(), "not started");
    require(idle.update(slots, 0, random, kIdle, kIdle2), "starts");
    require(idle.started() && slots.count() == 1 && slots.current()->start == 0, "started at 0");
    const bool second = (kRecorded[11] & kIdleRandomBit) != 0;
    require((idle.variant() == NpcIdleVariant::Idle2) == second, "variant from bit 2 of the first word");
    require(slots.current()->clip == (second ? 1u : 0u) && slots.current()->mode == kAnimationOnce && slots.current()->first == 0 &&
                slots.current()->end == (second ? 69 : 19) && slots.current()->blend == kIdleBlend,
            "mode 0, frame 0 to the length, blend 0.1 (0x21941c)");
}

void testVariantsFollowTheRandomBit() {
    GameRandom random = recordedGenerator();
    AnimationSlots slots;
    NpcIdle idle;
    std::int64_t now = 0;
    for (unsigned k = 11; k < kRecorded.size(); ++k) {
        now += 10 * kTicks;
        require(idle.update(slots, now, random, kIdle, kIdle2), "a finished clip restarts");
        const bool second = (kRecorded[k] & kIdleRandomBit) != 0;
        require((idle.variant() == NpcIdleVariant::Idle2) == second, "variant " + std::to_string(k));
    }
}

void testIdleLastsItsFrameCount() {
    GameRandom random = recordedGenerator();
    AnimationSlots slots;
    NpcIdle idle;
    idle.update(slots, 0, random, kIdle, kIdle2);
    const AnimationClipInfo& clip = idle.variant() == NpcIdleVariant::Idle ? kIdle : kIdle2;
    const std::int64_t length = std::int64_t(clip.length) * kTicks / clip.rate;
    const std::int64_t step = 100;
    std::int64_t now = 0;
    while (true) {
        now += step;
        if (idle.update(slots, now, random, kIdle, kIdle2)) break;
        require(now < 1000 * kTicks, "never finished");
    }
    require(now >= length && now - length < step, "restarts at the first update after the clip length");
    require(slots.current()->start == now && slots.count() == 2, "new animation starts at that update in a new slot");
}

void testPositionIsRateTimesTime() {
    GameRandom random = recordedGenerator();
    AnimationSlots slots;
    NpcIdle idle;
    idle.update(slots, 0, random, kIdle, kIdle2);
    idle.update(slots, 600, random, kIdle, kIdle2);
    require(std::abs(AnimationSlots::position(*slots.current(), 600).frame - 3.0f) < 1e-5f, "15 frames per second");
}

void testDegenerateClipRestartsEveryUpdate() {
    GameRandom random = recordedGenerator();
    AnimationSlots slots;
    NpcIdle idle;
    const AnimationClipInfo empty{2, 0, 15, false};
    idle.update(slots, 0, random, empty, empty);
    require(idle.update(slots, 100, random, empty, empty), "an empty clip is finished at once");
}

void testScriptedLoopKeepsIdleAway() {
    GameRandom random = recordedGenerator();
    AnimationSlots slots;
    NpcIdle idle;
    const AnimationClipInfo stuck{3, 42, 15, true};
    slots.start(stuck, kAnimationLoop, 0, kAnimationFrameDefault, 1.0f, 0.1f, 0);
    for (std::int64_t now = 100; now < 60 * kTicks; now += 100)
        require(!idle.update(slots, now, random, kIdle, kIdle2), "a looping animation is never finished (kind 1)");
    slots.stop(60 * kTicks);
    require(idle.update(slots, 60 * kTicks + 100, random, kIdle, kIdle2), "after StopCharAnim the idle state starts again");
}
}

int main() {
    try {
        testGeneratorContinuesRecordedSequence();
        testSeedExpansion();
        testCursorsWrapFromZeroToTen();
        testFirstUpdateStartsAnAnimation();
        testVariantsFollowTheRandomBit();
        testIdleLastsItsFrameCount();
        testPositionIsRateTimesTime();
        testDegenerateClipRestartsEveryUpdate();
        testScriptedLoopKeepsIdleAway();
    } catch (const std::exception& error) {
        std::cerr << "npc-idle-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "npc-idle-test ok\n";
    return 0;
}
