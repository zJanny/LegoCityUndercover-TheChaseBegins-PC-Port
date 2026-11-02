#pragma once
// What a destroyed prop does besides vanishing (docs/re/vehicles-damage.md): message 1 of the base prop handler 0x1f8948 runs 0x2aa5c8, which
// disables the entity, starts the particle definition `ExplosionParticleDef` at the entity box centre and, with `SpawnDebris`, the brick burst
// 0x23aebc -> 0x20c938 (runtime/debris.hpp) at the bottom of the box. No SDL dependency.
#include "assets/assets.hpp"
#include "runtime/debris.hpp"
#include "runtime/destroy_message.hpp"

#include <array>
#include <cstdint>
#include <optional>

namespace chase::runtime {

struct PropDestroyEffects {
    int explosionParticle = -1;   // ExplosionParticleDef (0x233b80 with the default -1); used when above 0
    bool spawnDebris = false;     // SpawnDebris (0x2451d0, default 0)
    unsigned debrisAmount = kDebrisDefaultCount;  // DebrisAmount when above 0 (0x20c938: its integer value's low byte), else 6
    std::uint32_t colour = kDebrisDefaultColour;        // DebrisColour
    std::uint32_t secondColour = kDebrisNoSecondColour; // DebrisColour2
    bool hasColour = false;       // the class or the object gives DebrisColour
    bool hasSecondColour = false; // the class or the object gives DebrisColour2
    float ratio = 0;              // DebrisRatio
    bool respawn = false;         // Respawn: 0x239898 queues the entity for its room (not ported)
    std::uint16_t sound = 0;      // the sound id the destroy requests through 0x23ae84 / 0x23fa54 (ATTR_SFX_DESTROYED, 29 for the default handler); 0 none
    // The super brick block 0x2cdca4 makes while 0x3743f2 is set (SuperBuildValue, the entity kind) with SpawnCone, SpawnLocal and SpawnDir; the node
    // matrix is the caller's.
    std::optional<DebrisSuperBlock> superBlock;
};

// The default handler 0x163f00 (kinds 64, 132) plays this sound (halfword 0x377160 + 6) and, when the class has no ExplosionParticleDef, starts
// this particle definition (byte 0x377160). Both are the initial values of the globals, read from the image (STAND-IN SCRIPT-S250: whether
// anything rewrites them is not read).
inline constexpr std::uint16_t kDefaultDestroySound = 29;
inline constexpr int kDefaultDestroyExplosion = 1;

// The parameters of a level object and its class as 0x1f8948, 0x2aa5c8, 0x2398d8 (case 1), 0x163f00 and 0x20c938 read them.
PropDestroyEffects propDestroyEffects(const assets::LevelObject& object, const assets::Level& cls, DestroyEffectSet set = DestroyEffectSet::BaseProp);

// 0x2aa5c8: the entity box centre (entity +0x60) in the node's world matrix (row major, translation in row 3).
DebrisVec3 propEffectCentre(const std::array<float, 3>& boxCentre, const std::array<float, 16>& node);
// 0x23aebc with its fifth argument 1: that centre minus the box half extent y (entity +0x70).
DebrisVec3 propDebrisPosition(const std::array<float, 3>& boxCentre, const std::array<float, 3>& boxHalf, const std::array<float, 16>& node);
// 0x20c938 for a prop: the burst of `DebrisAmount` bricks in the prop's colours (spread 1.0, no direction); with a super brick block (0x2cdca4
// answered) the block goes to the colour block's +0x14 and its count (9) replaces the amount.
DebrisBurst propDebrisBurst(const PropDestroyEffects& effects, const DebrisVec3& position, const std::optional<DebrisSuperBlock>& super = std::nullopt);

}
