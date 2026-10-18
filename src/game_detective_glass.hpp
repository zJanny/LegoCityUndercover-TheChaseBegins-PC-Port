#pragma once
#include "assets/assets.hpp"
#include "game_player.hpp"
#include "render/scene.hpp"
#include "runtime/hand_item.hpp"
#include <cstddef>
#include <optional>
#include <string>

namespace game_detective_glass {

// Chase's magnifying glass: the model of the players.blvl object Ability_MagnifyingGlass, hung on the bone the detect_* clips name and shown or
// hidden by their hand item track (docs/re/abilities-detective.md, DETECT-7). Only this one hand item is drawn (DETECT-8).
class Glass {
public:
    void load(chase::assets::FuseArchive& archive, chase::render::Scene& scene);
    void update(const game_player::PlayerRig& player);
    void draw(chase::render::Scene& scene, const game_player::PlayerRig& player) const;
    bool shown() const { return shown_; }

private:
    std::optional<chase::render::Mat4> boneWorld(const game_player::PlayerRig& player) const;
    chase::runtime::HandItemTrack track_;
    std::size_t instance_ = std::size_t(-1);
    int previousAnimation_ = -1;
    bool shown_ = false;
    std::string bone_;
};

}
