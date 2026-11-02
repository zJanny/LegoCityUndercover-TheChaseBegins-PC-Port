#pragma once
// The facts of the world the player's target search reads (docs/re/player-attack-characters.md): the line of sight 0x219990 of the grab test, the
// player's near list 0x23d4c0 and the box a character class gives its entity (0x23aff8). No SDL dependency.
#include "runtime/collision.hpp"
#include "runtime/player_attack.hpp"

#include <array>
#include <cstddef>
#include <vector>

namespace chase::runtime {

// The query words of 0x219990 (stack at 0x2199f4..0x219a2c): include mask 5, exclude 0x10 (static level, placed dynamic props and characters; not
// the vehicle boxes), owner flags +0x10 need 0x200 and neither bit 0 nor bit 1, the attacker is the excluded owner, no surface is excluded.
constexpr CollisionLayer kAttackLineLayer{5, 0x10};
constexpr std::uint16_t kEntityFlagsOwnerExcluded = 0x3;
constexpr std::uint64_t kAttackLineExcludedSurfaces = 0;

constexpr float kAttackNearRange = 5.0f;           // *0x377538, read in the oracle: the half extent of the player's near list box
constexpr std::size_t kAttackNearListMax = 100;    // 0x2e5408: the capacity passed to the grid gather 0x237f30

// 0x219990(attacker, target): the ray 0x21ab20 from the attacker's box centre to the target's box centre. Clear when it hits nothing, or when
// the nearest entry it meets is the target's. The entries are the world's (static level and placed props accepted by kAttackLineLayer) and the
// boxes of the character candidates other than the target (mask 4 entries: entity +0x60 / +0x6c in the node frame). Ties go to the target.
// STAND-IN PLAYER-S16: the characters in the ray are the candidates the glue supplies; the owner +0xc exclusion 0x1800 of the query is not applied.
bool attackLineClear(const CollisionWorld& world, const AttackBody& attacker, const std::vector<AttackCandidate>& candidates, std::size_t target);

// 0x2e5408: sets `listed` on the candidates that are in the player's near list (the search skips the others, the indices stay stable): the
// entries of masks 4 and 6 (characters and dynamic props) with owner flags 0x200 and neither bit 0 nor bit 1 whose cells meet the box of half extents kAttackNearRange around the player's node position. The cells are not modelled: an entry is kept when its world box meets
// that box, which keeps every candidate a search up to 5.0 can reach (the search never takes more than 3.5). The order of the candidates is kept
// (STAND-IN PLAYER-S16: the original's order is the grid's, which only decides exact score ties).
void markAttackNearList(std::vector<AttackCandidate>& candidates, const std::array<float, 3>& playerNode);

// 0x23aff8 without automatic box: the box of a character class is its "CollisionBound" record (centre +0x10, half extents +0x1c) and sets owner
// flag 0x200; a class without the record leaves the entity without a box and without collision, and "BoundCollision" 0 clears the flag again.
// STAND-IN PLAYER-S15: the "MeshCollision" branch (0x23b1c4), which takes the box from the model (its first collision item or box record), is not modelled.
struct EntityBox {
    std::array<float, 3> centre{};
    std::array<float, 3> half{};
    bool collision = false;
};
EntityBox characterEntityBox(const assets::Level& classFile, bool boundCollision);

}
