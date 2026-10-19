// Fast travel pad rules on a made-up pad (src/runtime/fast_travel_pad.cpp). Provenance: the use query 0x1d79a8, the pad update 0x1c8244, the
// request 0x16c188, the states 0x180 (enter 0x32c2ec, update 0x32c420 with the approach 0x2cd91c) and 0x181 (enter 0x32ea10, update 0x32ea20) and
// the query 0x28b930, read with REA (Ghidra) and llvm-objdump of artifacts/game/game.arm.elf (docs/re/fast-travel.md). Clip lengths: land 42
// frames, idle 12, takeoff 48, all 15 frames per second (models/veh_police_chopper/*.bfnanm, assets::animationLength). The update counts follow
// from the rule at dt 1/30; the oracle (RPC request r40-B-1, m13-5-helipad-built-menu) saw state 0x180 for 39 to 45 vblanks (about 20 to 22
// updates) and the idle start 124 to 126 vblanks after 0x181.
#include "runtime/fast_travel_pad.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase::runtime;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr float kDt = 1.0f / 30.0f;

FastTravelPads pads() {
    FastTravelPad pad;
    pad.instance = "holder.sbfasttravel";
    pad.holder = "holder";
    pad.helicopter = "holder.helicopter";
    pad.helicopterMesh = "Veh_Police_Chopper";
    pad.world = {0, 0, -1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 10, 2, 20, 1};
    pad.zoneCentre[1] = 1.0f;
    pad.zoneCentre[2] = 0.0253f;
    pad.zoneHalf[0] = pad.zoneHalf[2] = 1.0f;
    pad.zoneHalf[1] = 1.5f;
    pad.hasZone = true;
    FastTravelPads set;
    set.add(pad);
    set.setHelicopterClips(0, {AnimationClipInfo{0, 42, 15, false}, AnimationClipInfo{1, 12, 15, false}, AnimationClipInfo{2, 48, 15, false}});
    return set;
}

FastTravelInput at(float x, float y, float z, std::int16_t facing, bool press) {
    FastTravelInput input;
    input.dt = kDt;
    input.player[0] = x;
    input.player[1] = y;
    input.player[2] = z;
    input.facing = facing;
    input.playerFree = true;
    input.actionPressed = press;
    return input;
}

bool has(const std::vector<FastTravelEvent>& events, FastTravelEventKind kind) {
    for (const FastTravelEvent& event : events)
        if (event.kind == kind) return true;
    return false;
}

}

