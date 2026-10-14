#pragma once
// State of the placed objects when a level starts a new game, derived from the level's own data (rules and evidence:
// docs/re/scripts.md): the objects that a build-it, super build-it or detective reveal object hides until it is used,
// then the level's "startup" script run through ScriptRunner until its threads end. No SDL dependency.
#include "assets/object_collision.hpp"
#include "runtime/script_runner.hpp"

#include <cstdint>

namespace chase::runtime {

// Stand-in SCRIPT-S1: the update step and the update limit used to run the startup threads before the first frame
// (0.034 s is the free-running dt read from the oracle's scheduler at gameplay-start; the loading phase's own dt is not
// recovered).
constexpr float kStartupStepDt = 0.034f;
constexpr std::uint32_t kStartupMaxUpdates = 3000;

// The objects the reveal links hide (`classOf` finds the class file of an object class).
assets::PlacedObjectState levelInitialState(const assets::Level& level, const assets::ClassLookup& classOf);
// Starts the level's "startup" script and runs the runner with `dt` until no thread is left or `maxUpdates` passed.
// Returns the number of updates.
std::uint32_t runLevelStartup(ScriptRunner& runner, float dt, std::uint32_t maxUpdates);
// levelInitialState plus runLevelStartup with the stand-in step above. The natives that write a variable
// (IsLevelUnlocked, GetGameComplete, IsRedBrickExtraActive) return 0 as for a new game without a save.
assets::PlacedObjectState levelStartState(const assets::Level& level, const assets::ClassLookup& classOf);

}
