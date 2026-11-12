#pragma once
// The pedestrians in chase-game (runtime/pedestrian_system.hpp): the slots' entities, the clips their characters request and the
// posed bodies of the live ones.
#include "assets/assets.hpp"
#include "assets/object_collision.hpp"
#include "game_characters.hpp"
#include "game_pedestrian_blocking.hpp"
#include "game_pedestrian_fade.hpp"
#include "render/character.hpp"
#include "render/scene.hpp"
#include "runtime/animation_slots.hpp"
#include "runtime/collision.hpp"
#include "runtime/game_random.hpp"
#include "runtime/pedestrian_rooms.hpp"
#include "runtime/pedestrian_system.hpp"
#include "runtime/room_streaming.hpp"

#include <cstddef>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace game_pedestrians {

namespace assets = chase::assets;
namespace render = chase::render;
namespace runtime = chase::runtime;

struct SystemInputs {
    std::optional<bool> enabled;  // the last EnableTrafficAndPedestrians
    runtime::PedestrianView view;
    runtime::PedestrianVec3 playerPosition{};
    std::function<bool(int room)> roomLoaded;
    const game_pedestrian_blocking::Blocking* blocking = nullptr;
};

struct CharacterInputs {
    float dt = 0;
    runtime::PedestrianVec3 playerPosition{};
    bool playerInVehicle = false;
    std::uint16_t playerState = 0;
    const runtime::CollisionWorld* collision = nullptr;
    const game_pedestrian_blocking::Blocking* blocking = nullptr;
    runtime::PedestrianVec3 fadePoint{};  // 0x52a1b4: the camera look point (pedestrianFadePoint)
};

class LevelPedestrians {
public:
    // The level load 0x2ee4c8 with the creates of the slots' characters; every slot's entity is disabled.
    void load(assets::FuseArchive& archive, const assets::Level& level, const assets::ClassLookup& classOf,
              const std::vector<assets::CharacterObject>& characters, const std::vector<std::optional<render::CharacterInstance>>& placed,
              assets::PlacedObjectState& state);
    // The scene enter 0x2edbf8 with the level's own pregen file.
    void enterScene(assets::FuseArchive& archive, const assets::Level& level, const std::string& levelName, const runtime::StreamRooms* rooms,
                    const std::map<int, runtime::RoomBox>* boxes, int currentRoom);
    // Slot +0x48 of the system list: spawns enable a slot's entity, removals disable it.
    void updateSystem(const SystemInputs& inputs, assets::PlacedObjectState& state);
    // The entity pass: each live pedestrian's update, its clips and its posed body.
    void updateCharacters(render::Scene& scene, const CharacterInputs& inputs, const assets::PlacedObjectState& state);
    // The room pass 0x2e6ce4 for the enabled pedestrians (game_pedestrian_rooms.cpp).
    std::vector<runtime::PedestrianRoomMove> moveRooms(const runtime::StreamRooms& rooms, const std::map<int, runtime::RoomBox>& boxes,
                                                       int currentRoom, int playerRoom, const assets::PlacedObjectState& state);

private:
    struct Clip {
        assets::Animation animation;
        runtime::AnimationClipInfo info;
    };
    struct Slot {
        std::string animationClass;
        render::CharacterInstance instance;
        const render::CharacterRig* rig = nullptr;
        runtime::AnimationSlots animation;
        runtime::PedestrianVec3 boundsCentre{};
    };
    std::optional<std::uint32_t> clipOf(const std::string& animationClass, runtime::PedestrianClip clip);
    bool clipFinished(std::size_t slot, runtime::PedestrianClip clip);

    assets::FuseArchive* archive_ = nullptr;
    runtime::PedestrianSystem system_;
    game_pedestrian_fade::SlotFades fades_;
    std::vector<Slot> slots_;
    std::map<std::string, render::CharacterRig> rigs_;
    std::vector<Clip> clips_;
    std::map<std::string, std::uint32_t> clipIndex_;
    std::vector<std::uint8_t> pregenFile_;
    // STAND-IN PED-S6: one generator with the fixed seed of the level characters (LEVOBJ-S4), not the game's shared one.
    runtime::GameRandom random_{game_characters::kStandInSeed};
    std::int64_t clock_ = 0;
    runtime::ObjectRoomMover roomMover_;
};

}
