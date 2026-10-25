#include "game_pedestrians.hpp"
#include "game_level.hpp"
#include "runtime/player_controller_state.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <iostream>

namespace game_pedestrians {

namespace {
constexpr const char* kCharacterModelFolder = "models/char_minifig/";
constexpr const char* kGenericAnimationClass = "class_generic";

std::string lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return text;
}

std::string clipPath(const std::string& animationClass, runtime::PedestrianClip clip) {
    return lower(kCharacterModelFolder + animationClass + "/" + runtime::pedestrianClipName(clip) + ".bfnanm");
}

bool genericFallback(runtime::PedestrianClip clip) {
    return clip == runtime::PedestrianClip::DodgeRight01 || clip == runtime::PedestrianClip::DodgeLeft01;
}

render::Mat4 nodeMatrix(const std::array<runtime::PedestrianVec3, 4>& node) {
    render::Mat4 world{};
    for (std::size_t row = 0; row < 4; ++row)
        for (std::size_t axis = 0; axis < 3; ++axis) world[row * 4 + axis] = node[row][axis];
    world[15] = 1.0f;
    return world;
}

float arrivalRadiusOf(assets::FuseArchive& archive, const assets::CharacterObject& character) {
    float centre[3] = {0, 0, 0}, half[3] = {0, 0, 0};
    if (!assets::modelBounds(archive, assets::meshModelPath(character.appearance.body), centre, half)) return 0.0f;
    return runtime::pedestrianEntityBox({centre[0], centre[1], centre[2]}, {half[0], half[1], half[2]}).radius;
}

runtime::PedestrianVec3 boundsCentreOf(assets::FuseArchive& archive, const assets::CharacterObject& character) {
    float centre[3] = {0, 0, 0}, half[3] = {0, 0, 0};
    if (!assets::modelBounds(archive, assets::meshModelPath(character.appearance.body), centre, half)) return {0, 0, 0};
    return {centre[0], centre[1], centre[2]};
}
}

void LevelPedestrians::load(assets::FuseArchive& archive, const assets::Level& level, const assets::ClassLookup& classOf,
                            const std::vector<assets::CharacterObject>& characters,
                            const std::vector<std::optional<render::CharacterInstance>>& placed, assets::PlacedObjectState& state) {
    archive_ = &archive;
    system_.init();
    std::map<std::string, std::size_t> characterOf;
    for (std::size_t i = 0; i < characters.size(); ++i) characterOf[lower(characters[i].placement.instance)] = i;
    const auto& slots = system_.loadLevel(level, classOf, [&](const assets::LevelObject& object) {
        const auto found = characterOf.find(lower(object.instance));
        return found == characterOf.end() ? 0.0f : arrivalRadiusOf(archive, characters[found->second]);
    });
    slots_.clear();
    std::vector<runtime::PedestrianNodeBounds> nodeBounds;
    for (const runtime::PedestrianSlotInfo& info : slots) {
        state.disabled.insert(lower(info.instance));
        Slot slot;
        const auto found = characterOf.find(lower(info.instance));
        if (found != characterOf.end() && placed[found->second]) {
            const assets::CharacterObject& character = characters[found->second];
            slot.animationClass = character.animationClass.empty() ? kGenericAnimationClass : character.animationClass;
            slot.instance = *placed[found->second];
            slot.boundsCentre = boundsCentreOf(archive, character);
            const std::string bodyPath = assets::meshModelPath(character.appearance.body);
            auto rig = rigs_.find(bodyPath);
            if (rig == rigs_.end()) rig = rigs_.emplace(bodyPath, render::loadCharacterRig(archive, bodyPath)).first;
            slot.rig = &rig->second;
            nodeBounds.push_back(game_pedestrian_fade::nodeBoundsOf(archive, bodyPath));
        } else {
            nodeBounds.emplace_back();
        }
        slots_.push_back(std::move(slot));
    }
    fades_.load(system_, nodeBounds);
}

