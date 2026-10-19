#pragma once
#include <array>
#include <optional>
#include "assets/assets.hpp"
#include "assets/object_collision.hpp"
#include "frontend/canvas.hpp"
#include "frontend/hud.hpp"
#include "frontend/minimap.hpp"
#include "frontend/resources.hpp"
#include "game_audio.hpp"
#include "game_characters.hpp"
#include "game_cutscene.hpp"
#include "game_detective_glass.hpp"
#include "game_doors.hpp"
#include "game_effects.hpp"
#include "game_prop_parts.hpp"
#include "game_hud.hpp"
#include "game_options.hpp"
#include "game_particles.hpp"
#include "render/rope.hpp"
#include "runtime/zipline_rope.hpp"
#include "game_player.hpp"
#include "render/hud_overlay.hpp"
#include "render/scene.hpp"
#include "render/studs.hpp"
#include "runtime/animation_driver_chain.hpp"
#include "runtime/buildit_set.hpp"
#include "runtime/buildit_player.hpp"
#include "runtime/vehicle_break.hpp"
#include "runtime/vehicle_damage.hpp"
#include "game_carry.hpp"
#include "runtime/carry.hpp"
#include "runtime/scanner.hpp"
#include "runtime/scanner_reticle.hpp"
#include "runtime/zipline.hpp"
#include "runtime/brick_grab.hpp"
#include "runtime/pole.hpp"
#include "runtime/acrobat_pole.hpp"
#include "runtime/prop_bouncer.hpp"
#include "runtime/prop_wobbly_destructible.hpp"
#include "runtime/player_attack.hpp"
#include "runtime/camera_lock_icon.hpp"
#include "runtime/camera_rig.hpp"
#include "runtime/camera_near_plane.hpp"
#include "runtime/clip_planes.hpp"
#include "runtime/collision.hpp"
#include "runtime/debug_metrics.hpp"
#include "runtime/detective.hpp"
#include "runtime/use_marker.hpp"
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
#include <string>
#include <utility>
#include <vector>

