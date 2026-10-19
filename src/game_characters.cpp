#include "game_characters.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>

namespace game_characters {
using namespace chase::assets;

namespace {
constexpr const char* kPedestrianClass = "chars/pedestrian";
constexpr const char* kCharacterModelFolder = "models/char_minifig/";

std::string lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return text;
}

double lastSampleFrame(const Animation& animation, double position) {
    return std::clamp(position, 0.0, double(animation.frames > 0 ? animation.frames - 1 : 0));
}

std::string characterAnimationPath(const std::string& classFolder, const std::string& name) {
    return lower(kCharacterModelFolder + (classFolder.empty() ? std::string() : classFolder + "/") + name + ".bfnanm");
}
}

LevelCharacters::LevelCharacters(FuseArchive& archive, const std::vector<CharacterObject>& characters,
                                 const std::vector<std::optional<render::CharacterInstance>>& placed)
    : archive_(archive), characters_(characters) {
    for (std::size_t index = 0; index < characters.size(); ++index) {
        const CharacterObject& character = characters[index];
        if (!placed[index] || character.placement.className == kPedestrianClass) continue;
        const std::string bodyPath = meshModelPath(character.appearance.body);
        auto rig = rigs_.find(bodyPath);
        if (rig == rigs_.end()) rig = rigs_.emplace(bodyPath, render::loadCharacterRig(archive, bodyPath)).first;
        Entry entry;
        entry.character = index;
        entry.instance = *placed[index];
        entry.rig = &rig->second;
        entry.idle = *clipOf(render::levelCharacterAnimationPath(archive, character.animationClass, "idle"));
        entry.idle2 = *clipOf(render::levelCharacterAnimationPath(archive, character.animationClass, "idle2"));
        entry.world = placementMatrix(character.placement);
        byInstance_[lower(character.placement.instance)] = entries_.size();
        entries_.push_back(std::move(entry));
    }
}

std::optional<std::uint32_t> LevelCharacters::clipOf(const std::string& path) {
    const auto found = clipIndex_.find(path);
    if (found != clipIndex_.end()) return found->second;
    if (!archive_.find(path)) return std::nullopt;
    Clip clip;
    clip.path = path;
    clip.animation = render::loadAnimation(archive_, path);
    clip.info.clip = std::uint32_t(clips_.size());
    clip.info.length = animationLength(clip.animation);
    clip.info.rate = std::uint16_t(animationFramesPerSecond(clip.animation));
    clip.info.loops = animationLoops(clip.animation);
    clips_.push_back(std::move(clip));
    clipIndex_[path] = clips_.back().info.clip;
    return clips_.back().info.clip;
}

LevelCharacters::Entry* LevelCharacters::find(const std::string& instance) {
    const auto found = byInstance_.find(instance);
    return found == byInstance_.end() ? nullptr : &entries_[found->second];
}

const LevelCharacters::Entry* LevelCharacters::find(const std::string& instance) const {
    const auto found = byInstance_.find(instance);
    return found == byInstance_.end() ? nullptr : &entries_[found->second];
}

void LevelCharacters::play(const runtime::AnimationPlay& play) {
    Entry* entry = find(play.instance);
    if (!entry) return;
    if (play.request.character) entry->idleState.enterScriptAnimation();
    const auto clip = clipOf(characterAnimationPath(play.request.classFolder, play.request.name));
    if (!clip) return;
    const runtime::AnimationClipInfo& info = clips_[*clip].info;
    if (play.request.character) {
        const runtime::AnimationSlot* current = entry->slots.current();
        const bool loaded = *clip == entry->idle || *clip == entry->idle2 || (entry->scriptLoaded && current && current->clip == *clip);
        runtime::playCharacterAnimation(entry->slots, entry->scriptLoaded, info, loaded, play.request.loop, clock_);
    } else {
        runtime::playObjectAnimation(entry->slots, info, play.request, clock_);
    }
}

bool LevelCharacters::place(const std::string& instance, const Matrix& world) {
    Entry* entry = find(instance);
    if (!entry) return false;
    entry->world = world;
    return true;
}

void LevelCharacters::stop(const std::string& instance) {
    if (Entry* entry = find(instance)) entry->slots.stop(clock_);
}

std::optional<bool> LevelCharacters::currentNearEnd(const std::string& instance) const {
    const Entry* entry = find(instance);
    if (!entry) return std::nullopt;
    return entry->slots.currentNearEnd(clock_);
}

void LevelCharacters::update(render::Scene& scene, float dt, const std::function<bool(const CharacterObject&)>& active) {
    clock_ += std::llround(double(dt) * double(runtime::kAnimationClockTicksPerSecond));
    for (Entry& entry : entries_) {
        if (!active(characters_[entry.character])) continue;
        entry.idleState.update(entry.slots, clock_, random_, clips_[entry.idle].info, clips_[entry.idle2].info);
        const runtime::AnimationSlot* slot = entry.slots.current();
        if (!slot) continue;
        const Animation& animation = clips_[slot->clip].animation;
        const float frame = runtime::AnimationSlots::position(*slot, clock_).frame;
        entry.skin = render::poseBody(scene, *entry.rig, entry.instance, animation, lastSampleFrame(animation, frame));
        render::placeCharacter(scene, *entry.rig, entry.instance, entry.skin, entry.world);
    }
}

std::size_t LevelCharacters::started() const {
    return std::size_t(std::count_if(entries_.begin(), entries_.end(), [](const Entry& entry) { return entry.slots.current() != nullptr; }));
}

std::optional<CharacterPose> LevelCharacters::pose(const std::string& instance) const {
    const Entry* entry = find(lower(instance));
    if (!entry || !entry->slots.current()) return std::nullopt;
    const runtime::AnimationSlot& slot = *entry->slots.current();
    return CharacterPose{clips_[slot.clip].path, runtime::AnimationSlots::position(slot, clock_).frame};
}

std::optional<BodyPose> LevelCharacters::bodyPose(const std::string& instance) const {
    const Entry* entry = find(instance);
    if (!entry) return std::nullopt;
    BodyPose pose{entry->world, std::nullopt, std::nullopt};
    if (entry->skin.empty()) return pose;
    if (entry->rig->headJoint) pose.head = render::attachedModelWorld(entry->rig->headJoint, entry->skin, entry->world);
    if (entry->rig->hatJoint) pose.hat = render::attachedModelWorld(entry->rig->hatJoint, entry->skin, entry->world);
    return pose;
}

}
