#include "runtime/game_random.hpp"

namespace chase::runtime {

namespace {
std::uint32_t xorshift(std::uint32_t value) {
    value ^= value << 13;
    value ^= value >> 17;
    value ^= value << 5;
    return value;
}

std::uint32_t rotateRight29(std::uint32_t value) { return (value >> 29) | (value << 3); }

unsigned stepDown(unsigned cursor) { return cursor == 0 ? kGameRandomTableSize - 1 : cursor - 1; }
}

GameRandom::GameRandom(std::uint32_t seed) {
    std::uint32_t value = seed == 0 ? 0xffffffffu : seed;
    for (std::uint32_t& word : table_) {
        value = xorshift(value);
        word = value;
    }
}

GameRandom::GameRandom(const std::array<std::uint32_t, kGameRandomTableSize>& table, unsigned firstCursor, unsigned secondCursor)
    : table_(table), first_(firstCursor % kGameRandomTableSize), second_(secondCursor % kGameRandomTableSize) {}

std::uint32_t GameRandom::next() {
    const std::uint32_t value = rotateRight29(table_[first_] + table_[second_]);
    table_[first_] = value;
    first_ = stepDown(first_);
    second_ = stepDown(second_);
    return value;
}

}
