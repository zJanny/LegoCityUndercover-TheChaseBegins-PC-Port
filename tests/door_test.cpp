// Door state machine on synthetic parameters (rules from the door code 0x1d16b8 / 0x1a6b6c / 0x1749b8, docs/re/doors.md).
#include "runtime/door.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace chase::runtime;

namespace {
void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

DoorParams hingedDoor() {
    DoorParams params;
    params.openFrames = 22;
    params.restAngle = -1.5707964f;
    params.zoneHalf[0] = 0.75f;
    params.zoneHalf[1] = 1.8f;
    params.zoneHalf[2] = 0.05f;
    params.zoneCentre[1] = 1.8f;
    params.sounds = {605, 601, 0, 0};
    return params;
}
}

int main() {
    try {
        require(doorFrames(0.75f) == 22 && doorFrames(0.5f) == 15 && doorFrames(10.0f) == 44, "seconds to frames");
        require(doorEase(0.5f, 0) == 0.5f && std::fabs(doorEase(0.5f, 1) - 0.5f) < 1e-6f && doorEase(1.0f, 5) == 1.0f, "easing");

        const float world[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 10, 0, 10, 1};
        DoorParams zoneDoor = hingedDoor();
        const float inside[3] = {10.5f, 1.9f, 13.9f}, outside[3] = {10.5f, 1.9f, 14.2f}, wide[3] = {11.0f, 1.9f, 10.0f};
        require(doorZoneContains(world, inside, zoneDoor) && !doorZoneContains(world, outside, zoneDoor) && !doorZoneContains(world, wide, zoneDoor),
                "zone box");

        std::vector<DoorSoundEvent> sounds;
        {
            DoorParams params = hingedDoor();
            DoorState door;
            for (int i = 0; i < 5; ++i) doorUpdate(door, params, true, sounds);
            require(door.state == 1 && door.target == 1, "a door without AutoOpen ignores the player");
            params.autoOpen = true;
            doorUpdate(door, params, true, sounds);
            require(door.state == 2 && door.target == 2 && door.timer == 0 && door.openFlag, "auto open starts on the player");
            for (int i = 0; i < 22; ++i) doorUpdate(door, params, false, sounds);
            require(door.state == 2 && door.timer == 22, "22 frames of opening");
            doorUpdate(door, params, false, sounds);
            require(door.state == 3 && door.target == 3 && door.timer == 0, "open after one more update");
            doorUpdate(door, params, false, sounds);
            require(door.state == 3, "no auto close without autoclosetime");
            params.autoCloseFrames = 15;
            for (int i = 0; i < 14; ++i) doorUpdate(door, params, true, sounds);
            require(door.state == 3 && door.timer == 1, "the player keeps the door open");
            for (int i = 0; i < 13; ++i) doorUpdate(door, params, false, sounds);
            require(door.state == 3 && door.timer == 14, "still open before the auto close time");
            doorUpdate(door, params, false, sounds);
            require(door.state == 4 && door.timer == 0, "auto close after 15 frames without the player");
            for (int i = 0; i < 11; ++i) doorUpdate(door, params, false, sounds);
            doorUpdate(door, params, true, sounds);
            require(door.state == 2 && door.timer == 10, "a closing door opens again from the same place");
        }
        {
            DoorParams params = hingedDoor();
            DoorState door;
            doorMessage(door, params, DoorMessage::Open);
            require(door.target == 2, "open request");
            doorUpdate(door, params, false, sounds);
            doorMessage(door, params, DoorMessage::Close);
            require(door.target == 4, "close request");
            doorUpdate(door, params, false, sounds);
            require(door.state == 4 && door.timer == 22, "closing from the start of the opening");
            doorMessage(door, params, DoorMessage::OpenAtOnce);
            require(door.state == 3 && door.timer == 22, "open at once");
            doorMessage(door, params, DoorMessage::Reset);
            require(door.state == 1 && door.target == 1, "reset");
        }
    } catch (const std::exception& error) {
        std::cerr << "door-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "door-test: ok\n";
    return 0;
}
