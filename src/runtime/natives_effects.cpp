#include "runtime/natives_effects.hpp"

#include <algorithm>

namespace chase::runtime {
namespace {

struct NativeEntry {
    const char* name;
    EffectNativeKind kind;
    unsigned arguments;
};

constexpr NativeEntry kEntries[] = {
    {"effectfadeobject", EffectNativeKind::EffectFadeObject, 5},
    {"enabledistancefade", EffectNativeKind::EnableDistanceFade, 2},
    {"addtosemitransnearcameralist", EffectNativeKind::AddToSemiTransNearCameraList, 2},
};

float alphaMultiplier(float value, std::uint8_t flags) {
    if (!(flags & (kFadeFlagAlpha | kFadeFlagColour))) return 1.0f;
    const float scaled = value * 255.0f;
    const int alpha = scaled <= 0.0f ? 0 : scaled >= 255.0f ? 255 : static_cast<int>(scaled);
    return float(alpha) / 255.0f;
}

}

std::optional<EffectNativeKind> effectNativeKind(const std::string& lowerCaseName) {
    for (const NativeEntry& entry : kEntries)
        if (lowerCaseName == entry.name) return entry.kind;
    return std::nullopt;
}

std::vector<std::string> effectNativeNames() {
    std::vector<std::string> names;
    for (const NativeEntry& entry : kEntries) names.push_back(entry.name);
    return names;
}

unsigned effectNativeArguments(EffectNativeKind kind) {
    for (const NativeEntry& entry : kEntries)
        if (entry.kind == kind) return entry.arguments;
    return 0;
}

void EffectNativeState::apply(const Entry& entry, float value) {
    ObjectFade& fade = faded_[entry.instance];
    fade.hidden = value == 0.0f;
    fade.multiplier = alphaMultiplier(value == 0.0f ? entry.from : value, entry.flags);
}

void EffectNativeState::evaluate(std::size_t index, float elapsed) {
    Entry& entry = entries_[index];
    if (entry.duration != 0.0f) {
        float value = entry.from + (entry.to - entry.from) * (elapsed / entry.duration);
        value = value >= 1.0f ? 1.0f : value > 0.0f ? value : 0.0f;
        entry.value = value;
        apply(entry, value);
    }
}

bool EffectNativeState::finishes(std::size_t index) {
    const Entry& entry = entries_[index];
    if (entry.duration > entry.elapsed) return false;
    if (entry.to == 0.0f) faded_[entry.instance].hidden = true;
    entries_[index] = entries_.back();
    entries_.pop_back();
    return true;
}

void EffectNativeState::fadeObject(const std::string& instance, float from, float to, float duration, std::uint8_t flags) {
    std::size_t at = entries_.size();
    for (std::size_t i = 0; i < entries_.size(); ++i)
        if (entries_[i].instance == instance) {
            at = i;
            break;
        }
    if (at == entries_.size() && entries_.size() >= kNodeTweenLimit) {
        std::size_t oldest = 0;
        for (std::size_t i = 1; i < entries_.size(); ++i)
            if (entries_[i].elapsed > entries_[oldest].elapsed) oldest = i;
        evaluate(oldest, entries_[oldest].duration);
        entries_[oldest].elapsed = entries_[oldest].duration;
        finishes(oldest);
        at = entries_.size();
    }
    Entry entry;
    entry.instance = instance;
    entry.from = from;
    entry.to = to;
    entry.duration = duration;
    entry.flags = flags;
    entry.value = from;
    if (at == entries_.size()) entries_.push_back(entry);
    else entries_[at] = entry;
    apply(entry, from);
}

void EffectNativeState::update(float dt) {
    for (std::size_t i = 0; i < entries_.size();) {
        entries_[i].elapsed += dt;
        evaluate(i, entries_[i].elapsed);
        if (!finishes(i)) ++i;
    }
}

std::optional<ObjectFade> EffectNativeState::fade(const std::string& instance) const {
    const auto found = faded_.find(instance);
    if (found == faded_.end()) return std::nullopt;
    return found->second;
}

std::optional<bool> EffectNativeState::distanceFade(const std::string& instance) const {
    const auto found = distanceFade_.find(instance);
    if (found == distanceFade_.end()) return std::nullopt;
    return found->second;
}

void EffectNativeState::addSemiTransNearCamera(const std::string& model, const std::string& tag) {
    const SemiTransNearCameraEntry entry{model, tag};
    if (std::find(semiTransNearCamera_.begin(), semiTransNearCamera_.end(), entry) == semiTransNearCamera_.end()) semiTransNearCamera_.push_back(entry);
}

}
