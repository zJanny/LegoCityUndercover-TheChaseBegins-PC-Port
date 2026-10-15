#pragma once
// The level characters play their idle animations (docs/re/npc-animation.md): the state in runtime/npc_idle.hpp,
// posed here on the skinned bodies placeLevelCharacters made.
#include "assets/assets.hpp"
#include "render/character.hpp"
#include "render/scene.hpp"
#include "runtime/game_random.hpp"
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

class LevelCharacters {
public:
    LevelCharacters(assets::FuseArchive& archive, const std::vector<assets::CharacterObject>& characters,
                    const std::vector<std::optional<render::CharacterInstance>>& placed);

    // One update. `active` tells whether the character's entity exists and is enabled (room loaded, not disabled).
    void update(render::Scene& scene, float dt, const std::function<bool(const assets::CharacterObject&)>& active);

    std::size_t animated() const { return entries_.size(); }
    std::size_t started() const;
    std::optional<runtime::NpcIdlePose> pose(const std::string& instance) const;

private:
    struct Clips {
        assets::Animation idle, idle2;
        runtime::NpcIdleClip idleClip, idle2Clip;
    };
    struct Entry {
        std::size_t character = 0;
        render::CharacterInstance instance;
        const render::CharacterRig* rig = nullptr;
        const Clips* clips = nullptr;
        render::Mat4 world;
        runtime::NpcIdle idle;
    };
    const std::vector<assets::CharacterObject>& characters_;
    std::map<std::string, render::CharacterRig> rigs_;
    std::map<std::string, Clips> clips_;
    std::vector<Entry> entries_;
    runtime::GameRandom random_{kStandInSeed};
};

}
