#pragma once
// A character takes a hit (docs/re/combat-characters.md): message 0 of the character handler 0x18e4c8 goes to the hit receiver 0x1b8570, which
// applies the damage (0x15afb4, a health of 0 destroys the entity through 0x23cd80, the death path of runtime/character_death.hpp) and sends the
// reaction message 46 to the character itself (0x18ee2c: PUNCHED, KNOCK BACK or FALL). The locomotion states PUNCHED (190 / 191), KNOCKED DOWN (19)
// and GET UP (18) play their clips and hand back to IDLE (1). No SDL dependency.
#include "assets/assets.hpp"

#include <cstdint>
#include <optional>

namespace chase::runtime {

inline constexpr std::uint16_t kCharacterStateIdle = 1;
inline constexpr std::uint16_t kCharacterStateFall = 5;
inline constexpr std::uint16_t kCharacterStateGetUp = 18;
inline constexpr std::uint16_t kCharacterStateKnockedDown = 19;
inline constexpr std::uint16_t kCharacterStateKnockBack = 188;
inline constexpr std::uint16_t kCharacterStateKnockBackCont = 189;
inline constexpr std::uint16_t kCharacterStatePunched = 190;
inline constexpr std::uint16_t kCharacterStatePunchedRepeat = 191;
inline constexpr std::uint16_t kCharacterStateThrown = 224;
inline constexpr std::uint16_t kCharacterStateShocked = 233;

// tablesanimations sheet 1 rows of the enters: 0x1d6c84 (PUNCHED) 180 "punched01", 0x283c38 (KNOCKED DOWN) 177 "react_knockd", 0x1b8478 (GET UP)
// 178 "react_knockd_getup"; each starts a new slot with 0x21941c(blend 0.1, speed 1.0, mode 0).
inline constexpr int kClipPunched = 180;
inline constexpr int kClipKnockedDown = 177;
inline constexpr int kClipGetUp = 178;
inline constexpr float kHitClipBlend = 0.1f;
inline constexpr float kHitClipSpeed = 1.0f;

// 0x283c38: ctrl+0x2c0 counts the updates KNOCKED DOWN waits after its clip finished (2 for a boss, entity +0xc bit 14).
inline constexpr std::uint16_t kKnockedDownWait = 60;
inline constexpr std::uint16_t kKnockedDownWaitBoss = 2;

// 0x1b8570: ctrl+0x2ce below 136 (the come-back counter of runtime/character_respawn.hpp) and the hit list 0x593178 of 0x22c60c turn hits below 100
// away; the list entry lasts 20 updates for the player, 10 for others and 5 for a hit of kind 1.
inline constexpr std::uint8_t kHitSpawnGuard = 136;
inline constexpr std::int32_t kHitIgnoresGuards = 100;
inline constexpr std::uint8_t kHitCooldownPlayer = 20;
inline constexpr std::uint8_t kHitCooldownOther = 10;
inline constexpr std::uint8_t kHitCooldownPunch = 5;
inline constexpr std::int32_t kHitReactionLimit = 16000;
inline constexpr std::uint8_t kHitKindPunch = 1;
inline constexpr std::uint8_t kHitKindIgnored = 7;

// Who sent the record (+4): nobody, the player (*0x3772c0), or another entity; `character` is 0x245b00 of the sender (kinds 27 and 149, the
// character kinds of 0x3743ec, a boss).
struct CharacterHitSender {
    bool present = false;
    bool player = false;
    bool character = false;
    std::uint8_t kind = 0;   // the sender's +0x12
};

// The 28-byte record of message 0: +0 a combat move record (the takedown moves of 0x22e884; none for the punch; `move` is its byte +0x66), +4 the
// sender, +0xc the damage, +0x10 a force, +0x14 the kind, +0x17 the weapon type. The player's punch is {no move, player, damage 1, force 0, kind 1,
// weapon 0} (docs/re/player-attack.md).
struct CharacterHitMessage {
    std::optional<std::uint8_t> move;
    CharacterHitSender sender;
    std::int32_t damage = 1;
    float force = 0;
    std::uint8_t kind = kHitKindPunch;
    std::uint8_t weapon = 0;
};

// The record the player's attack sends (0x319ffc): sender the player, the attack's damage and kind, no move, force 0, weapon 0.
CharacterHitMessage characterHitFromPlayer(std::int32_t damage, std::uint8_t kind);

// What 0x1b8570 and 0x18ee2c read of the character hit. The weapon immunity 0x23ad70, the abilities 7 / 0x2b, the hidden-node test 0x23de70 and
// 0x1758d8 count as clear and the hit list is never counted down (STAND-IN COMBAT-S1).
struct CharacterHitTarget {
    bool player = false;           // the entity is *0x3772c0
    bool playerSlot = false;       // 0x23df6c: the entity is named Player1.Player01..10
    bool disabled = false;         // entity +0x10 bit 0
    bool destroyed = false;        // entity +0xc bit 4, set by the destroy 0x23cd80
    std::uint8_t kind = 27;        // entity +0x12
    bool boss = false;             // entity +0xc bit 14 (`Boss_IsBoss`)
    std::uint16_t state = kCharacterStateIdle;   // ctrl+0x6c
    std::optional<std::uint16_t> pending;        // a request not taken yet (ctrl+0x6e differs from +0x6c)
    std::uint16_t health = 4;      // ctrl+0x70
    std::uint8_t maxHealth = 4;    // sub +0x128 (`Health`, default 4)
    std::uint8_t spawnCounter = 255;  // ctrl+0x2ce (255 from the create at 0x236f54)
    std::uint8_t cooldown = 0;     // updates left in the hit list 0x593178, 0 when not listed
    bool onGround = true;          // ctrl+0x314 bit 6
    bool reactionLocked = false;   // ctrl+0xd8 bit 6, written by CharacterInvulnerable (AiCharacter::invulnerable)
    bool attackWindow = false;     // in 0xa3 / 0xa4: the attack record +9 is 0 or 3 and its +0x10 bit 4 is set (0x25ed90; goons do not attack in the port)
    bool oneHitKill = false;       // ctrl+0x315 bit 5
    std::uint8_t selectMode = 0;   // ctrl+0xda & 7 for 0x219ba0 (start value not read: STAND-IN COMBAT-S1)
    std::uint8_t arrestHealth = 1; // sub +0x122 (`ArrestHealth` + 1, 0x237268)
};

enum class CharacterHitOutcome : std::uint8_t { Ignored, Healed, Reacted, Damaged, Destroyed };

struct CharacterHitResult {
    CharacterHitOutcome outcome = CharacterHitOutcome::Ignored;
    std::int32_t damage = 0;              // what 0x15afb4 took off
    std::uint8_t cooldown = 0;            // the 0x22c60c entry, 0 when none
    std::optional<std::uint16_t> reaction;  // the state message 46 requested (0x18ee2c)
    bool aggro = false;                   // 0x1b8770: a character sender the target took calls 0x1d1170 (runtime/character_takedown.hpp)
};

// 0x1b8570 for the paths the port reaches; the takedown moves of 0x22e884, the knock-back push of kind 3 (0x215634) and the
// player's branches are not ported (COMBAT-2, COMBAT-4).
CharacterHitResult receiveHit(CharacterHitTarget& target, const CharacterHitMessage& hit);

// Message 46 (0x18ee2c): the reaction state for the record, none while a request is pending, ctrl+0xd8 bit 6 is set or 0x25ed90 refuses.
std::optional<std::uint16_t> hitReactionState(const CharacterHitTarget& target, const CharacterHitMessage& hit);

// 0x2b900c: ctrl+0xe0 +0x122 changes by `delta` and stays at or above 0; true when it reached 0 (0x28cbf8 then lists the character as arrestable
// through 0x192fd4, docs/re/combat-characters.md).
bool changeArrestHealth(CharacterHitTarget& target, int delta);

// 0x237234..0x237270: sub +0x122 and +0x123 = `ArrestHealth` + 1, at most 255.
std::uint8_t arrestHealthOf(std::int32_t arrestHealthParam);

// The target a placed character starts with: `Health` (0x236e48, default 4, read as an integer: 2.0 is 2) into sub +0x128 and ctrl+0x70,
// `ArrestHealth` (default 0) through arrestHealthOf, `Boss_IsBoss` into entity +0xc bit 14.
CharacterHitTarget characterHitTargetOf(const assets::LevelObject& object, const assets::Level& cls);

// The hit states of a character: the enter starts its clip, the update hands over when the clip finished (slot kind 6 of 0x22f560).
struct CharacterHitStep {
    std::optional<int> clip;                   // started this update (0x21941c)
    std::optional<std::uint16_t> request;      // 0x2414b4 for the next state
    bool facesSender = false;                  // 0x205fbc + 0x23deb4 of the PUNCHED enter
};

class CharacterHitStates {
public:
    // A request of 0x2414b4; the state is entered by the next update().
    void request(std::uint16_t state);
    // One update of the character: enter a requested state, else update the current one. `clipFinished` is the slot kind 6 test of the update.
    CharacterHitStep update(bool clipFinished, bool boss);
    std::uint16_t state() const { return state_; }
    std::optional<std::uint16_t> pending() const { return pending_; }
    bool active() const;
    std::uint16_t knockedDownWait() const { return wait_; }

private:
    std::uint16_t state_ = kCharacterStateIdle;
    std::optional<std::uint16_t> pending_;
    std::uint16_t wait_ = 0;
};

}
