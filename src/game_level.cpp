#include "game_level.hpp"
#include "frontend/minimap.hpp"
#include "frontend/pages.hpp"
#include "game_save.hpp"
#include "runtime/follow_camera.hpp"
#include "runtime/object_visibility.hpp"
#include <SDL.h>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <stdexcept>

namespace game_level {
using namespace chase::assets;
using namespace chase::render;
using game_doors::DoorObject;
using game_player::PlayerRig;
namespace runtime = chase::runtime;
namespace fe = chase::frontend;

namespace {
constexpr const char* kGameTextFile = "localisation/gametext_uk.loc";
constexpr const char* kSystemTextFile = "localisation/systemtext_uk.loc";
constexpr std::uint32_t kContinueTextKey = 0xe3f70996;  // systemtext "Continue": inferred from the loading page text, key not read in the code (TRANS-S3)

std::unique_ptr<Scene> makeScene(FuseArchive& archive, const std::string& levelPath) {
    auto scene = std::make_unique<Scene>(archive, std::vector<std::string>{"sky"});
    for (const auto& model : levelModels(archive, levelPath, ""))
        if (!scene->addModel(model)) throw std::runtime_error("not in archive: " + model);
    return scene;
}

Level readLevel(FuseArchive& archive, const std::string& levelPath) {
    const FuseEntry* entry = archive.find(levelPath);
    if (!entry) throw std::runtime_error("not in archive: " + levelPath);
    return parseLevel(archive.read(*entry));
}

std::vector<runtime::StudSpawn> studSpawns(const DrawableObjects& drawable) {
    std::vector<runtime::StudSpawn> spawns;
    for (const StudObject& stud : drawable.studs) {
        runtime::StudSpawn spawn;
        spawn.kind = runtime::StudKind(stud.kind);
        std::copy_n(stud.position, 3, spawn.position);
        spawn.group = stud.group;
        spawns.push_back(spawn);
    }
    return spawns;
}

// The level's collision: the items of the level's master model (models/<level>/<level>.bfnmdl) and the placed objects.
runtime::CollisionWorld loadCollision(FuseArchive& archive, const std::string& levelPath, const std::vector<ObjectCollision>& objects) {
    runtime::CollisionWorld world;
    const std::string path = lower(levelPath);
    const std::string folder = path.substr(0, path.find_last_of('/'));
    const std::string name = folder.substr(folder.find_last_of('/') + 1);
    const FuseEntry* entry = archive.find("models/" + name + "/" + name + ".bfnmdl");
    if (!entry) return world;
    for (const CollisionItem& item : parseModel(archive.read(*entry)).collision) world.add(item);
    for (const ObjectCollision& object : objects) world.addObject(object);
    return world;
}
}

std::uint32_t clockTicks(std::uint64_t counter, std::uint64_t frequency) {
    std::uint64_t seconds = counter / frequency, rest = counter % frequency;
    return std::uint32_t(seconds * runtime::kClockFrequency + rest * runtime::kClockFrequency / frequency);
}

void PlaceholderCamera::frameScene(const float centre[3], float radius) {
    float d = radius * 2.2f;
    float axisForward[3], axisRight[3];
    chase::render::cameraAxes(yaw, pitch, axisForward, axisRight);
    for (int i = 0; i < 3; ++i) eye[i] = centre[i] - d * axisForward[i];
    speed = radius * 0.5f;
}

void PlaceholderCamera::update(float dt, const runtime::PadDevice& pad) {
    yaw += 1.5f * dt * (pad.slot(kSlotButtonY).value - pad.slot(kSlotButtonX).value);
    float strafe = pad.slot(kSlotStickX).value, forward = pad.slot(kSlotStickY).value;
    float rise = pad.slot(kSlotRightShoulder).value - pad.slot(kSlotLeftShoulder).value;
    float step = speed * dt;
    float axisForward[3], axisRight[3];
    chase::render::cameraAxes(yaw, pitch, axisForward, axisRight);
    for (int i = 0; i < 3; ++i) eye[i] += step * (forward * axisForward[i] + strafe * axisRight[i]);
    eye[1] += step * rise;
}

LevelSession::LevelSession(const Shared& sharedState)
    : shared(sharedState),
      sceneOwner(makeScene(shared.archive, shared.options.level)),
      scene(*sceneOwner),
      level(readLevel(shared.archive, shared.options.level)),
      drawable(drawableObjects(shared.archive, level)),
      studs(studSpawns(drawable)),
      classOf(archiveClassLookup(shared.archive)),
      objectState(runtime::levelInitialState(level, classOf)),
      levelScripts(level, objectState),
      sceneStart(level, classOf, levelScripts),
      currentWorldLevel(runtime::worldLevelOfFile(shared.master, shared.options.level)),
      hudResources(shared.archive),
      hudCanvas(fe::kTopWidth, fe::kScreenHeight),
      mapCanvas(fe::kBottomWidth, fe::kScreenHeight) {
    const game_options::Options& options = shared.options;
    FuseArchive& archive = shared.archive;
    std::size_t placed = 0, unresolved = 0;
    std::map<std::string, std::pair<std::size_t, Mat4>> placedInstances;
    for (const auto& placement : drawable.objects) {
        const std::size_t instanceIndex = scene.instances();
        const Mat4 matrix = placementMatrix(placement);
        if (scene.addModel(meshModelPath(placement.mesh), matrix)) {
            ++placed;
            placedInstances[placement.instance] = {instanceIndex, matrix};
            placedSceneIndex.push_back(instanceIndex);
            placedObjects.push_back(placement);
            roomInstances[runtime::roomNumber(placement.room)].push_back(instanceIndex);
        } else {
            ++unresolved;
        }
    }
    const std::size_t characters = addLevelCharacters(scene, archive, drawable.characters);
    studSprites.emplace(archive);
    player = game_player::placePlayer(archive, scene, level);
    levelScripts.setAllRoomsLoaded();
    if (!options.saveFile.empty()) {
        const chase::save::SlotStart saved = game_save::load(options.saveFile, options.saveSlot, archive, level, options.level);
        game_save::apply(saved, levelScripts);
        std::cout << "save: slot " << options.saveSlot << ", level " << saved.level << ", "
                  << (saved.stateSaved ? "saved state of the level (StoryProgress " + std::to_string(saved.storyProgress) + ", " + std::to_string(saved.variables.size()) + " variables)" : std::string("no saved state of the level"))
                  << ", unlocked levels 0x" << std::hex << saved.levelUnlocked << std::dec << ", game complete " << saved.gameComplete << '\n';
    }
    runtime::SceneStartOptions startOptions;
    startOptions.dt = runtime::kStartupStepDt;
    startOptions.maxUpdates = runtime::kStartupMaxUpdates;
    startOptions.untilAllEnded = false;
    if (!options.startRoom.empty()) startOptions.startRoom = runtime::roomNumber(options.startRoom);
    else if (const std::optional<Placement> start = playerStart(level)) startOptions.startRoom = runtime::roomNumber(start->room);
    const std::uint32_t startupUpdates = sceneStart.run(startOptions);
    for (const std::string& name : options.callScripts)
        if (!levelScripts.start(name) && !shared.afterSwitch) throw std::runtime_error("no level script " + name);
    for (const runtime::ScriptPlacement& request : levelScripts.takePlacements()) applyPlacement(request);
    if (player && options.start) {
        player->motion.x = player->startX = (*options.start)[0];
        player->vertical.y = (*options.start)[1];
        player->motion.z = player->startZ = (*options.start)[2];
        player->pose(scene);
    }
    if (player && options.facing) {
        player->motion.facing = game_player::headingFromDegrees(*options.facing);
        player->motion.lastIntent = player->motion.facing;
        player->pose(scene);
    }
    reportUnhandled();
    std::cout << "scripts: startup ended after " << startupUpdates << " updates, " << levelScripts.threadCount() << " threads running\n";
    const std::vector<ObjectCollision> placedCollisions = objectCollisions(archive, level, objectState);
    collision = loadCollision(archive, options.level, placedCollisions);
    doors = game_doors::loadDoors(scene, archive, level, classOf, placedCollisions, placedInstances);
    for (const DoorObject& door : doors) game_doors::poseDoor(scene, door);
    std::cout << "doors: " << doors.size() << '\n';
    std::cout << "collision: " << collision.triangleCount() << " triangles in " << collision.entries().size() << " entries\n";

    float centre[3];
    scene.bounds(centre, radius);
    std::cout << "loaded " << scene.instances() << " instance(s) (" << placed << " placed, " << unresolved
              << " unresolved meshnames, " << characters << " characters standing in place, "
              << studs.studs().size() << " studs (" << studs.count(runtime::StudState::Idle) << " visible), "
              << drawable.missingModel << " without model in the archive, " << drawable.skeletal
              << " other skeletal, " << drawable.customCharacters << " custom character and " << drawable.markers
              << " marker objects skipped), " << scene.triangles() << " triangles, " << scene.missingTextures()
              << " missing textures\n";

    camera.frameScene(centre, radius);
    if (player) {
        std::string camText;
        if (const FuseEntry* camEntry = archive.find(options.level.substr(0, options.level.size() - 5) + ".cam")) {
            const Bytes camBytes = archive.read(*camEntry);
            camText.assign(camBytes.begin(), camBytes.end());
        }
        follow.emplace(level, camText);
        if (options.cameraCollision) follow->setCollision(&collision);
        const float target[3] = {player->motion.x, player->vertical.y, player->motion.z};
        follow->reset(target, runtime::headingRadians(player->motion.facing), runtime::headingRadians(player->motion.facing));
    }
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    chase::render::setFrontFace(true);

    if (player) {
        fe::HudConfig hudConfig;
        const std::string characterFile = game_player::kPlayerCharacter;
        const std::size_t nameStart = characterFile.find("char_") + 5;
        hudConfig.portrait = characterFile.substr(nameStart, characterFile.find(".blvl") - nameStart);
        hud.emplace(hudResources, hudConfig);
    }
    overlay.emplace();
    const std::string levelFolder = lower(options.level).substr(0, lower(options.level).find_last_of('/'));
    levelName = levelFolder.substr(levelFolder.find_last_of('/') + 1);
    startRoomStreaming();

    counterFrequency = SDL_GetPerformanceFrequency();
    counterOrigin = SDL_GetPerformanceCounter();
    scheduler.emplace(runtime::StepConfig{}, now());

    if (!options.script.empty()) {
        if (!player) throw std::runtime_error("--script needs a level with a player start");
        script = game_options::parseScript(options.script);
    }
    for (const game_options::ScriptStep& step : script) scriptLength += step.updates;
}

void LevelSession::draw(GLuint program, int width, int height) {
    const game_hud::Screens screens = game_hud::screensFor(width, height);
    glDisable(GL_SCISSOR_TEST);
    glViewport(0, 0, width, height);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_SCISSOR_TEST);
    glScissor(screens.top.x, screens.top.y, screens.top.width, screens.top.height);
    glViewport(screens.top.x, screens.top.y, screens.top.width, screens.top.height);
    glClearColor(0.16f, 0.17f, 0.2f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    beginFrame(program, 1.0f);
    const float* eye = follow ? follow->eye() : camera.eye;
    const float viewYaw = follow ? follow->yaw() : camera.yaw, viewPitch = follow ? follow->pitch() : camera.pitch;
    const float fieldOfView = follow ? follow->verticalFieldOfView() : 1.0f;
    const Mat4 viewMatrix = chase::render::view(eye, viewYaw, viewPitch);
    Mat4 mvp = chase::render::multiply(
        perspective(fieldOfView, float(fe::kTopWidth) / float(fe::kScreenHeight), radius * 0.002f, radius * 20), viewMatrix);
    const std::vector<bool> drawn = runtime::drawnPlacements(placedObjects, objectState);
    for (std::size_t i = 0; i < drawn.size(); ++i)
        scene.setInstanceVisible(placedSceneIndex[i], drawn[i] && roomShown(runtime::roomNumber(placedObjects[i].room)));
    scene.draw(program, mvp);
    std::vector<StudSprite> sprites;
    for (const runtime::Stud& stud : studs.studs())
        if (stud.state == runtime::StudState::Idle || stud.state == runtime::StudState::Collecting)
            sprites.push_back({{stud.centre[0], stud.centre[1], stud.centre[2]}, runtime::kStudTint[std::size_t(stud.kind)]});
    studSprites->draw(program, viewMatrix, mvp, sprites, runtime::studSpriteFrame(std::uint32_t(provisionalStudClockSeconds * double(runtime::kClockTicksPerSecond))),
                     runtime::kStudHalfSize);
    glDisable(GL_SCISSOR_TEST);
    if (hud) {
        hudCanvas.clearTransparent();
        hud->draw(hudCanvas, provisionalStudClockSeconds);
        fe::MinimapView mapView;
        mapView.x = player->motion.x;
        mapView.z = player->motion.z;
        mapView.cameraYaw = follow->yaw();
        mapView.playerYaw = runtime::headingRadians(player->motion.facing);
        fe::drawMinimap(mapCanvas, hudResources, levelName, mapView);
    } else {
        hudCanvas.clearTransparent();
        mapCanvas.clear(0, 0, 0);
    }
    if (levelScripts.dialogOpen()) {
        const runtime::ScriptDialog dialog = levelScripts.dialog();
        fe::drawWarningDialog(hudCanvas, hudResources, hudResources.textForKey(kGameTextFile, dialog.bodyKey),
                              hudResources.textForKey(kSystemTextFile, dialog.confirmKey), hudResources.textForKey(kSystemTextFile, dialog.cancelKey));
    }
    if (sceneChangeFlow.pageOpen()) {
        fe::drawLoadingTop(hudCanvas, hudResources);
        fe::drawSceneChangeBottom(mapCanvas, hudResources, hudResources.textForKey(kSystemTextFile, kContinueTextKey));
    }
    fe::drawBlackVeil(hudCanvas, 1.0f - levelScripts.fadeLevel());
    fe::drawBlackVeil(mapCanvas, 1.0f - levelScripts.fadeLevel());
    overlay->draw(hudCanvas, mapCanvas, screens, width, height);
}

void LevelSession::reportScripted() const {
    const float moveX = player->motion.x - player->startX, moveZ = player->motion.z - player->startZ;
    std::cout << "scripted: " << updates << " updates, player moved " << std::hypot(moveX, moveZ)
              << " units, facing " << player->motion.facing << ", run timer " << player->motion.runTimer << ", height " << player->vertical.y << '\n';
    std::cout << "studs: counter " << studs.counter() << ", " << studs.count(runtime::StudState::Collecting) << " collecting, "
              << studs.count(runtime::StudState::Done) << " collected, " << studs.count(runtime::StudState::Idle) << " left\n";
    if (hud) std::cout << "hud: " << hud->hearts() << " hearts, stud display " << hud->studDisplay() << ", bar state "
                       << int(hud->studBarState()) << '\n';
    std::cout << "doors:";
    for (const DoorObject& door : doors)
        if (door.state.state != 1) std::cout << ' ' << door.instance << '=' << door.state.state << '/' << int(door.state.timer);
    std::cout << '\n';
    std::cout << "sounds:";
    for (const auto& [update, id] : soundLog) std::cout << ' ' << update << ':' << id;
    std::cout << '\n';
    std::cout << "heights:";
    for (const float y : scriptedHeights) std::cout << ' ' << y;
    std::cout << '\n';
}

void LevelSession::reportRate() {
    std::cout << "updates/s " << updatesThisSecond << ", dt " << scheduler->lastDt() << " s, stick "
              << pad.slot(kSlotStickX).value << ' ' << pad.slot(kSlotStickY).value << '\n';
    updatesLastSecond = updatesThisSecond;
    updatesThisSecond = 0;
}

runtime::DebugGameValues LevelSession::debugValues() const {
    runtime::DebugGameValues values;
    values.updatesPerSecond = updatesLastSecond;
    values.level = levelName;
    values.scriptThreads = levelScripts.threadCount();
    if (roomStreamer) values.loadedRooms = roomStreamer->loadedRooms().size();
    if (player) {
        values.hasPlayer = true;
        const std::array<float, 3> position = player->position();
        std::copy(position.begin(), position.end(), values.position);
        values.headingDegrees = runtime::headingRadians(player->motion.facing) * 180.0f / float(M_PI);
    }
    return values;
}

void LevelSession::releaseOverlays() {
    studSprites.reset();
    overlay.reset();
}

void advanceToDestination(FuseArchive& archive, game_options::Options& options, const runtime::SceneChangeDestination& destination,
                          unsigned updates, unsigned& scriptOffset, bool& afterSwitch) {
    const FuseEntry* destinationEntry = archive.find(destination.worldLevel.levelFile);
    if (!destinationEntry) throw std::runtime_error("not in archive: " + destination.worldLevel.levelFile);
    const std::optional<Placement> destinationPoint = runtime::destinationPoint(parseLevel(archive.read(*destinationEntry)), destination);
    if (!destinationPoint) throw std::runtime_error("scene change destination point missing in " + destination.worldLevel.instance);
    std::cout << "scene change to " << destination.worldLevel.instance << " after " << updates << " updates\n";
    scriptOffset += updates;
    afterSwitch = true;
    options.level = destination.worldLevel.levelFile;
    options.start = std::array<float, 3>{destinationPoint->position[0], destinationPoint->position[1], destinationPoint->position[2]};
    options.facing = destinationPoint->orientation[1];
    options.startRoom = destinationPoint->room;
}

}
