#pragma once
// Script jump in and out of characters other than the player (docs/re/vehicles-waypoints.md): VehicleJumpIn 0x1cdec0 and VehicleJumpOut
// 0x1da8d8 post event 200 to the character's controller when 0x21aae0 holds (kinds 27 and 149, the kind list 0x3743ec, entity +0xc bit 14). The
// binder 0x25e770 hangs the enter event (0x515738, handler 0x322a04) on the states 1, 2 and 3 and the leave event (0x515744, handler 0x322aa4)
// on the Drive state 0x11c, so the character runs the states Drive Jump In 0x11a, Drive 0x11c and Drive Jump Out 0x11b of the player
// (vehicle_boarding.hpp) without their player-only parts. When the jump out ends the character is on foot again: CharacterLeftVehicle. No SDL dependency.
#include "runtime/vehicle_boarding.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace chase::runtime {

struct CharacterLeftVehicle {
    std::string character;                    // lower-case instance name
    std::string vehicle;                      // lower-case instance name of the vehicle it left
    std::array<float, 3> position = {0, 0, 0};  // where the jump out put it
};

// What the character's controller gives the event handlers in one update.
struct CharacterAboard {
    std::array<float, 3> position = {0, 0, 0};
    std::int16_t facing = 0;
    // STAND-IN SCRIPT-S338: the states 1, 2 and 3 that take event 200 on foot are not tracked for level characters; the caller answers whether the
    // character stands, walks or runs.
    bool enterEventState = true;
    float boxHalfY = BoardingPlayer{}.boxHalfY;
};

struct CharacterVehicleStep {
    std::string character;
    BoardingStep step;
};

class CharacterVehicleEntry {
public:
    // STAND-IN SCRIPT-S338: the length of the jump clip, role 3 of the character's class, is the caller's (the port passes the player's).
    explicit CharacterVehicleEntry(std::uint32_t jumpLengthTicks) : length_(jumpLengthTicks) {}

    // VehicleJumpIn 0x1cdec0: event 200 with the vehicle, handled at once by 0x322a04 in the states 1 to 3 (no search 0x225d94); none otherwise.
    std::optional<CharacterVehicleStep> jumpIn(const std::string& character, std::size_t vehicle, const std::vector<BoardingVehicle>& vehicles,
                                               const CharacterAboard& aboard);
    // VehicleJumpOut 0x1da8d8: event 200 while the character is in the Drive state (0x24255c), handled at once by the leave test 0x322aa4, which
    // refuses a vehicle whose jump out is denied, that is stuck or destroyed (BoardingVehicle::jumpOutAllowed).
    std::optional<CharacterVehicleStep> jumpOut(const std::string& character, const std::vector<BoardingVehicle>& vehicles, const CharacterAboard& aboard,
                                                const GroundBelow& ground);
    // One update of every character in a vehicle state; a character back on foot is dropped after its CharacterLeftVehicle.
    std::vector<CharacterVehicleStep> update(const std::vector<BoardingVehicle>& vehicles, const std::function<CharacterAboard(const std::string&)>& aboard,
                                             std::uint32_t ticks, const GroundBelow& ground);

    // 0x24255c: the vehicle while the character is in the Drive state.
    std::optional<std::size_t> seatedVehicle(const std::string& character) const;
    // 0x22ec10: the vehicle in the states 0x11a, 0x11b and 0x11c.
    std::optional<std::size_t> boardedVehicle(const std::string& character) const;
    // The Drive state's begin 0x307abc hides the character's node (0x23f734(node, 0)), its end 0x307bcc shows it again.
    bool hidden(const std::string& character) const;
    std::vector<std::string> boardedCharacters() const;
    std::vector<CharacterLeftVehicle> takeLeft();

private:
    CharacterVehicleStep record(const std::string& character, const BoardingStep& step, const std::vector<BoardingVehicle>& vehicles);

    std::uint32_t length_;
    std::map<std::string, VehicleBoarding> boarded_;
    std::vector<CharacterLeftVehicle> left_;
};

}
