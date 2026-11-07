#pragma once
// What an AI character in the idle state (AI state 0) does next (docs/re/scripts-missions-1-4-spawners-conduct.md): the decision routine 0x2a9874 of
// state 0 (word +0 of the state table *0x3771f0, called by the AI controller 0x200c30), its target search 0x1f8d5c over the two six-slot side
// lists 0x58deb0 (characters with entity +0xc bit 2, `AIBadGuy`) and 0x58dec8 (the others, slot 6 the player), the sight test 0x1f6958
// (`AIViewDistance`, `AIRangeBound`), the sighting 0x21102c (AI state 2 on the target), the engage 0x206d18 and the release 0x21decc of a
// side slot (ctrl+0xd8 bit 5). Every AI character runs the same rule; the parameters come from the AI set-up 0x2bc934 and the AI start
// 0x1c6e84. No SDL dependency.
#include "assets/assets.hpp"
#include "runtime/fall_arrest_engaged.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace chase::runtime {

constexpr std::size_t kAiSideSlots = kEngagedListSlots;                       // the loops of 0x206d18 and 0x21decc stop at 6
constexpr std::uint16_t kAiViewDistanceDefault = 18;          // 0x2451d0 default at 0x2bc974, stored as u16 at ctrl+0x7a
constexpr float kAiMeleeFireDistanceDefault = 2.2f;           // literal 0x400ccccd at 0x2bcd28 (ctrl+0x80)
constexpr float kAiFireDistanceDefault = 15.0f;               // literal 0x41700000 at 0x2bcd40 (ctrl+0x7c)
constexpr std::uint32_t kAiFriendlySightSineBits = 0x3f350481;  // 0x1f6ac0: the normalised y compared as integer bits (sin 45 degrees)
constexpr std::uint8_t kAiCombatState = 2;                    // 0x21102c stores 2 to ctrl+0x15
constexpr std::uint16_t kAiSightRefusedLocomotionState = 82;  // 0x211044: nothing in locomotion state 82
constexpr std::uint8_t kAiSeenBits = 2;                       // 0x21102c: ctrl+0xdc bits 3..4 = 2
constexpr float kAiRunSpeedDefault = 1.0f;                    // `AIRunSpeed` default, literal 0x2bcd00 (0x2bc94c)
constexpr float kAiRunSpeedScale = 0.23f;                     // *0x37450c (initialiser 0x343de4), the factor 0x2bc984 multiplies AIRunSpeed by
constexpr float kAiFriendlyHomeFactor = 8.0f;                 // literal 0x2a9d84: the friendly branch 0x2a9bc4 compares with 8 x ctrl+0x8c

// STAND-IN SCRIPT-S326: ctrl+0xdb bit 1, which 0x2bca0c fills from 0x21e798 (ability 20 of the character row, then the ability of the row's
// weapon entry), is taken as clear: the row columns behind byte +0x18 and the weapon entry's +0x16 are not read.
constexpr bool kAiCloseSightStandIn = false;

// The parameters 0x2bc934 and 0x1c6e84 read for a placed character, and the entity bits of its create.
struct AiConductParams {
    std::uint16_t viewDistance = kAiViewDistanceDefault;      // `AIViewDistance` (integer reader 0x2451d0: 100.0 is 100)
    float meleeFireDistance = kAiMeleeFireDistanceDefault;    // `AIMeleeFireDistance`
    float fireDistance = kAiFireDistanceDefault;              // `AIFireDistance`
    std::string rangeBound;                                   // `AIRangeBound`, the bound 0x23e798 names (ctrl+0xcc)
    std::uint8_t targetMode = 0;                              // `AIDontTarget` & 7 into ctrl+0xda bits 0..2 (and 3..5)
    bool ignorePlayer = false;                                // `AIIgnorePlayer`, ctrl+0xd9 bit 0
    bool seesInvisible = false;                               // `AISeesInvisibleChars`, ctrl+0xdb bit 6
    bool friendly = false;                                    // `AIFriendly`, ctrl+0xdb bit 2 (0x1c6e84)
    bool closeSight = kAiCloseSightStandIn;                   // ctrl+0xdb bit 1 (0x21e798), the shortcut of the sight test 0x1f6aa0
    float runSpeed = kAiRunSpeedDefault * kAiRunSpeedScale;   // ctrl+0x8c: `AIRunSpeed` x *0x37450c (0x2bc988, after the character set-up's write)
    bool returnToSpawnPoint = true;                           // `ReturnToSpawnPoint`, ctrl+0xdc bit 5 (0x1c6e84)
    bool millAbout = false;                                   // `AIMillAbout`, ctrl+0x315 bit 7 (0x1c6e84)
    bool patrolPath = false;                                  // `AIPatrolPath` names an object: 0x1c6e84 requests AI state 14 instead of 0
    bool sleepAnim = false;                                   // `AISleepAnim` is set and not empty (the sleep branch of 0x2a9874)
    bool badGuy = false;                                      // `AIBadGuy`, entity +0xc bit 2
    bool boss = false;                                        // `Boss_IsBoss`, entity +0xc bit 14
};
AiConductParams aiConductParamsOf(const assets::LevelObject& object, const assets::Level& cls);
// The request at the end of 0x1c6e84: AI state 14 with an `AIPatrolPath`, 9 for a friendly character that mills about, else 0.
std::uint8_t aiStartRequest(const AiConductParams& params);