namespace game_level {

inline constexpr std::size_t kSlotLeftShoulder = 12, kSlotRightShoulder = 13, kSlotStickX = 14, kSlotStickY = 15;
inline constexpr std::size_t kSlotConfirmButton = 4, kSlotCancelButton = 5;
inline constexpr std::size_t kSlotButtonX = 8, kSlotButtonY = 9, kSlotJumpButton = 7;

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
    unsigned updateCount() const { return updates; }
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
    void applyPlacement(const chase::runtime::ScriptPlacement& request);
    void updateDoors(const float playerPosition[3], const std::function<void(const chase::runtime::SoundCall&)>& emit);
    std::vector<chase::runtime::SoundCall> updateBuildits(const float playerPosition[3]);
    std::vector<chase::frontend::MinimapMarker> activeMapMarkers() const;
    void updateCarry(const float playerPosition[3]);
    void loadDetective(chase::assets::FuseArchive& archive);
    void updateUseSpots(bool actionFree);
    void updateUseMarkers(float dt);
    void drawUseMarkers(const chase::render::Mat4& viewProjection);
    void updateDetective(float dt, bool actionFree, const std::function<void(const chase::runtime::SoundCall&)>& emit);
    int detectiveClip(int role) const;
    void drawFootprints();
    void loadZiplineRopes(chase::assets::FuseArchive& archive);
    void drawZiplineRopes(const chase::render::Mat4& view, const chase::render::Mat4& viewProjection);
    void logDetectiveStates();
    void updateScanner();
    game_player::PlayerAnimation movePlayer(const chase::runtime::StickIntent& intent, bool jumpPressed, float dt);
    game_player::PlayerAnimation moveClimbingPlayer(const chase::runtime::StickIntent& intent, bool jumpPressed, bool usePressed, float dt);
    game_player::PlayerAnimation climbBarMovePlayer(const chase::runtime::StickIntent& intent, bool jumpPressed, bool usePressed, float dt);
    bool acrobatPoleHandOver();
    game_player::PlayerAnimation acrobatPoleMovePlayer(const chase::runtime::StickIntent& intent, float dt);
    game_player::PlayerAnimation movePlayerWithPole(const chase::runtime::StickIntent& intent, bool jumpPressed, float dt);
    void loadAttack(chase::assets::FuseArchive& archive, const std::vector<chase::assets::ObjectCollision>& placed);
    game_player::PlayerAnimation attackMovePlayer(const chase::runtime::StickIntent& intent, bool jumpPressed, bool usePressed, bool attackHeld,
                                                  bool attackAllowed, float dt);
    bool scannerView(float eye[3], float& yaw, float& pitch, float& fieldOfView) const;
    bool drawScannerOverlay(chase::frontend::Canvas& top);
    void updateLevelCharacters(float dt);
    void updateFollowCamera(float dt);
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
    void vehicleImpact(std::size_t vehicleIndex, float speed, const chase::runtime::ContactOther& other, const chase::runtime::Vector3& point);
    void vehicleBrickBurst(std::size_t vehicleIndex, const chase::runtime::Vector3& point);
    void vehicleBreakBurst(std::size_t vehicleIndex);
    void vehicleDamageEffects(std::size_t vehicleIndex, const chase::runtime::DamageOutcome& outcome);
    void stepVehicleFlashes();
    void propDestroyedEffects(const chase::runtime::SmashedProp& prop);
    void propDestroyMessage(const std::string& lowerInstance);
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
    void cancelBoardingByPlacement();
    game_player::PlayerAnimation applyBoarding(const chase::runtime::BoardingStep& step);
    bool holdForMovie(float dt);
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
    game_particles::LevelParticles particles;
    game_effects::LevelEffects effects;
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
    std::vector<std::size_t> wheelNumbers;
    std::vector<std::pair<chase::runtime::Vector3, float>> wheelBounds;
    chase::runtime::GameRandom breakRandomStandIn{game_characters::kStandInSeed};
    bool cameraSnapRequest = false;
    std::vector<chase::assets::Placement> placedObjects;
    std::vector<int> placedRooms;
    std::vector<std::size_t> waypointSceneIndex;
    std::vector<chase::assets::Placement> waypointObjects;
    chase::runtime::ScriptRunner levelScripts;
    std::optional<chase::runtime::ScriptThreadLog> threadLog;
    chase::save::SaveRequestWriter saveWriter;
    chase::runtime::SceneStart sceneStart;
    chase::runtime::WorldLevelEntry currentWorldLevel;
    chase::runtime::SceneChangeFlow sceneChangeFlow;
    unsigned updatesForLog = 0;
    std::size_t reportedUnhandled = 0, reportedEvents = 0;
    chase::runtime::CollisionWorld collision;
    std::vector<chase::runtime::BulletBox> vehicleBoxes;
    std::vector<game_doors::DoorObject> doors;
    chase::runtime::BuilditSet buildits;
    chase::runtime::BuilditPlayer builditPlayer;
    game_prop_parts::PropParts propParts;
    chase::runtime::CarrySet carry;
    game_carry::HeldObjects heldObjects;
    chase::runtime::DetectiveSet detective;
    chase::runtime::UseSpotSet useSpots;
    chase::runtime::UseMarkers useMarkers;
    std::optional<chase::render::ParticleRenderer> useMarkerRenderer;
    std::optional<chase::runtime::DetectiveRoles> detectiveRoles;
    std::vector<std::size_t> footprintInstances;
    std::array<chase::assets::Matrix, 2> footprintNodeMatrices;
    game_detective_glass::Glass detectiveGlass;
    std::vector<int> loggedDetectiveStates;
    chase::runtime::Scanner scanner;
    unsigned scannerCallsSeen = 0;
    chase::runtime::ScannerReticle scannerReticle;
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
    std::optional<chase::runtime::VehicleBoarding> boarding;
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
    chase::frontend::Resources hudResources;
    chase::frontend::MapMarkerField mapMarkerField;
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
    std::optional<game_characters::LevelCharacters> levelCharacters;
    chase::runtime::PropAnimations propAnimations;
    chase::runtime::AnimationDriverChain animationDrivers;
    std::optional<game_cutscene::InEngine> cutsceneHost;
    std::optional<game_cutscene::Movies> movies;
    bool movieStarted = false, introMovieSkip = false;
    std::string loggedCutscene;
};

void advanceToDestination(chase::assets::FuseArchive& archive, game_options::Options& options, const chase::runtime::SceneChangeDestination& destination,
                          unsigned updates, unsigned& scriptOffset, bool& afterSwitch);

}
