#include "game_characters.hpp"

#include <algorithm>

namespace game_characters {
using namespace chase::assets;

namespace {
constexpr const char* kPedestrianClass = "chars/pedestrian";

runtime::NpcIdleClip clipOf(const Animation& animation) {
    return {double(animation.frames), double(animationFramesPerSecond(animation))};
}

double lastSampleFrame(const Animation& animation, double position) {
    return std::min(position, double(animation.frames > 0 ? animation.frames - 1 : 0));
}
}

LevelCharacters::LevelCharacters(FuseArchive& archive, const std::vector<CharacterObject>& characters,
                                 const std::vector<std::optional<render::CharacterInstance>>& placed)
    : characters_(characters) {
    for (std::size_t index = 0; index < characters.size(); ++index) {
        const CharacterObject& character = characters[index];
        if (!placed[index] || character.placement.className == kPedestrianClass) continue;
        const std::string bodyPath = meshModelPath(character.appearance.body);
        auto rig = rigs_.find(bodyPath);
        if (rig == rigs_.end()) rig = rigs_.emplace(bodyPath, render::loadCharacterRig(archive, bodyPath)).first;
        auto clips = clips_.find(character.animationClass);
        if (clips == clips_.end()) {
            Clips loaded;
            loaded.idle = render::loadAnimation(archive, render::levelCharacterAnimationPath(archive, character.animationClass, "idle"));
            loaded.idle2 = render::loadAnimation(archive, render::levelCharacterAnimationPath(archive, character.animationClass, "idle2"));
            loaded.idleClip = clipOf(loaded.idle);
            loaded.idle2Clip = clipOf(loaded.idle2);
            clips = clips_.emplace(character.animationClass, std::move(loaded)).first;
        }
        Entry entry;
        entry.character = index;
        entry.instance = *placed[index];
        entry.rig = &rig->second;
        entry.clips = &clips->second;
        entry.world = placementMatrix(character.placement);
        entries_.push_back(std::move(entry));
    }
}

void LevelCharacters::update(render::Scene& scene, float dt, const std::function<bool(const CharacterObject&)>& active) {
    for (Entry& entry : entries_) {
        if (!active(characters_[entry.character])) continue;
        const Clips& clips = *entry.clips;
        entry.idle.update(double(dt), random_, clips.idleClip, clips.idle2Clip);
        const runtime::NpcIdlePose pose = entry.idle.pose(clips.idleClip, clips.idle2Clip);
        const bool second = pose.variant == runtime::NpcIdleVariant::Idle2;
        const Animation& animation = second ? clips.idle2 : clips.idle;
        const std::vector<Matrix> skin = render::poseBody(scene, *entry.rig, entry.instance, animation, lastSampleFrame(animation, pose.position));
        render::placeCharacter(scene, *entry.rig, entry.instance, skin, entry.world);
    }
}

std::size_t LevelCharacters::started() const {
    return std::size_t(std::count_if(entries_.begin(), entries_.end(), [](const Entry& entry) { return entry.idle.started(); }));
}

std::optional<runtime::NpcIdlePose> LevelCharacters::pose(const std::string& instance) const {
    for (const Entry& entry : entries_)
        if (characters_[entry.character].placement.instance == instance && entry.idle.started())
            return entry.idle.pose(entry.clips->idleClip, entry.clips->idle2Clip);
    return std::nullopt;
}

}
