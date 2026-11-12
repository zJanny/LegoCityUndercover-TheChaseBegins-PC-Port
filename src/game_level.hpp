#pragma once
#include <array>
#include <map>
#include <optional>
#include "assets/assets.hpp"
#include "assets/object_collision.hpp"
#include "frontend/canvas.hpp"
#include "frontend/hud.hpp"
#include "frontend/minimap.hpp"
#include "frontend/award_viewer.hpp"
#include "frontend/pause_menu.hpp"
#include "qol/pause_entry.hpp"
#include "qol/pc_mouse_camera_sdl.hpp"
#include "frontend/resources.hpp"
#include "game_audio.hpp"
#include "game_characters.hpp"
#include "game_pedestrians.hpp"
#include "game_traffic.hpp"
#include "runtime/traffic_driver_states.hpp"
#include "game_traffic_lights.hpp"
#include "game_cutscene.hpp"
#include "game_hand_items.hpp"
#include "game_doors.hpp"
#include "game_effects.hpp"
#include "game_shadows.hpp"
#include "game_stud_drops.hpp"
#include "game_super_bricks.hpp"
#include "game_fall_apart.hpp"
#include "game_object_visibility.hpp"
#include "game_prop_parts.hpp"
#include "game_super_buildit.hpp"
#include "game_fast_travel.hpp"
#include "game_hud.hpp"
#include "game_options.hpp"
#include "game_save.hpp"
#include "game_semi_trans.hpp"
#include "game_particles.hpp"
#include "render/rope.hpp"
#include "runtime/platform_riding.hpp"
#include "runtime/same_level_entry.hpp"
#include "runtime/zipline_pole.hpp"
#include "runtime/zipline_rope.hpp"
#include "game_player.hpp"
#include "render/hud_overlay.hpp"
#include "render/scene.hpp"
#include "render/view_region.hpp"
#include "render/glow.hpp"
#include "render/studs.hpp"
#include "runtime/animation_driver_chain.hpp"
#include "runtime/player_script_animation.hpp"
#include "runtime/buildit_set.hpp"
#include "runtime/buildit_player.hpp"
#include "runtime/super_buildit.hpp"
#include "runtime/vehicle_break.hpp"
#include "runtime/vehicle_damage.hpp"
#include "game_carry.hpp"
#include "runtime/carry.hpp"
#include "runtime/scanner.hpp"
#include "runtime/switch_set.hpp"
#include "runtime/ai_spawner.hpp"
#include "runtime/goon_conduct.hpp"
#include "runtime/fall_arrest_bounds.hpp"
#include "runtime/particle_object.hpp"
#include "runtime/scanner_info.hpp"
#include "runtime/scanner_bottom.hpp"
#include "runtime/scanner_markers.hpp"
#include "runtime/scanner_reticle.hpp"
#include "runtime/scanner_render.hpp"
#include "runtime/zipline.hpp"
#include "runtime/brick_grab.hpp"
#include "runtime/pole.hpp"
#include "runtime/pushable.hpp"
#include "runtime/ai_move_watch.hpp"
#include "runtime/ai_super_jump_vertical.hpp"
#include "runtime/ladder.hpp"
#include "runtime/fence_vault.hpp"
#include "runtime/acrobat_pole.hpp"
#include "runtime/prop_bouncer.hpp"
#include "runtime/prop_wobbly_destructible.hpp"
#include "runtime/player_attack.hpp"
#include "runtime/arrest_after_crash.hpp"
#include "runtime/character_arrest.hpp"
#include "runtime/character_takedown.hpp"
#include "runtime/fall_arrest_engaged.hpp"
#include "runtime/camera_lock_icon.hpp"
#include "runtime/screen_shake.hpp"
#include "runtime/camera_rig.hpp"
#include "runtime/camera_near_plane.hpp"
#include "runtime/clip_planes.hpp"
#include "runtime/collision.hpp"
#include "runtime/node_attach.hpp"
#include "runtime/attach_children.hpp"
#include "runtime/bone_attach.hpp"
#include "runtime/attached_drawn_log.hpp"
#include "runtime/debug_metrics.hpp"
#include "runtime/detective.hpp"
#include "runtime/ability_table.hpp"
#include "runtime/use_marker.hpp"
#include "runtime/cat_rescue.hpp"
#include "runtime/use_spot.hpp"
#include "runtime/level_start.hpp"
#include "runtime/pad.hpp"
#include "runtime/prop_animation.hpp"
#include "save/save_writer.hpp"
#include "runtime/object_rooms.hpp"
#include "runtime/room_streamer.hpp"
#include "runtime/head_icon.hpp"
#include "runtime/vehicle_prop_smash.hpp"
#include "runtime/vehicle_hud.hpp"
#include "runtime/scene_change.hpp"
#include "runtime/scene_start.hpp"
#include "runtime/frame_pacer.hpp"
#include "runtime/scheduler.hpp"
#include "runtime/vehicle_camera.hpp"
#include "runtime/script_runner.hpp"
#include "runtime/script_thread_log.hpp"
#include "runtime/studs.hpp"
#include "runtime/character_vehicle_entry.hpp"
#include "runtime/vehicle_boarding.hpp"
#include "runtime/vehicle_wheels.hpp"
#include "runtime/vehicle_world.hpp"
#include "runtime/vehicle_drive.hpp"
#include <SDL.h>
#include <map>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace game_level {

