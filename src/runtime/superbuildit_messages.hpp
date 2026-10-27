#pragma once
// The object messages, the save record and the restore of the super build-it class (kind 98): the handler 0x1bbec4 (messages 6, 8, 0xfe, 0xfc), the save
// callback 0x192608, the restore callback 0x192548 and the native DisableSuperBuiltItSave (0x2b4b78 -> 0x26d530). Rules and evidence: docs/re/super-buildit.md.
// No SDL dependency.
#include "runtime/natives_objects.hpp"
#include "runtime/script_runner.hpp"
#include "runtime/super_buildit.hpp"

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace chase::runtime {

constexpr std::uint8_t kSuperBuilditMessageStory = 6;
constexpr std::uint8_t kSuperBuilditMessageClose = kMessageUntrigger;
constexpr std::uint8_t kSuperBuilditMessageSounds = 0xfc;

// Message 6 acts only on the plate whose name hash equals the hash of this name (0x1bbf28 -> 0x24da28) and reads these names (strings at 0x1bc190..0x1bc1d0).
constexpr const char* kSuperBuilditStoryPlate = "1-3_Helipad.SBPlate";
constexpr const char* kSuperBuilditStoryVariable = "StoryProgress";
constexpr const char* kSuperBuilditBuiltVariable = "1-3_HelipadBuilt";
constexpr const char* kSuperBuilditStoryBrick = "1-3_rainbow_02";
constexpr std::int32_t kSuperBuilditStoryValue = 3;

// What a handled message asks of the host. `completeBrick` names the pickup whose class record gets value 0 and `TASK_COMPLETE` (0x19fc7c).
struct SuperBuilditMessageResult {
    bool handled = false;
    bool plateValueCleared = false;
    std::string completeBrick;
};

// The level variable by name (empty: the level has none).
using SuperBuilditVariable = std::function<std::optional<float>(const std::string&)>;

// 0x26d530 (DisableSuperBuiltItSave): bit 3 of +0x92 follows the argument.
void disableSuperBuilditSave(SuperBuilditObject& object, bool disabled);

// Handler 0x1bbec4 for the messages that do not need the player: 8 (reset: enabled, PropToReveal hidden, zone on, state 1 and requested 2), 0xfe (close: requested 1
// in state 2), 6 (the helipad plate of mission 1-3 with StoryProgress 3 and the built variable set) and 0xfc (hands ATTR_SFXBuilt to the sound list callback; the
// port loads no sound banks by object, so nothing happens). Any other message is not handled.
SuperBuilditMessageResult superBuilditMessage(SuperBuilditObject& object, std::uint8_t message, assets::PlacedObjectState& state, const SuperBuilditVariable& variable);

// Save callback 0x192608: !(bit 3) & (bit 1), one byte.
std::uint8_t superBuilditSaveByte(const SuperBuilditObject& object);

struct SuperBuilditRecord {
    std::uint32_t hash = 0;  // entity +8: the name hash of the instance
    std::uint8_t payload = 0;
};
// The records of every super build-it with SaveState (entity +0x82 bit 0), in load order.
std::vector<SuperBuilditRecord> superBuilditRecords(const SuperBuilditSet& set);

// Restore callback 0x192548 for the record of the object with this hash: with bit 0 set the built bit is set, the plate and HideObject are disabled, PropToReveal
// and UseObject enabled and BuildCompleteTriggerObject activated (0x244f40). Returns the number of objects it restored. Without SaveState the callback is not called.
std::size_t restoreSuperBuildits(SuperBuilditSet& set, const std::vector<SuperBuilditRecord>& saved, assets::PlacedObjectState& state, ScriptRunner& scripts);

}
