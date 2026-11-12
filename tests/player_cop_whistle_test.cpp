// Tests of src/runtime/player_cop_whistle.*: the player's request of the state 0x1b2 (player input 0x200354, the reads of 0x2008a4..0x200924:
// 0x2369a8(19), ctrl +0x6c = 1, the press edge of pad slot 1, ctrl +0x13c = 0, character row byte +0x20 = 3), its enter 0x276338 / 0x26e254
// and its update 0x29a2e0. The control flow was read by 20261112-traffic-E on 2026-11-12 from llvm-objdump --triple=armv6k --mattr=+vfp2 of
// artifacts/game/game.arm.elf; the stop's end is clock + random % 60 + 120 (docs/re/traffic-car-push.md). No emulator was used.
#include "runtime/player_cop_whistle.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

namespace {

using namespace chase::runtime;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

PlayerCopWhistleInput request() {
    PlayerCopWhistleInput input;
    input.extras = 1u << kCopWhistleExtra;
    input.state = kCopWhistleRequestState;
    input.downPressed = true;
    input.clock = 500;
    return input;
}

void testNoRequestWithoutExtra() {
    PlayerCopWhistle whistle;
    whistle.setClipUpdates(10);
    TrafficWhistleStop stop;
    GameRandom random(1);
    PlayerCopWhistleInput input = request();
    input.extras = 0;
    require(!whistle.update(input, stop, random).entered && !whistle.active() && stop.until == 0, "the extras mask is 0 after a load: no whistle");
    input.extras = ~(1u << kCopWhistleExtra);
    require(!whistle.update(input, stop, random).entered, "only bit 19 counts");
}

void testGateReads() {
    TrafficWhistleStop stop;
    GameRandom random(1);
    PlayerCopWhistle whistle;
    whistle.setClipUpdates(10);
    PlayerCopWhistleInput input = request();
    input.state = 2;
    require(!whistle.update(input, stop, random).entered, "only in the idle state");
    input = request();
    input.downPressed = false;
    require(!whistle.update(input, stop, random).entered, "needs the D-pad Down press edge");
    input = request();
    input.carrying = true;
    require(!whistle.update(input, stop, random).entered, "nothing carried (ctrl +0x13c = 0)");
    require(stop.until == 0, "no stop without the state");
}

void testEnterAndFinish() {
    TrafficWhistleStop stop;
    GameRandom random(7);
    PlayerCopWhistle whistle;
    whistle.setClipUpdates(3);
    const PlayerCopWhistleStep entered = whistle.update(request(), stop, random);
    require(entered.entered && !entered.finished && whistle.active(), "the request enters the state");
    require(stop.until >= 500 + kTrafficWhistleStopBase && stop.until < 500 + kTrafficWhistleStopBase + kTrafficWhistleStopRandom,
            "the stop ends clock + 120 + random % 60 after the enter");
    unsigned updates = 0;
    while (whistle.active()) {
        require(!whistle.update(request(), stop, random).entered, "no second enter while the state runs");
        if (++updates > 10) break;
    }
    require(updates == 3 && !whistle.active(), "the state ends once the clip has played (3 updates)");
    PlayerCopWhistleInput idle;
    require(!whistle.update(idle, stop, random).entered, "idle again");
}

}

int main() {
    try {
        testNoRequestWithoutExtra();
        testGateReads();
        testEnterAndFinish();
    } catch (const std::exception& error) {
        std::cerr << "player-cop-whistle: " << error.what() << '\n';
        return 1;
    }
    std::cout << "player-cop-whistle: ok\n";
}
