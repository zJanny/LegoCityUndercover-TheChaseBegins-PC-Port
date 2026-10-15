#pragma once
// The HUD while the player is in a vehicle (docs/re/vehicles-boarding.md): the vehicle portrait loaded by 0x262188 at the begin of the Drive
// state, the portrait choice of the draw callback 0x193604 and the hearts of SetHearts 0x2316b4 at the begin and the end of the Drive state
// (0x307abc, 0x307bcc). No SDL dependency.
#include "runtime/vehicle_boarding.hpp"

#include <cstdint>
#include <functional>
#include <optional>
#include <string>

namespace chase::runtime {

// 0x262188 formats "sprites/portraits/%s.png" (the archive holds .bpng) with the vehicle table name and loads it when the file exists, else
// "sprites/portraits/Question.tga" (.btga).
constexpr const char* kVehiclePortraitFallback = "sprites/portraits/question.btga";
std::string vehiclePortraitPath(const std::string& tableName, const std::function<bool(const std::string&)>& exists);

// 0x193604: the vehicle portrait is used while it is loaded and 0x22ec10 (states 0x11a, 0x11b, 0x11c) or 0x24255c (0x11c) finds a vehicle.
bool vehiclePortraitShown(bool loaded, BoardingPhase phase);

class VehicleHud {
public:
    struct Change {
        unsigned hearts = 0;
        bool vehicleImage = false;
    };

    // 0x262188 then SetHearts(record byte +0x30d) with the vehicle heart image (the player's state +0x6e is 0x11c). `currentHearts` is kept for the
    // end. STAND-IN HUD-S50: the player's health (ctrl+0x70) that 0x307bec restores is the hearts shown before the Drive state began, the
    // port has no player health value.
    Change begin(std::uint8_t healthStart, const std::string& tableName, unsigned currentHearts, const std::function<bool(const std::string&)>& exists);
    // 0x307bcc: SetHearts(ctrl+0x70) with the normal heart image (the state +0x6e is already 0x11b); nothing when no Drive state began.
    std::optional<Change> end();
    // 0x193604: the vehicle portrait path while it is shown; it is freed as soon as the player is on foot.
    std::optional<std::string> portrait(BoardingPhase phase);

private:
    std::string portrait_;
    unsigned savedHearts_ = 0;
    bool active_ = false;
};

}
