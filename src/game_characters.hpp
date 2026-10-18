#pragma once
// The level characters' animation players (docs/re/npc-animation.md): the slots in runtime/animation_slots.hpp, the idle
// state in runtime/npc_idle.hpp and the script requests of runtime/natives_animation.hpp, posed here on the skinned bodies
// placeLevelCharacters made.
#include "assets/assets.hpp"
#include "render/character.hpp"
#include "render/scene.hpp"
#include "runtime/animation_slots.hpp"
#include "runtime/game_random.hpp"
#include "runtime/natives_animation.hpp"
#include "runtime/npc_idle.hpp"

#include <functional>
#include <map>
#include <optional>
#include <vector>

namespace game_characters {

namespace render = chase::render;
namespace assets = chase::assets;
namespace runtime = chase::runtime;

// Seeding of the original is time based (0x105d8c); this fixed seed is a stand-in (LEVOBJ-S4).
constexpr std::uint32_t kStandInSeed = 0x2f6b9d41u;

struct CharacterPose {
    std::string animation;
    float frame = 0;
};

class LevelCharacters : public runtime::AnimationDriver {
public:
    LevelCharacters(assets::FuseArchive& archive, const std::vector<assets::CharacterObject>& characters,
                    const std::vector<std::optional<render::CharacterInstance>>& placed);

    // One update. `active` tells whether the character's entity exists and is enabled (room loaded, not disabled).
    void update(render::Scene& scene, float dt, const std::function<bool(const assets::CharacterObject&)>& active);

    void play(const runtime::AnimationPlay& play) override;
    void stop(const std::string& lowerCaseInstance) override;
    std::optional<bool> currentNearEnd(const std::string& lowerCaseInstance) const override;

    // PositionObject 0x1d9e6c: the character's node takes the target's matrix. False when the instance is not a level character.
    // STAND-IN SCRIPT-S237: the controller part of 0x1d9e6c (heading ctrl+8/+10, fall reset when 0x21aae0 is true) is not applied; the kind
    // byte of level characters was not read and nothing in the port reads a character's heading.
    bool place(const std::string& lowerCaseInstance, const assets::Matrix& world);

    std::size_t animated() const { return entries_.size(); }
    std::size_t started() const;
    std::optional<CharacterPose> pose(const std::string& instance) const;

private:
    struct Clip {
        std::string path;
        assets::Animation animation;
        runtime::AnimationClipInfo info;
    };
    struct Entry {
        std::size_t character = 0;
        render::CharacterInstance instance;
        const render::CharacterRig* rig = nullptr;
        std::uint32_t idle = 0, idle2 = 0;
        render::Mat4 world;
        runtime::AnimationSlots slots;
        runtime::NpcIdle idleState;
        bool scriptLoaded = false;
    };
    std::optional<std::uint32_t> clipOf(const std::string& path);
    Entry* find(const std::string& lowerCaseInstance);
    const Entry* find(const std::string& lowerCaseInstance) const;

    assets::FuseArchive& archive_;
    const std::vector<assets::CharacterObject>& characters_;
    std::map<std::string, render::CharacterRig> rigs_;
    std::vector<Clip> clips_;
    std::map<std::string, std::uint32_t> clipIndex_;
    std::vector<Entry> entries_;
    std::map<std::string, std::size_t> byInstance_;
    runtime::GameRandom random_{kStandInSeed};
    std::int64_t clock_ = 0;
};

}
