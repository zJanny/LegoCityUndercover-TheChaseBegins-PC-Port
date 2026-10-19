#include "game_level.hpp"
#include "runtime/collision_enable.hpp"
#include "frontend/script_text_pages.hpp"
#include "frontend/minimap.hpp"
#include "frontend/pages.hpp"
#include "game_save.hpp"
#include "game_screen_fade.hpp"
#include "runtime/fog.hpp"
#include "runtime/glow.hpp"
#include "runtime/follow_camera.hpp"
#include "runtime/nav_load.hpp"
#include "runtime/object_visibility.hpp"
#include "runtime/sky.hpp"
#include <SDL.h>
#include <algorithm>
#include <cmath>
#include <filesystem>
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
constexpr const char* kHintTextFile = "localisation/hinttext_uk.loc";
constexpr const char* kSystemTextFile = "localisation/systemtext_uk.loc";
constexpr std::uint32_t kContinueTextKey = 0xe3f70996;  // systemtext "Continue": inferred from the loading page text, key not read in the code (TRANS-S3)

std::unique_ptr<Scene> makeScene(FuseArchive& archive, const std::string& levelPath) {
    auto scene = std::make_unique<Scene>(archive, std::vector<std::string>{});
    for (const auto& model : levelModels(archive, levelPath, ""))
        if (!scene->addModel(model)) throw std::runtime_error("not in archive: " + model);
    return scene;
}

Level readLevel(FuseArchive& archive, const std::string& levelPath) {
    const FuseEntry* entry = archive.find(levelPath);
    if (!entry) throw std::runtime_error("not in archive: " + levelPath);
    return parseLevel(archive.read(*entry));
}

