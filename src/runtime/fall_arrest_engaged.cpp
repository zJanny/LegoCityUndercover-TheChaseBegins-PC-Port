#include "runtime/fall_arrest_engaged.hpp"

#include <algorithm>

namespace chase::runtime {

EngageResult EngagedTable::engage(const std::string& name, bool badGuy) {
    EngageResult result;
    if (engaged(name)) {
        result.alreadyEngaged = true;
        return result;
    }
    result.counterCleared = true;
    std::array<std::string, kEngagedListSlots>& slots = badGuy ? bad_ : other_;
    for (std::string& slot : slots)
        if (slot.empty()) {
            slot = name;
            members_[name] = badGuy;
            result.joined = true;
            return result;
        }
    result.listFull = true;
    return result;
}

bool EngagedTable::release(const std::string& name, bool badGuy, bool boss, bool aiRequestIsOne) {
    if (boss && !aiRequestIsOne) return false;
    if (!engaged(name)) return false;
    std::array<std::string, kEngagedListSlots>& slots = badGuy ? bad_ : other_;
    for (std::string& slot : slots)
        if (slot == name) {
            slot.clear();
            break;
        }
    members_.erase(name);
    return true;
}

std::size_t EngagedTable::count(bool badGuy) const {
    const std::array<std::string, kEngagedListSlots>& slots = badGuy ? bad_ : other_;
    return std::size_t(std::count_if(slots.begin(), slots.end(), [](const std::string& s) { return !s.empty(); }));
}

void EngagedTable::setOnGround(const std::string& name, bool onGround) {
    if (onGround) airborne_.erase(name);
    else airborne_[name] = true;
}

bool EngagedTable::onGround(const std::string& name) const { return airborne_.count(name) == 0; }

std::uint8_t EngagedTable::mark(const std::string& name) const {
    const auto found = marks_.find(name);
    return found == marks_.end() ? std::uint8_t(0) : found->second;
}

std::vector<FallArrestHit> EngagedTable::fallArrestUpdate(const assets::Level& level, const std::vector<std::size_t>& bounds,
                                                          const std::vector<std::uint8_t>& boundEnabled, const Describe& describe) {
    std::vector<FallArrestCharacter> characters;
    for (const std::string& slot : bad_) {
        if (slot.empty()) continue;
        std::optional<FallArrestCharacter> described = describe(slot);
        if (!described) continue;
        described->name = slot;
        described->onGround = onGround(slot);
        described->arrestMark = mark(slot);
        characters.push_back(*described);
    }
    std::vector<FallArrestHit> hits = chase::runtime::fallArrestUpdate(level, bounds, boundEnabled, characters);
    for (const FallArrestCharacter& character : characters) marks_[character.name] = character.arrestMark;
    return hits;
}

std::uint16_t fallArrestSoundId(const std::array<std::uint16_t, assets::kCharacterSoundSlots>& row) { return row[kFallArrestSoundSlot]; }

}
