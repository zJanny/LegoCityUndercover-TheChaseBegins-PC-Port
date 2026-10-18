#pragma once
// Script natives that write one flag of an object or send it a class message (batch SCRIPT-26, docs/re/script-natives.md).
// The handlers only store the value; the systems that read the flags (targeting, interaction prompts, shove, destruction,
// door update) are not ported yet, so the state is recorded here. No SDL dependency.
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
};

std::optional<ObjectNativeKind> objectNativeKind(const std::string& lowerCaseName);
std::vector<std::string> objectNativeNames();
bool objectNativeTakesValue(ObjectNativeKind kind);
bool objectNativeIsStandIn(ObjectNativeKind kind);

// ResetObject 0x1b25f8 and SkipToFinalState 0x270078 call the class message handler 0x244fc8 with these numbers.
constexpr std::uint8_t kMessageReset = 8;
constexpr std::uint8_t kMessageSkipToFinalState = 54;
// DestroyProp / KillObject 0x1a2fe4 -> 0x23cd80 send the class message 1 (the vehicle class handles it as its break, 0x159bd0).
constexpr std::uint8_t kMessageDestroy = 1;
struct ObjectClassMessage {
    std::string instance;
    std::uint8_t message = 0;
};

// Values the natives wrote, unset until a script wrote them. Meaning of the original bits:
//  usable: ToggleUseable, registry flags set by 0x236624 and 0x2219bc, true for an argument above 0;
//  targetable: EnableTargetable, false when the argument truncated to an integer has a zero low byte (+0xda of the component);
//  indestructible: SetIndestructible, bit 0 of +0xde of the component;
//  activeDuringDcam: ActiveDuringDCam, bit 1 of +0x312 of the component;
//  unshovable: ObjectUnshovable, bit 0x100 of the flag word at +0x10 and bit 0x20000 of the one at +0xc;
//  updateFlagBit: ToggleUpdateFlag, mask 0x4 of the halfword at +0x10, set for an argument of 0; the entity update loop 0x1a4094 skips an
//  entity whose halfword has mask 0x24 set (docs/re/script-natives-flags.md).
struct ObjectNativeFlags {
    std::optional<bool> usable;
    std::optional<bool> targetable;
    std::optional<bool> indestructible;
    std::optional<bool> activeDuringDcam;
    std::optional<bool> unshovable;
    std::optional<bool> updateFlagBit;
};

class ObjectNativeState {
public:
    void apply(ObjectNativeKind kind, const std::string& lowerCaseInstance, float value);
    // 0x23cd80: nothing when the entity is disabled (halfword +0x10 bit 0) or already destroyed (word +0xc bit 0x10). Otherwise message 1 goes to
    // the class (`classHandles` says whether its handler took it); an unhandled message destroys the entity (bit 0x10 of +0xc, node dropped
    // by 0x2320f8). True when the entity was destroyed by this call.
    using ClassHandles = std::function<bool(const std::string&, std::uint8_t)>;
    bool destroy(const std::string& lowerCaseInstance, bool disabled, const ClassHandles& classHandles);
    bool destroyed(const std::string& lowerCaseInstance) const { return destroyed_.count(lowerCaseInstance) != 0; }
    const ObjectNativeFlags* flags(const std::string& lowerCaseInstance) const;
    bool entityUpdateSkipped(const std::string& lowerCaseInstance) const;
    std::vector<ObjectClassMessage> takeMessages();

private:
    std::map<std::string, ObjectNativeFlags> flags_;
    std::vector<ObjectClassMessage> messages_;
    std::set<std::string> destroyed_;
};

}
