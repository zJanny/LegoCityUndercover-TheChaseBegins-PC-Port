#include "runtime/vehicle_boarding.hpp"

#include <algorithm>
#include <cmath>

namespace chase::runtime {

namespace {
constexpr float kPi = 3.14159265358979f;
constexpr float kTwoPi = 2.0f * kPi;

std::array<float, 3> lerp(const std::array<float, 3>& from, const std::array<float, 3>& to, float u) {
    return {from[0] + (to[0] - from[0]) * u, from[1] + (to[1] - from[1]) * u, from[2] + (to[2] - from[2]) * u};
}

std::int16_t headingUnits(float radians) {
    return static_cast<std::int16_t>(static_cast<std::int32_t>(std::lround(radians * 65536.0f / kTwoPi)) & 0xffff);
}
}

std::array<float, 3> boardingToLocal(const assets::Matrix& m, const float point[3]) {
    const float x = point[0] - m[12], y = point[1] - m[13], z = point[2] - m[14];
    return {x * m[0] + y * m[1] + z * m[2], x * m[4] + y * m[5] + z * m[6], x * m[8] + y * m[9] + z * m[10]};
}

std::array<float, 3> boardingToWorld(const assets::Matrix& m, const float p[3]) {
    return {p[0] * m[0] + p[1] * m[4] + p[2] * m[8] + m[12], p[0] * m[1] + p[1] * m[5] + p[2] * m[9] + m[13],
            p[0] * m[2] + p[1] * m[6] + p[2] * m[10] + m[14]};
}

float distanceToBox(const float point[3], const float centre[3], const float half[3]) {
    float sum = 0;
    for (const int a : {0, 2}) {
        const float low = centre[a] - half[a], high = centre[a] + half[a];
        float excess = 0;
        if (point[a] < low) excess = point[a] - low;
        else if (point[a] > high) excess = point[a] - high;
        sum += excess * excess;
    }
    return std::sqrt(sum);
}

std::optional<std::size_t> findVehicleToEnter(const std::vector<BoardingVehicle>& vehicles, const BoardingPlayer& player) {
    if (player.carryMode == 2) return std::nullopt;
    std::optional<std::size_t> found;
    float best = kBoardRangeSquared;
    for (std::size_t i = 0; i < vehicles.size(); ++i) {
        const BoardingVehicle& vehicle = vehicles[i];
        if (vehicle.disabled || vehicle.locked) continue;
        const std::array<float, 3> local = boardingToLocal(vehicle.matrix, player.position);
        const float distance = distanceToBox(local.data(), vehicle.boxCentre, vehicle.boxHalf);
        const float squared = distance * distance;
        if (!(squared < best)) continue;
        const float height = std::fabs(local[1] - vehicle.boxCentre[1]) - (vehicle.boxHalf[1] + player.boxHalfY);
        if (player.boxCentreY + kBoardHeightSlack >= height) {
            best = squared;
            found = i;
        }
    }
    return found;
}

std::uint32_t animationLengthTicks(unsigned frames, float framesPerSecond) {
    return static_cast<std::uint32_t>(std::lround(float(frames - 1) / framesPerSecond * 3000.0f));
}

std::array<float, 3> bodyPosition(const BoardingVehicle& vehicle) { return boardingToWorld(vehicle.matrix, vehicle.centreOfMass); }

std::array<float, 3> seatPosition(const BoardingVehicle& vehicle) { return boardingToWorld(vehicle.matrix, vehicle.driverPosition); }

float vehicleHeading(const BoardingVehicle& vehicle) { return std::atan2(vehicle.matrix[8], vehicle.matrix[10]); }

std::array<float, 3> chooseExitPosition(const BoardingVehicle& vehicle, const GroundBelow& ground, float playerBoxHalfY) {
    const float reachX = std::max(kExitMinimumDistance, kExitMargin + vehicle.boxHalf[0]);
    const float reachZ = std::max(kExitMinimumDistance, kExitMargin + vehicle.boxHalf[2]);
    const float degrees = kPi / 180.0f;
    for (const float angle : kExitAnglesDegrees) {
        const float local[3] = {reachX * std::sin(angle * degrees), 0.0f, reachZ * std::cos(angle * degrees)};
        const std::array<float, 3> world = boardingToWorld(vehicle.matrix, local);
        if (const std::optional<float> height = ground(world[0], world[1], world[2], kExitDepth)) return {world[0], *height, world[2]};
    }
    std::array<float, 3> top = boardingToWorld(vehicle.matrix, vehicle.boxCentre);
    top[1] += vehicle.boxHalf[1] + playerBoxHalfY;
    return top;
}

BoardingStep VehicleBoarding::step(BoardingPhase phase, const std::array<float, 3>& position, std::int16_t facing, bool visible, bool takesControl) const {
    BoardingStep result;
    result.phase = phase;
    result.takesControl = takesControl;
    result.placed = takesControl;
    result.position = position;
    result.facing = facing;
    result.visible = visible;
    result.jumpAnimation = phase == BoardingPhase::JumpIn || phase == BoardingPhase::JumpOut;
    return result;
}

std::array<float, 3> VehicleBoarding::arc(float u) const {
    std::array<float, 3> position = lerp(from_, to_, u);
    position[1] += kBoardArcHeight * std::sin(kPi * u);
    return position;
}

BoardingStep VehicleBoarding::update(const std::vector<BoardingVehicle>& vehicles, const BoardingInput& input) {
    const std::array<float, 3> here = {input.player.position[0], input.player.position[1], input.player.position[2]};
    const auto heading = [&](float radians) { return headingUnits(radians); };
    switch (phase_) {
    case BoardingPhase::OnFoot: {
        if (!input.action || !input.playerFree) return step(phase_, here, input.facing, true, false);
        const std::optional<std::size_t> found = findVehicleToEnter(vehicles, input.player);
        if (!found) return step(phase_, here, input.facing, true, false);
        vehicle_ = *found;
        phase_ = BoardingPhase::JumpIn;
        elapsed_ = 0;
        from_ = here;
        to_ = seatPosition(vehicles[vehicle_]);
        fromHeading_ = float(input.facing) * kTwoPi / 65536.0f;
        toHeading_ = vehicleHeading(vehicles[vehicle_]);
        if (fromHeading_ - toHeading_ < -kPi) fromHeading_ += kTwoPi;
        if (fromHeading_ - toHeading_ > kPi) fromHeading_ -= kTwoPi;
        BoardingStep begun = step(phase_, here, input.facing, true, true);
        begun.jumpAnimation = false;
        begun.events.push_back({BoardingEventKind::JumpInStarted, vehicle_});
        return begun;
    }
    case BoardingPhase::JumpIn: {
        elapsed_ += input.ticks;
        const BoardingVehicle& vehicle = vehicles[vehicle_];
        if (elapsed_ >= length_) {
            phase_ = BoardingPhase::Seated;
            BoardingStep seated = step(phase_, bodyPosition(vehicle), heading(toHeading_), false, true);
            seated.events.push_back({BoardingEventKind::Entered, vehicle_});
            return seated;
        }
        const float u = float(elapsed_) / float(length_);
        return step(phase_, arc(u), heading(fromHeading_ + (toHeading_ - fromHeading_) * u), true, true);
    }
    case BoardingPhase::Seated: {
        const BoardingVehicle& vehicle = vehicles[vehicle_];
        const std::array<float, 3> seatedAt = bodyPosition(vehicle);
        if (!input.action || !input.playerFree || !vehicle.jumpOutAllowed) return step(phase_, seatedAt, heading(toHeading_), false, true);
        phase_ = BoardingPhase::JumpOut;
        elapsed_ = 0;
        from_ = seatPosition(vehicle);
        to_ = chooseExitPosition(vehicle, input.ground, input.player.boxHalfY);
        BoardingStep leaving = step(phase_, from_, heading(toHeading_), true, true);
        leaving.jumpAnimation = false;
        leaving.events.push_back({BoardingEventKind::Exiting, vehicle_});
        return leaving;
    }
    case BoardingPhase::JumpOut: {
        elapsed_ += input.ticks;
        if (elapsed_ >= length_) {
            phase_ = BoardingPhase::OnFoot;
            BoardingStep landed = step(phase_, to_, heading(toHeading_), true, true);
            landed.events.push_back({BoardingEventKind::Left, vehicle_});
            return landed;
        }
        return step(phase_, arc(float(elapsed_) / float(length_)), heading(toHeading_), true, true);
    }
    }
    return step(phase_, here, input.facing, true, false);
}

}
