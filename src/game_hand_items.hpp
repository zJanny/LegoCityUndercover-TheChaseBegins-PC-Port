#pragma once
#include "assets/assets.hpp"
#include "game_player.hpp"
#include "render/scene.hpp"
#include "runtime/hand_item.hpp"
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace game_hand_items {

// The items the player's clips show in his hand: models of players.blvl objects (Ability_MagnifyingGlass of the detect_* clips,
// DETECT-7; Ability_Scanner of scanner_out / scanner_in, SCAN-14), hung on the bone the clip's hand item track names and shown or hidden by
// that track (docs/re/abilities-detective.md, docs/re/abilities-scanner.md).
class HandItems {
public:
    void load(chase::assets::FuseArchive& archive, chase::render::Scene& scene);
    void update(const game_player::PlayerRig& player);
    void draw(chase::render::Scene& scene, const game_player::PlayerRig& player) const;
    bool shown(const std::string& object) const;

private:
    struct Item {
        std::string object;
        std::size_t instance = std::size_t(-1);
        bool shown = false;
        std::string bone;
    };
    static std::optional<chase::render::Mat4> boneWorld(const game_player::PlayerRig& player, const std::string& bone);
    chase::runtime::HandItemTrack track_;
    std::vector<Item> items_;
    int previousAnimation_ = -1;
};

}
