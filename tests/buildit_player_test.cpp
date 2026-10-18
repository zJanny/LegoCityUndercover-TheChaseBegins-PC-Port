// The player's state 9 "BUILD IT" (src/runtime/buildit_player.cpp). Rules from the code: 0x176160 case 3 (reply 1 / 2), 0x276254 (the state stays while
// the reply is 1 and the held flag ctrl+0xc bit 2 is set), 0x1d4b14 (clip 183 "buildit"); docs/re/buildit.md. Oracle trace (r18-A, m11-4-van-parts,
// dt 0.033, docs/re/buildit.md §2026-10-18T10:08:04): entered in the press update, left in the update the build-it answers 2.
#include "runtime/buildit_player.hpp"
#include <iostream>
#include <string>

namespace {
using namespace chase::runtime;

int failures = 0;

void require(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

BuilditUse use(int reply, bool accepted, bool inZone = true) {
    BuilditUse result;
    result.inZone = inZone;
    result.reply = reply;
    result.accepted = accepted;
    return result;
}

void entersOnPressAndStaysWhileHeld() {
    BuilditPlayer player;
    player.update(use(1, false), false, false, true);
    require(!player.active() && player.clip() == 0, "idle in the zone without a press does nothing");
    player.update(use(1, true), true, true, true);
    require(player.active() && player.clip() == kAnimationBuildit && kAnimationBuildit == 183, "the press enters the state with clip 183");
    for (int i = 0; i < 80; ++i) player.update(use(1, false), false, true, true);
    require(player.active() && player.updates() == 80, "held A keeps the state");
    player.update(use(2, false), false, true, true);
    require(!player.active(), "reply 2 (state 10 and up) leaves it");
}

void leavesWhenReleased() {
    BuilditPlayer player;
    player.update(use(1, true), true, true, true);
    player.update(use(1, false), false, false, true);
    require(!player.active(), "releasing A leaves the state");
    player.update(use(1, false), true, true, true);
    require(player.active(), "a new press enters it again while the build-it is mid-brick (reply 1, not accepted)");
}

void refusals() {
    BuilditPlayer player;
    player.update(use(1, true), true, true, false);
    require(!player.active(), "a player who is not free does not enter");
    player.update(use(1, true, false), true, true, true);
    require(!player.active(), "outside the zone does not enter");
    player.update(use(0, false), true, true, true);
    require(!player.active(), "reply 0 does not enter");
}
}

int main() {
    entersOnPressAndStaysWhileHeld();
    leavesWhenReleased();
    refusals();
    if (failures) return 1;
    std::cout << "buildit-player-test: ok\n";
    return 0;
}
