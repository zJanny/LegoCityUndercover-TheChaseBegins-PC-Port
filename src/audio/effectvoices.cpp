#include "audio/effectvoices.hpp"
#include <algorithm>

namespace chase::audio {

void EffectVoices::prune() {
    entries_.erase(std::remove_if(entries_.begin(), entries_.end(), [&](const Entry& e) { return !mixer_.playing(e.voice); }), entries_.end());
}

void EffectVoices::add(int voice, std::uint32_t id, std::uint16_t owner) {
    prune();
    entries_.push_back({voice, id, owner});
}

int EffectVoices::find(std::uint32_t id, std::uint16_t owner) {
    prune();
    const auto found = std::find_if(entries_.begin(), entries_.end(), [&](const Entry& e) { return e.id == id && e.owner == owner; });
    return found == entries_.end() ? -1 : found->voice;
}

bool EffectVoices::stop(std::uint32_t id, std::uint16_t owner, float fadeSeconds) {
    prune();
    const auto found = std::find_if(entries_.begin(), entries_.end(), [&](const Entry& e) { return e.id == id && e.owner == owner; });
    if (found == entries_.end()) return false;
    mixer_.fadeOut(found->voice, fadeSeconds);
    if (fadeSeconds <= 0.0f) entries_.erase(found);
    return true;
}

} // namespace chase::audio
