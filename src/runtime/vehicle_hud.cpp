#include "runtime/vehicle_hud.hpp"

#include <cctype>

namespace chase::runtime {

std::string vehiclePortraitPath(const std::string& tableName, const std::function<bool(const std::string&)>& exists) {
    std::string lower = tableName;
    for (char& c : lower) c = char(std::tolower(static_cast<unsigned char>(c)));
    const std::string path = "sprites/portraits/" + lower + ".bpng";
    return exists(path) ? path : kVehiclePortraitFallback;
}

bool vehiclePortraitShown(bool loaded, BoardingPhase phase) {
    return loaded && phase != BoardingPhase::OnFoot;
}

VehicleHud::Change VehicleHud::begin(std::uint8_t hearts, const std::string& tableName, unsigned currentHearts,
                                     const std::function<bool(const std::string&)>& exists) {
    portrait_ = vehiclePortraitPath(tableName, exists);
    savedHearts_ = currentHearts;
    active_ = true;
    return {hearts, true};
}

std::optional<VehicleHud::Change> VehicleHud::end() {
    if (!active_) return std::nullopt;
    active_ = false;
    return Change{savedHearts_, false};
}

std::optional<std::string> VehicleHud::portrait(BoardingPhase phase) {
    if (vehiclePortraitShown(!portrait_.empty(), phase)) return portrait_;
    if (phase == BoardingPhase::OnFoot) portrait_.clear();
    return std::nullopt;
}

}
