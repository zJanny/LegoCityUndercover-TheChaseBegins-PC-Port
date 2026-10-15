#pragma once
// The game's random number generator 0x25b2b8 and its seeding 0x25b254 (docs/re/npc-animation.md). No SDL dependency.
#include <array>
#include <cstdint>

namespace chase::runtime {

constexpr unsigned kGameRandomTableSize = 11;
constexpr unsigned kGameRandomSecondCursorStart = 7;

class GameRandom {
public:
    // 0x25b254: seed 0 becomes 0xffffffff, the 11 table words are consecutive xorshift32 steps (13, 17, 5), the cursors start at 0 and 7.
    explicit GameRandom(std::uint32_t seed);
    GameRandom(const std::array<std::uint32_t, kGameRandomTableSize>& table, unsigned firstCursor, unsigned secondCursor);

    // 0x25b2b8: table[first] = ror(table[first] + table[second], 29), both cursors step down and wrap from 0 to 10; returns the new word.
    std::uint32_t next();

    const std::array<std::uint32_t, kGameRandomTableSize>& table() const { return table_; }
    unsigned firstCursor() const { return first_; }
    unsigned secondCursor() const { return second_; }

private:
    std::array<std::uint32_t, kGameRandomTableSize> table_{};
    unsigned first_ = 0;
    unsigned second_ = kGameRandomSecondCursorStart;
};

// 0x2345dc: the low 16 bits of the next word times 2^-16, in [0, 1).
inline float gameRandomUnit(GameRandom& random) { return static_cast<float>(static_cast<std::uint16_t>(random.next())) * (1.0f / 65536.0f); }

}
