#pragma once
// The player's state 9 "BUILD IT" (registered by the build-it class at 0x15d2ec; update 0x276254, enter 0x1d4b14, exit 0x1c72f0): while it runs the
// player plays the clip "buildit" and stands still. Rules and evidence: docs/re/buildit.md. No SDL dependency.
#include "runtime/buildit_set.hpp"

namespace chase::runtime {

// 0x1d4b14 asks 0x21941c for animation 183 (tablesanimations sheet 1, "buildit", models/char_minifig/class_generic/buildit_minifig.bfnanm).
constexpr int kAnimationBuildit = 183;

// The state is entered when the use query answers 1 and the action button was pressed (or taken by the query), and left when the query no longer
// answers 1 or the button is not held (0x276254: ctrl+0xc bit 2 is the held A).
class BuilditPlayer {
public:
    // One update, after BuilditSet::update. `free`: the player may enter the state (not carrying, no script or cutscene control, standing on the ground).
    void update(const BuilditUse& use, bool actionPressed, bool actionHeld, bool free);
    bool active() const { return active_; }
    // The clip while the state runs, 0 otherwise.
    int clip() const { return active_ ? kAnimationBuildit : 0; }
    // Updates since the state was entered (0 in the update of the entry).
    unsigned updates() const { return updates_; }

private:
    bool active_ = false;
    unsigned updates_ = 0;
};

}
