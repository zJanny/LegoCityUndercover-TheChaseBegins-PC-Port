#pragma once
// Character abilities (docs/re/use-spot.md): the 8-byte ability set of a character row (0x206888 copies bytes +15..+22 of the 40-byte row of
// tables/tablescharacters.bxls sheet 0) and the unlock rows (sheet 2, 8-byte rows, byte +6, read by 0x1be6c0). 0x23a87c(character, ability) asks
// 0x210e38 whether the set holds the ability bit. No SDL dependency.
#include "assets/assets.hpp"

#include <array>
#include <cstdint>
#include <vector>

namespace chase::runtime {

constexpr std::size_t kCharacterAbilityMaskOffset = 15;
constexpr std::size_t kCharacterAbilityMaskBytes = 8;
constexpr std::size_t kAbilityUnlockRowOffset = 6;

class AbilityTable {
public:
    void load(const assets::Table& characters);
    // 0x23a87c: the ability bit is set in the character's set; false for an unknown character or ability.
    bool characterHas(unsigned character, int ability) const;
    // 0x1be6c0 asks for the save bit only when the ability's row of sheet 2 has a non-zero byte +6 (the abilities that start locked).
    // The cheat byte 0x375961 bit 5, with which 0x1be6c0 answers 0 for every ability, is not modelled (no cheats in the port).
    bool startsLocked(int ability) const;

private:
    std::vector<std::array<std::uint8_t, kCharacterAbilityMaskBytes>> masks_;
    std::vector<bool> startsLocked_;
};

}
