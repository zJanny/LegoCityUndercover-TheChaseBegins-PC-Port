#pragma once
// The player's cop whistle state 0x1b2 as the player update runs it: the request of the player input 0x200354 (extras bit 19, idle state, D-pad
// Down press edge, nothing carried in ctrl +0x13c, character row byte +0x20 = 3), the enter 0x276338 (clip row 709 and the traffic stop 0x26e254)
// and the update 0x29a2e0 (back to the idle state when the clip has played). No SDL dependency.
#include "runtime/game_random.hpp"
#include "runtime/traffic_car_push.hpp"

#include <cstdint>

namespace chase::runtime {

// STAND-IN TRAFFIC-S43: the byte +0x20 of the player's character row (*0x372e10 + ctrl +0x2cc x 0x28) must be 3 and its meaning is unread; the
// port runs one player character and takes the byte as 3.
constexpr std::uint8_t kPlayerCharacterClassStandIn = kCopWhistleCharacterClass;

struct PlayerCopWhistleInput {
    std::uint32_t extras = 0;       // the extras mask *0x374724
    std::uint16_t state = 0;        // ctrl +0x6c as playerControllerState reports it
    bool downPressed = false;       // press edge of pad slot 1 (channel 13, D-pad Down)
    bool carrying = false;          // ctrl +0x13c, the carried object, is set
    std::uint32_t clock = 0;        // the game clock 0x241098
};

struct PlayerCopWhistleStep {
    bool entered = false;           // 0x276338 ran in this update: the caller plays the clip `kCopWhistleClipRow`
    bool finished = false;          // 0x29a2e0 requested the idle state in this update
};

class PlayerCopWhistle {
public:
    // The length of the clip row 709 in updates (slot kind 6 of 0x22f560 once it has played).
    void setClipUpdates(unsigned updates) { clipUpdates_ = updates; }
    PlayerCopWhistleStep update(const PlayerCopWhistleInput& input, TrafficWhistleStop& stop, GameRandom& random);
    bool active() const { return active_; }

private:
    bool active_ = false;
    unsigned clipUpdates_ = 0;
    unsigned elapsed_ = 0;
};

}
