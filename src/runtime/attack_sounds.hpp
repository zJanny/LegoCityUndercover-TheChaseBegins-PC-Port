#pragma once
// The sounds of Chase's attack on foot (docs/re/player-attack.md): the enter of the attack state 0x307e7c plays the swing sound of the character's melee
// weapon row through 0x23ae84, the hit event 0x319ffc plays its hit sound the same way when the hit test passes. No SDL dependency.
#include "assets/assets.hpp"
#include "runtime/player_attack.hpp"
#include "runtime/sound_events.hpp"

#include <array>
#include <vector>

namespace chase::runtime {

// STAND-IN PLAYER-S11: the owner of the calls is the u16 at the player entity +0x14, which is not read; the owner only keys a later stop, and the attack
// sounds are never stopped.
constexpr std::uint16_t kAttackSoundOwnerStandIn = 0;

class AttackSounds {
public:
    AttackSounds() = default;
    explicit AttackSounds(const assets::WeaponSounds& sounds) : sounds_(sounds) {}
    std::uint16_t swingId() const { return sounds_.swing; }
    std::uint16_t hitId() const { return sounds_.hit; }

    // Calls of one attack update at the player's node position, in the order of the original: the swing sound of 0x307e7c when the update entered
    // an attack state, the hit sound of 0x319ffc for every hit message. A sound id of 0 plays nothing (0x23fa54 masks the id and returns).
    std::vector<SoundCall> calls(const AttackStep& step, const std::array<float, 3>& position) const;

private:
    assets::WeaponSounds sounds_;
};

AttackSounds attackSoundsOf(const assets::Table& characters, const assets::Table& weapons, std::size_t characterRow);

}
