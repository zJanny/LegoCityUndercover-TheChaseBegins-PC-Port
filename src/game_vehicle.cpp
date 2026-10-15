#include "game_level.hpp"
#include "runtime/scheduler.hpp"
#include <algorithm>
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
        VehicleDrive drive;
        drive.placedIndex = std::size_t(std::find_if(placedObjects.begin(), placedObjects.end(), [&](const Placement& p) { return p.instance == record.instance; }) - placedObjects.begin());
        vehicleDrives.push_back(drive);
    }
    loadVehicleCameras();
    std::cout << "boardable vehicles: " << boardingVehicles.size() << '\n';
    levelScripts.vehicles().setBoardingHandler([this](runtime::BoardingRequestKind kind, const std::string& vehicle) { scriptBoardingRequest(kind, vehicle); });
    levelScripts.vehicles().setOccupantLookup([this](const std::string& character) -> std::string {
        const std::optional<std::size_t> seated = boarding ? boarding->seatedVehicle() : std::nullopt;
        return character == "player1" && seated ? boardingVehicles[*seated].instance : std::string();
    });
}

std::optional<runtime::WaypointVehicle> LevelSession::waypointVehicle() const {
    const std::optional<std::size_t> boarded = boarding ? boarding->boardedVehicle() : std::nullopt;
    if (!boarded) return std::nullopt;
    const runtime::BoardingVehicle& vehicle = boardingVehicles[*boarded];
    runtime::WaypointVehicle result;
    result.instance = vehicle.instance;
    result.type = std::int8_t(levelScripts.vehicles().records()[boardingRecords[*boarded]].vehicleType);
    result.matrix = vehicle.matrix;
    std::copy(vehicle.boxCentre, vehicle.boxCentre + 3, result.boxCentre);
    std::copy(vehicle.boxHalf, vehicle.boxHalf + 3, result.boxHalf);
    return result;
}

void LevelSession::driveVehicles(float dt) {
    const std::optional<std::size_t> seated = boarding ? boarding->seatedVehicle() : std::nullopt;
    runtime::DriveInput playerInput;
    if (!levelScripts.playerControlTaken() && !carry.busy() && !cutscenePlaying())
        playerInput = runtime::readDriveInput({pad.slot(kSlotStickX).value, pad.channel(14).value, pad.channel(15).value, pad.channel(16).value, pad.channel(17).value});
    for (std::size_t i = 0; i < boardingVehicles.size(); ++i) {
        runtime::VehicleRecord& record = *levelScripts.vehicles().mutableRecord(boardingRecords[i]);
        VehicleDrive& drive = vehicleDrives[i];
        runtime::BoardingVehicle& vehicle = boardingVehicles[i];
        const bool driven = seated && *seated == i && record.state == runtime::kVehicleStateDriven;
        if (!driven && drive.body.speed == 0.0f) continue;
        if (!drive.started) {
            drive.started = true;
            suppressDrivenVehicleCollision();
            drive.body.position = {vehicle.matrix[12], vehicle.matrix[13], vehicle.matrix[14]};
            drive.body.heading = runtime::vehicleHeading(vehicle);
            drive.rigid = runtime::vehicleRigidBody(record, drive.body.position, drive.body.heading);
        }
        if (record.pendingSpeedLimit) {
            drive.body.speed = std::clamp(drive.body.speed, -*record.pendingSpeedLimit, *record.pendingSpeedLimit);
            record.pendingSpeedLimit.reset();
        }
        const runtime::DriveCommand command = runtime::commandForState(record.state, driven ? playerInput : runtime::DriveInput{}, record.forcedStop != 0, drive.body.speed);
        runtime::stepDriveBody(drive.body, command, record.topSpeed, dt);
        if (runtime::updateStuckTimer(record.stuckTimer, 1.0f, false, std::fabs(drive.body.speed), dt)) levelScripts.vehicles().requestState(boardingRecords[i], runtime::kVehicleStateStuck);
        runtime::stepVehiclePhysics(drive.rigid, record, drive.body.heading, drive.body.speed,
                                    [this](const runtime::Vector3& from, const runtime::Vector3& to) { return runtime::castWheelRay(collision, from, to); }, dt);
        vehicle.matrix = runtime::vehicleNodeMatrix(drive.rigid, record.centreOfMass);
        drive.body.position = {vehicle.matrix[12], vehicle.matrix[13], vehicle.matrix[14]};
        scene.setTransform(placedSceneIndex[drive.placedIndex], vehicle.matrix);
        for (std::size_t w = 0; w < wheelSceneIndex.size(); ++w)
            if (wheelSceneIndex[w].first == drive.placedIndex) scene.setTransform(wheelSceneIndex[w].second, chase::assets::multiply(placementMatrix(wheelLocalPlacements[w]), vehicle.matrix));
    }
}