inline constexpr std::size_t kSlotLeftShoulder = 12, kSlotRightShoulder = 13, kSlotStickX = 14, kSlotStickY = 15;
inline constexpr std::size_t kSlotConfirmButton = 4, kSlotCancelButton = 5;
inline constexpr std::size_t kSlotButtonX = 8, kSlotButtonY = 9, kSlotJumpButton = 7;
inline constexpr std::size_t kSlotButtonA = 6;  // A (0x376a00); the jump button B is slot 7

std::uint32_t clockTicks(std::uint64_t counter, std::uint64_t frequency);

// Development free camera for levels without a player start (not game logic). With a player the recovered
// FollowCamera is used.
struct PlaceholderCamera {
    float eye[3] = {0, 0, 0};
    float yaw = 35.0f * float(M_PI) / 180, pitch = -25.0f * float(M_PI) / 180;
    float speed = 1.0f;

    void frameScene(const float centre[3], float radius);
    void update(float dt, const chase::runtime::PadDevice& pad);
};

struct Shared {
    chase::assets::FuseArchive& archive;
    const game_options::Options& options;
    std::optional<GameAudio>& audio;
    const chase::assets::Level& master;
    const std::vector<chase::runtime::SceneChangeObject>& sceneChanges;
    unsigned scriptOffset;
    bool afterSwitch;
    game_save::Progress& progress;
    qol::StoresPorts qolStores;
};

// One level of a run: its scene, the player, the level scripts, the doors, the studs, the camera and the HUD.
class LevelSession {
public:
    explicit LevelSession(const Shared& shared);
    LevelSession(const LevelSession&) = delete;
    LevelSession& operator=(const LevelSession&) = delete;

