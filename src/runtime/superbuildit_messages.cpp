#include "runtime/superbuildit_messages.hpp"

namespace chase::runtime {

void disableSuperBuilditSave(SuperBuilditObject& object, bool disabled) { object.saveDisabled = disabled; }

SuperBuilditMessageResult superBuilditMessage(SuperBuilditObject& object, std::uint8_t message, assets::PlacedObjectState& state, const SuperBuilditVariable& variable) {
    SuperBuilditMessageResult result;
    switch (message) {
    case kMessageReset:
        state.disabled.erase(object.instance);
        if (!object.propToReveal.empty()) state.disabled.insert(object.propToReveal);
        object.zoneActive = true;
        object.state = std::uint16_t(SuperBuilditPhase::Off);
        object.requested = std::uint16_t(SuperBuilditPhase::Waiting);
        result.handled = true;
        break;
    case kSuperBuilditMessageClose:
        if (object.state == std::uint16_t(SuperBuilditPhase::Waiting)) object.requested = std::uint16_t(SuperBuilditPhase::Off);
        result.handled = true;
        break;
    case kSuperBuilditMessageStory: {
        result.handled = true;
        if (assets::nameHash(object.instance) != assets::nameHash(kSuperBuilditStoryPlate)) break;
        const std::optional<float> story = variable(kSuperBuilditStoryVariable);
        const std::optional<float> built = variable(kSuperBuilditBuiltVariable);
        if (!story || std::int32_t(*story) != kSuperBuilditStoryValue) break;
        if (!built || std::int32_t(*built) == 0) break;
        object.value = 0;
        result.plateValueCleared = true;
        result.completeBrick = kSuperBuilditStoryBrick;
        break;
    }
    case kSuperBuilditMessageSounds:
        result.handled = true;
        break;
    default:
        break;
    }
    return result;
}

std::uint8_t superBuilditSaveByte(const SuperBuilditObject& object) { return object.saveDisabled ? 0 : (object.built ? 1 : 0); }

std::vector<SuperBuilditRecord> superBuilditRecords(const SuperBuilditSet& set) {
    std::vector<SuperBuilditRecord> records;
    for (const SuperBuilditObject& object : set.objects())
        if (object.saveState) records.push_back({assets::nameHash(object.instance), superBuilditSaveByte(object)});
    return records;
}

std::size_t restoreSuperBuildits(SuperBuilditSet& set, const std::vector<SuperBuilditRecord>& saved, assets::PlacedObjectState& state, ScriptRunner& scripts) {
    std::size_t restored = 0;
    for (const SuperBuilditRecord& record : saved) {
        if (!(record.payload & 1)) continue;
        for (const SuperBuilditObject& candidate : set.objects()) {
            if (!candidate.saveState || assets::nameHash(candidate.instance) != record.hash) continue;
            SuperBuilditObject& object = *set.findMutable(candidate.instance);
            object.built = true;
            state.disabled.insert(object.instance);
            if (!object.hideObject.empty()) state.disabled.insert(object.hideObject);
            if (!object.propToReveal.empty()) state.disabled.erase(object.propToReveal);
            if (!object.buildCompleteTrigger.empty()) scripts.triggerObjectEnabling(object.buildCompleteTrigger);
            if (!object.useObject.empty()) state.disabled.erase(object.useObject);
            ++restored;
            break;
        }
    }
    return restored;
}

}
