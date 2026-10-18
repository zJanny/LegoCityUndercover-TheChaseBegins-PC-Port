#include "game_level.hpp"
#include "runtime/scheduler.hpp"
#include "runtime/head_icon.hpp"
#include "runtime/vehicle_wheel_pose.hpp"
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
        float nodeCentre[3], nodeHalf[3];
        if (modelBounds(shared.archive, meshModelPath(placement->mesh), nodeCentre, nodeHalf))
            drive.nodeBounds = runtime::SmashBox{{nodeCentre[0], nodeCentre[1], nodeCentre[2]}, {nodeHalf[0], nodeHalf[1], nodeHalf[2]}};
        drive.placedIndex = std::size_t(std::find_if(placedObjects.begin(), placedObjects.end(), [&](const Placement& p) { return p.instance == record.instance; }) - placedObjects.begin());
        drive.history = runtime::VehicleMatrixHistory(vehicle.matrix);
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
    const std::optional<std::size_t> massScaled = boarding ? boarding->massScaledVehicle() : std::nullopt;
    runtime::DriveInput playerInput;
    if (!levelScripts.playerControlTaken() && !carry.busy() && !cutscenePlaying())
        playerInput = runtime::readDriveInput({pad.slot(kSlotStickX).value, pad.channel(14).value, pad.channel(15).value, pad.channel(16).value, pad.channel(17).value});
    const int steps = runtime::vehicleWorldSteps(vehicleWorldTime, dt);
    const CollisionItem* floorMesh = floorItem();
    std::vector<runtime::VehicleWorldBody> bodies;
    std::vector<runtime::VehicleRecord> scaled;
    std::vector<std::optional<runtime::VehicleFloor>> floors;
    std::vector<std::size_t> members;
    scaled.reserve(boardingVehicles.size());
    floors.reserve(boardingVehicles.size());
    for (std::size_t i = 0; i < boardingVehicles.size(); ++i) {
        runtime::VehicleRecord& record = *levelScripts.vehicles().mutableRecord(boardingRecords[i]);
        VehicleDrive& drive = vehicleDrives[i];
        runtime::BoardingVehicle& vehicle = boardingVehicles[i];
        if (drive.wreck.active && stepVehicleWreck(i, dt)) continue;
        const bool driven = seated && *seated == i && record.state == runtime::kVehicleStateDriven;
        if (seated && *seated == i) recordVehicleReturnPlace(i);
        if (driven && drive.started)
            if (const std::optional<runtime::DamageMessage> message = runtime::stuckDestroyMessage(drive.stuck, record.forcedStop != 0)) applyVehicleDamage(i, *message, 0.0f);
        if (drive.wreck.active) continue;
        if (!runtime::vehicleHasDynamicBody(record, driven || drive.started, objectState.isDisabled(vehicle.instance))) continue;
        const float massScale = runtime::vehicleMassScale(massScaled && *massScaled == i);
        if (!drive.started) {
            drive.started = true;
            drive.awake = true;
            drive.shape = runtime::vehicleShapeBoxes(record, vehicle.boxCentre, vehicle.boxHalf);
            drive.massScale = massScale;
            drive.rigid = runtime::vehicleRigidBody(record, {vehicle.matrix[12], vehicle.matrix[13], vehicle.matrix[14]}, runtime::vehicleHeading(vehicle),
                                                    drive.shape.shapeLow, drive.shape.shapeHigh, massScale);
            drive.drivetrain = runtime::vehicleDrivetrain(record);
            drive.boxContacts = runtime::vehicleBoxContacts(drive.shape.collisionLow, drive.shape.collisionHigh, &vehicleBoxes);
            std::cout << "vehicle " << vehicle.instance << ": Bullet body, mass scale " << massScale << " at update " << updatesForLog << '\n';
        } else if (drive.massScale != massScale) {
            drive.massScale = massScale;
            runtime::setVehicleMassScale(drive.rigid, record, drive.shape.shapeLow, drive.shape.shapeHigh, massScale);
        }
        if (record.pendingSpeedLimit) {
            runtime::limitForwardSpeed(drive.rigid, *record.pendingSpeedLimit);
            record.pendingSpeedLimit.reset();
        }
        const float forwardSpeed = runtime::forwardSpeed(drive.rigid);
        const runtime::DriveCommand command = runtime::commandForState(record.state, driven ? playerInput : runtime::DriveInput{}, record.forcedStop != 0, forwardSpeed);
        scaled.push_back(runtime::vehicleRecordAtMassScale(record, massScale));
        floors.emplace_back();
        if (floorMesh)
            floors.back() = runtime::VehicleFloor{runtime::FloorMesh(*floorMesh), drive.floorTracker,
                                                  runtime::vehicleCollisionBody(scaled.back(), drive.shape.collisionLow, drive.shape.collisionHigh, massScale), &drive.boxContacts};
        runtime::VehicleWorldBody body{i, &drive.rigid, &drive.drivetrain, &drive.awake, &scaled.back(), runtime::bodyInputs(command), floors.back() ? &*floors.back() : nullptr, &drive.wheels};
        body.controls.driver = driven;
        body.destroyed = record.destroyed;
        if (driven) body.impact = [this, i](float speed, const runtime::ContactOther& other) { vehicleImpact(i, speed, other); };
        runtime::tickVehicleDamage(record, dt);
        bodies.push_back(body);
        members.push_back(i);
    }
    for (const std::size_t i : members) smashProps(i);
    runtime::stepVehicleBodies(bodies, vehiclePairs,
                               [this](const runtime::Vector3& from, const runtime::Vector3& to) {
                                   return runtime::castWheelRay(collision, from, to, [this](const runtime::CollisionWorld::Entry& entry) { return std::any_of(boardingVehicles.begin(), boardingVehicles.end(), [&](const runtime::BoardingVehicle& vehicle) { return vehicle.instance == entry.owner; }); });
                               }, steps);
    for (std::size_t n = 0; n < members.size(); ++n) {
        const std::size_t i = members[n];
        VehicleDrive& drive = vehicleDrives[i];
        if (drive.wreck.active) continue;
        if (floors[n]) drive.floorTracker = floors[n]->tracker;
        if (bodies[n].controls.driver && drive.awake)
            for (int step = 0; step < steps; ++step)
                runtime::stepDrivenStuckTimer(drive.stuck, runtime::forwardSpeed(drive.rigid), bodies[n].controls.accelerate, bodies[n].controls.brake,
                                              drive.rigid.angularVelocity[1], runtime::kVehicleWorldStep);
        const bool driven = seated && *seated == i && levelScripts.vehicles().records()[boardingRecords[i]].state == runtime::kVehicleStateDriven;
        if (!driven && !drive.awake && drive.settled) continue;
        if (driven || drive.settled) {
            if (!drive.moved && !driven) std::cout << "vehicle " << boardingVehicles[i].instance << ": moved by a contact at update " << updatesForLog << '\n';
            drive.moved = true;
        }
        if (!drive.awake) drive.settled = true;
        runtime::VehicleRecord& record = *levelScripts.vehicles().mutableRecord(boardingRecords[i]);
        if (drive.moved && runtime::updateStuckTimer(record.stuckTimer, 1.0f, false, std::fabs(runtime::forwardSpeed(drive.rigid)), dt))
            levelScripts.vehicles().requestState(boardingRecords[i], runtime::kVehicleStateStuck);
        boardingVehicles[i].matrix = runtime::vehicleNodeMatrix(drive.rigid, record.centreOfMass);
        drive.returnPlaceGood = runtime::vehicleReturnPlaceGood(std::size_t(std::count_if(drive.wheels.begin(), drive.wheels.end(), [](const runtime::WheelState& w) { return w.contact; })),
                                                                record.wheels.size(), boardingVehicles[i].matrix[5], drive.rigid.velocity[1], record.destroyed);
        scene.setTransform(placedSceneIndex[drive.placedIndex], boardingVehicles[i].matrix);
        poseVehicleWheels(i);
    }
    suppressDrivenVehicleCollision();
}

