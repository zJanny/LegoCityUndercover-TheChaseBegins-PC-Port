#pragma once
// The arrest of a character that has just left a wrecked car (mission 1-4: `1-4_Car_Crash` makes `1-4_Booster_Strange_Chaser` jump out, the player arrests
// him, CharacterArrested starts `1-4_RobberCaught`, which spawns `1-4_MissionEnd`; docs/re/combat-characters-arrest.md, entry on the arrest after a car crash). The arrest itself is the
// A tap's ARRESTING / ARREST / ARRESTED of character_arrest.*. No SDL dependency.
#include "runtime/character_arrest.hpp"
#include "runtime/character_vehicle_entry.hpp"

#include <optional>
#include <set>
#include <string>

namespace chase::runtime {

// STAND-IN COMBAT-S9: the original's rule that makes the chaser arrestable after the crash is not recovered (0x2107e0 passes only for the stun states
// with arrest health 0, and the chaser has ArrestHealth 2). A character that left a vehicle is arrestable by an A tap until it is arrested, as if it lay
// in a stun state with arrest health 0 and a non-zero arrest damage; the CharacterAvoidArrest trigger of 0x2107e0 still refuses it.
class CrashArrestTargets {
public:
    void characterLeft(const CharacterLeftVehicle& event);
    bool contains(const std::string& character) const;
    void remove(const std::string& character);
    std::optional<ArrestCandidate> candidate(const std::string& character, bool avoidArrestTrigger) const;

private:
    std::set<std::string> characters_;
};

}
