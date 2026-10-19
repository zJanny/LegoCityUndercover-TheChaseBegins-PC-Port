#include "runtime/attack_sounds.hpp"

namespace chase::runtime {

std::vector<SoundCall> AttackSounds::calls(const AttackStep& step, const std::array<float, 3>& position) const {
    std::vector<SoundCall> result;
    if (step.started && sounds_.swing != 0) result.push_back({sounds_.swing, position, kAttackSoundOwnerStandIn});
    if (sounds_.hit != 0)
        for (const AttackMessage& message : step.messages)
            if (message.id == kAttackMessageHit) result.push_back({sounds_.hit, position, kAttackSoundOwnerStandIn});
    return result;
}

AttackSounds attackSoundsOf(const assets::Table& characters, const assets::Table& weapons, std::size_t characterRow) {
    return AttackSounds(assets::weaponSounds(weapons, assets::characterMeleeWeapon(characters, characterRow)));
}

}