void LevelPedestrians::enterScene(assets::FuseArchive& archive, const assets::Level& level, const std::string& levelName,
                                  const runtime::StreamRooms* rooms, const std::map<int, runtime::RoomBox>* boxes, int currentRoom) {
    runtime::PedestrianSceneEnter enter;
    enter.loadedLevels = {&level};
    enter.pedestrianSystem = runtime::levelHasPedestrianSystem(level);
    if (const assets::FuseEntry* entry = archive.find(assets::pregenSpawnPath(assets::PregenSpawnKind::Pedestrians, levelName))) {
        pregenFile_ = archive.read(*entry);
        enter.pregenFile = &pregenFile_;
    }
    enter.rooms = {rooms, boxes, currentRoom};
    system_.enterScene(enter);
    std::cout << "pedestrians: " << system_.slots().size() << " slots, limit " << system_.manager().limit() << ", "
              << system_.spawnPointCount() << " spawn points, spawn data " << (system_.manager().spawnDataLoaded() ? "loaded" : "not loaded") << '\n';
}

void LevelPedestrians::updateSystem(const SystemInputs& inputs, assets::PlacedObjectState& state) {
    if (inputs.enabled) system_.setEnabled(*inputs.enabled);
    runtime::PedestrianSystemStep step;
    step.view = inputs.view;
    step.playerPosition = inputs.playerPosition;
    step.entityEnabled = [&](std::size_t slot) { return !state.isDisabled(system_.slots()[slot].instance); };
    step.room = [&](int room) { return std::optional<runtime::PedestrianRoom>(runtime::pedestrianRoomObject(room, inputs.roomLoaded(room))); };
    game_pedestrian_blocking::wireSystemStep(step, inputs.blocking, system_, random_);
    step.meshNodeType = runtime::kMeshNodeType;
    const runtime::PedestrianUpdate update = system_.update(step, random_);
    if (update.spawned) {
        state.disabled.erase(lower(system_.slots()[*update.spawned].instance));
        fades_.spawned(system_, *update.spawned);
    }
    for (const runtime::PedestrianRemoval& removal : update.removed) {
        if (removal.stopAnimations) slots_[removal.slot].animation.stop(clock_);
        state.disabled.insert(lower(system_.slots()[removal.slot].instance));
    }
}

std::optional<std::uint32_t> LevelPedestrians::clipOf(const std::string& animationClass, runtime::PedestrianClip clip) {
    std::string path = clipPath(animationClass, clip);
    if (!archive_->find(path) && genericFallback(clip)) path = clipPath(kGenericAnimationClass, clip);
    const auto found = clipIndex_.find(path);
    if (found != clipIndex_.end()) return found->second;
    if (!archive_->find(path)) return std::nullopt;
    Clip loaded;
    loaded.animation = render::loadAnimation(*archive_, path);
    loaded.info.clip = std::uint32_t(clips_.size());
    loaded.info.length = assets::animationLength(loaded.animation);
    loaded.info.rate = std::uint16_t(assets::animationFramesPerSecond(loaded.animation));
    loaded.info.loops = assets::animationLoops(loaded.animation);
    clips_.push_back(std::move(loaded));
    clipIndex_[path] = clips_.back().info.clip;
    return clips_.back().info.clip;
}

// STAND-IN PED-S4: 0x245f84(clip) == 6 is taken as the end of the clip in the slot's newest animation slot.
bool LevelPedestrians::clipFinished(std::size_t slot, runtime::PedestrianClip clip) {
    const runtime::AnimationSlot* current = slots_[slot].animation.current();
    const std::optional<std::uint32_t> index = clipOf(slots_[slot].animationClass, clip);
    return current && index && current->clip == *index && runtime::AnimationSlots::position(*current, clock_).finished;
}