void LevelSession::suppressDrivenVehicleCollision() {
    for (std::size_t i = 0; i < boardingVehicles.size(); ++i)
        if (vehicleDrives[i].started)
            for (runtime::CollisionWorld::Entry& entry : collision.mutableEntries())
                if (entry.owner == boardingVehicles[i].instance) entry.active = false;
}

runtime::BoardingInput LevelSession::boardingInput(float dt) {
    const std::vector<runtime::VehicleRecord>& records = levelScripts.vehicles().records();
    for (std::size_t i = 0; i < boardingVehicles.size(); ++i) {
        const runtime::VehicleRecord& record = records[boardingRecords[i]];
        boardingVehicles[i].disabled = objectState.isDisabled(boardingVehicles[i].instance);
        boardingVehicles[i].locked = record.locked();
        boardingVehicles[i].jumpOutAllowed = record.jumpOutAllowed() && record.stuckTimer <= 0.0f;
    }
    runtime::BoardingInput input;
    input.action = pad.slot(kSlotButtonX).press > 0;
    input.groundState = player->vertical.posture == runtime::Posture::Ground;
    input.playerFree = !levelScripts.playerControlTaken() && !carry.busy() && input.groundState;
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
    return input;
}

void LevelSession::handleBoardingEvents(const std::vector<runtime::BoardingEvent>& events) {
    for (const runtime::BoardingEvent& event : events) {
        const std::size_t record = boardingRecords[event.vehicle];
        const std::string& instance = boardingVehicles[event.vehicle].instance;
        static const char* const names[] = {"jump in started", "entered", "exiting", "left"};
        std::cout << "vehicle " << instance << ": " << names[int(event.kind)] << " at update " << updatesForLog << '\n';
        if (event.kind == runtime::BoardingEventKind::Entered) {
            levelScripts.vehicles().playerEntered(record);
            levelScripts.fireObjectEvent("playervehicleentered", instance);
        } else if (event.kind == runtime::BoardingEventKind::Exiting) {
            levelScripts.vehicles().playerExited(record);
            levelScripts.fireObjectEvent("playervehicleexiting", instance);
        }
    }
}

runtime::BoardingStep LevelSession::stepBoarding(float dt) {
    levelScripts.vehicles().promotePending();
    driveVehicles(dt);
    if (!boarding || !player || cutscenePlaying()) return {};
    const runtime::BoardingStep step = boarding->update(boardingVehicles, boardingInput(dt));
    handleBoardingEvents(step.events);
    return step;
}

void LevelSession::scriptBoardingRequest(runtime::BoardingRequestKind kind, const std::string& vehicle) {
    if (!boarding || !player) return;
    runtime::BoardingInput input = boardingInput(0.0f);
    input.ticks = 0;
    input.action = false;
    if (kind == runtime::BoardingRequestKind::JumpOut) input.scriptJumpOut = true;
    else
        for (std::size_t i = 0; i < boardingVehicles.size(); ++i)
            if (boardingVehicles[i].instance == vehicle) input.scriptJumpIn = i;
    const runtime::BoardingStep step = boarding->update(boardingVehicles, input);
    handleBoardingEvents(step.events);
    if (step.takesControl) applyBoarding(step);
}

void LevelSession::cancelBoardingByPlacement() {
    if (!boarding || !player || boarding->phase() == runtime::BoardingPhase::OnFoot) return;
    const runtime::BoardingStep step = boarding->placed(boardingInput(0.0f));
    handleBoardingEvents(step.events);
    player->setVisible(scene, true);
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
