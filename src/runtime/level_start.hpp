#pragma once
// State of the placed objects when a level starts a new game, derived from the level's own data (rules and evidence:
// docs/re/scripts.md): the level's "startup" script run through the script words of the executor 0x234204, and the
// objects that a build-it, super build-it or detective reveal object hides until it is used. No SDL dependency.
#include "assets/object_collision.hpp"

namespace chase::runtime {

// `classOf` finds the class file of an object class (the reveal links exist only for the parameters the class lists).
// Stand-ins: waits and yielding natives count as elapsed (the state is read after the intro), the natives that write a
// variable (IsLevelUnlocked, GetGameComplete, IsRedBrickExtraActive) return 0 as for a new game without a save.
assets::PlacedObjectState levelStartState(const assets::Level& level, const assets::ClassLookup& classOf);

}
