// The game's random generator 0x25b2b8 and the idle state of level characters (docs/re/npc-animation.md).
// Provenance of the recorded values: oracle state m11-1-donut-shop, `ctrace 0x25b304 30 rng2` (the word returned by every call of
// 0x25b2b8 in 30 stepped updates, 134 calls; the first 22 are pinned); the clip lengths and rates are the slot fields of the idle
// animations of 1-1_AI_DonutShopMan read in the same state (frames 0x13 and 0x45, rate 15.0).
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

constexpr NpcIdleClip kIdle{19, 15};
constexpr NpcIdleClip kIdle2{69, 15};

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
    NpcIdle idle;
    require(!idle.started(), "not started");
    idle.update(0.0, random, kIdle, kIdle2);
    require(idle.started() && idle.elapsedSeconds() == 0.0, "started at 0");
    const bool second = (kRecorded[11] & kIdleRandomBit) != 0;
    require((idle.variant() == NpcIdleVariant::Idle2) == second, "variant from bit 2 of the first word");
}

void testVariantsFollowTheRandomBit() {
    GameRandom random = recordedGenerator();
    NpcIdle idle;
    for (unsigned k = 11; k < kRecorded.size(); ++k) {
        idle.update(10.0, random, kIdle, kIdle2);
        const bool second = (kRecorded[k] & kIdleRandomBit) != 0;
        require((idle.variant() == NpcIdleVariant::Idle2) == second, "variant " + std::to_string(k));
    }
}

void testIdleLastsItsFrameCount() {
    GameRandom random = recordedGenerator();
    NpcIdle idle;
    idle.update(0.0, random, kIdle, kIdle2);
    const NpcIdleVariant first = idle.variant();
    const NpcIdleClip& clip = first == NpcIdleVariant::Idle ? kIdle : kIdle2;
    const double seconds = clip.frames / clip.framesPerSecond;
    const double dt = 0.1;
    unsigned updates = 0;
    while (true) {
        const unsigned before = random.firstCursor();
        idle.update(dt, random, kIdle, kIdle2);
        ++updates;
        if (random.firstCursor() != before) break;
        require(updates < 1000, "never finished");
    }
    require(std::abs(double(updates) * dt - seconds) < dt, "restarts after the clip length");
    require(idle.elapsedSeconds() == 0.0, "new animation starts at 0");
}

void testPositionIsRateTimesTime() {
    GameRandom random = recordedGenerator();
    NpcIdle idle;
    idle.update(0.0, random, kIdle, kIdle2);
    const NpcIdleVariant variant = idle.variant();
    const NpcIdleClip& clip = variant == NpcIdleVariant::Idle ? kIdle : kIdle2;
    idle.update(0.2, random, kIdle, kIdle2);
    require(std::abs(idle.pose(kIdle, kIdle2).position - 3.0) < 1e-9, "15 frames per second");
    require(idle.pose(kIdle, kIdle2).variant == variant, "same animation");
    require(clip.frames == (variant == NpcIdleVariant::Idle ? 19 : 69), "clip");
}

void testDegenerateClipRestartsEveryUpdate() {
    GameRandom random = recordedGenerator();
    NpcIdle idle;
    const NpcIdleClip empty{0, 15};
    idle.update(0.0, random, empty, empty);
    const unsigned cursor = random.firstCursor();
    idle.update(0.1, random, empty, empty);
    require(random.firstCursor() != cursor, "an empty clip is finished at once");
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
    } catch (const std::exception& error) {
        std::cerr << "npc-idle-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "npc-idle-test ok\n";
    return 0;
}