void LevelPedestrians::updateCharacters(render::Scene& scene, const CharacterInputs& inputs, const assets::PlacedObjectState& state) {
    clock_ += std::llround(double(inputs.dt) * double(runtime::kAnimationClockTicksPerSecond));
    runtime::PedestrianCharacterStep step;
    step.dt = inputs.dt;
    step.playerPosition = inputs.playerPosition;
    step.playerInVehicle = inputs.playerInVehicle;
    step.playerState = inputs.playerState;
    // STAND-IN PED-S4: the draw test 0x2930e0 and the hidden test 0x23de70 are false.
    // STAND-IN PED-S4: the ground probe 0x2058cc is the player's ground probe of the collision world.
    if (inputs.collision)
        step.groundProbe = [&](std::size_t, runtime::PedestrianVec3& position) {
            runtime::GroundProbe probe;
            probe.x = position[0];
            probe.y = position[1];
            probe.z = position[2];
            probe.dt = inputs.dt;
            if (const std::optional<float> height = runtime::groundHeight(*inputs.collision, probe)) position[1] = *height;
        };
    step.clipFinished = [this](std::size_t slot, runtime::PedestrianClip clip) { return clipFinished(slot, clip); };
    game_pedestrian_blocking::wireCharacterStep(step, inputs.blocking, system_, random_);
    step.drawTest = [&](std::size_t slot) { return fades_.drawTest(system_, slot, inputs.fadePoint); };
    for (const runtime::PedestrianCharacterUpdate& update : system_.updateCharacters(step, random_)) {
        Slot& slot = slots_[update.slot];
        for (const runtime::PedestrianAnimationRequest& request : update.walk.animations)
            if (const std::optional<std::uint32_t> clip = clipOf(slot.animationClass, request.clip))
                slot.animation.start(clips_[*clip].info, unsigned(request.mode), request.first, request.end, request.speed, request.blend, clock_);
    }
    std::vector<bool> live(slots_.size());
    for (std::size_t i = 0; i < slots_.size(); ++i) live[i] = !state.isDisabled(system_.slots()[i].instance);
    fades_.step(system_, live, inputs.fadePoint);
    for (std::size_t i = 0; i < slots_.size(); ++i) {
        const Slot& slot = slots_[i];
        if (!slot.rig || !live[i]) continue;
        const runtime::AnimationSlot* current = slot.animation.current();
        if (!current) continue;
        const assets::Animation& animation = clips_[current->clip].animation;
        const float frame = runtime::AnimationSlots::position(*current, clock_).frame;
        const double shown = std::clamp(double(frame), 0.0, double(animation.frames > 0 ? animation.frames - 1 : 0));
        const render::Mat4 world = nodeMatrix(system_.body(i).node);
        render::placeCharacter(scene, *slot.rig, slot.instance, render::poseBody(scene, *slot.rig, slot.instance, animation, shown), world);
        game_pedestrian_fade::applyOpacity(scene, slot.instance, runtime::nodeFadeOpacity(fades_.fades().fade(i)));
    }
}

}

namespace game_level {

void LevelSession::updatePedestrianSystem() {
    if (!player || !follow) return;
    float forward[3], right[3];
    chase::render::cameraAxes(follow->yaw(), follow->pitch(), forward, right);
    game_pedestrians::SystemInputs inputs;
    inputs.enabled = levelScripts.progressNatives().trafficAndPedestriansEnabled();
    inputs.view.position = {follow->eye()[0], follow->eye()[1], follow->eye()[2]};
    inputs.view.row2 = {forward[0], forward[1], forward[2]};
    if (const std::optional<std::size_t> seated = boarding ? boarding->seatedVehicle() : std::nullopt)
        inputs.view.playerVehicleSpeed = chase::runtime::forwardSpeed(vehicleDrives[*seated].rigid);
    inputs.playerPosition = player->position();
    inputs.roomLoaded = [this](int room) { return roomShown(room); };
    const game_pedestrian_blocking::Blocking blocking = pedestrianBlocking();
    inputs.blocking = &blocking;
    pedestrians.updateSystem(inputs, objectState);
}

void LevelSession::updatePedestrianCharacters(float dt) {
    if (!player) return;
    game_pedestrians::CharacterInputs inputs;
    inputs.dt = dt;
    inputs.playerPosition = player->position();
    inputs.playerInVehicle = boarding && boarding->seatedVehicle();
    inputs.playerState = chase::runtime::playerControllerState(player->vertical, player->animation);
    inputs.collision = &collision;
    const game_pedestrian_blocking::Blocking blocking = pedestrianBlocking();
    inputs.blocking = &blocking;
    if (follow) {
        const float* look = follow->lookAt();
        const float* eye = follow->eye();
        inputs.fadePoint = chase::runtime::pedestrianFadePoint({look[0], look[1], look[2]}, {eye[0], eye[1], eye[2]}, false /* STAND-IN PED-S15: pose flag 0x40000000 not modelled */);
    }
    pedestrians.updateCharacters(scene, inputs, objectState);
}

void LevelSession::enterPedestrianScene() {
    pedestrians.enterScene(shared.archive, level, levelName, roomStreamer ? &roomStreamer->rooms() : nullptr,
                           roomStreamer ? &roomStreamer->boxes() : nullptr, roomStreamer ? roomStreamer->currentRoom() : 0);
}

}
