#include "runtime/switch_set.hpp"

#include "assets/object_classes.hpp"
#include "runtime/level_start.hpp"
#include "runtime/stud_drop_classes.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>

namespace chase::runtime {
namespace {

constexpr std::uint8_t kCreateFlagNeverCreated = 1;

std::string lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return text;
}

LeverPart readLever(const assets::LevelObject& object, const assets::Level& cls) {
    LeverPart lever;
    lever.leverType = switchIntParam(object, cls, "LeverType", 0) & 0x1f;
    lever.strong = switchIntParam(object, cls, "Strong", 0) != 0;
    lever.leverAbility = switchIntParam(object, cls, "LeverAbility", 0);
    lever.holdMe = switchIntParam(object, cls, "HoldMe", 0) != 0;
    lever.usable = switchIntParam(object, cls, "UseStartActive", 1) != 0;
    const auto bound = std::find_if(cls.bounds.begin(), cls.bounds.end(), [](const assets::LevelBound& b) { return lower(b.name) == "usebound"; });
    if (bound != cls.bounds.end()) {
        lever.hasZone = true;
        std::copy_n(bound->centre, 3, lever.zoneCentre);
        std::copy_n(bound->half, 3, lever.zoneHalf);
    }
    return lever;
}

}

std::vector<int> leverAbilities(const LeverPart& lever) {
    std::vector<int> abilities = {1, int(lever.leverAbility)};
    if (lever.strong) abilities.push_back(7);
    return abilities;
}

void SwitchSet::load(const assets::Level& level, const assets::ClassLookup& classOf,
                     const std::function<bool(const assets::LevelObject&, const assets::Level&)>& hasUseAnimation) {
    objects_.clear();
    for (std::size_t index = 0; index < level.objects.size(); ++index) {
        const assets::LevelObject& object = level.objects[index];
        if (object.createFlags & kCreateFlagNeverCreated) continue;
        const assets::Level* cls = classOf(object.className);
        if (!cls) continue;
        const std::uint8_t kind = std::uint8_t(assets::objectKind(object, *cls));
        if (kind != kSwitchKindLight && kind != kSwitchKindLever) continue;
        const std::optional<assets::Placement> placed = assets::objectStart(level, object.instance);
        SwitchObject sw;
        sw.instance = lower(object.instance);
        sw.kind = kind;
        sw.objectIndex = index;
        if (placed) {
            std::copy_n(placed->position, 3, sw.position);
            sw.matrix = assets::placementMatrix(*placed);
        }
        sw.params = readSwitchParams(object, *cls);
        sw.params.hasUseAnim = hasUseAnimation && hasUseAnimation(object, *cls);
        if (kind == kSwitchKindLever) sw.lever = readLever(object, *cls);
        sw.studValue = studValueParameter(object, *cls, 0);
        sw.studSpawn = studSpawnSource(level, object, *cls);
        objects_.push_back(std::move(sw));
    }
}

std::vector<SwitchEvent> SwitchSet::link(ReferenceTriggers& references) {
    std::vector<SwitchEvent> events;
    for (std::size_t i = 0; i < objects_.size(); ++i) {
        SwitchObject& sw = objects_[i];
        sw.reference = references.findOwner(sw.instance);
        if (switchPostLink(sw.record, sw.params) && sw.reference) references.setKind(*sw.reference, ReferenceKind::Registered);
        const ReferenceKind kind = sw.reference ? references.kind(*sw.reference) : ReferenceKind::Idle;
        if (kind == ReferenceKind::Registered || kind == ReferenceKind::Fired) {
            if (sw.params.hasUseAnim) events.push_back({i, {SwitchEffectKind::UseAnimAtEnd, 0}});
            events.push_back({i, {SwitchEffectKind::LightsOn, 0}});
        } else {
            events.push_back({i, {SwitchEffectKind::LightsOff, 0}});
        }
    }
    return events;
}

std::optional<std::size_t> SwitchSet::find(const std::string& lowerInstance) const {
    for (std::size_t i = 0; i < objects_.size(); ++i)
        if (objects_[i].instance == lowerInstance) return i;
    return std::nullopt;
}

bool SwitchSet::inZone(const SwitchObject& object, const float player[3]) {
    if (!object.lever || !object.lever->hasZone) return false;
    for (int a = 0; a < 3; ++a)
        if (!(std::fabs(player[a] - object.position[a] - object.lever->zoneCentre[a]) < object.lever->zoneHalf[a])) return false;
    return true;
}

void SwitchSet::runReference(std::size_t index, bool activate, ReferenceTriggers& references, ScriptRunner& scripts, std::vector<SwitchEvent>& events) {
    const std::optional<std::size_t> id = objects_[index].reference;
    if (!id) return;
    const ReferenceActivation activation = activate ? references.activate(*id) : references.deactivate(*id);
    for (const ReferenceEffect& effect : activation.effects) {
        switch (effect.kind) {
        case ReferenceEffectKind::Event: scripts.fireObjectEvent(referenceEventName(effect.event), effect.object); break;
        case ReferenceEffectKind::Enable: scripts.enableObject(effect.object); break;
        case ReferenceEffectKind::Message:
            if (const std::optional<std::size_t> receiver = find(effect.object)) handle(*receiver, effect.message, false, 1.0f, references, scripts, events);
            else scripts.sendObjectMessage(effect.object, effect.message);
            break;
        case ReferenceEffectKind::NavNode: break;
        }
    }
}

