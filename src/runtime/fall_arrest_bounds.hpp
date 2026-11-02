#pragma once
// FallArrestBoundSystem (docs/re/scripts-missions-1-4.md): the static system object 0x515214 (name string 0x34c7cc, vtable 0x363d00). Its scene
// set-up 0x307890 keeps every bound of the scene whose name begins with "FallArrest" (0x22c28c, ten characters, case ignored); its update 0x307980
// tests the characters of the engaged table 0x58deb0 against them and marks a character that falls into one as auto-arrested (sub +0x131 bits 6..7
// = 2), which the takedown turns into AUTO ARRESTED (0x22e884, CharacterTakedownStates). No SDL dependency.
#include "assets/assets.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace chase::runtime {

constexpr std::size_t kFallArrestPrefixLength = 10;      // 0x22c28c(name, "FallArrest", 10)
constexpr std::size_t kFallArrestEngagedSlots = 6;       // the engaged table 0x58deb0
constexpr std::size_t kFallArrestSoundSlot = 20;         // u16 +0x28 of the character's sound row (0x375f7c + category x 0x44)

// 0x307890: the indices of the level bounds named FallArrest*, in the order of the scene's bound list.
std::vector<std::size_t> fallArrestBounds(const assets::Level& level);

// One entry of the engaged table as 0x307980 reads it.
struct FallArrestCharacter {
    std::string name;
    bool enabled = true;              // entity +0x10 bit 0 clear
    bool onGround = true;             // ctrl+0x314 bit 6
    std::uint8_t arrestMark = 0;      // sub (ctrl+0xe0) +0x131 bits 6..7
    std::array<float, 3> position{};  // the node's world translation (0x250810 + 0x30)
};

struct FallArrestHit {
    std::string character;
    std::size_t bound = 0;            // index into Level::bounds
};

// 0x307980: an enabled character in the air without an arrest mark that stands in an enabled FallArrest bound (0x22f650 without the bound's node
// matrix) gets the mark 2 and plays slot 20 of its sound row (0x23ae84); the first bound in the list wins.
std::vector<FallArrestHit> fallArrestUpdate(const assets::Level& level, const std::vector<std::size_t>& bounds, const std::vector<std::uint8_t>& boundEnabled,
                                            std::vector<FallArrestCharacter>& engaged);

// 0x22f650(point, bound, 0): the switch +0xa and the shape test around the centre, without the node rotation.
bool pointInFallArrestBound(const assets::LevelBound& bound, bool enabled, const std::array<float, 3>& point);

}
