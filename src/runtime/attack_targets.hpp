#pragma once
// Which entities the player's attack may pick (docs/re/player-attack.md): the entity gate 0x21a2d4, the target mode test 0x219ba0, the
// character test 0x245b00, the weight 0x1b7358, the grab and fight test 0x28265c with 0x23ad70 and 0x23d610, and the choice of 0x1fdbd4 between
// the fight move on a character (0x1d3980 / 0x19bfc0) and the punch (0x214f40 variants 0 and 3). No SDL dependency.
#include "assets/assets.hpp"
#include "runtime/game_random.hpp"

#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace chase::runtime {

constexpr std::uint8_t kEntityKindCharacter = 27;       // 0x245b00 / 0x22716c
constexpr std::uint8_t kEntityKindBossCharacter = 149;
constexpr std::uint8_t kEntityKindWobblyProp = 119;
constexpr std::uint16_t kEntityFlagGone = 0x1;          // entity +0x10 bit 0: 0x21a2d4 drops it
constexpr std::uint16_t kEntityFlagCollision = 0x200;   // entity +0x10 bit 9: 0x21a2d4 needs it
constexpr std::uint32_t kEntityWordBadGuy = 0x4;        // entity +0xc bit 2: `AIBadGuy` (create 0x182f0c), toggled by SwapAllegiance
constexpr std::uint32_t kEntityWordNoHitReset = 0x10;   // entity +0xc bit 4
constexpr std::uint32_t kEntityWordBoss = 0x4000;       // entity +0xc bit 14: `Boss_IsBoss`

// 0x21a2d4 flag bits.
constexpr unsigned kAttackGateSameSide = 0x1;           // drop a character on the attacker's side (+0xc bit 2 equal)
constexpr unsigned kAttackGateOtherSide = 0x2;          // drop a character on the other side
constexpr unsigned kAttackGatePlayerSides = 0x4;        // instead: drop a character on the attacker's side unless the attacker is the player
constexpr unsigned kAttackGateVisible = 0x8;            // drop an entity whose node is hidden (0x23de70)
constexpr unsigned kAttackGateNotState19 = 0x20;        // drop a character in state 0x13
constexpr unsigned kAttackSearchFlags = 9;              // 0x1fdbd4: both searches of the Y tap
constexpr unsigned kAttackStrikeFlags = 0xc;            // 0x319ffc: the check of the held target and the search at the strike
constexpr unsigned kAttackStrikeFlagsPunch = 0xb;       // the search instead for the player's variants 0 and 3 when the weapon row is not 16

constexpr float kGrabSearchRange = 3.0f;                // *0x372e50 (read in the oracle, docs/re/player-attack.md)
constexpr float kGrabSearchAngle = 45.0f;               // *0x372e4c
constexpr float kBadGuyAngle = 180.0f;                  // 0x219c08: a candidate with +0xc bit 2 is searched all around (0x43340000)
constexpr std::uint8_t kWeaponRowCharacterMelee = 16;   // 0x1fdbd4: with this weapon row the punch keeps a character target
constexpr float kFightGroundGap = 0.5f;                 // 0x28265c: |ctrl+0x240 difference| at or below 0x3f000000

// The 64 ability bits of a character (character table row +0xe, 0x206888), as 0x1d4dfc hands them to 0x219a8c / 0x23a70c.
using AbilitySet = std::array<std::uint8_t, 8>;
bool hasAbility(const AbilitySet& set, unsigned ability);
bool hasAnyAbility(const AbilitySet& set, const AbilitySet& wanted);

// What the target functions read of a character's controller.
struct AttackCharacter {
    int state = 1;                   // ctrl+0x6c
    std::uint8_t targetMode = 0;     // ctrl+0xda & 7 (0x219ba0): 0 anyone, 2 only ctrl+0xc4, 3 only the player, 4 and others nobody
    bool modeTargetIsAttacker = false;
    bool onGround = true;            // ctrl+0x314 bit 6
    float groundY = 0;               // ctrl+0x240
    AbilitySet abilities{};          // 0x1d4dfc
    bool classFlag0 = false;         // character table row +0x1c bit 0 (0x23ad70 with ability 0)
    bool heldByOther = false;        // 0x28265c: an engaged entity (0x58deb0) has this as ctrl+0x140 with next state ctrl+0x6e 0x132
};

