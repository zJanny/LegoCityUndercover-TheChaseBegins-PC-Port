#pragma once
// What message 1 of the entity destroy 0x23cd80 does per class kind (docs/re/script-natives-world.md, entry 2026-10-19T08:53:30). No class handler
// returns non-zero for it, so every destroyed entity gets the destroyed bit (+0xc bit 0x10) and loses its collision body (0x2320f8); the handlers
// differ in whether they disable (hide) the entity (0x2428c0) and in their own effects. No SDL dependency.
#include <cstdint>

namespace chase::runtime {

enum class DestroyReaction {
    NoReaction,   // the handler returns 0 without disabling the entity (it stays visible), or the kind has no handler
    HidesEntity,  // the handler reaches the disable 0x2428c0: default handler 0x221760, base prop handler 0x1f8948, hit machine 0x2398d8, characters
    BreaksVehicle // the vehicle class: the break 0x16cccc, the entity stays visible
};

DestroyReaction destroyReaction(std::uint8_t kind);
// The kinds whose message 1 runs the base prop handler 0x1f8948 (85, 119, 134, 214): explosion, bricks and the TriggerObject parameter.
bool destroyRunsPropHandler(std::uint8_t kind);
// False for the kinds whose message 1 path has calls that were not read (31, 45, 100, 125, 159, 208, 222, 243); they count as NoReaction
// (stand-in SCRIPT-S245).
bool destroyReactionRead(std::uint8_t kind);

// Which routine of message 1 makes the visible effects (docs/re/script-natives-world.md, entry 2026-10-19T10:02:26). BaseProp: 0x1f8948 (85, 119, 134, 214); HitMachine:
// 0x2398d8 case 1 (67, 204): the same explosion, screen shake and bricks as 0x2aa5c8 without its light flash, and the sound at the entity;
// Pushable: 0x1a958c (71) = 0x2aa5c8 and the sound; DefaultHandler: 0x221760 (64, 132) = 0x163f00: the default sound, the default explosion
// when the class names none, the bricks. CharacterFallApart: the character handler 0x18e93c (27, 121, 242), ported apart from the props in
// runtime/character_death.hpp; the topple (102) is not ported.
enum class DestroyEffectSet { None, BaseProp, HitMachine, Pushable, DefaultHandler, CharacterFallApart, Topple };
DestroyEffectSet destroyEffectSet(std::uint8_t kind);
// The sets ported as prop effects (the props of VehiclePropSmasher); CharacterFallApart has its own path.
bool destroyEffectsPorted(DestroyEffectSet set);

}
