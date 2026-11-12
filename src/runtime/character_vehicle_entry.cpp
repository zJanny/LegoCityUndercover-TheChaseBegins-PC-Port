#include "runtime/character_vehicle_entry.hpp"

#include <algorithm>
#include <iterator>

namespace chase::runtime {

namespace {

BoardingInput inputOf(const CharacterAboard& aboard, std::uint32_t ticks, const GroundBelow& ground) {
    BoardingInput input;
    input.action = false;
    input.playerFree = false;
    input.groundState = aboard.enterEventState;
    std::copy(aboard.position.begin(), aboard.position.end(), input.player.position);
    input.player.boxHalfY = aboard.boxHalfY;
    input.facing = aboard.facing;
    input.ticks = ticks;
    input.ground = ground;
    return input;
}

}

CharacterVehicleStep CharacterVehicleEntry::record(const std::string& character, const BoardingStep& step, const std::vector<BoardingVehicle>& vehicles) {
    for (const BoardingEvent& event : step.events)
        if (event.kind == BoardingEventKind::Left && event.vehicle < vehicles.size())
            left_.push_back({character, vehicles[event.vehicle].instance, step.position});
    return {character, step};
}

std::optional<CharacterVehicleStep> CharacterVehicleEntry::jumpIn(const std::string& character, std::size_t vehicle, const std::vector<BoardingVehicle>& vehicles,
                                                                  const CharacterAboard& aboard) {
    if (vehicle >= vehicles.size() || boarded_.count(character) || !aboard.enterEventState) return std::nullopt;
    VehicleBoarding boarding(length_);
    BoardingInput input = inputOf(aboard, 0, {});
    input.scriptJumpIn = vehicle;
    const BoardingStep step = boarding.update(vehicles, input);
    if (step.phase != BoardingPhase::JumpIn) return std::nullopt;
    boarded_.emplace(character, boarding);
    return record(character, step, vehicles);
}

std::optional<CharacterVehicleStep> CharacterVehicleEntry::jumpOut(const std::string& character, const std::vector<BoardingVehicle>& vehicles,
                                                                   const CharacterAboard& aboard, const GroundBelow& ground) {
    const auto found = boarded_.find(character);
    if (found == boarded_.end() || found->second.phase() != BoardingPhase::Seated) return std::nullopt;
    BoardingInput input = inputOf(aboard, 0, ground);
    input.groundState = true;
    input.scriptJumpOut = true;
    const BoardingStep step = found->second.update(vehicles, input);
    if (step.phase != BoardingPhase::JumpOut) return std::nullopt;
    return record(character, step, vehicles);
}

std::vector<CharacterVehicleStep> CharacterVehicleEntry::update(const std::vector<BoardingVehicle>& vehicles,
                                                                const std::function<CharacterAboard(const std::string&)>& aboard, std::uint32_t ticks,
                                                                const GroundBelow& ground) {
    std::vector<CharacterVehicleStep> steps;
    for (auto it = boarded_.begin(); it != boarded_.end();) {
        BoardingInput input = inputOf(aboard(it->first), ticks, ground);
        input.groundState = false;
        steps.push_back(record(it->first, it->second.update(vehicles, input), vehicles));
        it = it->second.phase() == BoardingPhase::OnFoot ? boarded_.erase(it) : std::next(it);
    }
    return steps;
}

std::optional<std::size_t> CharacterVehicleEntry::seatedVehicle(const std::string& character) const {
    const auto found = boarded_.find(character);
    return found == boarded_.end() ? std::nullopt : found->second.seatedVehicle();
}

std::optional<std::size_t> CharacterVehicleEntry::boardedVehicle(const std::string& character) const {
    const auto found = boarded_.find(character);
    return found == boarded_.end() ? std::nullopt : found->second.boardedVehicle();
}

bool CharacterVehicleEntry::hidden(const std::string& character) const { return seatedVehicle(character).has_value(); }

std::vector<std::string> CharacterVehicleEntry::boardedCharacters() const {
    std::vector<std::string> names;
    for (const auto& entry : boarded_) names.push_back(entry.first);
    return names;
}

std::vector<CharacterLeftVehicle> CharacterVehicleEntry::takeLeft() {
    std::vector<CharacterLeftVehicle> taken;
    taken.swap(left_);
    return taken;
}

}