void LevelSession::smashProps(std::size_t vehicleIndex) {
    const VehicleDrive& drive = vehicleDrives[vehicleIndex];
    const runtime::BoardingVehicle& vehicle = boardingVehicles[vehicleIndex];
    runtime::SmashingVehicle smashing;
    smashing.node = vehicle.matrix;
    smashing.nodeBounds = drive.nodeBounds;
    smashing.entityBox = runtime::SmashBox{{vehicle.boxCentre[0], vehicle.boxCentre[1], vehicle.boxCentre[2]}, {vehicle.boxHalf[0], vehicle.boxHalf[1], vehicle.boxHalf[2]}};
    std::copy(drive.rigid.velocity.begin(), drive.rigid.velocity.end(), smashing.velocity);
    for (const std::string& name : propSmasher.smash(smashing, objectState)) {
        objectState.disabled.insert(lowerCase(name));
        std::cout << "prop " << name << ": smashed by " << vehicle.instance << " at update " << updatesForLog << '\n';
    }
}

void LevelSession::poseVehicleWheels(std::size_t vehicleIndex) {
    const VehicleDrive& drive = vehicleDrives[vehicleIndex];
    const runtime::VehicleRecord& record = levelScripts.vehicles().records()[boardingRecords[vehicleIndex]];
    const chase::assets::Matrix origin = runtime::bodyOriginMatrix(drive.rigid);
    for (std::size_t w = 0; w < wheelSceneIndex.size(); ++w) {
        if (wheelSceneIndex[w].first != drive.placedIndex) continue;
        const std::size_t number = wheelNumbers[w];
        const bool simulated = number < drive.wheels.size() && number < drive.drivetrain.wheels.size();
        scene.setTransform(wheelSceneIndex[w].second,
                           simulated ? runtime::wheelPoseMatrix(record.wheels[number].suspension, drive.drivetrain.wheels[number], number, drive.wheels[number].distance, origin)
                                     : chase::assets::multiply(placementMatrix(wheelLocalPlacements[w]), boardingVehicles[vehicleIndex].matrix));
    }
}

