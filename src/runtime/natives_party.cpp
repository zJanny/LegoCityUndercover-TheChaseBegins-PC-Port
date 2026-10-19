#include "runtime/natives_party.hpp"

#include <cmath>
#include <limits>

#include "runtime/natives_progress.hpp"

namespace chase::runtime {
namespace {

struct Entry {
    const char* name;
    PartyNativeKind kind;
    PartyNativeShape shape;
    bool standIn;
};

constexpr Entry kEntries[] = {
    {"swapallegiance", PartyNativeKind::SwapAllegiance, {1, 0, false, 0}, true},
    {"setweaponenabled", PartyNativeKind::SetWeaponEnabled, {1, 1, false, 0}, true},
    {"charsetallowvehicleentry", PartyNativeKind::CharSetAllowVehicleEntry, {1, 1, false, 0}, true},
    {"superfreeplayswap", PartyNativeKind::SuperFreeplaySwap, {1, 1, false, 0}, true},
    {"relockchaseplainclothes", PartyNativeKind::RelockChasePlainClothes, {0, 0, false, 0}, true},
    {"walnuts_hit", PartyNativeKind::WalnutsHit, {1, 0, false, 0}, true},
    {"1-8_rejoinfight", PartyNativeKind::RejoinFight, {1, 0, false, 0}, true},
    {"getinmech", PartyNativeKind::GetInMech, {2, 0, false, 0}, true},
    {"1-8_runfromarrest", PartyNativeKind::RunFromArrest, {1, 0, false, 0}, true},
    {"charsetskydivemode", PartyNativeKind::CharSetSkydiveMode, {1, 1, false, 0}, true},
    {"firebeamweapon", PartyNativeKind::FireBeamWeapon, {1, 3, false, 0}, true},
    {"getoutmech", PartyNativeKind::GetOutMech, {1, 0, false, 0}, true},
    {"hack_playdowntownintro", PartyNativeKind::HackPlayDowntownIntro, {0, 0, true, 0}, false},
    {"rex_axesmash_done", PartyNativeKind::RexAxeSmashDone, {1, 0, false, 0}, true},
    {"rex_grapplepull_done", PartyNativeKind::RexGrapplePullDone, {1, 0, false, 0}, true},
    {"usepet", PartyNativeKind::UsePet, {0, 0, false, 2}, false},
    {"walnuts_startphase2", PartyNativeKind::WalnutsStartPhase2, {1, 0, false, 0}, true},
};

// vcvt.s32.f32: truncation, saturation, NaN gives 0.
std::int32_t toInt(float value) {
    if (std::isnan(value)) return 0;
    if (value >= 2147483648.0f) return std::numeric_limits<std::int32_t>::max();
    if (value <= -2147483648.0f) return std::numeric_limits<std::int32_t>::min();
    return static_cast<std::int32_t>(value);
}

// The player entity is the global 0x3772c0 (0x3772b8 equals it with one player); 0x22dacc resolves the object `Player1` to it.
bool isPlayer(const std::string& object) { return object == "player1"; }

}

std::optional<PartyNativeKind> partyNativeKind(const std::string& lowerCaseName) {
    for (const Entry& entry : kEntries)
        if (lowerCaseName == entry.name) return entry.kind;
    return std::nullopt;
}

std::vector<std::string> partyNativeNames() {
    std::vector<std::string> names;
    for (const Entry& entry : kEntries) names.push_back(entry.name);
    return names;
}

PartyNativeShape partyNativeShape(PartyNativeKind kind) {
    for (const Entry& entry : kEntries)
        if (entry.kind == kind) return entry.shape;
    return {};
}

bool partyNativeIsStandIn(PartyNativeKind kind) {
    for (const Entry& entry : kEntries)
        if (entry.kind == kind) return entry.standIn;
    return false;
}

WeaponHand weaponHandOf(float value) {
    if (value == 0.0f) return WeaponHand::None;
    return value == 2.0f ? WeaponHand::A : WeaponHand::B;
}

std::uint32_t PartyNativeState::allegianceSwaps(const std::string& object) const {
    const auto found = allegianceSwaps_.find(object);
    return found == allegianceSwaps_.end() ? 0 : found->second;
}

std::optional<bool> PartyNativeState::vehicleEntryAllowed(const std::string& object) const {
    const auto found = vehicleEntry_.find(object);
    if (found == vehicleEntry_.end()) return std::nullopt;
    return found->second;
}

std::optional<std::string> PartyNativeState::pilotOf(const std::string& mech) const {
    const auto found = pilots_.find(mech);
    if (found == pilots_.end() || found->second.empty()) return std::nullopt;
    return found->second;
}

PartyNativeResult PartyNativeState::run(PartyNativeKind kind, const std::vector<std::string>& objects, const std::vector<float>& numbers,
                                        const ProgressNativeState& progress) {
    PartyNativeResult result;
    const std::string object = objects.empty() ? std::string() : objects[0];
    switch (kind) {
    case PartyNativeKind::SwapAllegiance:
        ++allegianceSwaps_[object];
        break;
    case PartyNativeKind::SetWeaponEnabled: {
        const float value = numbers[0];
        const bool player = isPlayer(object);
        weaponRequests_.push_back({object, value, player && value != 0.0f ? WeaponHand::None : weaponHandOf(value)});
        if (value != 0.0f && player) playerWeapon_ = PlayerWeaponBytes{true, true, 2.0f < value};
        break;
    }
    case PartyNativeKind::CharSetAllowVehicleEntry:
        vehicleEntry_[object] = toInt(numbers[0]) != 0;
        break;
    case PartyNativeKind::SuperFreeplaySwap:
        freeplaySwap_ = FreeplaySwapRecord{object, static_cast<std::uint16_t>(static_cast<std::uint32_t>(toInt(numbers[0])) & 0xffff)};
        if (isPlayer(object)) ++freeplaySwapEffects_;
        break;
    case PartyNativeKind::RelockChasePlainClothes:
        ++plainClothesRemovals_;
        if (currentCharacter_ == kCharacterPlainClothes) freeplaySwap_ = FreeplaySwapRecord{"player1", kCharacterChase};
        break;
    case PartyNativeKind::WalnutsHit: bossRequests_.push_back({object, 5}); break;
    case PartyNativeKind::RejoinFight: bossRequests_.push_back({object, 6}); break;
    case PartyNativeKind::RexAxeSmashDone: bossRequests_.push_back({object, 6}); break;
    case PartyNativeKind::RexGrapplePullDone: bossRequests_.push_back({object, 8}); break;
    case PartyNativeKind::WalnutsStartPhase2: bossRequests_.push_back({object, 2}); break;
    case PartyNativeKind::RunFromArrest:
        bossRequests_.push_back({object, 2});
        runFromArrest_.push_back(object);
        break;
    case PartyNativeKind::GetInMech: pilots_[object] = objects.size() > 1 ? objects[1] : std::string(); break;
    case PartyNativeKind::GetOutMech: pilots_[object].clear(); break;
    case PartyNativeKind::CharSetSkydiveMode: {
        const bool on = toInt(numbers[0]) != 0;
        skydiveRequests_.push_back({object, on, on ? kSkydiveState : kNoSkydiveState});
        floorNormalMin_ = on ? kSkydiveFloorNormalMin : kNormalFloorNormalMin;
        break;
    }
    case PartyNativeKind::FireBeamWeapon:
        beamShots_.push_back({object, static_cast<std::uint8_t>(static_cast<std::uint32_t>(toInt(numbers[0])) & 0xff), numbers[1], numbers[2]});
        break;
    case PartyNativeKind::HackPlayDowntownIntro: {
        float output = 0.0f;
        const std::vector<std::uint8_t>& slot = progress.slotImage();
        const std::uint16_t story = slot.size() > kStoryProgressOffset + 1 ? static_cast<std::uint16_t>(slot[kStoryProgressOffset] | slot[kStoryProgressOffset + 1] << 8) : 0;
        if (progress.bit(kLevelUnlockOffset, 2 * kDowntownLevel) && story <= 1 && !downtownIntroPlayed_) {
            downtownIntroPlayed_ = true;
            output = 1.0f;
        }
        result.output = output;
        break;
    }
    case PartyNativeKind::UsePet:
        break;
    }
    return result;
}

}