    void advance(const chase::runtime::PadState& liveInput);
    void draw(int width, int height);
    bool scriptDone() const { return !script.empty() && updates + shared.scriptOffset >= scriptLength; }
    bool scripted() const { return !script.empty(); }
    void reportScripted() const;
    void reportRate();
    chase::runtime::DebugGameValues debugValues() const;
    void releaseOverlays();
    // The live slot and the counters go back to the run's progress before the next level session (scene change).
    void handOverProgress();
    unsigned updateCount() const { return updates; }
    bool pauseSettingsConsume(const SDL_Event& event, SDL_Window* window);
    bool playerSeated() const;
    bool screenOpen() const;
    chase::frontend::PausePage pausePage() const;
    void mouseCameraEvent(const SDL_Event& event, SDL_Window* window);
    void mouseCameraFrame(SDL_Window* window);
    bool frameDue() const { return !script.empty() || pacer.due(now()); }
    std::optional<chase::runtime::SceneChangeDestination> switchTo;

private:
    void simulate(float dt, const chase::runtime::PadState& padInput);
    chase::runtime::PadState scriptedPad(unsigned index) const;
    void applyScriptTeleports(unsigned index);
    void logCutsceneChange();
    void placePlayer(const float position[3], std::optional<float> facingDegrees);
    void reportUnhandled();
    void requestSceneChange(const chase::runtime::SceneChangeObject* object);
    void requestSameLevelEntry(const chase::runtime::SameLevelEntry* entry);
    void applySameLevelEntry(const chase::runtime::SameLevelEntry& entry);
    void applyPlacement(const chase::runtime::ScriptPlacement& request);
    void updateDoors(const float playerPosition[3], const std::function<void(const chase::runtime::SoundCall&)>& emit);
    std::vector<chase::runtime::SoundCall> updateBuildits(const float playerPosition[3]);
    std::vector<chase::frontend::MinimapMarker> activeMapMarkers() const;
    bool waypointOnMap(const std::string& instance) const;
    void updateCarry(const float playerPosition[3]);
    void updateCarryArrows(const std::vector<chase::runtime::CarryAction>& actions, bool hintActive, const float playerPosition[3]);
    void loadDetective(chase::assets::FuseArchive& archive);
    void updateUseSpots(bool actionFree);
    void loadSwitches(chase::assets::FuseArchive& archive);
    void useSwitches(bool actionFree);
    void deliverSwitchMessage(const chase::runtime::ScriptObjectMessage& sent);
    void updateSwitches(float dt, const std::function<void(const chase::runtime::SoundCall&)>& emit);
    void loadSwitchUse(chase::assets::FuseArchive& archive);
    void syncSwitchZones();
    void stepSwitchUse(unsigned stateBefore);
    void loadUseMarkers(chase::assets::FuseArchive& archive);
    void updateUseMarkers(float dt);
    void drawUseMarkers(const chase::render::Mat4& viewProjection);
    void updateDetective(float dt, bool actionFree, const std::function<void(const chase::runtime::SoundCall&)>& emit);
    int detectiveClip(int role) const;
    void drawFootprints();
    void loadZiplineRopes(chase::assets::FuseArchive& archive);
    void placeZiplinePoles();
    void placeNestedProps(chase::assets::FuseArchive& archive);
    void showNestedProps(const std::vector<bool>& drawn);
    void drawZiplineRopes(const chase::render::Mat4& view, const chase::render::Mat4& viewProjection);
    void logDetectiveStates();
    void updateScanner(const chase::runtime::PadState& padInput);
    game_player::PlayerAnimation movePlayer(const chase::runtime::StickIntent& intent, bool jumpPressed, float dt);
    game_player::PlayerAnimation moveClimbingPlayer(const chase::runtime::StickIntent& intent, bool jumpPressed, bool usePressed, float dt);
    game_player::PlayerAnimation climbBarMovePlayer(const chase::runtime::StickIntent& intent, bool jumpPressed, bool usePressed, float dt);
    bool acrobatPoleHandOver();
    game_player::PlayerAnimation acrobatPoleMovePlayer(const chase::runtime::StickIntent& intent, float dt);
    game_player::PlayerAnimation movePlayerWithPole(const chase::runtime::StickIntent& intent, bool jumpPressed, float dt);
    game_player::PlayerAnimation movePlayerWithLadder(const chase::runtime::StickIntent& intent, bool jumpPressed, float dt);
    game_player::PlayerAnimation movePlayerWithPush(const chase::runtime::StickIntent& intent, bool jumpPressed, float dt);
    game_player::PlayerAnimation movePlayerWithVault(const chase::runtime::StickIntent& intent, bool jumpPressed, float dt);
    game_player::PlayerAnimation fenceVaultMovePlayer(float dt);
    void loadPushables(chase::assets::FuseArchive& archive);
    void updatePushables(const std::function<void(const chase::runtime::SoundCall&)>& emit);
    void logDrawnPositions();
    void landReleasedFlights(float dt);
    void loadAttack(chase::assets::FuseArchive& archive, const std::vector<chase::assets::ObjectCollision>& placed);
    void loadAttackCharacters(chase::assets::FuseArchive& archive);
    void installAttackFacts();
    void appendAttackCharacters(std::vector<chase::runtime::AttackCandidate>& candidates);
    void deliverAttackCharacterMessages(const chase::runtime::AttackStep& step, const std::vector<chase::runtime::AttackCandidate>& candidates);
    game_player::PlayerAnimation attackMovePlayer(const chase::runtime::StickIntent& intent, bool jumpPressed, bool usePressed, bool attackHeld,
                                                  bool attackAllowed, float dt);
    bool scannerView(float eye[3], float& yaw, float& pitch, float& fieldOfView) const;
    bool drawScannerOverlay(chase::frontend::Canvas& top);
    void updateLevelCharacters(float dt);
    void updatePedestrianSystem();
    void updatePedestrianCharacters(float dt);
    void enterPedestrianScene();
    game_pedestrian_blocking::Blocking pedestrianBlocking() const;
    void movePedestrianRooms();
    void loadTraffic();
    void enterTrafficScene();
    void updateTrafficSystem();
    void loadTrafficBodies();
    const chase::runtime::FloorMesh* trafficBodyFloor();
    void stepTrafficBodies(int worldSteps);
    void updateTrafficJunctions(float dt);
    void driveTrafficCars(float dt);
    void applyTrafficFades();
    void setupTrafficCarLights();
    void applyTrafficCarLights();
    void cullTrafficCars(const chase::render::Mat4& view, float fieldOfView, float aspect, float nearZ, float farZ);
    game_traffic::SystemInputs trafficInputs();
    bool placeTrafficCar(const chase::runtime::TrafficCarPlacement& placement);
    void updatePropAnimations(float dt);
    void updatePropRocking();
    // The placed props and lever zones the moving platform nodes carry, right after the scripts so the characters meet the moved collision
    // (game_platform_nodes.cpp); attached level characters follow their
    // parent's node in game_attached_characters.cpp.
    void carryPlatformProps();
    void attachPlacedChildren();
    void followPlacedChildren();
    // The player's ground entity (game_platform_riding.cpp): its move goes into the next movement, and the movement's ground contact sets it.
    void rideGroundEntity();
    void linkGroundEntity();
    bool objectNodePosition(const std::string& instance, float out[3]) const;
    void attachLevelCharacters();
    void followAttachedCharacters();
    void followBoneAttached();
    void snapAiMovedCharacters(float dt);
    std::optional<chase::assets::Matrix> placedNodeWorld(const std::string& lowerInstance);
    std::optional<std::array<float, 3>> objectNodeTranslation(const std::string& instance) const;
    void updateSuperBricks(float dt);
    void applySuperBricks();
    std::vector<chase::runtime::SoundCall> updateDebrisPickup(float dt, const std::array<float, 3>& pickupPoint);
    void updateFollowCamera(float dt);
    void applyScreenShake(float dt);
    void loadBoardingVehicles(const std::map<std::string, std::pair<std::size_t, chase::render::Mat4>>& placedInstances);
    void driveVehicles(float dt);
    void smashProps(std::size_t vehicleIndex);
    void loadDestructibleProps(chase::assets::FuseArchive& archive);
    bool deliverPropHit(const std::string& lowerInstance, const chase::runtime::WobbleHitMessage& message);
    void deliverAttackHits();
    void bounceOnLanding(chase::runtime::Posture before, float fallSpeedBefore);
    void playPropSound(std::uint16_t id, const std::array<float, 3>& position);
    void poseVehicleWheels(std::size_t vehicle);
    void syncVehicleHud();
    void updateHeadIcon(float dt);
    void drawHeadIcon(chase::frontend::Canvas& top);
    void loadObjectiveBoxes(const std::vector<chase::assets::ObjectCollision>& placed);
    bool objectiveTargetPosition(const std::string& instance, float out[3]) const;
    bool objectiveUseMarker(const std::string& instance, float out[3], float& alpha) const;
    void updateObjectiveMarkers(float dt);
    void addObjectiveMapIcons(chase::frontend::MinimapView& view) const;
    void drawObjectiveArrows(const chase::render::Mat4& viewProjection, const chase::render::Mat4& viewToWorld);
    void vehicleImpact(std::size_t vehicleIndex, float speed, const chase::runtime::ContactOther& other, const chase::runtime::Vector3& point);
    void chasedCarImpact(std::size_t vehicleIndex, float speed, const chase::runtime::ContactOther& other);
    std::optional<chase::runtime::BodyInputs> driveScriptCar(std::size_t vehicleIndex, float dt);
    void postStepScriptCar(std::size_t vehicleIndex, float dt);
    std::optional<chase::runtime::BodyInputs> runScriptCar(std::size_t vehicleIndex, float dt, bool postStep);
    void vehicleBrickBurst(std::size_t vehicleIndex, const chase::runtime::Vector3& point, int superBrickValue);
    std::optional<chase::runtime::DebrisSuperBlock> destroyedSuperBlock(const std::optional<chase::runtime::DebrisSuperBlock>& block, const std::array<float, 16>& node);
    std::optional<chase::runtime::DebrisSuperBlock> contactSuperBlock(std::size_t vehicleIndex, int value);
    void vehicleBreakBurst(std::size_t vehicleIndex);
    void vehicleDamageEffects(std::size_t vehicleIndex, const chase::runtime::DamageOutcome& outcome);
    void stepVehicleFlashes();
    void propDestroyedEffects(const chase::runtime::SmashedProp& prop);
    void propDestroyMessage(const std::string& lowerInstance);
    bool characterDeathMessage(const std::string& lowerInstance);
    // Message 0 to a level character and its hit states (runtime/character_hit.hpp; glue src/runtime/game_character_hit.cpp).
    struct CharacterHitEntry {
        chase::runtime::CharacterHitTarget target;
        chase::runtime::CharacterHitStates states;
    };
    std::map<std::string, CharacterHitEntry> characterHits;
    void characterHit(const std::string& lowerInstance, const chase::runtime::CharacterHitMessage& hit);
    void updateCharacterHits();
    // The takedown of a level character and the aggro (runtime/character_takedown.hpp; glue src/runtime/game_character_takedown.cpp).
    struct CharacterTakedownEntry {
        chase::runtime::TakedownCharacter character;
        chase::runtime::CharacterTakedownStates states;
        unsigned startedAt = 0;  // the update of the fight move: the oracle's goon enters MELEE PUTDOWN one update later (20261101-mission-D-1)
        chase::runtime::CharacterArrestStates arrest;
        unsigned arrestStartedAt = 0;  // the update of the A tap: the target enters ARREST in its next update, as for the fight move
    };
    std::map<std::string, CharacterTakedownEntry> characterTakedowns;
    chase::runtime::TakedownTables takedownTables;
    chase::runtime::ArrestableList arrestables;
    std::function<void(const std::string& event, const std::string& character)> characterArrestEventSink;
    void loadCharacterTakedowns(chase::assets::FuseArchive& archive);
    void characterTakedownStart(const chase::runtime::CharacterFightStart& fight);
    void updateCharacterTakedowns(float dt);
    std::optional<std::uint8_t> characterTakedownTargetMode(const std::string& lowerInstance) const;
    bool enabledObjectTrigger(const std::string& lowerEvent, const std::string& lowerInstance) const;
    // The player's arrest by A (runtime/character_arrest.hpp; glue src/runtime/game_character_arrest.cpp).
    chase::runtime::AttackTap arrestTap;
    chase::runtime::PlayerArrest playerArrest;
    std::optional<chase::runtime::AttackClip> arrestClip;
    void loadCharacterArrest(chase::assets::FuseArchive& archive);
    game_player::PlayerAnimation arrestMovePlayer(const chase::runtime::StickIntent& intent, bool jumpPressed, bool usePressed, bool attackHeld, bool arrestHeld,
                                                  bool attackAllowed, float dt);
    void updateCharacterArrests(float dt);
    // The arrest of a character that left a wrecked car (runtime/arrest_after_crash.hpp; glue src/runtime/game_arrest_after_crash.cpp).
    chase::runtime::CrashArrestTargets crashArrests;
    void characterLeftVehicle(const chase::runtime::CharacterLeftVehicle& event);
    void applyScriptVehicleExits(unsigned index);
    void characterAggro(const std::string& lowerInstance, const chase::runtime::CharacterHitSender& sender);
    // The AI spawners, kind 14 (runtime/ai_spawner.hpp; glue src/runtime/game_ai_spawner.cpp).
    chase::runtime::AiSpawnerSet aiSpawners;
    void withAiSpawnerWorld(const std::function<void(chase::runtime::AiSpawnerWorld&)>& use);
    void loadAiSpawners();
    void deliverAiSpawnerMessage(const chase::runtime::ScriptObjectMessage& sent);
    void updateAiSpawners();
    // What idle AI characters do next, their side slots and sightings (runtime/goon_conduct.hpp; glue src/runtime/game_goon_conduct.cpp).
    chase::runtime::AiConduct aiConduct;
    std::set<std::string> aiConductUnslottedSightings;
    void loadAiConduct();
    void updateAiConduct(float dt);
    // The particle objects, kind 1, and the FallArrest bounds (runtime/particle_object.hpp, runtime/fall_arrest_bounds.hpp; glue
    // src/runtime/game_particle_objects.cpp), and the BulletTime system's step on the real clock (src/game_level_natives.cpp).
    chase::runtime::ParticleObjects particleObjects;
    std::set<std::string> particleObjectsStarted;
    std::vector<std::size_t> fallArrestBoundList;
    void loadParticleObjects();
    void deliverParticleObjectMessage(const chase::runtime::ScriptObjectMessage& sent);
    void updateParticleObjects(float dt);
    // The allegiance lists and the FallArrest update (runtime/fall_arrest_engaged.hpp; glue src/runtime/game_fall_arrest.cpp).
    chase::runtime::EngagedTable engagedTable;
    void updateFallArrest(const std::function<void(const chase::runtime::SoundCall&)>& emit);
    void updateBulletTime(float realDt);
    void loadScriptDestroyProps(chase::assets::FuseArchive& archive);
    // STAND-IN VEH-S104: the line tests 0x21ab20 / 0x22b108 of a brick burst are the wheel ray of the collision world, vehicles skipped.
    chase::runtime::DebrisLineTest debrisLineStandIn() const;
    void applyVehicleDamage(std::size_t vehicleIndex, const chase::runtime::DamageMessage& message, float speed);
    void breakVehicleNode(std::size_t vehicleIndex);
    bool stepVehicleWreck(std::size_t vehicleIndex, float dt);
    void returnVehicle(std::size_t vehicleIndex);
    void recordVehicleReturnPlace(std::size_t vehicleIndex);
    chase::assets::Matrix wheelWorldMatrix(std::size_t wheelEntry) const;
    bool wreckHides(std::size_t placedIndex, std::optional<std::size_t> wheelNumber) const;
    void enterVehicleHud(const chase::runtime::VehicleRecord& record);
    void leaveVehicleHud();
    bool placeVehicle(const std::string& instance, const chase::assets::Placement& target);
    void placeVehicleAt(std::size_t index, const chase::render::Mat4& matrix);
    bool placeProp(const std::string& instance, const chase::assets::Placement& target);
    bool placeSeatedVehicle(const float position[3], std::optional<float> facingDegrees);
    std::optional<chase::runtime::WaypointVehicle> waypointVehicle() const;
    void suppressDrivenVehicleCollision();
    const chase::assets::CollisionItem* floorItem() const;
    chase::runtime::BoardingInput boardingInput(float dt);
    void handleBoardingEvents(const std::vector<chase::runtime::BoardingEvent>& events);
    void loadVehicleCameras();
    void selectCameraSubject(chase::runtime::FollowCameraInput& input);
    chase::runtime::BoardingStep stepBoarding(float dt);
    void scriptBoardingRequest(chase::runtime::BoardingRequestKind kind, const std::string& vehicle);
    void characterBoardingRequest(chase::runtime::BoardingRequestKind kind, const std::string& character, const std::string& vehicle);
    chase::runtime::CharacterAboard characterAboard(const std::string& character) const;
    void applyCharacterVehicleStep(const chase::runtime::CharacterVehicleStep& step);
    void stepCharacterVehicles(float dt);
    void cancelBoardingByPlacement();
    game_player::PlayerAnimation applyBoarding(const chase::runtime::BoardingStep& step);
    bool holdForMovie(float dt);
    void loadSuperBuildits();
    void deliverSuperBuilditMessage(const std::string& instance, std::uint8_t message);
    void updateSuperBuilditSaves();
    bool holdForSuperBuildit(float dt);
    bool holdForPause(float dt, const chase::runtime::PadState& padInput);
    bool drawPause(int width, int height);
    bool holdForAwardViewer(float dt, const chase::runtime::PadState& padInput);
    bool drawAwardViewer(int width, int height);
    void updateSuperBuildits(const float playerPosition[3], float dt);
    void loadFastTravel();
    void updateFastTravel(float dt, const chase::runtime::PadState& padInput);
    void updateMapButtons(float dt, const chase::runtime::PadState& padInput, bool playerFree);
    void drawSuperBuildit(chase::frontend::Canvas& top);
    void drawSuperBuilditPrompt(chase::frontend::Canvas& top, const chase::runtime::SuperBuilditPrompt& prompt);
    bool cutscenePlaying() const { return levelScripts.cutscenes().playing() != nullptr; }
    void startRoomStreaming();
    void updateRoomStreaming();
    void moveVehicleRooms();
    void installScriptHooks();
    void applyScriptNatives();
    void applyObjectFades(std::vector<bool>& drawn);
    void showLoadedRooms();
    bool roomShown(int room) const;
    void applyFog(const chase::runtime::ClipPlanes& planes) const;
    std::uint32_t now() const { return clockTicks(SDL_GetPerformanceCounter() - counterOrigin, counterFrequency); }

