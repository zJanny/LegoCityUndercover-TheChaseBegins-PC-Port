#include "runtime/destroy_message.hpp"

#include <algorithm>
#include <iterator>

namespace chase::runtime {
namespace {

constexpr std::uint8_t kHidingKinds[] = {27, 64, 67, 71, 85, 102, 119, 121, 132, 134, 138, 155, 204, 214, 242};
constexpr std::uint8_t kVehicleKind = 183;
constexpr std::uint8_t kPropHandlerKinds[] = {85, 119, 134, 214};
constexpr std::uint8_t kUnreadKinds[] = {31, 45, 100, 125, 159, 208, 222, 243};

template <std::size_t N>
bool contains(const std::uint8_t (&kinds)[N], std::uint8_t kind) {
    return std::find(std::begin(kinds), std::end(kinds), kind) != std::end(kinds);
}

constexpr std::uint8_t kHitMachineKinds[] = {67, 204};
constexpr std::uint8_t kDefaultHandlerKinds[] = {64, 132};
constexpr std::uint8_t kFallApartKinds[] = {27, 121, 242};

}

DestroyReaction destroyReaction(std::uint8_t kind) {
    if (kind == kVehicleKind) return DestroyReaction::BreaksVehicle;
    return contains(kHidingKinds, kind) ? DestroyReaction::HidesEntity : DestroyReaction::NoReaction;
}

bool destroyRunsPropHandler(std::uint8_t kind) { return contains(kPropHandlerKinds, kind); }

bool destroyReactionRead(std::uint8_t kind) { return !contains(kUnreadKinds, kind); }

DestroyEffectSet destroyEffectSet(std::uint8_t kind) {
    if (destroyRunsPropHandler(kind)) return DestroyEffectSet::BaseProp;
    if (contains(kHitMachineKinds, kind)) return DestroyEffectSet::HitMachine;
    if (kind == 71) return DestroyEffectSet::Pushable;
    if (contains(kDefaultHandlerKinds, kind)) return DestroyEffectSet::DefaultHandler;
    if (contains(kFallApartKinds, kind)) return DestroyEffectSet::CharacterFallApart;
    if (kind == 102) return DestroyEffectSet::Topple;
    return DestroyEffectSet::None;
}

bool destroyEffectsPorted(DestroyEffectSet set) {
    return set == DestroyEffectSet::BaseProp || set == DestroyEffectSet::HitMachine || set == DestroyEffectSet::Pushable ||
           set == DestroyEffectSet::DefaultHandler;
}

}
