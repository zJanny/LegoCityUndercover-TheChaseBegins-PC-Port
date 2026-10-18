#include "runtime/natives_objects.hpp"

#include "assets/object_classes.hpp"
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <stdexcept>

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
    {"destroyprop", ObjectNativeKind::DestroyProp, false, true},
    {"killobject", ObjectNativeKind::KillObject, false, true},
    {"enableshadow", ObjectNativeKind::EnableShadow, true, true},
    {"enableswitch", ObjectNativeKind::EnableSwitch, true, true},
};

constexpr std::uint8_t kKindCharacter = 27;
constexpr std::uint8_t kKindShadowSecond = 149;
constexpr std::uint8_t kSwitchResetKinds[] = {94, 139, 100, 58};
constexpr std::uint16_t kSwitchOffState = 3;
constexpr int kMaxNesting = 16;

std::string lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return text;
}

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
    case ObjectNativeKind::ToggleUseable: {
        if (kindOf(instance) == kKindPushable) {
            const auto handles = pushHandles_.find(instance);
            if (handles != pushHandles_.end())
                for (const std::string& handle : handles->second) flags_[handle].usable = value > 0.0f;
        }
        flags_[instance].usable = value > 0.0f;
        break;
    }
    case ObjectNativeKind::EnableTargetable: flags_[instance].targetable = (static_cast<std::int32_t>(value) & 0xff) != 0; break;
    case ObjectNativeKind::SetIndestructible: flags_[instance].indestructible = value != 0.0f; break;
    case ObjectNativeKind::ActiveDuringDcam: flags_[instance].activeDuringDcam = value != 0.0f; break;
    case ObjectNativeKind::ObjectUnshovable: flags_[instance].unshovable = value != 0.0f; break;
    case ObjectNativeKind::ToggleUpdateFlag: flags_[instance].updateFlagBit = value == 0.0f; break;
    case ObjectNativeKind::ResetObject: messages_.push_back({instance, kMessageReset}); break;
    case ObjectNativeKind::SkipToFinalState: messages_.push_back({instance, kMessageSkipToFinalState}); break;
    case ObjectNativeKind::EnableShadow: {
        const auto kind = kindOf(instance);
        if (kind && (*kind == kKindCharacter || *kind == kKindShadowSecond)) flags_[instance].shadow = value > 0.0f;
        break;
    }
    case ObjectNativeKind::EnableSwitch: {
        ObjectNativeFlags& flags = flags_[instance];
        if (value > 0.0f) {
            messages_.push_back({instance, kMessageReset});
            flags.usable = true;
            break;
        }
        const auto kind = kindOf(instance);
        if (kind && std::find(std::begin(kSwitchResetKinds), std::end(kSwitchResetKinds), *kind) != std::end(kSwitchResetKinds))
            flags.switchState = kSwitchOffState;
        flags.usable = false;
        break;
    }
    case ObjectNativeKind::PrintDebugString:
    case ObjectNativeKind::RevealCollectable:
    case ObjectNativeKind::DestroyProp:
    case ObjectNativeKind::KillObject: break;
    }
}

namespace {
std::vector<std::string> registerNested(const std::string& path, const assets::Level& classFile, const assets::ClassLookup& classOf,
                                        std::map<std::string, std::uint8_t>& kinds, int depth) {
    if (depth > kMaxNesting) throw std::runtime_error("object classes nest too deeply at " + path);
    std::vector<std::string> descendants;
    for (const assets::LevelObject& child : classFile.objects) {
        const assets::Level* childClass = classOf(child.className);
        if (!childClass) continue;
        const std::string childPath = path + "." + lower(child.instance);
        kinds[childPath] = assets::objectKind(child, *childClass);
        descendants.push_back(childPath);
        for (std::string& below : registerNested(childPath, *childClass, classOf, kinds, depth + 1)) descendants.push_back(std::move(below));
    }
    return descendants;
}
}

void ObjectNativeState::loadKinds(const assets::Level& level, const assets::ClassLookup& classOf) {
    kinds_.clear();
    pushHandles_.clear();
    for (const assets::LevelObject& object : level.objects) {
        const assets::Level* classFile = classOf(object.className);
        if (!classFile) continue;
        const std::string path = lower(object.instance);
        kinds_[path] = assets::objectKind(object, *classFile);
        std::vector<std::string> descendants = registerNested(path, *classFile, classOf, kinds_, 0);
        if (kinds_[path] != kKindPushable) continue;
        std::vector<std::string>& handles = pushHandles_[path];
        for (std::string& descendant : descendants)
            if (kinds_[descendant] == kKindPushHandle) handles.push_back(std::move(descendant));
    }
}

std::optional<std::uint8_t> ObjectNativeState::kindOf(const std::string& instance) const {
    const auto found = kinds_.find(instance);
    if (found == kinds_.end()) return std::nullopt;
    return found->second;
}

bool ObjectNativeState::destroy(const std::string& instance, bool disabled, const ClassHandles& classHandles) {
    if (disabled || destroyed_.count(instance)) return false;
    messages_.push_back({instance, kMessageDestroy});
    if (classHandles && classHandles(instance, kMessageDestroy)) return false;
    destroyed_.insert(instance);
    return true;
}

const ObjectNativeFlags* ObjectNativeState::flags(const std::string& instance) const {
    const auto found = flags_.find(instance);
    return found == flags_.end() ? nullptr : &found->second;
}

bool ObjectNativeState::entityUpdateSkipped(const std::string& instance) const {
    const ObjectNativeFlags* found = flags(instance);
    return found && found->updateFlagBit.value_or(false);
}

std::vector<ObjectClassMessage> ObjectNativeState::takeMessages() {
    std::vector<ObjectClassMessage> taken;
    taken.swap(messages_);
    return taken;
}

}
