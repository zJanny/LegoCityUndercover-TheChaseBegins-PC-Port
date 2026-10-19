#include "runtime/prop_animation.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>

namespace chase::runtime {
namespace {

std::string lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return text;
}

double lastSampleFrame(const assets::Animation& animation, double position) {
    return std::clamp(position, 0.0, double(animation.frames > 0 ? animation.frames - 1 : 0));
}

}

std::set<std::string> animationTargets(const assets::Level& level) {
    std::set<std::string> targets;
    for (const assets::LevelScript& script : level.scripts)
        for (const assets::ScriptCall& call : script.calls) {
            const std::string function = lower(call.function);
            if ((function == "playanimation" || function == "loadanimation") && !call.arguments.empty()) targets.insert(lower(call.arguments[0]));
        }
    return targets;
}

std::string propSceneKey(const std::string& modelPath, const std::string& lowerCaseInstance) { return modelPath + "@" + lowerCaseInstance; }

std::string PropAnimations::animationPath(const std::string& mesh, const std::string& name) {
    return "models/" + lower(mesh.substr(0, mesh.find('/'))) + "/" + lower(name) + ".bfnanm";
}

PropAnimations::PropAnimations(assets::FuseArchive& archive, const std::vector<AnimatedProp>& props) : archive_(&archive) {
    for (const AnimatedProp& prop : props) {
        const std::string skeletonPath = prop.modelPath.substr(0, prop.modelPath.rfind('.')) + ".bfnskl";
        const assets::FuseEntry* skeleton = archive.find(skeletonPath);
        if (!skeleton) continue;
        Entry entry;
        entry.prop = prop;
        entry.key = propSceneKey(prop.modelPath, prop.instance);
        entry.skeleton = assets::parseSkeleton(archive.read(*skeleton));
        entry.node = prop.node;
        entries_.push_back(std::move(entry));
    }
}

std::optional<std::uint32_t> PropAnimations::clipOf(const std::string& path) {
    const auto found = clipIndex_.find(path);
    if (found != clipIndex_.end()) return found->second;
    const assets::FuseEntry* entry = archive_ ? archive_->find(path) : nullptr;
    if (!entry) return std::nullopt;
    Clip clip;
    clip.animation = assets::parseAnimation(archive_->read(*entry));
    clip.info.clip = std::uint32_t(clips_.size());
    clip.info.length = assets::animationLength(clip.animation);
    clip.info.rate = std::uint16_t(assets::animationFramesPerSecond(clip.animation));
    clip.info.loops = assets::animationLoops(clip.animation);
    clips_.push_back(std::move(clip));
    clipIndex_[path] = clips_.back().info.clip;
    return clips_.back().info.clip;
}

PropAnimations::Entry* PropAnimations::find(const std::string& instance) {
    for (Entry& entry : entries_)
        if (entry.prop.instance == instance) return &entry;
    return nullptr;
}

const PropAnimations::Entry* PropAnimations::find(const std::string& instance) const {
    for (const Entry& entry : entries_)
        if (entry.prop.instance == instance) return &entry;
    return nullptr;
}

void PropAnimations::play(const AnimationPlay& play) {
    Entry* entry = find(play.instance);
    if (!entry || play.request.character) return;
    const auto clip = clipOf(animationPath(entry->prop.mesh, play.request.name));
    if (!clip) return;
    playObjectAnimation(entry->slots, clips_[*clip].info, play.request, clock_);
}

void PropAnimations::stop(const std::string& instance) {
    if (Entry* entry = find(instance)) entry->slots.stop(clock_);
}

std::optional<bool> PropAnimations::currentNearEnd(const std::string& instance) const {
    const Entry* entry = find(instance);
    if (!entry) return std::nullopt;
    return entry->slots.currentNearEnd(clock_);
}

void PropAnimations::update(float dt) {
    clock_ += std::llround(double(dt) * double(kAnimationClockTicksPerSecond));
    for (Entry& entry : entries_) {
        const AnimationSlot* slot = entry.slots.current();
        if (!slot) continue;
        const assets::Animation& animation = clips_[slot->clip].animation;
        const std::optional<RootSample> root = rootSample(animation, lastSampleFrame(animation, AnimationSlots::position(*slot, clock_).frame));
        if (!root) continue;
        if (entry.lastRoot && entry.lastRootStart == slot->startId) {
            entry.node = applyRootDelta(entry.node, rootDelta(*entry.lastRoot, *root));
            entry.moved = true;
        }
        entry.lastRoot = root;
        entry.lastRootStart = slot->startId;
    }
}

bool PropAnimations::place(const std::string& instance, const assets::Matrix& node) {
    Entry* entry = find(instance);
    if (!entry) return false;
    entry->node = node;
    entry->moved = true;
    return true;
}

std::vector<PropPose> PropAnimations::poses() const {
    std::vector<PropPose> result;
    for (const Entry& entry : entries_) {
        const AnimationSlot* slot = entry.slots.current();
        if (!slot) continue;
        const assets::Animation& animation = clips_[slot->clip].animation;
        const double frame = lastSampleFrame(animation, AnimationSlots::position(*slot, clock_).frame);
        result.push_back({entry.key, assets::skinMatrices(entry.skeleton, animation, frame), entry.prop.instance, entry.moved, entry.node});
    }
    return result;
}

std::size_t PropAnimations::started() const {
    return std::size_t(std::count_if(entries_.begin(), entries_.end(), [](const Entry& entry) { return entry.slots.current() != nullptr; }));
}

}