    Shared shared;
    std::unique_ptr<chase::render::Scene> sceneOwner;
    chase::render::Scene& scene;
    const chase::assets::Level level;
    chase::assets::DrawableObjects drawable;
    chase::runtime::StudField studs;
    std::vector<std::pair<unsigned, std::uint16_t>> soundLog;
    std::optional<chase::render::StudSprites> studSprites;
    std::optional<chase::render::GlowPass> glowPass;
    game_particles::LevelParticles particles;
    game_stud_drops::LevelStudDrops studDrops;
    game_super_bricks::LevelSuperBricks superBricks;
    game_effects::LevelEffects effects;
    game_shadows::LevelShadows shadows;
    game_fall_apart::LevelFallApart fallApart;
    game_semi_trans::NearCameraFades nearCameraFades;
    double provisionalStudClockSeconds = 0;
    std::optional<game_player::PlayerRig> player;
    chase::assets::ClassLookup classOf;
    chase::assets::PlacedObjectState objectState;
    std::vector<chase::assets::FogObject> fogObjects;
    std::vector<chase::assets::LightObject> lightObjects;
    std::array<std::uint8_t, 3> backgroundColour;
    std::vector<std::size_t> placedSceneIndex;
    std::optional<std::size_t> skyInstance;
    std::optional<float> seaHeight;
    std::vector<std::pair<std::size_t, std::size_t>> wheelSceneIndex;
    std::vector<std::pair<std::size_t, std::size_t>> nestedPropSceneIndex;
    std::vector<std::size_t> wheelNumbers;
    std::vector<std::pair<chase::runtime::Vector3, float>> wheelBounds;
    chase::runtime::GameRandom breakRandomStandIn{game_characters::kStandInSeed};
    chase::runtime::GameRandom scriptCarRandomStandIn{game_characters::kStandInSeed};
    std::optional<chase::runtime::AiRouteFunctions> scriptCarRoutes;
    bool cameraSnapRequest = false;
    std::vector<chase::assets::Placement> placedObjects;
    std::vector<int> placedRooms;
    std::vector<std::size_t> waypointSceneIndex;
    std::vector<chase::assets::Placement> waypointObjects;
    chase::runtime::ScriptRunner levelScripts;
    std::optional<chase::runtime::ScriptThreadLog> threadLog;
    unsigned seenSaveRequests = 0;
    unsigned saveLevelId = 0;
    chase::save::LevelStateInput saveLevelState;
    chase::runtime::SceneStart sceneStart;
    chase::runtime::WorldLevelEntry currentWorldLevel;
    chase::runtime::SceneChangeFlow sceneChangeFlow;
    std::vector<chase::runtime::SameLevelEntry> sameLevelEntries;
    chase::runtime::SameLevelEntryFlow sameLevelFlow;
    unsigned updatesForLog = 0;
    std::set<std::string> heldPropsLogged;
    std::map<std::string, bool> propRockingLogged;
    std::map<std::string, std::optional<std::size_t>> propRockingScene;
    std::set<std::string> platformCarried;
    chase::runtime::GroundEntityLink playerGround;
    chase::runtime::NodeAttachments characterAttachments;
    std::map<std::string, std::uint32_t> characterAttachCalls;
    chase::runtime::BoneAttachments boneAttachments;
    chase::runtime::AttachedChildren propAttachments;
    std::map<std::string, chase::runtime::PlatformVector> propOwnOffsets;
    std::map<std::string, std::optional<std::size_t>> placedSceneByInstance;
    std::size_t reportedUnhandled = 0, reportedEvents = 0;
    chase::runtime::CollisionWorld collision;
    std::vector<chase::runtime::BulletBox> vehicleBoxes;
    std::vector<game_doors::DoorObject> doors;
    chase::runtime::BuilditSet buildits;
    chase::runtime::PushableSet pushables;
    chase::runtime::PushGrip pushGrip;
    chase::runtime::AiMoveWatch aiMoveWatch;
    chase::runtime::AiSuperJumpVertical aiSuperJumpVertical;
    bool pushActionHeld = false;
    std::vector<std::optional<std::size_t>> pushableScene;
    std::set<std::size_t> pushableDrawnPending;
    chase::runtime::AttachedDrawnLog attachedDrawnLog;
    chase::runtime::BuilditPlayer builditPlayer;
    chase::runtime::SuperBuilditSet superBuildits;
    game_super_buildit::State superBuild;
    chase::runtime::FastTravelPads fastTravel;
    game_fast_travel::State fastTravelState;
    std::optional<chase::frontend::PauseMenu> pauseMenu;
    qol::PauseSettings pauseSettings;
    qol::MouseCameraSdl mouseCamera;
    bool pauseRequested = false;
    std::optional<chase::frontend::AwardViewer> awardViewer;
    std::size_t pauseStickDirection = chase::runtime::kNoStickDirection;
    game_prop_parts::PropParts propParts;
    game_traffic_lights::TrafficLightLooks trafficLights;
    chase::runtime::CarrySet carry;
    game_carry::HeldObjects heldObjects;
    chase::runtime::DetectiveSet detective;
    chase::runtime::UseSpotSet useSpots;
    chase::runtime::SwitchSet switches;
    std::vector<chase::runtime::SwitchEvent> pendingSwitchEvents;
    std::map<std::string, bool> switchZoneUsable;
    chase::runtime::UseMarkers useMarkers;
    chase::runtime::AbilityTable abilityTable;
    std::map<std::string, float> objectiveBoxTops;
    std::optional<chase::render::ParticleRenderer> objectiveArrowRenderer;
    float objectiveArrowClock = 0;
    std::optional<chase::render::ParticleRenderer> useMarkerRenderer;
    std::optional<chase::runtime::DetectiveRoles> detectiveRoles;
    std::vector<std::size_t> footprintInstances;
    game_hand_items::HandItems handItems;
    std::vector<int> loggedDetectiveStates;
    chase::runtime::Scanner scanner;
    unsigned scannerCallsSeen = 0;
    chase::runtime::ScannerReticle scannerReticle;
    chase::runtime::ScannerMarkers scannerMarkers;
    chase::runtime::ScannerInfo scannerInfo;
    chase::runtime::ScannerBottom scannerBottom;
    std::vector<chase::runtime::ZiplineAnchor> ziplines;
    chase::runtime::ZiplineRider ziplineRider;
    std::vector<chase::runtime::ZiplineRope> ziplineRopes;
    std::optional<chase::render::RopeRenderer> ropeRenderer;
    std::optional<chase::assets::RootMotion> ziplineDismount;
    std::vector<chase::runtime::GrabBrick> grabBricks;
    chase::runtime::BrickClimber brickClimber;
    std::map<int, chase::assets::RootMotion> brickMotions;
    chase::runtime::ClimbBarSet climbBars;
    chase::runtime::ClimbBarRider climbBarRider;
    std::optional<chase::assets::RootMotion> ledgeLeftMotion, ledgeRightMotion;
    chase::runtime::AcrobatPoleRider acrobatPoleRider;
    std::optional<chase::runtime::AcrobatPoleClips> acrobatPoleClips;
    chase::runtime::PoleSession pole;
    chase::runtime::LadderSession ladder;
    std::vector<chase::runtime::FenceVaultObject> fenceVaults;
    chase::runtime::FenceVaulter fenceVaulter;
    std::map<int, chase::assets::RootMotion> vaultMotions;
    std::optional<chase::runtime::VehicleBoarding> boarding;
    std::optional<chase::runtime::CharacterVehicleEntry> characterVehicles;
    std::vector<chase::runtime::BoardingVehicle> boardingVehicles;
    std::vector<std::size_t> boardingRecords;
    std::optional<chase::runtime::WaypointVehicle> previousWaypointVehicle;
    struct VehicleDrive {
        chase::runtime::VehicleRigidBody rigid;
        chase::runtime::DrivetrainState drivetrain;
        std::vector<chase::runtime::WheelState> wheels;
        chase::runtime::FloorTracker floorTracker;
        chase::runtime::VehicleBoxContacts boxContacts;
        chase::runtime::VehicleShapeBoxes shape;
        float massScale = 1.0f;
        bool started = false;
        bool awake = false;
        bool settled = false;
        bool moved = false;
        std::size_t placedIndex = 0;
        chase::runtime::SmashBox nodeBounds;
        chase::runtime::DrivenStuckTimer stuck;
        chase::runtime::VehicleWreck wreck;
        chase::runtime::VehicleMatrixHistory history;
        bool returnPlaceGood = false;
    };
    std::vector<VehicleDrive> vehicleDrives;
    chase::runtime::VehiclePropSmasher propSmasher;
    chase::runtime::WobblyProps wobblyProps;
    chase::runtime::BouncerSet bouncers;
    chase::runtime::BounceMemory bounceMemory;
    chase::runtime::AttackTap attackTap;
    chase::runtime::PlayerAttack attack;
    chase::runtime::AttackClips attackClips;
    std::vector<chase::runtime::AttackTargetObject> attackTargets;
    std::vector<std::pair<std::string, chase::runtime::AttackMessage>> attackMessages;  // the messages the attack sent in this update, by target instance
    struct AttackCharacter {
        std::string name;
        int room = 0;
        chase::runtime::AttackEntity entity;
        std::uint16_t fallArrestSound = 0;  // slot 20 of the sound row of the character's category
    };
    std::vector<AttackCharacter> attackCharacters;
    chase::runtime::PlayerAttackTables attackTables;
    chase::runtime::AttackEntity attackPlayer;
    chase::runtime::CharacterHitSink characterHitSink;      // message 0 of the attack to a character (until set: a log line, PLAYER-S12)
    chase::runtime::CharacterFightSink characterFightSink;  // the fight move or struggle a tap started on a character (until set: a log line)
    chase::runtime::VehiclePairManifolds vehiclePairs;
    std::vector<std::string> vehicleTableNames;
    chase::runtime::VehicleHud vehicleHud;
    chase::runtime::HeadIcon headIcon;
    float vehicleWorldTime = 0;
    std::vector<chase::runtime::VehicleCameraSubject> vehicleCameras;
    std::optional<std::size_t> cameraVehicle;
    std::vector<chase::assets::Placement> wheelLocalPlacements;
    float radius = 0;
    PlaceholderCamera camera;
    std::optional<chase::runtime::CameraRig> follow;
    chase::runtime::CameraLockIcon cameraLockIcon;
    chase::runtime::ScreenShake screenShake;
    chase::frontend::Resources hudResources;
    chase::frontend::MapMarkerField mapMarkerField;
    std::optional<chase::frontend::MapProjection> mapProjection;
    std::optional<chase::frontend::Hud> hud;
    chase::frontend::Canvas hudCanvas, mapCanvas;
    std::optional<chase::render::HudOverlay> overlay;
    std::string levelName;
    chase::runtime::PadDevice pad;
    std::uint64_t counterFrequency = 0, counterOrigin = 0;
    std::optional<chase::runtime::Scheduler> scheduler;
    chase::runtime::FramePacer pacer;
    std::vector<game_options::ScriptStep> script;
    unsigned scriptLength = 0;
    unsigned updates = 0, updatesThisSecond = 0, updatesLastSecond = 0;
    bool dialogReported = false;
    std::vector<float> scriptedHeights;
    std::optional<chase::runtime::RoomStreamer> roomStreamer;
    std::map<int, std::vector<std::size_t>> roomInstances;
    std::set<int> scriptedRooms;
    std::vector<std::size_t> pendingVehicleBreaks;
    chase::runtime::ObjectRoomMover vehicleRoomMover;
    std::vector<std::optional<chase::render::CharacterInstance>> placedCharacters;
    game_object_visibility::CharacterVisibility characterVisibility;
    std::optional<game_characters::LevelCharacters> levelCharacters;
    game_pedestrians::LevelPedestrians pedestrians;
    game_traffic::LevelTraffic traffic;
    std::vector<std::optional<std::size_t>> trafficBoarding;
    std::vector<int> trafficFadeShown;
    chase::runtime::TrafficCarBodies trafficBodies;
    chase::runtime::TrafficCarBodies::World trafficBodyWorld;
    std::optional<chase::runtime::FloorMesh> trafficFloorMesh;
    chase::runtime::TrafficCarLights trafficCarLights;
    std::vector<bool> trafficLightsDisabled;
    std::vector<std::array<std::array<float, 3>, 4>> trafficGlowLocal;
    struct TrafficCullBounds {
        std::array<float, 3> centre{};
        float radius = 0.0f;
    };
    std::vector<TrafficCullBounds> trafficCullBounds;
    chase::runtime::PropAnimations propAnimations;
    chase::runtime::PlayerScriptAnimation playerScriptAnimation;
    chase::runtime::CatRescueSet catRescue;
    chase::runtime::AnimationDriverChain animationDrivers;
    std::optional<game_cutscene::InEngine> cutsceneHost;
    std::optional<game_cutscene::Movies> movies;
    bool movieStarted = false, introMovieSkip = false;
    std::string loggedCutscene;
};

void advanceToDestination(chase::assets::FuseArchive& archive, game_options::Options& options, const chase::runtime::SceneChangeDestination& destination,
                          unsigned updates, unsigned& scriptOffset, bool& afterSwitch);

}