std::uint32_t SwitchSet::handle(std::size_t index, std::uint8_t message, bool fromPlayer, float argument, ReferenceTriggers& references,
                                ScriptRunner& scripts, std::vector<SwitchEvent>& events) {
    SwitchObject& sw = objects_[index];
    if (sw.lever) {
        LeverPart& lever = *sw.lever;
        switch (message) {
        case kSwitchMessageUse:
            if (sw.record.state == SwitchState::Off) sw.record.useRequested = true;
            return 0;
        case kLeverMessageCancelUse:
            if (sw.record.state == SwitchState::Used) sw.record.target = SwitchState::SwitchingOff;
            return 0;
        case kLeverMessageHoldMe: return lever.holdMe ? 1 : 0;
        case kSwitchMessageOff: sw.record.target = SwitchState::SwitchingOff; break;
        case kSwitchMessageRestorePose: lever.twitched = false; break;
        default: break;
        }
    }
    const std::uint8_t referenceKind = sw.reference ? std::uint8_t(references.kind(*sw.reference)) : 0;
    SwitchMessageResult result = switchBaseMessage(sw.record, sw.params, message, fromPlayer, argument, referenceKind);
    if (result.setRegistered && sw.reference) references.setKind(*sw.reference, ReferenceKind::Registered);
    for (const SwitchEffect& effect : result.effects) events.push_back({index, effect});
    (void)scripts;
    return result.returned;
}

bool SwitchSet::message(const std::string& lowerInstance, std::uint8_t message, ReferenceTriggers& references, ScriptRunner& scripts,
                        std::vector<SwitchEvent>& events, bool fromPlayer, float argument) {
    const std::optional<std::size_t> index = find(lowerInstance);
    if (!index) return false;
    handle(*index, message, fromPlayer, argument, references, scripts, events);
    return true;
}

std::optional<SwitchUse> SwitchSet::use(const assets::PlacedObjectState& state, const SwitchUseInput& input, const ReferenceTriggers& references) {
    if (!input.actionPressed || !input.playerFree || playerUse_) return std::nullopt;
    for (std::size_t i = 0; i < objects_.size(); ++i) {
        SwitchObject& sw = objects_[i];
        if (!sw.lever || state.isDisabled(sw.instance) || !inZone(sw, input.player)) continue;
        const LeverPart& lever = *sw.lever;
        if (!lever.usable) continue;
        if (sw.reference && references.kind(*sw.reference) == ReferenceKind::Registered) continue;
        if (sw.record.state != SwitchState::Off) continue;
        const std::vector<int> abilities = leverAbilities(lever);
        const bool able = std::all_of(abilities.begin(), abilities.end(), [&](int ability) { return input.playerHasAbility && input.playerHasAbility(ability); });
        if (!able) return SwitchUse{i, 0xff};
        playerUse_ = PlayerUse{i, 0};
        return SwitchUse{i, 1};
    }
    return std::nullopt;
}

std::vector<SwitchEvent> SwitchSet::update(const assets::PlacedObjectState& state, float dt, ReferenceTriggers& references, ScriptRunner& scripts) {
    std::vector<SwitchEvent> events;
    for (std::size_t i = 0; i < objects_.size(); ++i) {
        if (state.isDisabled(objects_[i].instance)) continue;
        std::vector<SwitchEffect> effects;
        if (objects_[i].record.state == objects_[i].record.target) switchStableUpdate(objects_[i].record, objects_[i].params, true, effects);
        switchStateChange(objects_[i].record, objects_[i].params, dt, [&](bool activate) {
            for (const SwitchEffect& effect : effects) events.push_back({i, effect});
            effects.clear();
            runReference(i, activate, references, scripts, events);
        }, effects);
        for (const SwitchEffect& effect : effects) events.push_back({i, effect});
        SwitchObject& sw = objects_[i];
        if (!sw.lever) continue;
        LeverPart& lever = *sw.lever;
        switch (sw.record.state) {
        case SwitchState::Off: lever.twitched = true; break;
        case SwitchState::Used:
        case SwitchState::Held: lever.usable = false; break;
        case SwitchState::SwitchingOff:
            lever.twitched = false;
            lever.usable = !lever.zoneBit6;
            break;
        default: break;
        }
    }
    if (playerUse_) {
        ++playerUse_->updates;
        if (playerUse_->updates == kLeverUseRequestUpdate) objects_[playerUse_->index].record.useRequested = true;
        if (playerUse_->updates >= kLeverUseEndUpdate) playerUse_.reset();
    }
    return events;
}

}
