#include "game_detective_glass.hpp"
#include "render/character.hpp"
#include <algorithm>
#include <cctype>

namespace game_detective_glass {
using namespace chase;

namespace {
constexpr const char* kPlayersLevel = "levels/players.blvl";

std::string lowerCase(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return text;
}
}

void Glass::load(assets::FuseArchive& archive, render::Scene& scene) {
    const assets::FuseEntry* entry = archive.find(kPlayersLevel);
    if (!entry) return;
    for (const assets::Placement& placement : assets::placements(assets::parseLevel(archive.read(*entry)))) {
        if (lowerCase(placement.instance) != lowerCase(runtime::kMagnifyingGlassObject)) continue;
        const std::size_t instance = scene.instances();
        if (!scene.addModel(assets::meshModelPath(placement.mesh))) return;
        scene.setInstanceVisible(instance, false);
        instance_ = instance;
        return;
    }
}

void Glass::update(const game_player::PlayerRig& player) {
    const assets::Animation& animation = player.animations.at(player.animation);
    const bool restarted = player.animation != previousAnimation_;
    previousAnimation_ = player.animation;
    const std::optional<runtime::HandItemEvent> event = track_.update(animation, float(player.frame / double(animation.frames)), restarted);
    if (!event || lowerCase(event->item.object) != lowerCase(runtime::kMagnifyingGlassObject)) return;
    shown_ = event->show;
    if (event->show) bone_ = lowerCase(event->item.bone);
}

// STAND-IN DETECT-S8: the item's local matrix is the bone's whole bind matrix (0x2ca88c with matrix 0 copies 0x227788's record, rotation included),
// so with the skin matrix it sits on the animated bone with its rotation; the per-frame update that follows the attach (0x242814) was not read.
std::optional<render::Mat4> Glass::boneWorld(const game_player::PlayerRig& player) const {
    const std::vector<assets::Joint>& joints = player.character.skeleton.joints;
    const auto found = std::find_if(joints.begin(), joints.end(), [&](const assets::Joint& joint) { return lowerCase(joint.name) == bone_; });
    if (found == joints.end()) return std::nullopt;
    const std::size_t index = std::size_t(found - joints.begin());
    if (index >= player.skin.size()) return std::nullopt;
    return assets::multiply(assets::multiply(found->world, player.skin[index]), player.matrix());
}

void Glass::draw(render::Scene& scene, const game_player::PlayerRig& player) const {
    if (instance_ == std::size_t(-1)) return;
    const std::optional<render::Mat4> joint = bone_.empty() ? std::nullopt : boneWorld(player);
    const bool visible = shown_ && joint && scene.instanceVisible(player.instance.bodyInstance);
    scene.setInstanceVisible(instance_, visible);
    if (visible) scene.setTransform(instance_, *joint);
}

}
