#include "game_hand_items.hpp"
#include "render/character.hpp"
#include <algorithm>
#include <cctype>

namespace game_hand_items {
using namespace chase;

namespace {
constexpr const char* kPlayersLevel = "levels/players.blvl";

std::string lowerCase(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return text;
}
}

void HandItems::load(assets::FuseArchive& archive, render::Scene& scene) {
    const assets::FuseEntry* entry = archive.find(kPlayersLevel);
    if (!entry) return;
    const std::vector<assets::Placement> placements = assets::placements(assets::parseLevel(archive.read(*entry)));
    for (const char* object : {runtime::kMagnifyingGlassObject, runtime::kScannerObject}) {
        for (const assets::Placement& placement : placements) {
            if (lowerCase(placement.instance) != lowerCase(object)) continue;
            const std::size_t instance = scene.instances();
            if (!scene.addModel(assets::meshModelPath(placement.mesh))) break;
            scene.setInstanceVisible(instance, false);
            Item item;
            item.object = lowerCase(object);
            item.instance = instance;
            items_.push_back(std::move(item));
            break;
        }
    }
}

bool HandItems::shown(const std::string& object) const {
    for (const Item& item : items_)
        if (item.object == lowerCase(object)) return item.shown;
    return false;
}

void HandItems::update(const game_player::PlayerRig& player) {
    const assets::Animation& animation = player.animations.at(player.animation);
    const bool restarted = player.animation != previousAnimation_;
    previousAnimation_ = player.animation;
    const std::optional<runtime::HandItemEvent> event = track_.update(animation, float(player.frame / double(animation.frames)), restarted);
    if (!event) return;
    for (Item& item : items_) {
        if (item.object != lowerCase(event->item.object)) continue;
        item.shown = event->show;
        if (event->show) item.bone = lowerCase(event->item.bone);
    }
}

// STAND-IN DETECT-S8: the item's local matrix is the bone's whole bind matrix (0x2ca88c with matrix 0 copies 0x227788's record, rotation included),
// so with the skin matrix it sits on the animated bone with its rotation; the per-frame update that follows the attach (0x242814) was not read.
// The scanner device uses the same attach (SCAN-S10).
std::optional<render::Mat4> HandItems::boneWorld(const game_player::PlayerRig& player, const std::string& bone) {
    const std::vector<assets::Joint>& joints = player.character.skeleton.joints;
    const auto found = std::find_if(joints.begin(), joints.end(), [&](const assets::Joint& joint) { return lowerCase(joint.name) == bone; });
    if (found == joints.end()) return std::nullopt;
    const std::size_t index = std::size_t(found - joints.begin());
    if (index >= player.skin.size()) return std::nullopt;
    return assets::multiply(assets::multiply(found->world, player.skin[index]), player.matrix());
}

void HandItems::draw(render::Scene& scene, const game_player::PlayerRig& player) const {
    for (const Item& item : items_) {
        const std::optional<render::Mat4> joint = item.bone.empty() ? std::nullopt : boneWorld(player, item.bone);
        const bool visible = item.shown && joint && scene.instanceVisible(player.instance.bodyInstance);
        scene.setInstanceVisible(item.instance, visible);
        if (visible) scene.setTransform(item.instance, *joint);
    }
}

}
