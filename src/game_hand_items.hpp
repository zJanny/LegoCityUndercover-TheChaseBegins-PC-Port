#pragma once
#include "assets/assets.hpp"
#include "game_player.hpp"
#include "render/scene.hpp"
#include "runtime/hand_item.hpp"
#include "runtime/hand_item_set.hpp"
#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace game_hand_items {

// The items the player's clips show in his hand: every object that the hand item track of a played clip names (Ability_MagnifyingGlass of the
// detect_* clips, Ability_Scanner of scanner_out / scanner_in, Ability_Handcuffs of the police zipline clips, ...) and that levels/players.blvl
// lists. The model is loaded when the first event names it, hung on the bone the clip's track names and shown or hidden by that track
// (docs/re/abilities-detective.md, docs/re/abilities-scanner.md, docs/re/hand-items.md).
class HandItems {
public:
    void load(chase::assets::FuseArchive& archive);
    std::optional<chase::runtime::HandItemEvent> update(chase::render::Scene& scene, const game_player::PlayerRig& player);
    void draw(chase::render::Scene& scene, const game_player::PlayerRig& player) const;
    bool shown(const std::string& object) const;

private:
    static std::optional<chase::render::Mat4> boneWorld(const game_player::PlayerRig& player, const std::string& bone);
    void ensureInstance(chase::render::Scene& scene, const std::string& object);
    chase::runtime::HandItemTrack track_;
    chase::runtime::HandItemSet set_;
    std::map<std::string, std::string> meshes_;
    std::map<std::string, std::size_t> instances_;
    int previousAnimation_ = -1;
};

}
