#include "runtime/natives_objects.hpp"

#include <cstdint>

namespace chase::runtime {
namespace {

struct Entry {
    const char* name;
    ObjectNativeKind kind;
    bool takesValue;
    bool standIn;
};

constexpr Entry kEntries[] = {
    {"toggleuseable", ObjectNativeKind::ToggleUseable, true, true},
    {"enabletargetable", ObjectNativeKind::EnableTargetable, true, true},
    {"setindestructible", ObjectNativeKind::SetIndestructible, true, true},
    {"activeduringdcam", ObjectNativeKind::ActiveDuringDcam, true, true},
    {"objectunshovable", ObjectNativeKind::ObjectUnshovable, true, true},
    {"toggleupdateflag", ObjectNativeKind::ToggleUpdateFlag, true, true},
    {"resetobject", ObjectNativeKind::ResetObject, false, true},
    {"skiptofinalstate", ObjectNativeKind::SkipToFinalState, false, true},
    {"printdebugstring", ObjectNativeKind::PrintDebugString, false, false},
    {"revealcollectable", ObjectNativeKind::RevealCollectable, false, false},
};

}

std::optional<ObjectNativeKind> objectNativeKind(const std::string& lowerCaseName) {
    for (const Entry& entry : kEntries)
        if (lowerCaseName == entry.name) return entry.kind;
    return std::nullopt;
}

std::vector<std::string> objectNativeNames() {
    std::vector<std::string> names;
    for (const Entry& entry : kEntries) names.push_back(entry.name);
    return names;
}

bool objectNativeTakesValue(ObjectNativeKind kind) {
    for (const Entry& entry : kEntries)
        if (entry.kind == kind) return entry.takesValue;
    return false;
}

bool objectNativeIsStandIn(ObjectNativeKind kind) {
    for (const Entry& entry : kEntries)
        if (entry.kind == kind) return entry.standIn;
    return false;
}

void ObjectNativeState::apply(ObjectNativeKind kind, const std::string& instance, float value) {
    switch (kind) {
    case ObjectNativeKind::ToggleUseable: flags_[instance].usable = value > 0.0f; break;
    case ObjectNativeKind::EnableTargetable: flags_[instance].targetable = (static_cast<std::int32_t>(value) & 0xff) != 0; break;
    case ObjectNativeKind::SetIndestructible: flags_[instance].indestructible = value != 0.0f; break;
    case ObjectNativeKind::ActiveDuringDcam: flags_[instance].activeDuringDcam = value != 0.0f; break;
    case ObjectNativeKind::ObjectUnshovable: flags_[instance].unshovable = value != 0.0f; break;
    case ObjectNativeKind::ToggleUpdateFlag: flags_[instance].updateFlagBit = value == 0.0f; break;
    case ObjectNativeKind::ResetObject: messages_.push_back({instance, kMessageReset}); break;
    case ObjectNativeKind::SkipToFinalState: messages_.push_back({instance, kMessageSkipToFinalState}); break;
    case ObjectNativeKind::PrintDebugString:
    case ObjectNativeKind::RevealCollectable: break;
    }
}

const ObjectNativeFlags* ObjectNativeState::flags(const std::string& instance) const {
    const auto found = flags_.find(instance);
    return found == flags_.end() ? nullptr : &found->second;
}

std::vector<ObjectClassMessage> ObjectNativeState::takeMessages() {
    std::vector<ObjectClassMessage> taken;
    taken.swap(messages_);
    return taken;
}

}