int main() {
    try {
        chase::assets::PlacedObjectState state;
        {
            FastTravelPads set = pads();
            require(FastTravelPads::heading(set.pads()[0].world) == 16384, "forward (1, 0, 0) is a quarter turn: " +
                                                                                   std::to_string(FastTravelPads::heading(set.pads()[0].world)));
            FastTravelPlayer player;
            float position[3] = {10.5f, 2.0f, 20.5f};
            std::int16_t facing = 0;
            set.update(state, at(position[0], position[1], position[2], facing, false), player);
            require(set.playerState() == 0 && set.mapReady(), "no press, no state; 0x28b930 answers 1 outside the fast travel states");
            set.update(state, at(position[0], position[1], position[2], facing, true), player);
            require(set.playerState() == 0, "the use query only requests state 0x180 (0x16c188 -> 0x2414b4)");
            std::vector<FastTravelEvent> events = set.update(state, at(position[0], position[1], position[2], facing, false), player);
            require(set.playerState() == kPlayerStateFastTravelEnter && has(events, FastTravelEventKind::Entered), "the next update enters 0x180");
            require(set.helicopter(0).shown && set.currentClip(0) == FastTravelClip::Land, "the enter shows the Helicopter and starts land");
            require(set.takesPlayerControl() && set.playerClip() == kAnimationTypingStood && !set.mapReady(), "typing_stood, no map in 0x180");
            require(player.resetRunTimer && player.lastIntent && *player.lastIntent == 16384, "ctrl+0x24c = 0 and ctrl+0xa = the pad's heading");
            require(player.moved && player.position[0] == 10.5f && player.position[2] == 20.5f, "the first approach step has factor 0");
            require(player.turned && player.facing == 0x800, "the facing turns by at most 0x800");
            facing = player.facing;
            unsigned enterUpdates = 1;
            while (set.playerState() == kPlayerStateFastTravelEnter) {
                events = set.update(state, at(position[0], position[1], position[2], facing, false), player);
                if (player.moved) std::copy_n(player.position, 3, position);
                if (player.turned) facing = player.facing;
                ++enterUpdates;
                require(enterUpdates < 100, "state 0x180 ends");
            }
            require(enterUpdates == 22, "21 approach updates and the update that clears 0x374328, then 0x181: " + std::to_string(enterUpdates));
            require(has(events, FastTravelEventKind::Active) && set.playerState() == kPlayerStateFastTravelActive, "state 0x181");
            require(std::fabs(position[0] - 10.0f) < 0.01f && std::fabs(position[2] - 20.0f) < 0.01f, "the player stands on the pad");
            require(facing == 16384, "the facing reached the pad's heading");
            require(!set.mapReady(), "0x181 answers its flag, cleared by the enter 0x32ea10");
            unsigned activeUpdates = 0;
            while (!set.mapReady()) {
                events = set.update(state, at(position[0], position[1], position[2], facing, false), player);
                ++activeUpdates;
                require(activeUpdates < 200, "the landing ends");
            }
            require(has(events, FastTravelEventKind::HelicopterIdle) && set.currentClip(0) == FastTravelClip::Idle, "idle loops after land");
            require(activeUpdates == 63, "land (2.8 s, 84 updates) started with the first of the 22 updates of 0x180: " + std::to_string(activeUpdates));
            set.update(state, at(position[0], position[1], position[2], facing, true), player);
            require(set.playerState() == kPlayerStateFastTravelActive, "no new request while a fast travel state runs (gate LEVOBJ-S96)");
            for (int i = 0; i < 120; ++i) set.update(state, at(position[0], position[1], position[2], facing, false), player);
            require(set.playerState() == kPlayerStateFastTravelActive && set.currentClip(0) == FastTravelClip::Idle && set.mapReady(),
                    "0x181 holds with idle looping until the map's exit (not ported, LEVOBJ-120)");
        }
        {
            FastTravelPads set = pads();
            FastTravelPlayer player;
            state.disabled.insert("holder.sbfasttravel");
            set.update(state, at(10, 2.5f, 20, 0, true), player);
            set.update(state, at(10, 2.5f, 20, 0, false), player);
            require(set.playerState() == 0, "a disabled pad (the super build-it's UseObject before the build) takes no press");
            state.disabled.clear();
            set.update(state, at(10, 4.6f, 20, 0, true), player);
            set.update(state, at(10, 4.6f, 20, 0, false), player);
            require(set.playerState() == 0, "outside UseBound (centre y 1, half 1.5)");
            set.update(state, at(10.9f, 2.0f, 20.3f, 0, true), player);
            set.update(state, at(10.9f, 2.0f, 20.3f, 0, false), player);
            require(set.playerState() == kPlayerStateFastTravelEnter, "inside the turned zone");
        }
        {
            FastTravelPad pad;
            pad.instance = "pad";
            pad.world = {0, 0, -1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1};
            pad.zoneHalf[0] = pad.zoneHalf[2] = 1.0f;
            pad.zoneHalf[1] = 1.5f;
            pad.hasZone = true;
            pad.zoneCentre[2] = 0.5f;
            require(FastTravelPads::inZone(pad, std::array<float, 3>{0.9f, 0, 0}.data()), "local z = x: the centre 0.5 lies at x 0.5");
            require(!FastTravelPads::inZone(pad, std::array<float, 3>{-0.6f, 0, 0}.data()), "x -0.6 is local z -0.6, outside");
        }
        std::cout << "fast-travel-pad: ok\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "fast-travel-pad: " << error.what() << '\n';
        return 1;
    }
}
