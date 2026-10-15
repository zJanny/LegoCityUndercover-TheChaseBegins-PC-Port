#pragma once
#include <array>
#include "assets/assets.hpp"
#include "assets/object_collision.hpp"
#include "frontend/canvas.hpp"
#include "frontend/hud.hpp"
#include "frontend/resources.hpp"
#include "game_audio.hpp"
#include "game_cutscene.hpp"
#include "game_doors.hpp"
#include "game_hud.hpp"
#include "game_options.hpp"
#include "game_player.hpp"
#include "render/hud_overlay.hpp"
#include "render/scene.hpp"
#include "render/studs.hpp"
#include "runtime/buildit_set.hpp"
#include "runtime/carry.hpp"
#include "runtime/camera_lock_icon.hpp"
#include "runtime/camera_rig.hpp"
#include "runtime/collision.hpp"
#include "runtime/debug_metrics.hpp"
#include "runtime/level_start.hpp"
#include "runtime/pad.hpp"
#include "save/save_writer.hpp"
#include "runtime/room_streamer.hpp"
#include "runtime/scene_change.hpp"
#include "runtime/scene_start.hpp"
#include "runtime/scheduler.hpp"
#include "runtime/script_runner.hpp"
#include "runtime/studs.hpp"
#include "runtime/vehicle_boarding.hpp"
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
    std::optional<chase::runtime::SceneChangeDestination> switchTo;

private:
    void simulate(float dt, const chase::runtime::PadState& padInput);
    chase::runtime::PadState scriptedPad(unsigned index) const;
    void reportUnhandled();
    void requestSceneChange(const chase::runtime::SceneChangeObject* object);
    void applyPlacement(const chase::runtime::ScriptPlacement& request);
    void updateDoors(const float playerPosition[3], const std::function<void(const chase::runtime::SoundCall&)>& emit);
    void updateBuildits(const float playerPosition[3]);
    void updateCarry(const float playerPosition[3]);
    void updateFollowCamera(float dt);
    void loadBoardingVehicles(const std::map<std::string, std::pair<std::size_t, chase::render::Mat4>>& placedInstances);
    chase::runtime::BoardingStep stepBoarding(float dt);
    game_player::PlayerAnimation applyBoarding(const chase::runtime::BoardingStep& step);
    bool holdForMovie(float dt);
    bool cutscenePlaying() const { return levelScripts.cutscenes().playing() != nullptr; }
    void startRoomStreaming();
    void updateRoomStreaming();
    void showLoadedRooms();
    bool roomShown(int room) const;
    void applyFog() const;
    std::uint32_t now() const { return clockTicks(SDL_GetPerformanceCounter() - counterOrigin, counterFrequency); }

    Shared shared;
    std::unique_ptr<chase::render::Scene> sceneOwner;
    chase::render::Scene& scene;
    const chase::assets::Level level;
    chase::assets::DrawableObjects drawable;
    chase::runtime::StudField studs;
    std::vector<std::pair<unsigned, std::uint16_t>> soundLog;
    std::optional<chase::render::StudSprites> studSprites;
    double provisionalStudClockSeconds = 0;
    std::optional<game_player::PlayerRig> player;
    chase::assets::ClassLookup classOf;
    chase::assets::PlacedObjectState objectState;
    std::vector<chase::assets::FogObject> fogObjects;
    std::array<std::uint8_t, 3> backgroundColour;
    std::vector<std::size_t> placedSceneIndex;
    std::vector<std::pair<std::size_t, std::size_t>> wheelSceneIndex;
    std::vector<chase::assets::Placement> placedObjects;
    std::vector<std::size_t> waypointSceneIndex;
    std::vector<chase::assets::Placement> waypointObjects;
    chase::runtime::ScriptRunner levelScripts;
    chase::save::SaveRequestWriter saveWriter;
    chase::runtime::SceneStart sceneStart;
    chase::runtime::WorldLevelEntry currentWorldLevel;
    chase::runtime::SceneChangeFlow sceneChangeFlow;
    unsigned updatesForLog = 0;
    std::size_t reportedUnhandled = 0, reportedEvents = 0;
    chase::runtime::CollisionWorld collision;
    std::vector<game_doors::DoorObject> doors;
    chase::runtime::BuilditSet buildits;
    chase::runtime::CarrySet carry;
    std::optional<chase::runtime::VehicleBoarding> boarding;
    std::vector<chase::runtime::BoardingVehicle> boardingVehicles;
    std::vector<std::size_t> boardingRecords;
    float radius = 0;
    PlaceholderCamera camera;
    std::optional<chase::runtime::CameraRig> follow;
    chase::runtime::CameraLockIcon cameraLockIcon;
    chase::frontend::Resources hudResources;
    std::optional<chase::frontend::Hud> hud;
    chase::frontend::Canvas hudCanvas, mapCanvas;
    std::optional<chase::render::HudOverlay> overlay;
    std::string levelName;
    chase::runtime::PadDevice pad;
    std::uint64_t counterFrequency = 0, counterOrigin = 0;
    std::optional<chase::runtime::Scheduler> scheduler;
    std::vector<game_options::ScriptStep> script;
    unsigned scriptLength = 0;
    unsigned updates = 0, updatesThisSecond = 0, updatesLastSecond = 0;
    bool dialogReported = false;
    std::vector<float> scriptedHeights;
    std::optional<chase::runtime::RoomStreamer> roomStreamer;
    std::map<int, std::vector<std::size_t>> roomInstances;
    std::optional<game_cutscene::InEngine> cutsceneHost;
    std::optional<game_cutscene::Movies> movies;
    bool movieStarted = false, introMovieSkip = false;
};

void advanceToDestination(chase::assets::FuseArchive& archive, game_options::Options& options, const chase::runtime::SceneChangeDestination& destination,
                          unsigned updates, unsigned& scriptOffset, bool& afterSwitch);

}
