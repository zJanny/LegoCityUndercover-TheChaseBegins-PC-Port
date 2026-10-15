#include "runtime/director_hold.hpp"

namespace chase::runtime {

CharacterControl characterControl(bool directorHolds, bool isPlayer, bool aiControlled, bool dcamActive) {
    if (!directorHolds) return isPlayer && !aiControlled ? CharacterControl::Pad : CharacterControl::Ai;
    if (isPlayer && !aiControlled) return CharacterControl::None;
    return dcamActive ? CharacterControl::Ai : CharacterControl::None;
}

}