std::vector<runtime::VehicleTableRow> vehicleTable(FuseArchive& archive) {
    const FuseEntry* entry = archive.find("tables/tablescharacters.bxls");
    return entry ? runtime::parseVehicleTable(parseTable(archive.read(*entry))) : std::vector<runtime::VehicleTableRow>();
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

// The level's collision: the items of the level's master model (models/<level>/<level>.bfnmdl) and the placed objects; the master model's
// static Bullet boxes for the vehicles.
runtime::CollisionWorld loadCollision(FuseArchive& archive, const std::string& levelPath, const std::vector<ObjectCollision>& objects,
                                      std::vector<runtime::BulletBox>& vehicleBoxes) {
    runtime::CollisionWorld world;
    const std::string path = lower(levelPath);
    const std::string folder = path.substr(0, path.find_last_of('/'));
    const std::string name = folder.substr(folder.find_last_of('/') + 1);
    const FuseEntry* entry = archive.find("models/" + name + "/" + name + ".bfnmdl");
    if (!entry) return world;
    const Model model = parseModel(archive.read(*entry));
    for (const CollisionItem& item : model.collision) world.add(item);
    vehicleBoxes = runtime::levelVehicleBoxes(model);
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
      particles(shared.archive, shared.options.particles),
      classOf(archiveClassLookup(shared.archive)),
      objectState(runtime::levelInitialState(level, classOf)),
      fogObjects(chase::assets::fogObjects(level, classOf)),
      lightObjects(chase::assets::lightObjects(level, classOf)),
      backgroundColour(chase::assets::levelBackgroundColour(level)),
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
    std::set<std::string> animationTargets = runtime::animationTargets(level);
    catRescue.load(level, classOf);
    for (const std::string& cat : catRescue.cats()) animationTargets.insert(cat);
    std::vector<runtime::AnimatedProp> animatedProps;
    if (const auto sea = chase::assets::objectStart(level, "Sea")) seaHeight = sea->position[1];
    for (const auto& placement : drawable.objects) {
        const std::size_t instanceIndex = scene.instances();
        const Mat4 matrix = placementMatrix(placement);
        if (placement.className == runtime::kSkyClass) {
            // STAND-IN RENDER-S51: the original's choice among several sky objects (lvl_06 places two) is not read; the first is used.
            if (!skyInstance && scene.addModel(meshModelPath(placement.mesh), identity())) {
                skyInstance = instanceIndex;
                scene.setInstanceSky(instanceIndex);
            }
            continue;
        }
        const std::string lowerInstance = game_doors::lowerCase(placement.instance);
        const bool animated = animationTargets.count(lowerInstance) != 0;
        const std::string modelPath = meshModelPath(placement.mesh);
        if (scene.addModel(modelPath, matrix, {}, animated ? runtime::propSceneKey(modelPath, lowerInstance) : std::string())) {
            if (animated) animatedProps.push_back({lowerInstance, modelPath, placement.mesh, matrix});
            ++placed;
            placedInstances[placement.instance] = {instanceIndex, matrix};
            placedSceneIndex.push_back(instanceIndex);
            placedObjects.push_back(placement);
            placedRooms.push_back(runtime::roomNumber(placement.room));
            roomInstances[runtime::roomNumber(placement.room)].push_back(instanceIndex);
        } else {
            ++unresolved;
        }
    }
    placeNestedProps(archive);
    for (const auto& waypoint : drawable.waypoints) {
        const std::size_t instanceIndex = scene.instances();
        if (!scene.addModel(meshModelPath(waypoint.mesh), placementMatrix(waypoint))) continue;
        waypointSceneIndex.push_back(instanceIndex);
        waypointObjects.push_back(waypoint);
        propParts.addWaypointChildren(scene, classOf, waypointObjects.size() - 1, waypoint);
    }
    placedCharacters = placeLevelCharacters(scene, archive, drawable.characters);
    const std::size_t characters = std::size_t(std::count_if(placedCharacters.begin(), placedCharacters.end(), [](const auto& placed) { return placed.has_value(); }));
    levelCharacters.emplace(archive, drawable.characters, placedCharacters);
    propAnimations = runtime::PropAnimations(archive, animatedProps);
    animationDrivers.add(&*levelCharacters);
    animationDrivers.add(&propAnimations);
    playerScriptAnimation = runtime::PlayerScriptAnimation(runtime::archiveAnimationLoader(archive));
    animationDrivers.add(&playerScriptAnimation);
    levelScripts.setAnimationDriver(&animationDrivers);
    levelScripts.loadAnimationOffsets(classOf);
    levelScripts.loadObjectKinds(classOf);
    installScriptHooks();
    fallApart.load(archive, particles.random(), chase::render::readBaseAppearance(archive));
    studDrops.load(particles.random());
    superBricks.load(drawable.superBricks, placedObjects);
    studSprites.emplace(archive);
    glowPass.emplace();
    player = game_player::placePlayer(archive, scene, level);
    levelScripts.setAllRoomsLoaded();
    levelScripts.loadWaypoints(classOf);
    const std::vector<runtime::VehicleTableRow> vehicleRows = vehicleTable(archive);
    for (const runtime::VehicleTableRow& row : vehicleRows) vehicleTableNames.push_back(row.name);
    levelScripts.loadVehicles(classOf, vehicleRows);
    for (const runtime::VehicleRecord& vehicle : levelScripts.vehicles().records()) {
        const auto owner = placedInstances.find(vehicle.instance);
        if (owner == placedInstances.end()) continue;
        const std::size_t placedIndex = std::size_t(std::find(placedSceneIndex.begin(), placedSceneIndex.end(), owner->second.first) - placedSceneIndex.begin());
        for (const runtime::VehicleWheel& wheel : vehicle.wheels) {
            const std::size_t wheelIndex = scene.instances();
            if (scene.addModel(meshModelPath(wheel.local.mesh), chase::assets::multiply(placementMatrix(wheel.local), owner->second.second))) {
                wheelSceneIndex.push_back({placedIndex, wheelIndex});
                wheelLocalPlacements.push_back(wheel.local);
                wheelNumbers.push_back(std::size_t(&wheel - vehicle.wheels.data()));
                float centre[3] = {0, 0, 0}, half[3] = {0, 0, 0};
                modelBounds(archive, meshModelPath(wheel.local.mesh), centre, half);
                wheelBounds.push_back({{centre[0], centre[1], centre[2]}, half[0]});
            }
        }
    }
    loadBoardingVehicles(placedInstances);
    levelName = lower(options.level).substr(0, lower(options.level).find_last_of('/'));
    levelName = levelName.substr(levelName.find_last_of('/') + 1);
    levelScripts.setLevelHeadingKey(fe::levelHeadingKey(hudResources, levelName));
    levelScripts.setObjectPositionSource([this](const std::string& instance, float out[3]) {
        if (carry.position(instance, out)) return true;
        const std::optional<Placement> placement = objectStart(level, instance);
        if (placement) std::copy(placement->position, placement->position + 3, out);
        return placement.has_value();
    });
    std::vector<runtime::NavWorld> navWorlds = runtime::loadNavWorlds(archive, options.level, level);
    if (!navWorlds.empty()) levelScripts.ai().setNavigation(std::move(navWorlds));
    if (!options.saveFile.empty()) {
        const chase::save::SlotStart saved = game_save::load(options.saveFile, options.saveSlot, archive, level, options.level);
        game_save::apply(saved, levelScripts);
        saveWriter = game_save::requestWriter(options, archive);
        std::cout << "save: slot " << options.saveSlot << ", level " << saved.level << ", "
                  << (saved.stateSaved ? "saved state of the level (StoryProgress " + std::to_string(saved.storyProgress) + ", " + std::to_string(saved.variables.size()) + " variables)" : std::string("no saved state of the level"))
                  << ", unlocked levels 0x" << std::hex << saved.levelUnlocked << std::dec << ", game complete " << saved.gameComplete << '\n';
    }
    if (!shared.afterSwitch)
        for (const auto& [name, value] : options.variables) {
            const std::uint32_t hash = chase::assets::nameHash(name);
            const bool known = std::any_of(level.defaults.begin(), level.defaults.end(), [&](const chase::assets::LevelParam& param) { return param.hash == hash; });
            if (!known) throw std::runtime_error("--set-variable: no level variable named '" + name + "'");
            levelScripts.setVariable(hash, value);
        }
    runtime::SceneStartOptions startOptions;
    startOptions.dt = runtime::kStartupStepDt;
    startOptions.maxUpdates = runtime::kStartupMaxUpdates;
    startOptions.untilAllEnded = false;
    if (!options.startRoom.empty()) startOptions.startRoom = runtime::roomNumber(options.startRoom);
    else if (const std::optional<Placement> start = playerStart(level)) startOptions.startRoom = runtime::roomNumber(start->room);
    cutsceneHost.emplace(archive, scene, shared.audio);
    if (player)
        cutsceneHost->setRoster(game_cutscene::makeRoster(archive, drawable.characters, placedCharacters, game_player::playerAppearance(archive), player->instance,
                                                          game_cutscene::levelProps(level, classOf, placedObjects, placedSceneIndex, animatedProps)));
    movies.emplace(archive, hudResources, (std::filesystem::path(options.fib).parent_path() / "cutscenes").string(), int(options.language));
    introMovieSkip = options.introMovieShown;
    const auto hostCutscenes = [this]() {
        levelScripts.cutscenes().setLengthSource([this](const std::string& name) { return cutsceneHost->load(name); });
        levelScripts.cutscenes().setHostPlaysMovies(true);
    };
    if (options.startCutscenes) {
        hostCutscenes();
        startOptions.yieldWhen = [this]() { return levelScripts.cutscenes().playing() != nullptr || levelScripts.cutscenes().movieOpen(); };
    }
    if (shared.audio) shared.audio->startLevelMusic(archive, levelName);
    const std::uint32_t startupUpdates = sceneStart.run(startOptions);
    levelScripts.vehicles().settle();
    if (!options.startCutscenes) hostCutscenes();
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
    threadLog.emplace(level);
    for (const std::string& name : options.callScripts) threadLog->learn(name);
    levelScripts.takeThreadEvents();
    std::cout << "scripts: startup ended after " << startupUpdates << " updates, " << levelScripts.threadCount() << " threads running\n";
    const std::vector<ObjectCollision> placedCollisions = objectCollisionCandidates(archive, level);
    collision = loadCollision(archive, options.level, placedCollisions, vehicleBoxes);
    propSmasher.load(level, classOf, placedCollisions);
    loadScriptDestroyProps(archive);
    loadDestructibleProps(archive);
    loadAttack(archive, placedCollisions);
    loadObjectiveBoxes(placedCollisions);
    runtime::applyObjectCollisionState(collision, objectState);
    doors = game_doors::loadDoors(archive, level, classOf, placedCollisions, placedInstances);
    for (const DoorObject& door : doors) game_doors::poseDoor(scene, door);
    std::cout << "doors: " << doors.size() << '\n';
    buildits.load(archive, level, classOf);
    std::cout << "build-its: " << buildits.objects().size() << '\n';
    propParts.loadBuildits(scene, archive, buildits, placedObjects, placedSceneIndex);
    effects.load(scene, archive, particles.random(), studDrops.planes());
    std::cout << "debris objects: " << effects.debrisInstances() << '\n';
    carry.load(archive, level, classOf);
    if (player) game_carry::applyClipTimings(carry, *player);
    std::cout << "carry objects: " << carry.objects().size() << ", carry targets: " << carry.targets().size() << '\n';
    loadDetective(archive);
    handItems.load(archive, scene);
    useSpots.load(level, classOf);
    scanner.load(archive, level, classOf);
    std::cout << "scanner zones: " << scanner.zones().size() << ", scannable objects: " << scanner.objects().size() << '\n';
    ziplines = runtime::loadZiplineAnchors(level, classOf);
    loadZiplineRopes(archive);
    placeZiplinePoles();
    std::cout << "zipline anchors: " << ziplines.size() << '\n';
    grabBricks = runtime::loadGrabBricks(level, classOf);
    std::cout << "climbing bricks: " << grabBricks.size() << '\n';
    climbBars = runtime::loadClimbBars(level, classOf);
    std::cout << "climb bars: " << climbBars.bars.size() << ", corner rails: " << climbBars.corners.size() << '\n';
    pole.poles = runtime::loadPoles(level, classOf);
    std::cout << "poles: " << pole.poles.size() << '\n';
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
    chase::render::setDepthTest(true);
    chase::render::setCullFace(true);
    chase::render::setFrontFace(true);

    if (player) {
        fe::HudConfig hudConfig;
        const std::string characterFile = game_player::kPlayerCharacter;
        const std::size_t nameStart = characterFile.find("char_") + 5;
        hudConfig.portrait = characterFile.substr(nameStart, characterFile.find(".blvl") - nameStart);
        hud.emplace(hudResources, hudConfig);
    }
    overlay.emplace(fe::kTopWidth, fe::kBottomWidth, fe::kScreenHeight);
    startRoomStreaming();

    counterFrequency = SDL_GetPerformanceFrequency();
    counterOrigin = SDL_GetPerformanceCounter();
    scheduler.emplace(runtime::StepConfig{}, pacer.clock());

    if (!options.script.empty()) {
        if (!player) throw std::runtime_error("--script needs a level with a player start");
        script = game_options::parseScript(options.script);
    }
    for (const game_options::ScriptStep& step : script) scriptLength += step.updates;
}

std::vector<fe::MinimapMarker> LevelSession::activeMapMarkers() const {
    std::vector<fe::MinimapMarker> markers;
    for (std::size_t i = 0; i < waypointObjects.size(); ++i)
        if (levelScripts.objectives().waypointActive(game_doors::lowerCase(waypointObjects[i].instance)))
            markers.push_back({int(i), waypointObjects[i].position[0], waypointObjects[i].position[2]});
    return markers;
}

void LevelSession::draw(int width, int height) {
    const game_hud::Screens screens = game_hud::screensFor(width, height);
    setScissorTest(false);
    setViewport({0, 0, width, height});
    clearWindow({0, 0, 0, 1}, true);
    setScissorTest(true);
    setScissorBox(screens.top);
    setViewport(screens.top);
    clearWindow({backgroundColour[0] / 255.0f, backgroundColour[1] / 255.0f, backgroundColour[2] / 255.0f, 1}, true);
    beginFrame(1.0f);
    const runtime::ClipPlanes followPlanes = runtime::followClipPlanes(follow ? follow->follow().nearPlane() : runtime::kCameraNearLimit);
    const runtime::ScannerRender scannerLook = runtime::scannerRender(scanner.viewShown());
    if (!scannerLook.noFog) applyFog(followPlanes);
    const float* eye = follow ? follow->eye() : camera.eye;
    float viewYaw = follow ? follow->yaw() : camera.yaw, viewPitch = follow ? follow->pitch() : camera.pitch;
    float fieldOfView = follow ? follow->verticalFieldOfView() : 1.0f;
    float scannerEye[3];
    if (scannerView(scannerEye, viewYaw, viewPitch, fieldOfView)) eye = scannerEye;
    Mat4 viewMatrix = chase::render::view(eye, viewYaw, viewPitch);
    float shownFieldOfView = fieldOfView, nearPlane = radius * 0.002f, farPlane = radius * 20;
    if (follow) {
        nearPlane = followPlanes.nearZ;
        farPlane = followPlanes.farZ;
    }
    if (const auto cutsceneCamera = cutsceneHost->camera(levelScripts.cutscenes())) {
        viewMatrix = cutsceneCamera->view;
        shownFieldOfView = cutsceneCamera->fieldOfView;
        nearPlane = game_cutscene::kNearPlane;
        farPlane = game_cutscene::kFarPlane;
    }
    Mat4 mvp = chase::render::multiply(
        perspective(shownFieldOfView, float(fe::kTopWidth) / float(fe::kScreenHeight), nearPlane, farPlane), viewMatrix);
    const Mat4 viewToWorld = chase::render::rigidInverse(viewMatrix);
    scene.setLighting(runtime::levelLights(lightObjects, roomStreamer ? roomStreamer->currentRoom() : 0), &viewToWorld[12]);
    std::vector<bool> drawn = runtime::drawnPlacements(placedObjects, objectState);
    for (std::size_t i = 0; i < drawn.size(); ++i) drawn[i] = runtime::drawnWhileHeld(drawn[i], cutsceneHost->holdsObject(placedObjects[i].objectIndex));
    applyObjectFades(drawn);
    applySuperBricks();
    characterVisibility.apply(scene, drawable.characters, placedCharacters, objectState,
                              [this](std::size_t objectIndex) { return cutsceneHost->holdsObject(objectIndex); });
    for (std::size_t i = 0; i < drawn.size(); ++i)
        scene.setInstanceVisible(placedSceneIndex[i], drawn[i] && roomShown(placedRooms[i]) && !wreckHides(i, std::nullopt));
    showNestedProps(drawn);
    for (std::size_t w = 0; w < wheelSceneIndex.size(); ++w) {
        const auto& [placedIndex, wheelIndex] = wheelSceneIndex[w];
        scene.setInstanceVisible(wheelIndex, runtime::nestedObjectDrawn(drawn[placedIndex]) && roomShown(placedRooms[placedIndex]) && !wreckHides(placedIndex, wheelNumbers[w]));
    }
    const runtime::ObjectiveState& objectives = levelScripts.objectives();
    std::vector<float> badgeFades(waypointObjects.size(), 1.0f);
    for (std::size_t i = 0; i < waypointObjects.size(); ++i) {
        const runtime::Waypoint* waypoint = levelScripts.waypoints().find(game_doors::lowerCase(waypointObjects[i].instance));
        scene.setInstanceFade(waypointSceneIndex[i], waypoint ? waypoint->columnFade() : 1.0f);
        if (waypoint) badgeFades[i] = waypoint->badgeFade();
        const bool columnShown = waypoint ? waypoint->columnShown() : objectives.waypointActive(game_doors::lowerCase(waypointObjects[i].instance));
        scene.setInstanceVisible(waypointSceneIndex[i], columnShown && roomShown(runtime::roomNumber(waypointObjects[i].room)));
    }
    std::vector<bool> shownPlaced(drawn.size()), shownWaypoints(waypointObjects.size());
    for (std::size_t i = 0; i < drawn.size(); ++i) shownPlaced[i] = drawn[i] && roomShown(placedRooms[i]);
    for (std::size_t i = 0; i < waypointObjects.size(); ++i)
        shownWaypoints[i] = objectives.waypointActive(game_doors::lowerCase(waypointObjects[i].instance)) && roomShown(runtime::roomNumber(waypointObjects[i].room));
    propParts.apply(scene, buildits, placedObjects, placedSceneIndex, shownPlaced, shownWaypoints, badgeFades);
    effects.apply(scene);
    fallApart.apply(scene);
    drawFootprints();
    if (player) handItems.draw(scene, *player);
    if (skyInstance) {
        const float lift = player && seaHeight ? runtime::skyLift(player->vertical.y, *seaHeight) : 0.0f;
        const Mat4 cameraToWorld = chase::render::rigidInverse(viewMatrix);
        const float skyEye[3] = {cameraToWorld[12], cameraToWorld[13], cameraToWorld[14]};
        const auto offset = runtime::skyTranslation(skyEye, lift);
        Mat4 skyModel = identity();
        std::copy(offset.begin(), offset.end(), skyModel.begin() + 12);
        scene.setTransform(*skyInstance, skyModel);
        if (scannerLook.overrideSky) scene.setMaterialOverride(scannerLook.world);
        scene.drawSky(mvp);
    }
    if (scannerLook.active) scene.setMaterialOverride(scannerLook.drawList);
    scene.draw(mvp);
    scene.setMaterialOverride(std::nullopt);
    std::vector<StudSprite> sprites;
    for (const runtime::Stud& stud : studs.studs())
        if (stud.state == runtime::StudState::Idle || stud.state == runtime::StudState::Collecting)
            sprites.push_back({{stud.centre[0], stud.centre[1], stud.centre[2]}, runtime::kStudTint[std::size_t(stud.kind)]});
    studDrops.appendSprites(sprites);
    studSprites->draw(viewMatrix, mvp, sprites, runtime::studSpriteFrame(std::uint32_t(provisionalStudClockSeconds * double(runtime::kClockTicksPerSecond))),
                     runtime::kStudHalfSize);
    drawUseMarkers(mvp);
    drawObjectiveArrows(mvp, viewToWorld);
    particles.draw(viewMatrix, mvp);
    drawZiplineRopes(viewMatrix, mvp);
    if (runtime::glowRuns(scannerLook.glowStrength, false)) glowPass->apply(screens.top, runtime::glowComposite(scannerLook.glowStrength, scannerLook.glowSecond));
    setScissorTest(false);
    if (hud && !cutscenePlaying()) {
        hudCanvas.clearTransparent();
        if (!drawScannerOverlay(hudCanvas)) {
            hud->draw(hudCanvas, provisionalStudClockSeconds);
            drawHeadIcon(hudCanvas);
        }
        fe::MinimapView mapView;
        mapView.x = player->motion.x;
        mapView.z = player->motion.z;
        mapView.cameraYaw = follow->yaw();
        mapView.playerYaw = runtime::headingRadians(player->motion.facing);
        mapView.markers = activeMapMarkers();
        mapView.scanner = scanner.viewShown();
        mapView.markerField = &mapMarkerField;
        addObjectiveMapIcons(mapView);
        fe::drawMinimap(mapCanvas, hudResources, levelName, mapView);
    } else {
        hudCanvas.clearTransparent();
        mapCanvas.clear(0, 0, 0);
    }
    const runtime::ScriptTextState& scriptText = levelScripts.text();
    if (scriptText.dialogueBackground()) fe::drawDialogueBackground(mapCanvas, hudResources);
    if (scriptText.heading().active)
        fe::drawHeadingBox(mapCanvas, hudResources, hudResources.textForKey(scriptText.heading().hintText ? kHintTextFile : kGameTextFile, scriptText.heading().key), scriptText.heading().alpha);
    if (scriptText.tip().active)
        fe::drawTipPage(mapCanvas, hudResources, hudResources.textForKey(kHintTextFile, scriptText.tip().key), scriptText.tip().alpha);
    if (scriptText.dialogue().active)
        fe::drawDialogueBox(hudCanvas, hudResources, scriptText.dialogue().character,
                            scriptText.dialogue().characterIntro
                                ? hudResources.textForKey(kGameTextFile, fe::characterNameKey(hudResources, scriptText.dialogue().character))
                                : hudResources.textForKey(kHintTextFile, scriptText.dialogue().key),
                            scriptText.dialogue().alpha);
    if (levelScripts.dialogOpen()) {
        const runtime::ScriptDialog dialog = levelScripts.dialog();
        if (dialog.cancelKey == runtime::kInformationBoxCancelKey)
            fe::drawInformationBox(hudCanvas, hudResources, hudResources.textForKey(kGameTextFile, dialog.bodyKey),
                                   hudResources.textForKey(kSystemTextFile, dialog.confirmKey));
        else
            fe::drawWarningDialog(hudCanvas, hudResources, hudResources.textForKey(kGameTextFile, dialog.bodyKey),
                                  hudResources.textForKey(kSystemTextFile, dialog.confirmKey), hudResources.textForKey(kSystemTextFile, dialog.cancelKey));
    }
    if (sceneChangeFlow.pageOpen()) {
        fe::drawLoadingTop(hudCanvas, hudResources);
        fe::drawSceneChangeBottom(mapCanvas, hudResources, hudResources.textForKey(kSystemTextFile, kContinueTextKey));
    }
    game_screen_fade::draw(levelScripts.screenFade(), hudCanvas, mapCanvas);
    if (movies->playing()) movies->draw(hudCanvas, mapCanvas);
    overlay->draw(game_hud::pixelsOf(hudCanvas), game_hud::pixelsOf(mapCanvas), screens.top, screens.bottom, width, height);
}

void LevelSession::reportScripted() const {
    const float moveX = player->motion.x - player->startX, moveZ = player->motion.z - player->startZ;
    std::cout << "scripted: " << updates << " updates, player moved " << std::hypot(moveX, moveZ)
              << " units, facing " << player->motion.facing << ", run timer " << player->motion.runTimer << ", height " << player->vertical.y << '\n';
    std::cout << "StoryProgress " << levelScripts.variable("StoryProgress") << '\n';
    std::cout << "screen brightness " << levelScripts.fadeLevel() << '\n';
    std::cout << "player ended at " << player->motion.x << ' ' << player->vertical.y << ' ' << player->motion.z << '\n';
    std::cout << "studs: counter " << studs.counter() << ", " << studs.count(runtime::StudState::Collecting) << " collecting, "
              << studs.count(runtime::StudState::Done) << " collected, " << studs.count(runtime::StudState::Idle) << " left\n";
    std::cout << superBricks.report() << '\n';
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
    glowPass.reset();
    particles.releaseGl();
    useMarkerRenderer.reset();
    ropeRenderer.reset();
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