// An entity as 0x21a2d4, 0x245b00 and 0x1b7358 see it.
struct AttackEntity {
    std::uint8_t kind = kEntityKindWobblyProp;   // entity byte +0x12
    std::uint16_t flags = kEntityFlagCollision;  // entity +0x10
    std::uint32_t word = 0;                      // entity +0xc
    bool hidden = false;                         // 0x23de70
    bool listedKind = false;                     // 0x236b6c: the kind is in the list 0x3743ec (kinds 121, 159, 242)
    bool pathRecord = false;                     // kind 19: entity +0x90 not null
    bool player = false;                         // the entity is *0x3772c0
    AttackCharacter character;
};

// 0x245b00 (also 0x21aae0): kind 27 or 149, a listed kind, or +0xc bit 14.
bool isAttackCharacter(const AttackEntity& entity);
// 0x219ba0(candidate, attacker, 1).
bool targetModeAllows(const AttackEntity& candidate, const AttackEntity& attacker);
// 0x21a2d4(candidate, attacker, flags) without the height test of flag 0x40 and the kinds 0x43 / 0xcc (0x22103c), which no caller here passes.
bool attackCandidatePasses(const AttackEntity& candidate, const AttackEntity& attacker, unsigned flags);
// 0x1b7358: 1.0 for a hidden node or a non-character, 0.3 for a boss, 0.25 for the player, 0.4 for kind 100 and for a bad guy character that is
// not in state 0x13 and has no +0xc bit 4.
float attackWeight(const AttackEntity& candidate);
// The angle 0x219c08 uses for a candidate: 180 for a bad guy when the caller's fourth argument is set (every call of the player's attack).
float attackSearchAngle(const AttackEntity& candidate, float angle, bool allAround);

// 0x23d610: states in which a character cannot be grabbed (0x122..0x12f, 0x134, 0x135, 0x139..0x13e, 0x143, 0x144).
bool grabStateAllows(int state);
// 0x23ad70(ctrl, 0) for a character other than the player: the class bit, or state 0xdb with ability 0x2b.
bool fightRefused(const AttackCharacter& character);

// A fight move: tables/tablescombat.bxls sheet 1 rows 0..7 (stride 20, the table *0x372e28): +0 and +4 the two floats 0x1d3980 hands to the
// pair set-up 0x231d20, +8 the player's clip (grab_putdown, grab_putdown_02..08), +0xc the ability the player needs for it (46 in all eight rows).
struct FightMove {
    float pairA = 0, pairB = 0;
    int clip = 0;
    std::uint8_t ability = 0;
};
constexpr std::size_t kFightMoveCount = 8;
constexpr int kFightTargetState = 0xe3;      // 0x1d3980 requests it for the target
constexpr int kStruggleState = 0xfe;         // 0x19bfc0: the player's state; the target gets 0xff
constexpr int kStruggleTargetState = 0xff;
std::vector<FightMove> fightMoves(const assets::Table& combat);
AbilitySet fightMoveAbilities(const std::vector<FightMove>& moves);
// 0x1d3980: a random move (0x235498: next word % count) among those other than the last one (combat record byte +0xb, 0xff at the start) whose
// ability the player has; without one the last move stays when the player still has its ability. None when that leaves no move (the original
// then indexes the table with -1).
std::optional<int> chooseFightMove(int last, const AbilitySet& player, const std::vector<FightMove>& moves, GameRandom& random);

// What the attack hands to the hit character: message 0 of 0x319ffc, record {0, player, 0, damage, 0, kind, 0}.
struct CharacterHit {
    std::size_t entity = 0;          // index into the attack candidates
    std::string name;                // lower-case instance name
    std::int32_t damage = 1;
    std::uint32_t kind = 1;          // 1 for the punch (variants 0 and 3), 0 for the other variants
    bool senderPlayer = true;
};
using CharacterHitSink = std::function<void(const CharacterHit&)>;
// What 0x1d3980 / 0x19bfc0 ask of the target character when the tap starts a fight move or a grab struggle.
struct CharacterFightStart {
    std::size_t entity = 0;
    std::string name;
    int state = kFightTargetState;   // 0xe3 for a fight move, 0xff for a struggle
    int move = -1;                   // fight move index (pair record +0x478), -1 for a struggle
    bool senderPlayer = true;
};
using CharacterFightSink = std::function<void(const CharacterFightStart&)>;

// 0x28265c(target, player) without the line of sight 0x219990, which the caller passes as `clearLine`: a character the player may grab, with
// both on the ground, ctrl+0x240 within 0.5, and the player holding one of the abilities of the fight moves.
bool fightTargetPasses(const AttackEntity& target, const AttackEntity& player, const AbilitySet& fightMoveAbilities, bool clearLine);

}
