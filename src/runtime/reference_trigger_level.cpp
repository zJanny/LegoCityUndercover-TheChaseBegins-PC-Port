#include "runtime/reference_trigger_level.hpp"

#include "assets/object_classes.hpp"
#include "runtime/level_start.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <map>

namespace chase::runtime {
namespace {

constexpr std::uint32_t kLoaderTriggerObject = 0x233de8;
constexpr std::uint32_t kLoaderSwitch = 0x23e84c;
constexpr std::uint8_t kCreateFlagNeverCreated = 1;

std::string lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return text;
}

std::uint8_t navNodeValue(const assets::LevelParam* param) {
    if (!param) return kReferenceNoNavNode;
    if (param->type == 5 || param->type == 7) return std::uint8_t(param->value & 0xff);
    if (param->type != 0) return kReferenceNoNavNode;
    float value;
    std::memcpy(&value, &param->value, 4);
    return std::uint8_t(std::int32_t(value) & 0xff);
}

}

const std::vector<ReferenceLoaderKind>& referenceLoaderKinds() {
    static const std::vector<ReferenceLoaderKind> kinds = {
        {14, kLoaderTriggerObject, 0x1a5b70},   {21, kLoaderTriggerObject, 0x16c570},  {42, kLoaderTriggerObject, 0x169650},
        {64, kLoaderTriggerObject, 0x18b7ec},   {67, kLoaderTriggerObject, 0x16aa74},  {98, kLoaderTriggerObject, 0x1af0d0},
        {102, kLoaderTriggerObject, 0x181714},  {38, kLoaderSwitch, 0x1de734},         {58, kLoaderSwitch, 0x175218},
        {100, kLoaderSwitch, 0x181590},         {138, kLoaderSwitch, 0x15a060},        {139, kLoaderSwitch, 0x19e04c},
        {185, kLoaderSwitch, 0x18d864},         {186, kLoaderSwitch, 0x25fe30},        {201, kLoaderSwitch, 0x19f748},
        {222, kLoaderSwitch, 0x1adc4c},         {227, kLoaderSwitch, 0x1c81d4},        {232, kLoaderSwitch, 0x1d8ce4},
    };
    return kinds;
}

bool kindRegistersReference(std::uint8_t kind) {
    const std::vector<ReferenceLoaderKind>& kinds = referenceLoaderKinds();
    return std::any_of(kinds.begin(), kinds.end(), [kind](const ReferenceLoaderKind& entry) { return entry.kind == kind; });
}

void registerLevelReferences(ReferenceTriggers& references, const assets::Level& level, const assets::ClassLookup& classOf) {
    references.clear();
    std::map<std::uint32_t, std::string> names;
    for (const assets::LevelObject& object : level.objects) names[assets::nameHash(object.instance)] = lower(object.instance);
    const std::uint32_t triggerObject = assets::nameHash("TriggerObject"), navNode = assets::nameHash("NavNode");
    for (const assets::LevelObject& object : level.objects) {
        if (object.createFlags & kCreateFlagNeverCreated) continue;
        const assets::Level* cls = classOf(object.className);
        if (!cls || !kindRegistersReference(assets::objectKind(object, *cls))) continue;
        std::string target;
        if (const assets::LevelParam* param = effectiveParam(object, *cls, triggerObject)) {
            const auto found = names.find(assets::objectReferenceHash(*param));
            if (found != names.end()) target = found->second;
        }
        references.add(lower(object.instance), target, navNodeValue(effectiveParam(object, *cls, navNode)));
    }
}

}
