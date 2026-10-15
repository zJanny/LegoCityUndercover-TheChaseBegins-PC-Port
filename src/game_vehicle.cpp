#include "game_level.hpp"
#include "runtime/scheduler.hpp"
#include <cmath>
#include <iostream>

namespace game_level {
using namespace chase::assets;
namespace runtime = chase::runtime;

namespace {
std::string lowerCase(std::string text) {
    for (char& c : text) c = char(std::tolower(static_cast<unsigned char>(c)));
    return text;
}
}

void LevelSession::loadBoardingVehicles(const std::map<std::string, std::pair<std::size_t, chase::render::Mat4>>& placedInstances) {
    if (!player) return;
    const Animation& jump = player->animations.at(runtime::kAnimationJump);
    boarding.emplace(runtime::animationLengthTicks(jump.frames, animationFramesPerSecond(jump)));
    const std::vector<runtime::VehicleRecord>& records = levelScripts.vehicles().records();
    for (std::size_t index = 0; index < records.size(); ++index) {
        const runtime::VehicleRecord& record = records[index];
        const auto owner = placedInstances.find(record.instance);
        const auto placement = std::find_if(drawable.objects.begin(), drawable.objects.end(), [&](const Placement& p) { return p.instance == record.instance; });
        if (owner == placedInstances.end() || placement == drawable.objects.end()) continue;
        runtime::BoardingVehicle vehicle;
        if (!modelCollisionBox(shared.archive, meshModelPath(placement->mesh), vehicle.boxCentre, vehicle.boxHalf)) continue;
        vehicle.instance = lowerCase(record.instance);
        vehicle.matrix = owner->second.second;
        std::copy(record.driverPosition.begin(), record.driverPosition.end(), vehicle.driverPosition);
        std::copy(record.centreOfMass.begin(), record.centreOfMass.end(), vehicle.centreOfMass);
        boardingVehicles.push_back(vehicle);
        boardingRecords.push_back(index);
    }
    std::cout << "boardable vehicles: " << boardingVehicles.size() << '\n';
    levelScripts.vehicles().setOccupantLookup([this](const std::string& character) -> std::string {
        const std::optional<std::size_t> seated = boarding ? boarding->seatedVehicle() : std::nullopt;
        return character == "player1" && seated ? boardingVehicles[*seated].instance : std::string();
    });
}

runtime::BoardingStep LevelSession::stepBoarding(float dt) {
    levelScripts.vehicles().promotePending();
    if (!boarding || !player || cutscenePlaying()) return {};
    const std::vector<runtime::VehicleRecord>& records = levelScripts.vehicles().records();
    for (std::size_t i = 0; i < boardingVehicles.size(); ++i) {
        const runtime::VehicleRecord& record = records[boardingRecords[i]];
        boardingVehicles[i].disabled = objectState.isDisabled(boardingVehicles[i].instance);
        boardingVehicles[i].locked = record.locked();
        boardingVehicles[i].jumpOutAllowed = record.jumpOutAllowed();
    }
    runtime::BoardingInput input;
    input.action = pad.slot(kSlotButtonX).press > 0;
    input.playerFree = !levelScripts.playerControlTaken() && !carry.busy() && player->vertical.posture == runtime::Posture::Ground;
    input.player.position[0] = player->motion.x;
    input.player.position[1] = player->vertical.y;
    input.player.position[2] = player->motion.z;
    input.player.carryMode = carry.carryingMode();
    input.facing = player->motion.facing;
    input.ticks = runtime::stepTicks(dt);
    input.ground = [this, dt](float x, float y, float z, float depth) -> std::optional<float> {
        runtime::GroundProbe probe;
        probe.x = x;
        probe.y = y;
        probe.z = z;
        probe.dt = dt;
        probe.distance = depth;
        const std::optional<runtime::GroundContact> contact = runtime::groundContact(collision, probe);
        if (!contact) return std::nullopt;
        return contact->height;
    };
    const runtime::BoardingStep step = boarding->update(boardingVehicles, input);
    for (const runtime::BoardingEvent& event : step.events) {
        const std::size_t record = boardingRecords[event.vehicle];
        const std::string& instance = boardingVehicles[event.vehicle].instance;
        static const char* const names[] = {"jump in started", "entered", "exiting", "left"};
        std::cout << "vehicle " << instance << ": " << names[int(event.kind)] << " at update " << updatesForLog << '\n';
        if (event.kind == runtime::BoardingEventKind::Entered) {
            levelScripts.vehicles().requestState(record, runtime::kVehicleRequestDriven);
            levelScripts.fireObjectEvent("playervehicleentered", instance);
        } else if (event.kind == runtime::BoardingEventKind::Exiting) {
            levelScripts.vehicles().requestState(record, runtime::kVehicleRequestHandBrake);
            levelScripts.fireObjectEvent("playervehicleexiting", instance);
        }
    }
    return step;
}

game_player::PlayerAnimation LevelSession::applyBoarding(const runtime::BoardingStep& step) {
    player->motion.x = step.position[0];
    player->motion.z = step.position[2];
    player->vertical.y = step.position[1];
    player->motion.facing = step.facing;
    player->motion.lastIntent = step.facing;
    player->setVisible(scene, step.visible);
    return {step.jumpAnimation ? runtime::kAnimationJump : 0, 1.0f};
}

}
