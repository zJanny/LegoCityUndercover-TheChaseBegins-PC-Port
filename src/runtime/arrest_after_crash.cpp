#include "runtime/arrest_after_crash.hpp"

namespace chase::runtime {

namespace {
constexpr std::uint8_t kStandInArrestDamage = 1;
}

void CrashArrestTargets::characterLeft(const CharacterLeftVehicle& event) { characters_.insert(event.character); }

bool CrashArrestTargets::contains(const std::string& character) const { return characters_.count(character) != 0; }

void CrashArrestTargets::remove(const std::string& character) { characters_.erase(character); }

std::optional<ArrestCandidate> CrashArrestTargets::candidate(const std::string& character, bool avoidArrestTrigger) const {
    if (!contains(character)) return std::nullopt;
    ArrestCandidate candidate;
    candidate.state = kTakedownPutdownStun;
    candidate.arrestHealth = 0;
    candidate.avoidArrestTrigger = avoidArrestTrigger;
    candidate.arrestDamage = kStandInArrestDamage;
    return candidate;
}

}
