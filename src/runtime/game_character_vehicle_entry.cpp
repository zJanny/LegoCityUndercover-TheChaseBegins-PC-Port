#include "game_level.hpp"

#include <cmath>
#include <iostream>

namespace game_level {
namespace runtime = chase::runtime;

namespace {

constexpr float kTwoPi = 6.28318530718f;

std::int16_t facingOf(const chase::assets::Matrix& world) {
    return std::int16_t(std::lround(std::atan2(world[8], world[10]) * 65536.0f / kTwoPi));
}

chase::assets::Matrix facingMatrix(const std::array<float, 3>& position, std::int16_t facing) {
    const float heading = float(facing) * kTwoPi / 65536.0f, s = std::sin(heading), c = std::cos(heading);
    return {c, 0, -s, 0, 0, 1, 0, 0, s, 0, c, 0, position[0], position[1], position[2], 1};
}

}

runtime::CharacterAboard LevelSession::characterAboard(const std::string& character) const {
    runtime::CharacterAboard aboard;
    if (const std::optional<chase::assets::Matrix> world = levelCharacters ? levelCharacters->world(character) : std::nullopt) {
        aboard.position = {(*world)[12], (*world)[13], (*world)[14]};
        aboard.facing = facingOf(*world);
    }
    const runtime::AiCharacter* ai = levelScripts.ai().character(character);
    aboard.enterEventState = !objectState.isDisabled(character) && !(ai && ai->superJump);
    return aboard;
}

void LevelSession::characterBoardingRequest(runtime::BoardingRequestKind kind, const std::string& character, const std::string& vehicle) {
    if (!characterVehicles || !player || !levelCharacters || !levelCharacters->world(character)) return;
    const runtime::BoardingInput refreshed = boardingInput(0.0f);
    std::optional<runtime::CharacterVehicleStep> step;
    if (kind == runtime::BoardingRequestKind::JumpOut) {
        step = characterVehicles->jumpOut(character, boardingVehicles, characterAboard(character), refreshed.ground);
    } else {
        for (std::size_t i = 0; i < boardingVehicles.size() && !step; ++i)
            if (boardingVehicles[i].instance == vehicle) step = characterVehicles->jumpIn(character, i, boardingVehicles, characterAboard(character));
    }
    if (step) applyCharacterVehicleStep(*step);
}

void LevelSession::applyCharacterVehicleStep(const runtime::CharacterVehicleStep& step) {
    for (const runtime::BoardingEvent& event : step.step.events) {
        const std::size_t record = boardingRecords[event.vehicle];
        const std::string& instance = boardingVehicles[event.vehicle].instance;
        static const char* const names[] = {"jump in started", "entered", "exiting", "left"};
        std::cout << "character " << step.character << ": vehicle " << instance << ' ' << names[int(event.kind)] << " at update " << updatesForLog << '\n';
        if (event.kind == runtime::BoardingEventKind::Entered) levelScripts.vehicles().playerEntered(record);
        else if (event.kind == runtime::BoardingEventKind::Exiting) levelScripts.vehicles().playerExited(record);
    }
    if (step.step.takesControl && levelCharacters) levelCharacters->place(step.character, facingMatrix(step.step.position, step.step.facing));
    for (const runtime::CharacterLeftVehicle& left : characterVehicles->takeLeft())
        std::cout << "character " << left.character << ": left vehicle " << left.vehicle << " at " << left.position[0] << ' ' << left.position[1] << ' '
                  << left.position[2] << " at update " << updatesForLog << '\n';
}

void LevelSession::stepCharacterVehicles(float dt) {
    if (!characterVehicles || !player || characterVehicles->boardedCharacters().empty()) return;
    const runtime::BoardingInput refreshed = boardingInput(dt);
    for (const runtime::CharacterVehicleStep& step :
         characterVehicles->update(boardingVehicles, [this](const std::string& character) { return characterAboard(character); }, refreshed.ticks, refreshed.ground))
        applyCharacterVehicleStep(step);
}

}
