#include "runtime/natives_progress.hpp"

#include <cmath>
#include <limits>

namespace chase::runtime {
namespace {

struct Entry {
    const char* name;
    ProgressNativeKind kind;
    ProgressNativeShape shape;
    bool standIn;
};

constexpr Entry kEntries[] = {
    {"enablestudsbygroup", ProgressNativeKind::EnableStudsByGroup, {2, false, false}, true},
    {"givereward", ProgressNativeKind::GiveReward, {1, false, false}, true},
    {"unlockability", ProgressNativeKind::UnlockAbility, {1, false, false}, true},
    {"unlockcharacter", ProgressNativeKind::UnlockCharacter, {1, false, false}, true},
    {"lockcharacter", ProgressNativeKind::LockCharacter, {1, false, false}, true},
    {"setraceinprogress", ProgressNativeKind::SetRaceInProgress, {1, false, false}, true},
    {"addstuds", ProgressNativeKind::AddStuds, {1, false, false}, true},
    {"quicksave", ProgressNativeKind::QuickSave, {0, false, false}, true},
    {"awardracingcar", ProgressNativeKind::AwardRacingCar, {0, false, false}, true},
    {"countraceswon", ProgressNativeKind::CountRacesWon, {0, false, true}, false},
    {"isracingcarawarded", ProgressNativeKind::IsRacingCarAwarded, {0, false, true}, false},
    {"setracewon", ProgressNativeKind::SetRaceWon, {1, false, false}, true},
    {"unlocklevel", ProgressNativeKind::UnlockLevel, {1, false, false}, false},
    {"disablesuperbuiltitsave", ProgressNativeKind::DisableSuperBuiltItSave, {1, true, false}, true},
    {"setparachutedeployheight", ProgressNativeKind::SetParachuteDeployHeight, {1, false, false}, true},
    {"lockability", ProgressNativeKind::LockAbility, {1, false, false}, true},
    {"enablesuperbricks", ProgressNativeKind::EnableSuperBricks, {1, false, false}, true},
    {"getcharacterunlocked", ProgressNativeKind::GetCharacterUnlocked, {1, false, true}, false},
    {"getlanguage", ProgressNativeKind::GetLanguage, {0, false, true}, true},
    {"getplayernumber", ProgressNativeKind::GetPlayerNumber, {0, false, true}, true},
    {"isredbrickbought", ProgressNativeKind::IsRedbrickBought, {1, false, true}, false},
    {"unlockscanner", ProgressNativeKind::UnlockScanner, {0, false, false}, true},
};

constexpr const char* kLanguageNames[] = {"English", "French", "Italian", "German", "Spanish", "Dutch",
                                          "Danish", "Portuguese", "Russian", "Polish", "Japanese", "Korean"};

// vcvt.s32.f32: truncation, saturation, NaN gives 0.
std::int32_t toInt(float value) {
    if (std::isnan(value)) return 0;
    if (value >= 2147483648.0f) return std::numeric_limits<std::int32_t>::max();
    if (value <= -2147483648.0f) return std::numeric_limits<std::int32_t>::min();
    return static_cast<std::int32_t>(value);
}

// vcvt.u32.f32: negative values and NaN give 0.
std::uint32_t toUnsigned(float value) {
    if (std::isnan(value) || value <= 0.0f) return 0;
    if (value >= 4294967296.0f) return std::numeric_limits<std::uint32_t>::max();
    return static_cast<std::uint32_t>(value);
}

// Characters 1..7 stand for the first character of their group (bytes at 0x3741b4, read by 0x213444).
constexpr std::int32_t kGroupFirstCharacter[7] = {9, 19, 29, 39, 49, 59, 69};
constexpr std::uint32_t kCharacterIdLimit = 141;
constexpr std::size_t kIdSetOffset = 0x51e3;  // 0x205db0
constexpr std::size_t kIdSetNewOffset = kIdSetOffset + 64;

// UnlockCharacter 0x263918 sets bit n of the byte at 0x52086b for these ids.
int groupBit(std::int32_t id) {
    switch (id) {
    case 9: return 0;
    case 19: return 1;
    case 29: return 2;
    case 39: return 3;
    case 49: return 4;
    case 59: return 5;
    case 69: return 6;
    default: return -1;
    }
}

}

std::optional<ProgressNativeKind> progressNativeKind(const std::string& lowerCaseName) {
    for (const Entry& entry : kEntries)
        if (lowerCaseName == entry.name) return entry.kind;
    return std::nullopt;
}

std::vector<std::string> progressNativeNames() {
    std::vector<std::string> names;
    for (const Entry& entry : kEntries) names.push_back(entry.name);
    return names;
}

ProgressNativeShape progressNativeShape(ProgressNativeKind kind) {
    for (const Entry& entry : kEntries)
        if (entry.kind == kind) return entry.shape;
    return {};
}

bool progressNativeIsStandIn(ProgressNativeKind kind) {
    for (const Entry& entry : kEntries)
        if (entry.kind == kind) return entry.standIn;
    return false;
}

const char* languageParameterName(unsigned language) {
    return language >= 1 && language <= 12 ? kLanguageNames[language - 1] : nullptr;
}

ProgressNativeState::ProgressNativeState() : slot_(kSlotImageSize, 0) {}

void ProgressNativeState::setSlotImage(const std::vector<std::uint8_t>& image) {
    slot_ = image;
    slot_.resize(kSlotImageSize, 0);
}

bool ProgressNativeState::bit(std::size_t offset, std::uint32_t index) const {
    const std::size_t byte = offset + (index >> 3);
    return byte < slot_.size() && (slot_[byte] >> (index & 7) & 1);
}

void ProgressNativeState::setBit(std::size_t offset, std::uint32_t index, bool on) {
    const std::size_t byte = offset + (index >> 3);
    if (byte >= slot_.size()) {
        ++outOfRange_;
        return;
    }
    const std::uint8_t mask = std::uint8_t(1u << (index & 7));
    slot_[byte] = on ? std::uint8_t(slot_[byte] | mask) : std::uint8_t(slot_[byte] & ~mask);
}

bool ProgressNativeState::characterUnlocked(std::int32_t id) const {
    if (id >= 1 && id <= 8) {
        if (id == 8) return true;
        id = kGroupFirstCharacter[id - 1];
    }
    const std::uint32_t unsignedId = static_cast<std::uint32_t>(id);
    return unsignedId < kCharacterIdLimit && bit(kCharacterBitsOffset, 2 * unsignedId);
}

// 0x232028(id, 0): the unlocked bit, and the "new" bit for every id but 9 the first time. The second bit and the
// recursion into the ids of the character table (0x231fc0) are not done (SCRIPT-61).
void ProgressNativeState::unlockCharacter(std::uint32_t id) {
    if (!bit(kCharacterBitsOffset, 2 * id) && id != 9) setBit(kCharacterNewSetOffset, id, true);
    setBit(kCharacterBitsOffset, 2 * id, true);
}

ProgressNativeResult ProgressNativeState::run(ProgressNativeKind kind, const std::vector<float>& values, const std::string& object) {
    ProgressNativeResult result;
    const float first = values.empty() ? 0.0f : values[0];
    const std::int32_t number = toInt(first);
    const std::uint32_t index = static_cast<std::uint32_t>(number);
    switch (kind) {
    case ProgressNativeKind::EnableStudsByGroup:
        studGroupRequests_.push_back({std::uint8_t(number & 0xff), std::uint8_t(toInt(values.size() > 1 ? values[1] : 0.0f) & 0xff)});
        break;
    case ProgressNativeKind::GiveReward:
        rewards_.push_back(index & 0xff);
        ++saveRequests_;
        break;
    case ProgressNativeKind::UnlockAbility: setBit(kAbilityOffset, index, true); break;
    case ProgressNativeKind::LockAbility: setBit(kAbilityOffset, index, false); break;
    case ProgressNativeKind::UnlockCharacter: {
        charactersUnlocked_.push_back(index);
        const int group = groupBit(number);
        if (group >= 0) slot_[kCharacterGroupOffset] = std::uint8_t(slot_[kCharacterGroupOffset] | 1u << group);
        if ((group >= 0 || number == 16) && !characterUnlocked(number)) unlockCharacter(index & 0xff);
        break;
    }
    case ProgressNativeKind::LockCharacter: {
        const int group = groupBit(number);
        if (group < 0) break;
        charactersLocked_.push_back(index);
        slot_[kCharacterGroupOffset] = std::uint8_t(slot_[kCharacterGroupOffset] & ~(1u << group));
        break;
    }
    case ProgressNativeKind::SetRaceInProgress: raceInProgress_ = number != 0; break;
    case ProgressNativeKind::AddStuds: studsAdded_ += toUnsigned(first); break;
    case ProgressNativeKind::QuickSave: ++saveRequests_; break;
    case ProgressNativeKind::AwardRacingCar:
        if (!bit(kIdSetOffset, kRacingCarId)) setBit(kIdSetNewOffset, kRacingCarId, true);
        setBit(kIdSetOffset, 2 * kRacingCarId - 2, true);
        slot_[kRacingCarOffset] = 1;
        ++saveRequests_;
        break;
    case ProgressNativeKind::CountRacesWon: {
        float count = 0.0f;
        for (unsigned race = 0; race < kRaceBits; ++race)
            if (bit(kRaceWonOffset, race)) count += 1.0f;
        result.output = count;
        break;
    }
    case ProgressNativeKind::IsRacingCarAwarded: result.output = slot_[kRacingCarOffset] & 1 ? 1.0f : 0.0f; break;
    case ProgressNativeKind::SetRaceWon:
        if (!bit(kRaceWonOffset, index - 2)) {
            setBit(kRaceWonOffset, index - 2, true);
            ++saveRequests_;
        }
        break;
    case ProgressNativeKind::UnlockLevel: {
        const std::int32_t level = static_cast<std::int8_t>(number & 0xff);
        setBit(kLevelUnlockOffset, 2 * static_cast<std::uint32_t>(level), true);
        if (level >= 0 && level < 9) result.unlockedLevel = static_cast<unsigned>(level);
        break;
    }
    case ProgressNativeKind::DisableSuperBuiltItSave: superBuiltItSaves_.push_back({object, (number & 1) != 0}); break;
    case ProgressNativeKind::SetParachuteDeployHeight: parachuteHeight_ = first; break;
    case ProgressNativeKind::EnableSuperBricks: superBricks_ = number != 0; break;
    case ProgressNativeKind::GetCharacterUnlocked: result.output = characterUnlocked(number) ? 1.0f : 0.0f; break;
    case ProgressNativeKind::GetLanguage:
        if (const char* name = languageParameterName(language_)) result.languageParameter = std::string("Language_") + name;
        else result.output = -1.0f;
        break;
    case ProgressNativeKind::GetPlayerNumber: result.output = 1.0f; break;
    case ProgressNativeKind::IsRedbrickBought: result.output = bit(kRedBrickSetOffset, index) ? 1.0f : 0.0f; break;
    case ProgressNativeKind::UnlockScanner: scannerUnlocked_ = true; break;
    }
    return result;
}

std::vector<StudGroupRequest> ProgressNativeState::takeStudGroupRequests() {
    std::vector<StudGroupRequest> taken;
    taken.swap(studGroupRequests_);
    return taken;
}

std::uint64_t ProgressNativeState::takeStudsAdded() {
    const std::uint64_t taken = studsAdded_;
    studsAdded_ = 0;
    return taken;
}

}
