#include "runtime/reference_trigger.hpp"

namespace chase::runtime {

const char* referenceEventName(ReferenceEvent event) {
    switch (event) {
    case ReferenceEvent::ObjectTriggered: return "objecttriggered";
    case ReferenceEvent::ObjectUntriggered: return "objectuntriggered";
    case ReferenceEvent::ObjectSwitched: return "objectswitched";
    case ReferenceEvent::ObjectUnswitched: return "objectunswitched";
    }
    return "";
}

std::size_t ReferenceTriggers::add(const std::string& lowerOwner, const std::string& lowerTarget, std::uint8_t navNode) {
    entries_.push_back({lowerOwner, lowerTarget, navNode, ReferenceKind::Idle});
    return entries_.size() - 1;
}

ReferenceEffect ReferenceTriggers::event(ReferenceEvent type, const std::string& object, const std::string& instigator) {
    ReferenceEffect effect;
    effect.kind = ReferenceEffectKind::Event;
    effect.event = type;
    effect.object = object;
    effect.instigator = instigator;
    return effect;
}

ReferenceEffect ReferenceTriggers::message(const std::string& object, std::uint8_t message, const std::string& instigator) {
    ReferenceEffect effect;
    effect.kind = ReferenceEffectKind::Message;
    effect.object = object;
    effect.instigator = instigator;
    effect.message = message;
    return effect;
}

ReferenceEffect ReferenceTriggers::navNodeEffect(std::uint8_t navNode, bool active) {
    ReferenceEffect effect;
    effect.kind = ReferenceEffectKind::NavNode;
    effect.navNode = navNode;
    effect.navNodeActive = active;
    return effect;
}

bool ReferenceTriggers::otherIdleReference(const std::string& target) const {
    for (const Entry& other : entries_)
        if (other.target == target && other.kind == ReferenceKind::Idle) return true;
    return false;
}

ReferenceActivation ReferenceTriggers::activate(std::size_t id) {
    ReferenceActivation result;
    Entry& entry = entries_[id];
    if (entry.kind == ReferenceKind::Registered) return result;
    result.ran = true;
    entry.kind = ReferenceKind::Registered;
    result.target = entry.target;
    const std::string owner = entry.owner;
    result.effects.push_back(event(ReferenceEvent::ObjectSwitched, owner, owner));
    if (!entry.target.empty()) {
        if (otherIdleReference(entry.target)) {
            result.joinWaiting = true;
            result.effects.push_back(message(entry.target, kReferenceMessageJoinWaiting, owner));
            return result;
        }
        result.effects.push_back(message(entry.target, kReferenceMessageJoinWaiting, owner));
        for (std::size_t i = 0; i < entries_.size(); ++i)
            if (entries_[i].target == entry.target) {
                entries_[i].kind = ReferenceKind::Fired;
                result.owners.push_back(i);
                result.effects.push_back(message(entries_[i].owner, kReferenceMessageOn, entries_[i].owner));
            }
        ReferenceEffect enable;
        enable.kind = ReferenceEffectKind::Enable;
        enable.object = entry.target;
        result.effects.push_back(enable);
        result.effects.push_back(message(entry.target, kReferenceMessageOn, owner));
        result.effects.push_back(event(ReferenceEvent::ObjectTriggered, entry.target, owner));
        result.fired = true;
    }
    if (entry.navNode != kReferenceNoNavNode) result.effects.push_back(navNodeEffect(entry.navNode, true));
    return result;
}

ReferenceActivation ReferenceTriggers::deactivate(std::size_t id) {
    ReferenceActivation result;
    Entry& entry = entries_[id];
    if (entry.kind == ReferenceKind::Idle) return result;
    result.ran = true;
    result.target = entry.target;
    const std::string owner = entry.owner;
    if (entry.kind == ReferenceKind::Fired && !entry.target.empty()) {
        for (Entry& other : entries_)
            if (other.target == entry.target) {
                other.kind = ReferenceKind::Registered;
                result.effects.push_back(message(other.owner, kReferenceMessageOff, other.owner));
            }
        result.effects.push_back(message(entry.target, kReferenceMessageOff, owner));
        result.effects.push_back(event(ReferenceEvent::ObjectUntriggered, entry.target, owner));
    }
    if (entry.navNode != kReferenceNoNavNode) result.effects.push_back(navNodeEffect(entry.navNode, false));
    result.effects.push_back(event(ReferenceEvent::ObjectUnswitched, owner, owner));
    entry.kind = ReferenceKind::Idle;
    return result;
}

std::optional<std::size_t> ReferenceTriggers::findOwner(const std::string& lowerOwner) const {
    std::optional<std::size_t> found;
    for (std::size_t i = 0; i < entries_.size(); ++i)
        if (entries_[i].owner == lowerOwner) found = i;
    return found;
}

ReferenceActivation ReferenceTriggers::activateOwner(const std::string& lowerOwner) {
    const std::optional<std::size_t> id = findOwner(lowerOwner);
    return id ? activate(*id) : ReferenceActivation{};
}

ReferenceActivation ReferenceTriggers::deactivateOwner(const std::string& lowerOwner) {
    const std::optional<std::size_t> id = findOwner(lowerOwner);
    return id ? deactivate(*id) : ReferenceActivation{};
}

}
