#pragma once
// The switches of a level that run the switch base (switch_base.hpp): kind 38 (lightonoff: create 0x1de6dc, update 0x1de740, the base handler 0x23aac0)
// and kind 58 (levers, key switches, pull switches, keypads, valves: create 0x17510c, post-link 0x16a15c, update 0x175338, handler 0x17f6f4).
// docs/re/switches.md. No SDL dependency.
#include "assets/assets.hpp"
#include "assets/object_collision.hpp"
#include "runtime/reference_trigger.hpp"
#include "runtime/script_runner.hpp"
#include "runtime/stud_callers.hpp"
#include "runtime/switch_base.hpp"

#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace chase::runtime {

constexpr std::uint8_t kSwitchKindLight = 38;
constexpr std::uint8_t kSwitchKindLever = 58;
constexpr std::uint8_t kLeverMessageUseQuery = 3;
constexpr std::uint8_t kLeverMessageCancelUse = 12;
constexpr std::uint8_t kLeverMessageHoldMe = 48;
constexpr unsigned kLeverPlayerUseState = 76;  // 0x2366bc(player, lever, 76, 0) from message 3
// Counted from the update that sees the A press, recorded from a save state by RPC 20261025-garage-A-1 (2026-10-25, stepped at dt 1/30, the mission 1-3
// key switch): the switch record's use bit (+0x24 bit 0) is set after update 31 without any message reaching 0x17f6f4, and the player's state 76 ends
// (state 1) at update 81.
constexpr unsigned kLeverUseRequestUpdate = 31;
constexpr unsigned kLeverUseEndUpdate = 81;

// The lever part of the kind 58 record (0x74 bytes) and its use zone (registration 0x23e130 with the record +0x40).
struct LeverPart {
    bool usable = true;            // zone record byte +4 bit 4 (+0x44): UseStartActive, cleared in states 1 and 3, set again in state 5
    bool zoneBit6 = false;         // +0x44 bit 6: state 5 makes the zone usable only when it is clear; its writer is not read
    bool hasZone = false;
    float zoneCentre[3] = {0, 0, 0};  // class bound UseBound, relative to the position, axis-aligned
    float zoneHalf[3] = {0, 0, 0};
    std::int32_t leverAbility = 0;  // LeverAbility +0x60
    bool strong = false;            // Strong, +0x70 bit 2
    std::int32_t leverType = 0;     // LeverType, +0x70 bits 3..7
    bool holdMe = false;            // HoldMe, answered by message 48
    bool twitched = false;          // +0x68
};

struct SwitchObject {
    std::string instance;  // lower case
    std::uint8_t kind = 0;
    std::size_t objectIndex = 0;
    float position[3] = {0, 0, 0};
    std::array<float, 16> matrix{};
    SwitchParams params;
    SwitchRecord record;
    std::optional<std::size_t> reference;  // the record +0xc in the reference list
    std::optional<LeverPart> lever;
    std::int32_t studValue = 0;
    StudSpawnSource studSpawn;
};

struct SwitchEvent {
    std::size_t index = 0;
    SwitchEffect effect;
};

struct SwitchUseInput {
    float player[3] = {0, 0, 0};
    bool actionPressed = false;
    bool playerFree = false;
    std::function<bool(int ability)> playerHasAbility;
};

struct SwitchUse {
    std::size_t index = 0;
    std::uint8_t answer = 0;  // what message 3 returned: 1 taken, 0xff refused for the abilities
};

class SwitchSet {
public:
    // Every created object of kind 38 or 58 in object order; `hasUseAnimation` says whether the object's model has the animation "use" (0x23e5b0).
    void load(const assets::Level& level, const assets::ClassLookup& classOf, const std::function<bool(const assets::LevelObject&, const assets::Level&)>& hasUseAnimation);
    // Post-link and link: the reference record of each switch, StartOn (0x242494), the lights and the use animation of 0x23e5b0.
    std::vector<SwitchEvent> link(ReferenceTriggers& references);
    // The A press (0x1fee94 sends message 3 to the zones that hold the player; the first answer that is not 0 takes it).
    // STAND-IN SCRIPT-S292: the player's use state 76 is not ported; a lever that takes the press gets its use bit kLeverUseRequestUpdate updates later
    // (update()) and the player stays busy until kLeverUseEndUpdate, with no animation and no turn to the switch.
    std::optional<SwitchUse> use(const assets::PlacedObjectState& state, const SwitchUseInput& input, const ReferenceTriggers& references);
    // One update of every enabled switch (0x1de740, 0x175338). STAND-IN SCRIPT-S293: the use animation is not played and counts as ended, disabled
    // switches are skipped and a lever without a class UseBound has no zone (0x23e130 would build one from the node bounds).
    std::vector<SwitchEvent> update(const assets::PlacedObjectState& state, float dt, ReferenceTriggers& references, ScriptRunner& scripts);
    // A message to a switch (0x244fc8 -> the class handler); false when `lowerInstance` is no switch.
    bool message(const std::string& lowerInstance, std::uint8_t message, ReferenceTriggers& references, ScriptRunner& scripts,
                 std::vector<SwitchEvent>& events, bool fromPlayer = false, float argument = 1.0f);
    const std::vector<SwitchObject>& objects() const { return objects_; }
    std::optional<std::size_t> find(const std::string& lowerInstance) const;
    static bool inZone(const SwitchObject& object, const float player[3]);
    bool playerUsing() const { return playerUse_.has_value(); }

private:
    std::uint32_t handle(std::size_t index, std::uint8_t message, bool fromPlayer, float argument, ReferenceTriggers& references, ScriptRunner& scripts,
                         std::vector<SwitchEvent>& events);
    void runReference(std::size_t index, bool activate, ReferenceTriggers& references, ScriptRunner& scripts, std::vector<SwitchEvent>& events);
    std::vector<SwitchObject> objects_;
    struct PlayerUse {
        std::size_t index = 0;
        unsigned updates = 0;
    };
    std::optional<PlayerUse> playerUse_;
};

// The abilities message 3 of a lever asks the player for (0x227b5c -> 0x1f8b44, kind 58): 1, LeverAbility and 7 for a Strong lever.
std::vector<int> leverAbilities(const LeverPart& lever);

}
