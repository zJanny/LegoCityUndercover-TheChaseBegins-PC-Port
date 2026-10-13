// Door state machine on synthetic parameters (rules from the door code 0x1d16b8 / 0x1a6b6c / 0x1749b8, docs/re/doors.md).
#include "runtime/door.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

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

using Event = std::pair<DoorSoundEvent::Action, std::uint16_t>;
constexpr auto kPlay = DoorSoundEvent::Action::Play;
constexpr auto kStop = DoorSoundEvent::Action::Stop;

std::vector<Event> updateEvents(DoorState& door, const DoorParams& params, bool playerInZone) {
    std::vector<DoorSoundEvent> sounds;
    doorUpdate(door, params, playerInZone, sounds);
    std::vector<Event> events;
    for (const DoorSoundEvent& sound : sounds) events.emplace_back(sound.action, sound.id);
    return events;
}

std::vector<Event> quietUpdates(DoorState& door, const DoorParams& params, int count) {
    std::vector<Event> events;
    for (int i = 0; i < count; ++i)
        for (const Event& event : updateEvents(door, params, false)) events.push_back(event);
    return events;
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
        {
            // Sound events of a full open and close cycle: 0x1a6b6c plays SFXOpening (record +0x2c) in state 2 when the timer equals the open delay
            // (+0x10), at target 3 stops +0x2c and plays SFXOpen (+0x32), at target 4 plays SFXClosing (+0x2e), at target 1 stops +0x2e and plays
            // SFXShut (+0x30); the message handler 0x1749b8 and the update 0x1d16b8 make no sound call (docs/re/doors.md).
            DoorParams params = hingedDoor();
            params.autoOpen = true;
            params.sounds = {605, 601, 602, 606};
            DoorState door;
            require(updateEvents(door, params, false).empty(), "a closed door without the player is silent");
            require(updateEvents(door, params, true) == std::vector<Event>{{kPlay, 605}}, "opening sound at the start (delay 0)");
            require(quietUpdates(door, params, 22).empty() && door.state == 2, "the moving door is silent");
            require(updateEvents(door, params, false) == (std::vector<Event>{{kStop, 605}, {kPlay, 606}}) && door.state == 3, "opened: stop opening, play open");
            require(quietUpdates(door, params, 5).empty(), "an open door is silent");
            doorMessage(door, params, DoorMessage::Close);
            require(updateEvents(door, params, false) == std::vector<Event>{{kPlay, 601}} && door.state == 4, "closing sound at the start of closing");
            require(quietUpdates(door, params, 22).empty() && door.state == 4, "the closing door is silent");
            require(updateEvents(door, params, false) == (std::vector<Event>{{kStop, 601}, {kPlay, 602}}) && door.state == 1, "closed: stop closing, play shut");
            require(quietUpdates(door, params, 5).empty(), "a closed door is silent");
        }
        {
            DoorParams params = hingedDoor();
            params.sounds = {605, 601, 602, 606};
            params.soundOpenDelay = 5;
            DoorState door;
            doorMessage(door, params, DoorMessage::Open);
            int played = -1;
            for (int update = 1; update <= 10; ++update)
                for (const Event& event : updateEvents(door, params, false))
                    if (event == Event{kPlay, 605}) played = update;
            require(played == 6 && door.timer == 9, "the opening sound waits for timer == delay");
        }
        {
            DoorParams params = hingedDoor();
            params.sounds = {605, 601, 602, 606};
            DoorState door;
            doorMessage(door, params, DoorMessage::Open);
            updateEvents(door, params, false);
            quietUpdates(door, params, 10);
            doorMessage(door, params, DoorMessage::Close);
            require(updateEvents(door, params, false) == std::vector<Event>{{kPlay, 601}} && door.state == 4, "closing while opening plays the closing sound without stopping the opening sound");
        }
    } catch (const std::exception& error) {
        std::cerr << "door-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "door-test: ok\n";
    return 0;
}
