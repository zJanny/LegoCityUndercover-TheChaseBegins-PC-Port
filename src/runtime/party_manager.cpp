#include "runtime/party_manager.hpp"

#include <cstdio>
#include <utility>

namespace chase::runtime {
namespace {
constexpr std::array<std::uint8_t, kPartyLevelIds> kCheatPartyIds = {9, 19, 29, 39, 49, 59, 69, 80};
constexpr std::array<std::uint8_t, 7> kGroupCharacters = {9, 19, 29, 39, 49, 59, 69};
constexpr std::uint8_t kSwapNoIndex = 255;
constexpr std::uint8_t kCycleCheatBit = 0x80;
constexpr std::uint16_t kNoSlot = 0xffff;

bool groupCharacter(std::int32_t id) {
    for (const std::uint8_t group : kGroupCharacters)
        if (id == group) return true;
    return false;
}

std::string memberName(unsigned instance, unsigned member) {
    char name[32];
    std::snprintf(name, sizeof name, "Player%u.Player%02u", instance, member);
    return name;
}

std::string existingMember(unsigned instance, unsigned member) {
    return instance == 1 && member >= 1 && member <= kPlayerMembers ? memberName(instance, member) : std::string();
}
}

PartyList partyList(const std::array<std::uint8_t, kPartyLevelIds>& rowIds, const std::array<std::uint8_t, kPartyLevelExtras>& rowExtras,
                    const std::array<std::uint8_t, kPartyLevelIds>& slotIds, bool ownParty, bool cheatAllCharacters) {
    PartyList list;
    const std::array<std::uint8_t, kPartyLevelIds>& source = ownParty ? slotIds : rowIds;
    for (unsigned i = 0; i < kPartyLevelIds; ++i) list.ids[i] = cheatAllCharacters ? kCheatPartyIds[i] : source[i];
    unsigned listed = 0;
    while (listed < kPartySlots && list.ids[listed] != 0) ++listed;
    unsigned extras = 0;
    while (extras < kPartyLevelExtras && rowExtras[extras] != 0 && listed + extras < kPartySlots) {
        list.ids[listed + extras] = rowExtras[extras];
        ++extras;
    }
    list.walkCount = std::uint16_t(listed + extras);
    if (ownParty)
        for (unsigned i = 0; i < kPartyLevelIds; ++i) list.ids[i] = slotIds[i];
    return list;
}

void PartyManager::setLevel(std::int8_t level) {
    level_ = level;
    entryLevel_ = level >= kCustomisationLevel ? std::uint8_t(level) : 0;
}

void PartyManager::reset() {
    usable_ = 0;
    for (PartySlot& slot : slots_) {
        slot.id = 0;
        slot.standFor = 0;
        slot.removed = false;
        slot.skipped = false;
    }
    effectGate_ = 0;
}

void PartyManager::setUp(const PartyList& list) {
    playerEntity_ = kFirstPlayerMember;
    reset();
    walk_ = list.walkCount;
    characterByte_ = list.ids[0];
    secondCharacterByte_ = list.ids[1];
    unsigned member = 1;
    for (unsigned i = 0; i < kPartySlots; ++i) {
        const std::uint8_t id = list.ids[i];
        if (id == 0) continue;
        const std::string entity = existingMember(1, member++);
        add(entity, id);
        if (!entity.empty()) memberCharacters_[entity] = id;
        if (entity == playerEntity_) playerCharacter_ = id;
        if (i < 1 || flags_.cheatAllCharacters) continue;
        if (!(flags_.groupBytes[i >> 3] >> (i & 7) & 1)) remove(id);
    }
}

void PartyManager::add(const std::string& entity, std::uint8_t id) {
    if (level_ == kCustomisationLevel) return;
    if (id == 0) id = memberCharacter(entity);
    for (PartySlot& slot : slots_) {
        if (slot.id != id) continue;
        slot.entity = entity;
        if (slot.removed) ++usable_;
        slot.removed = false;
        slot.skipped = false;
        return;
    }
    for (unsigned i = 0; i < kPartyAddSlots; ++i) {
        PartySlot& slot = slots_[i];
        if (slot.id != 0) continue;
        slot.id = id;
        slot.entity = entity;
        slot.removed = false;
        ++usable_;
        return;
    }
}

bool PartyManager::remove(std::uint8_t id) {
    unsigned index = 0;
    while (index < kPartySlots && slots_[index].id != id) ++index;
    if (index == kPartySlots) return false;
    if (std::uint16_t(slotOf(playerCharacter_)) == index) cycle(false, false);
    PartySlot& slot = slots_[index];
    if (!slot.removed) --usable_;
    slot.removed = true;
    slot.entity.clear();
    return true;
}

int PartyManager::slotOf(std::uint8_t id) const {
    if (id == 0) return -1;
    for (unsigned i = 0; i < kPartySlots; ++i)
        if (slots_[i].id == id) return int(i);
    return -1;
}

std::optional<PartySwap> PartyManager::cycle(bool backward, bool flag) {
    const std::uint16_t count = walk_;
    const std::uint16_t current = std::uint16_t(slotOf(playerCharacter_));
    std::uint16_t start = current;
    if (current >= count && count > 0) {
        const std::uint8_t currentId = current < kPartySlots ? slots_[current].id : 0;
        unsigned j = 0;
        while (j < count && !(j < kPartySlots && slots_[j].standFor != 0 && slots_[j].standFor == currentId)) ++j;
        if (j < count && j < kPartySlots && slots_[j].standFor != 0) start = std::uint16_t(j);
    }
    std::uint16_t index = start;
    for (unsigned step = 0; step < count; ++step) {
        if (backward) {
            if (index == 0) index = count;
            index = std::uint16_t(index - 1);
        } else {
            index = std::uint16_t(index + 1);
            if (count <= index) index = 0;
        }
        const PartySlot empty;
        const PartySlot& slot = index < kPartySlots ? slots_[index] : empty;
        const bool usable = slot.id != 0 && (!slot.removed || flags_.cheatAllCharacters) && !slot.skipped;
        if (!usable && !(index == kCycleCheatSlot && (flags_.groupBytes[0] & kCycleCheatBit))) continue;
        if (!flags_.partyNativesOff && !flag && (index == start || index == kNoSlot)) continue;
        if (slot.standFor != 0) {
            const int target = slotOf(slot.standFor);
            if (target >= 0 && std::uint16_t(target) == start) continue;
            if (target >= 0) index = std::uint16_t(target);
        }
        return swap(0, std::uint8_t(index & 0xff), SwapOptions{});
    }
    return std::nullopt;
}

std::optional<PartySwap> PartyManager::swap(unsigned player, std::uint8_t index, const SwapOptions& options) {
    if (index == kSwapNoIndex) return std::nullopt;
    const int current = slotOf(playerCharacter_);
    if (current == int(index)) return std::nullopt;
    PartySwap result;
    result.player = player;
    result.index = index;
    const bool allowed = entryLevel_ != std::uint8_t(kCustomisationLevel);
    if (allowed && !options.quiet && clock_ > effectGate_) {
        result.effects = true;
        effectGate_ = clock_ + kSwapEffectGateUpdates;
    }
    const unsigned instance = playerEntity_.rfind("Player1.", 0) == 0 ? 1 : 2;
    result.member = memberName(instance, unsigned(index) + 1);
    if (allowed && index < kPartySlots && !existingMember(instance, unsigned(index) + 1).empty()) {
        playerEntity_ = result.member;
        playerCharacter_ = memberCharacter(result.member);
        result.swapped = true;
        result.scripts = !options.quiet;
        result.sceneScript = result.scripts && !options.noScripts;
    }
    characterByte_ = playerCharacter_;
    result.character = playerCharacter_;
    if (options.updateStandFor) updateStandFor(index, current);
    swaps_.push_back(result);
    return result;
}

void PartyManager::updateStandFor(std::uint8_t index, int current) {
    if (usable_ > index) {
        slots_[index].standFor = 0;
    } else if (int(usable_) <= current) {
        const std::uint8_t currentId = slots_[current].id;
        for (unsigned j = 0; j < walk_ && j < kPartySlots; ++j)
            if (slots_[j].standFor == currentId) slots_[j].standFor = slots_[index].id;
    } else if (current >= 0) {
        slots_[current].standFor = slots_[index].id;
    } else {
        slots_[kPartySlots - 1].id = slots_[index].id;
    }
}

bool PartyManager::setPartyCharacter(unsigned player, std::uint16_t character, bool flag, std::optional<PartySwap>& swapped) {
    int slot = -1;
    for (unsigned i = 0; i < kPartySlots && character != 0; ++i)
        if (slots_[i].id == character) {
            slot = int(i);
            break;
        }
    if (walk_ <= std::uint16_t(slot)) return false;
    SwapOptions options;
    options.quiet = flag;
    swapped = swap(player, std::uint8_t(slot), options);
    return true;
}

void PartyManager::unlockCharacter(std::int32_t id) {
    if (!groupCharacter(id)) return;
    std::string entity;
    const int slot = slotOf(std::uint8_t(id));
    if (slot >= 0) entity = existingMember(1, unsigned(slot) + 1);
    add(entity, std::uint8_t(id));
}

void PartyManager::lockCharacter(std::int32_t id) {
    if (groupCharacter(id)) remove(std::uint8_t(id));
}

std::vector<PartySwap> PartyManager::takeSwaps() { return std::exchange(swaps_, {}); }

std::uint8_t PartyManager::memberCharacter(const std::string& member) const {
    const auto found = memberCharacters_.find(member);
    return found == memberCharacters_.end() ? 0 : found->second;
}

}
