#pragma once
// The reference-trigger step 0x23211c (docs/re/script-triggers.md): one entry per reference record that a class registered in the list at
// 0x591d88 (count 0x377424) through 0x269d2c, called by the loaders 0x233de8 and 0x23e84c with the class parameter TriggerObject. No SDL dependency.

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace chase::runtime {

enum class ReferenceKind : std::uint8_t { Idle = 0, Registered = 1, Fired = 2 };

// The trigger event types the step raises: the globals *0x3775d0, *0x3775d4, *0x3775d8 and *0x3775dc (types registered by 0x16bac4 as
// "ObjectTriggered", "ObjectUntriggered", "ObjectSwitched" and "ObjectUnswitched").
enum class ReferenceEvent : std::uint8_t { ObjectTriggered, ObjectUntriggered, ObjectSwitched, ObjectUnswitched };
const char* referenceEventName(ReferenceEvent event);

constexpr std::uint8_t kReferenceMessageJoinWaiting = 10;
constexpr std::uint8_t kReferenceMessageOn = 0xff;
constexpr std::uint8_t kReferenceMessageOff = 0xfe;
constexpr std::uint8_t kReferenceNoNavNode = 0xff;

enum class ReferenceEffectKind : std::uint8_t {
    Event,    // 0x23cee8(type, object, instigator, 0xff, 0)
    Message,  // 0x244fc8(object, message, instigator)
    Enable,   // 0x242cf0(object)
    NavNode,  // 0x22c470(*0x37754c, navNode, active)
};

struct ReferenceEffect {
    ReferenceEffectKind kind = ReferenceEffectKind::Event;
    ReferenceEvent event = ReferenceEvent::ObjectTriggered;
    std::string object;      // lower case; the receiver of an event, message or enable
    std::string instigator;  // lower case
    std::uint8_t message = 0;
    std::uint8_t navNode = kReferenceNoNavNode;
    bool navNodeActive = false;
};

struct ReferenceActivation {
    bool ran = false;         // false when the step returned at once (no record, kind 1 on activate, kind 0 on deactivate)
    bool fired = false;       // activate: every reference to the target is active, the target is enabled, messaged and its ObjectTriggered raised
    bool joinWaiting = false; // activate: another reference to the same target is still idle
    std::string target;       // lower case, empty when the reference is unset
    std::vector<std::size_t> owners;  // activate: references switched to Fired (message 0xff to each owner)
    std::vector<ReferenceEffect> effects;  // in the order the original performs them
};

class ReferenceTriggers {
public:
    void clear() { entries_.clear(); }
    // 0x269d2c plus the NavNode value of the loader (255 when the object has none). `lowerTarget` is empty when the reference is unset or names no object.
    std::size_t add(const std::string& lowerOwner, const std::string& lowerTarget, std::uint8_t navNode = kReferenceNoNavNode);
    // Activate mode (r2 = 1). Returns nothing when the reference is already Registered (0x2321b4). Otherwise it becomes Registered and the owner's
    // ObjectSwitched is raised; with a target set, message 10 goes to the target and, only when no other reference to it is Idle (0x232234),
    // every reference to it becomes Fired and gets message 0xff to its owner, then the target is enabled, gets message 0xff and its ObjectTriggered.
    ReferenceActivation activate(std::size_t id);
    // Deactivate mode (r2 = 0, 0x23226c): returns at once for an Idle reference; a Fired reference with a target sets every reference to it back to
    // Registered (message 0xfe to each owner), sends 0xfe to the target and raises its ObjectUntriggered; then the owner's ObjectUnswitched is
    // raised and the reference is Idle.
    ReferenceActivation deactivate(std::size_t id);
    // The same with ref = 0: the last entry whose owner is `lowerOwner` (0x23211c searches the list from the start and keeps the last match).
    std::optional<std::size_t> findOwner(const std::string& lowerOwner) const;
    ReferenceActivation activateOwner(const std::string& lowerOwner);
    ReferenceActivation deactivateOwner(const std::string& lowerOwner);
    ReferenceKind kind(std::size_t id) const { return entries_[id].kind; }
    const std::string& owner(std::size_t id) const { return entries_[id].owner; }
    const std::string& target(std::size_t id) const { return entries_[id].target; }
    std::uint8_t navNode(std::size_t id) const { return entries_[id].navNode; }
    std::size_t size() const { return entries_.size(); }

private:
    struct Entry {
        std::string owner;
        std::string target;
        std::uint8_t navNode = kReferenceNoNavNode;
        ReferenceKind kind = ReferenceKind::Idle;
    };
    bool otherIdleReference(const std::string& target) const;
    static ReferenceEffect event(ReferenceEvent type, const std::string& object, const std::string& instigator);
    static ReferenceEffect message(const std::string& object, std::uint8_t message, const std::string& instigator);
    static ReferenceEffect navNodeEffect(std::uint8_t navNode, bool active);
    std::vector<Entry> entries_;
};

}
