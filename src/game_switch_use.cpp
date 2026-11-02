#include "game_level.hpp"
#include "runtime/lever_use.hpp"
#include "runtime/prop_animation.hpp"

#include <iostream>

namespace game_level {
namespace runtime = chase::runtime;
namespace assets = chase::assets;

void LevelSession::loadSwitchUse(assets::FuseArchive& archive) {
    std::map<std::string, std::string> meshes;
    for (const assets::Placement& placed : placedObjects) meshes[placed.instance] = placed.mesh;
    for (std::size_t i = 0; i < switches.objects().size(); ++i) {
        const runtime::SwitchObject& sw = switches.objects()[i];
        const std::string& instance = level.objects[sw.objectIndex].instance;
        const auto mesh = meshes.find(instance);
        const std::string meshName = mesh == meshes.end() ? std::string() : mesh->second;
        if (sw.params.hasUseAnim && !meshName.empty())
            if (const assets::FuseEntry* entry = archive.find(runtime::PropAnimations::animationPath(meshName, "use"))) {
                const assets::Animation use = assets::parseAnimation(archive.read(*entry));
                switches.setUseAnimation(i, {0, assets::animationLength(use), std::uint16_t(assets::animationFramesPerSecond(use)), assets::animationLoops(use)});
            }
        if (!sw.lever || !player || !player->carryAnimations) continue;
        const int clip = sw.lever->use.clipId;
        const std::string path = std::string(game_player::kCarryAnimationDirectory) + player->carryAnimations->name(clip) + game_player::kPlayerAnimationSuffix;
        const assets::FuseEntry* clipEntry = clip > 0 ? archive.find(path) : nullptr;
        if (clipEntry && !player->animations.count(clip)) player->animations[clip] = assets::parseAnimation(archive.read(*clipEntry));
        std::optional<runtime::UseLerpTarget> target;
        if (const assets::FuseEntry* model = meshName.empty() ? nullptr : archive.find(assets::meshModelPath(meshName)))
            target = runtime::leverUseTarget(assets::parseModel(archive.read(*model)), sw.matrix);
        switches.setLeverUse(i, player->animations.count(clip) ? runtime::leverClipTiming(player->animations.at(clip)) : runtime::LeverClipTiming{}, target);
    }
}

void LevelSession::syncSwitchZones() {
    for (std::size_t i = 0; i < switches.objects().size(); ++i) {
        const runtime::SwitchObject& sw = switches.objects()[i];
        if (!sw.lever) continue;
        const runtime::ObjectNativeFlags* flags = levelScripts.objectNatives().flags(sw.instance);
        if (!flags || !flags->usable) continue;
        const auto applied = switchZoneUsable.find(sw.instance);
        if (applied != switchZoneUsable.end() && applied->second == *flags->usable) continue;
        switchZoneUsable[sw.instance] = *flags->usable;
        switches.setZoneUsable(i, *flags->usable);
    }
}

// STAND-IN SCRIPT-S298: x, z and facing are written from the lerp of 0x27fecc without collision (0x215804) and the height is left to the fall code.
void LevelSession::stepSwitchUse(unsigned stateBefore) {
    const runtime::PlayerLeverUse& use = switches.playerUse();
    if (use.state() != stateBefore)
        std::cout << "switch " << switches.objects()[use.lever()].instance << ": player state " << (use.state() ? use.state() : runtime::kPlayerStateAfterLeverUse)
                  << " at update " << updates << '\n';
    if (!player || (use.state() != runtime::kPlayerStateUseLerp && use.state() != runtime::kPlayerStatePullLeverUse)) return;
    player->motion.x = use.position()[0];
    player->motion.z = use.position()[2];
    player->motion.facing = player->motion.lastIntent = use.facing();
}

}
