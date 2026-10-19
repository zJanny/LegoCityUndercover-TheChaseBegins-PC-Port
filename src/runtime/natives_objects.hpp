#pragma once
// Script natives that write one flag of an object or send it a class message (batch SCRIPT-26, docs/re/script-natives.md).
// The handlers only store the value; the systems that read the flags (targeting, interaction prompts, shove, destruction,
// door update) are not ported yet, so the state is recorded here. No SDL dependency.
#include "assets/assets.hpp"
#include "runtime/destroy_message.hpp"
#include <array>
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace chase::runtime {

enum class ObjectNativeKind {
    ToggleUseable,
    EnableTargetable,
    SetIndestructible,
    ActiveDuringDcam,
    ObjectUnshovable,
    ToggleUpdateFlag,
    ResetObject,
    SkipToFinalState,
    PrintDebugString,
    RevealCollectable,
    DestroyProp,
    KillObject,
    EnableShadow,
    EnableSwitch,
    EnableObjectFade,
};

std::optional<ObjectNativeKind> objectNativeKind(const std::string& lowerCaseName);
std::vector<std::string> objectNativeNames();
bool objectNativeTakesValue(ObjectNativeKind kind);
bool objectNativeIsStandIn(ObjectNativeKind kind);

// ResetObject 0x1b25f8 and SkipToFinalState 0x270078 call the class message handler 0x244fc8 with these numbers.
constexpr std::uint8_t kMessageReset = 8;
constexpr std::uint8_t kMessageSkipToFinalState = 54;
// DestroyProp / KillObject 0x1a2fe4 -> 0x23cd80 send the class message 1 (the vehicle class answers it with its break, 0x159bd0; destroy_message.hpp).
constexpr std::uint8_t kMessageDestroy = 1;
// Kind bytes (entity +0x12, the class file's ObjectType, docs/re/object-kinds.md) the natives test.
constexpr std::uint8_t kKindPushable = 71;
constexpr std::uint8_t kKindPushHandle = 72;
// Kinds whose create or post-link function registers a use zone with 0x23e130, the only function that adds to the registry 0x236624
// searches (docs/re/object-kinds.md). Conditions inside those functions are stand-ins (SCRIPT-S239).
bool objectKindRegistersUseZone(std::uint8_t kind);
// Kinds whose class message handler compares the message with 24 (enableobjectfade 0x26fac4); the other kinds do not answer it.
bool objectKindAnswersFadeMessage(std::uint8_t kind);
// What message 8 of the default handler 0x221800 (kinds 64, 132) wrote: `initialposition` and `initialorientation` of the object (degrees, the
// handler multiplies by 0.0174533), read from the level object and its class. Nothing in the port moves such an object yet, so this is the record.
struct PlacementReset {
    std::array<float, 3> position{};
    std::array<float, 3> orientationDegrees{};
    bool hasPosition = false;
    bool hasOrientation = false;
};
struct ObjectClassMessage {
    std::string instance;
    std::uint8_t message = 0;
};

// Values the natives wrote, unset until a script wrote them. Meaning of the original bits:
//  usable: ToggleUseable, bit 4 of the use-zone registry record set by 0x236624, only for kinds that register a zone, true for an argument above 0;
//  targetable: EnableTargetable, false when the argument truncated to an integer has a zero low byte (+0xda of the component);
//  indestructible: SetIndestructible, bit 0 of +0xde of the component;
//  activeDuringDcam: ActiveDuringDCam, bit 1 of +0x312 of the component;
//  unshovable: ObjectUnshovable, bit 0x100 of the flag word at +0x10 and bit 0x20000 of the one at +0xc;
//  shadow: EnableShadow 0x1bf84c, bit 2 of +0x316 of the component, written only for the kinds 27, 149 and the shadow list kinds 121, 159, 242;
//  switchState: EnableSwitch 0x1bf8a4, halfword +4 of the component, 3 when switched off and the kind is 94, 139, 100 or 58;
//  updateFlagBit: ToggleUpdateFlag, mask 0x4 of the halfword at +0x10, set for an argument of 0; the entity update loop 0x1a4094 skips an
//  entity whose halfword has mask 0x24 set (docs/re/script-natives-flags.md).
struct ObjectNativeFlags {
    std::optional<bool> usable;
    std::optional<bool> targetable;
    std::optional<bool> indestructible;
    std::optional<bool> activeDuringDcam;
    std::optional<bool> unshovable;
    std::optional<bool> shadow;
    std::optional<std::uint16_t> switchState;
    std::optional<bool> updateFlagBit;
};

class ObjectNativeState {
public:
    void apply(ObjectNativeKind kind, const std::string& lowerCaseInstance, float value);
    // Kind byte of every placed object (key: lower-case instance) and of the objects nested in class files (key: parent.child, as scripts
    // name them). ToggleUseable on a kind 71 object also writes its nested kind 72 objects (0x1cdcf8).
    void loadKinds(const assets::Level& level, const assets::ClassLookup& classOf);
    std::optional<std::uint8_t> kindOf(const std::string& lowerCaseInstance) const;
    // enableobjectfade sends message 24 first; true when no handler of the instance's kind answers it, so the native ends like EnableObject
    // without the message 251 (0x26fb10 / 0x26fb1c). An instance of an unknown kind counts as not answering.
    bool enableObjectFadeFallsBack(const std::string& lowerCaseInstance) const;
    // 0x23cd80: nothing when the entity is `inactive` (disabled, halfword +0x10 bit 0, or destroyed, word +0xc bit 0x10). Otherwise message 1 goes
    // to the class (`classMessage` is the host's effect of the handler) and the entity is destroyed whatever the class does, because no handler
    // returns non-zero for message 1 (bit 0x10 of +0xc, collision body freed by 0x2320f8). `reaction` says what the class's message does (an
    // instance of an unknown kind has none); `triggerObject` is the object the base prop handler 0x1f8948 enables and triggers (parameter
    // TriggerObject of the kinds destroyRunsPropHandler), empty for the others.
    using ClassMessage = std::function<void(const std::string&, std::uint8_t)>;
    struct DestroyResult {
        DestroyReaction reaction = DestroyReaction::NoReaction;
        std::string triggerObject;
    };
    std::optional<DestroyResult> destroy(const std::string& lowerCaseInstance, bool inactive, const ClassMessage& classMessage);
    // Message 8 of the default handler 0x221800: for kinds 64 and 132 the placement is restored, the entity enabled and the destroyed bit cleared
    // (the caller clears the state bits); false for every other kind (their handlers are not ported).
    bool resetDefaultHandler(const std::string& lowerCaseInstance);
    const PlacementReset* restoredPlacement(const std::string& lowerCaseInstance) const;
    bool destroyed(const std::string& lowerCaseInstance) const { return destroyed_.count(lowerCaseInstance) != 0; }
    const ObjectNativeFlags* flags(const std::string& lowerCaseInstance) const;
    bool entityUpdateSkipped(const std::string& lowerCaseInstance) const;
    std::vector<ObjectClassMessage> takeMessages();

private:
    void setUsable(const std::string& instance, bool value);
    std::map<std::string, ObjectNativeFlags> flags_;
    std::map<std::string, std::uint8_t> kinds_;
    std::map<std::string, std::vector<std::string>> pushHandles_;
    std::vector<ObjectClassMessage> messages_;
    std::set<std::string> destroyed_;
    std::map<std::string, PlacementReset> initialPlacements_;
    std::map<std::string, PlacementReset> restoredPlacements_;
    std::map<std::string, std::string> destroyTriggers_;
};

}