const CollisionItem* LevelSession::floorItem() const {
    for (const runtime::CollisionWorld::Entry& entry : collision.entries())
        if (entry.item.floor) return &entry.item;
    return nullptr;
}

void LevelSession::suppressDrivenVehicleCollision() {
    for (std::size_t i = 0; i < boardingVehicles.size(); ++i)
        if (vehicleDrives[i].moved)
            for (runtime::CollisionWorld::Entry& entry : collision.mutableEntries())
                if (entry.owner == boardingVehicles[i].instance) entry.active = false;
}

runtime::BoardingInput LevelSession::boardingInput(float dt) {
    const std::vector<runtime::VehicleRecord>& records = levelScripts.vehicles().records();
    for (std::size_t i = 0; i < boardingVehicles.size(); ++i) {
        const runtime::VehicleRecord& record = records[boardingRecords[i]];
        boardingVehicles[i].disabled = objectState.isDisabled(boardingVehicles[i].instance);
        boardingVehicles[i].locked = record.locked();
        boardingVehicles[i].jumpOutAllowed = record.jumpOutAllowed() && record.stuckTimer <= 0.0f && !record.destroyed;
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
            enterVehicleHud(levelScripts.vehicles().records()[record]);
            levelScripts.vehicles().playerEntered(record);
            levelScripts.fireObjectEvent("playervehicleentered", instance);
        } else if (event.kind == runtime::BoardingEventKind::Exiting) {
            levelScripts.vehicles().playerExited(record);
            levelScripts.fireObjectEvent("playervehicleexiting", instance);
        }
        if (event.kind == runtime::BoardingEventKind::Exiting || event.kind == runtime::BoardingEventKind::Left) leaveVehicleHud();
    }
}

void LevelSession::vehicleImpact(std::size_t vehicleIndex, float speed, const runtime::ContactOther& other) {
    runtime::VehicleRecord& record = *levelScripts.vehicles().mutableRecord(boardingRecords[vehicleIndex]);
    const runtime::ContactReply reply = runtime::vehicleContactReply(record, speed, other);
    if (reply.brickBurst) std::cout << "vehicle " << record.instance << ": contact at " << speed << " m/s, brick burst of " << reply.brickCount << " (stand-in: no bricks)\n";
    if (reply.damage) applyVehicleDamage(vehicleIndex, *reply.damage, speed);
}

void LevelSession::enterVehicleHud(const runtime::VehicleRecord& record) {
    if (!hud) return;
    const std::string name = record.vehicleType < vehicleTableNames.size() ? vehicleTableNames[record.vehicleType] : std::string();
    const runtime::VehicleHud::Change change =
        vehicleHud.begin(record.hearts, name, hud->hearts(), [this](const std::string& path) { return shared.archive.find(path) != nullptr; });
    hud->setHearts(change.hearts, change.vehicleImage);
}

void LevelSession::leaveVehicleHud() {
    if (!hud) return;
    if (const std::optional<runtime::VehicleHud::Change> change = vehicleHud.end()) hud->setHearts(change->hearts, change->vehicleImage);
}

void LevelSession::syncVehicleHud() {
    if (!hud || !boarding) return;
    hud->setVehiclePortrait(vehicleHud.portrait(boarding->phase()).value_or(std::string()));
}

void LevelSession::updateHeadIcon(float dt) {
    headIcon.update(dt);
    if (!boarding || !player || boarding->phase() != runtime::BoardingPhase::OnFoot) return;
    if (runtime::findVehicleToEnter(boardingVehicles, boardingInput(0.0f).player)) headIcon.request();
}

void LevelSession::drawHeadIcon(chase::frontend::Canvas& top) {
    if (!player || !follow) return;
    const float node[3] = {player->motion.x, player->vertical.y, player->motion.z};
    const std::array<float, 3> position = runtime::headIconPosition(node, runtime::kPlayerEntityScaleMeasured);
    const float* eye = follow->eye();
    const float dx = position[0] - eye[0], dy = position[1] - eye[1], dz = position[2] - eye[2];
    const int alpha = runtime::headIconAlphaByte(headIcon.fade(), std::sqrt(dx * dx + dy * dy + dz * dz));
    if (alpha <= 0) return;
    const runtime::HeadIconScreen screen = runtime::projectHeadIcon(position, eye, follow->yaw(), follow->pitch(), follow->verticalFieldOfView());
    if (!screen.visible) return;
    const Image& icon = hudResources.image(runtime::kHeadIconButtonX);
    top.drawScaled(icon, {0, 0, icon.width, icon.height}, screen.centreX - screen.width * 0.5f, screen.centreY - screen.height * 0.5f, screen.width, screen.height,
                   {1, 1, 1, float(alpha) / 255.0f});
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
    if (!cutscenePlaying()) player->setVisible(scene, true);
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