// One AI character, or the player (`player`). The host fills the first block every update; the conduct keeps the second.
struct AiConductEntity {
    std::string name;
    bool player = false;
    AiConductParams params;
    std::optional<std::size_t> rangeBound;  // index of params.rangeBound in Level::bounds (0x23e798 finds none: no bound)

    std::array<float, 3> position{};        // node translation (node +0x30)
    std::optional<std::array<float, 3>> spawn;  // ctrl+0xac..0xb4, the spawn point
    bool present = true;                    // a node (+0x3c), entity +0x10 bit 0 and +0xc bit 4 clear
    bool hidden = false;                    // 0x23de70: no node or its hidden flags
    bool thinks = true;                     // the AI controller 0x200c30 runs this update (director hold, +0xc bit 4)
    std::uint8_t aiState = 0;               // ctrl+0x15 (the port takes requests at once, so +0x14 equals it)
    std::uint16_t locomotionState = 1;      // ctrl+0x6c
    bool invisible = false;                 // ctrl+0x316 & 0xc0 (0x1f6b18)

    bool engaged = false;                   // ctrl+0xd8 bit 5: the entity holds a slot of its side list
    std::string target;                     // ctrl+0xc4
    std::uint8_t aggroCounter = 0;          // ctrl+0x72
    std::uint32_t countdown = 0;            // ctrl+0x84, counted down by 0x2a9874
    bool cantGetThere = false;              // ctrl+0xd9 bit 2, set by 0x1f6c58 when the path search fails
    std::uint8_t seenBits = 0;              // ctrl+0xdc bits 3..4
};

enum class AiConductEventKind : std::uint8_t { Engaged, Released, Sighted, MillAbout };
struct AiConductEvent {
    AiConductEventKind kind = AiConductEventKind::Engaged;
    std::string character;
    std::string target;     // Sighted
    float distance = 0;     // Sighted: the distance 0x1f6958 measured
    bool badSide = false;   // Engaged, Released: list 0x58deb0
    std::size_t slot = 0;   // Engaged, Released
    bool combat = false;    // Sighted: the character holds a slot and AI state 2 was requested
};

// 0x22f650(point, bound, 0) of the host: the bound's switch +0xa and its shape around the centre.
using AiConductBoundTest = std::function<bool(std::size_t bound, const std::array<float, 3>& point)>;

class AiConduct {
public:
    // The level's AI characters and the player; slots and AI fields start clear. `table` is the one owner of the side lists and must outlive the
    // conduct; it is cleared.
    void reset(EngagedTable& table, std::vector<AiConductEntity> entities);
    std::vector<AiConductEntity>& entities() { return entities_; }
    const std::vector<AiConductEntity>& entities() const { return entities_; }
    AiConductEntity* find(const std::string& name);

    // One update: the state enters for entities whose AI state changed since the last update (0x289650 for 0 and 0x2a9870 for 1 release the
    // slot), then the decision of state 0 for every entity that thinks in state 0, in entity order.
    void update(const AiConductBoundTest& inside);
    std::vector<AiConductEvent> takeEvents();

    // 0x206d18 and 0x21decc.
    void engage(AiConductEntity& self);
    void release(AiConductEntity& self);
    // 0x1f6958(viewer, target, out, limit, second, 0): a negative limit takes AIViewDistance, a negative second the melee or fire distance; a viewer
    // with ctrl+0xdb bit 1 sees a target within the second distance and below 45 degrees whatever its bound.
    bool sees(const AiConductEntity& viewer, const AiConductEntity& target, float limit, float second, const AiConductBoundTest& inside,
              float* distance = nullptr) const;
    // 0x1f8d5c(self, position, 0, limit, second, 45.0): the entity to attack, or null.
    const AiConductEntity* findTarget(const AiConductEntity& self, float limit, float second, const AiConductBoundTest& inside);
    // 0x2a9874 for one entity in state 0.
    void thinkIdle(AiConductEntity& self, const AiConductBoundTest& inside);

    const std::array<std::string, kAiSideSlots>& badSide() const { return table_->list(true); }
    const std::array<std::string, kAiSideSlots>& goodSide() const { return table_->list(false); }

private:
    AiConductEntity* entityNamed(const std::string& name);
    const AiConductEntity* entityNamed(const std::string& name) const;
    const AiConductEntity* player() const;
    const AiConductEntity* validTarget(const AiConductEntity& self) const;
    void sighted(AiConductEntity& self, const AiConductEntity& target, float distance);

    std::vector<AiConductEntity> entities_;
    std::vector<std::uint8_t> lastState_;
    EngagedTable* table_ = nullptr;
    std::vector<AiConductEvent> events_;
};

// STAND-IN SCRIPT-S326: the spawn point is the position of the first update the character is present (the original stores its node position at
// the AI start 0x1c6e84), 0x20bf3c, which can clear ReturnToSpawnPoint, answers 1, and a friendly character that goes back to its spawn point
// only takes a slot (the move 0x1f6c58 is not applied).
// STAND-IN SCRIPT-S326: the global byte tests the port does not track answer 0: 0x3771d6 (the boss branch of 0x1f8d5c) and 0x22b0b4
// (byte 0x5163e8 + 37, mill about for every idle character).
constexpr bool kAiConductBossSearchStandIn = false;
constexpr bool kAiConductGlobalMillAboutStandIn = false;

}
