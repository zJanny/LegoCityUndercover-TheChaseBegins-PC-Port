#include "runtime/natives_animation.hpp"

#include <algorithm>
#include <cctype>

namespace chase::runtime {
namespace {

struct Entry {
    const char* name;
    AnimationNativeKind kind;
    unsigned arguments;
};

constexpr Entry kEntries[] = {
    {"playcharanim", AnimationNativeKind::PlayCharAnim, 4},
    {"playanimation", AnimationNativeKind::PlayAnimation, 5},
    {"loadanimation", AnimationNativeKind::LoadAnimation, 4},
    {"unloadanimation", AnimationNativeKind::UnloadAnimation, 2},
    {"stopcharanim", AnimationNativeKind::StopCharAnim, 1},
    {"stopanim", AnimationNativeKind::StopAnim, 1},
    {"waitforcurrentanim", AnimationNativeKind::WaitForCurrentAnim, 1},
    {"waitforcurrentcharanim", AnimationNativeKind::WaitForCurrentCharAnim, 1},
    {"attachobject", AnimationNativeKind::AttachObject, 2},
    {"attachtobone", AnimationNativeKind::AttachToBone, 4},
    {"attachtobonerot", AnimationNativeKind::AttachToBoneRot, 6},
    {"attachtoworldroot", AnimationNativeKind::AttachToWorldRoot, 1},
    {"attachtoroom", AnimationNativeKind::AttachToRoom, 1},
};

std::string lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return text;
}

// 0x1bfa7c..: PlayCharAnim and PlayAnimation compare the loop argument with 0 (vcmpe, greater than).
bool positive(float value) { return value > 0.0f; }

constexpr float kPlayCharAnimSpeed = 1.0f;
constexpr float kPlayCharAnimBlend = 0.1f;

}

std::optional<AnimationNativeKind> animationNativeKind(const std::string& lowerCaseName) {
    for (const Entry& entry : kEntries)
        if (lowerCaseName == entry.name) return entry.kind;
    return std::nullopt;
}

std::vector<std::string> animationNativeNames() {
    std::vector<std::string> names;
    for (const Entry& entry : kEntries) names.push_back(entry.name);
    return names;
}

unsigned animationNativeArguments(AnimationNativeKind kind) {
    for (const Entry& entry : kEntries)
        if (entry.kind == kind) return entry.arguments;
    return 0;
}

void playCharacterAnimation(AnimationSlots& slots, bool& scriptLoaded, const AnimationClipInfo& clip, bool loaded, bool loop, std::int64_t now) {
    const unsigned mode = loop ? kAnimationLoop : kAnimationOnce;
    const AnimationSlot* current = slots.current();
    if (loaded && current && current->clip == clip.clip && scriptLoaded) {
        slots.setMode(mode);
        slots.setOffset(0.0f, now);
        return;
    }
    scriptLoaded = false;
    slots.start(clip, mode, 0, kAnimationFrameDefault, kPlayCharAnimSpeed, kPlayCharAnimBlend, now);
    if (!loaded) scriptLoaded = true;
}

void playObjectAnimation(AnimationSlots& slots, const AnimationClipInfo& clip, const AnimationRequest& request, std::int64_t now) {
    slots.start(clip, request.mode, 0, kAnimationFrameDefault, request.speed, request.blend, now);
    if (request.offset != 0) slots.setOffset(float(request.offset), now);
}

bool AnimationNativeState::apply(AnimationNativeKind kind, const std::vector<std::string>& args, const std::vector<float>& numbers,
                                 AnimationDriver* driver) {
    if (args.size() < animationNativeArguments(kind)) return true;
    const std::string instance = lower(args[0]);
    switch (kind) {
    case AnimationNativeKind::PlayCharAnim: {
        AnimationRequest& request = current_[instance];
        const std::uint32_t requests = request.requests + 1;
        request = AnimationRequest{};
        request.name = args[1];
        request.classFolder = lower(args[2]);
        request.character = true;
        request.loop = positive(numbers[3]);
        request.mode = request.loop ? kAnimationLoop : kAnimationOnce;
        request.speed = kPlayCharAnimSpeed;
        request.blend = kPlayCharAnimBlend;
        request.requests = requests;
        if (driver) driver->play({instance, request});
        break;
    }
    case AnimationNativeKind::PlayAnimation: {
        const auto loadedSet = loaded_.find(instance);
        if (loadedSet == loaded_.end()) break;
        const auto loadedName = loadedSet->second.find(lower(args[1]));
        if (loadedName == loadedSet->second.end()) break;
        AnimationRequest& request = current_[instance];
        const std::uint32_t requests = request.requests + 1;
        request = AnimationRequest{};
        request.name = args[1];
        request.classFolder = loadedName->second;
        request.speed = numbers[2];
        request.mode = std::uint32_t(static_cast<std::int32_t>(numbers[3])) & 0xffu;
        request.loop = request.mode != kAnimationOnce;
        request.blend = numbers[4];
        const auto offset = offsets_.find(instance);
        request.offset = offset == offsets_.end() ? 0 : offset->second;
        request.requests = requests;
        if (driver) driver->play({instance, request});
        break;
    }
    case AnimationNativeKind::LoadAnimation: loaded_[instance].emplace(lower(args[1]), lower(args[2])); break;
    case AnimationNativeKind::UnloadAnimation: {
        const auto found = loaded_.find(instance);
        if (found != loaded_.end()) found->second.erase(lower(args[1]));
        break;
    }
    case AnimationNativeKind::StopCharAnim:
    case AnimationNativeKind::StopAnim:
        current_.erase(instance);
        ++stops_[instance];
        if (driver) driver->stop(instance);
        break;
    case AnimationNativeKind::WaitForCurrentAnim:
    case AnimationNativeKind::WaitForCurrentCharAnim: {
        ++waitCalls_;
        const std::optional<bool> nearEnd = driver ? driver->currentNearEnd(instance) : std::nullopt;
        return nearEnd.value_or(true);
    }
    case AnimationNativeKind::AttachObject: {
        Attachment& attachment = attachments_[lower(args[1])];
        attachment = Attachment{};
        attachment.kind = AttachKind::Object;
        attachment.parent = instance;
        break;
    }
    case AnimationNativeKind::AttachToBone: {
        Attachment& attachment = attachments_[lower(args[1])];
        attachment = Attachment{};
        attachment.kind = AttachKind::Bone;
        attachment.parent = instance;
        attachment.parentBone = lower(args[2]);
        attachment.childBone = lower(args[3]);
        break;
    }
    case AnimationNativeKind::AttachToBoneRot: {
        Attachment& attachment = attachments_[lower(args[1])];
        attachment = Attachment{};
        attachment.kind = AttachKind::BoneRotated;
        attachment.parent = instance;
        attachment.parentBone = lower(args[2]);
        for (int axis = 0; axis < 3; ++axis) attachment.rotationDegrees[axis] = numbers[3 + axis];
        break;
    }
    case AnimationNativeKind::AttachToWorldRoot: {
        Attachment& attachment = attachments_[instance];
        attachment = Attachment{};
        attachment.kind = AttachKind::WorldRoot;
        break;
    }
    case AnimationNativeKind::AttachToRoom: {
        Attachment& attachment = attachments_[instance];
        attachment = Attachment{};
        attachment.kind = AttachKind::Room;
        break;
    }
    }
    return true;
}

const AnimationRequest* AnimationNativeState::current(const std::string& instance) const {
    const auto found = current_.find(instance);
    return found == current_.end() ? nullptr : &found->second;
}

bool AnimationNativeState::loaded(const std::string& instance, const std::string& name) const {
    const auto found = loaded_.find(instance);
    return found != loaded_.end() && found->second.count(name) != 0;
}

const Attachment* AnimationNativeState::attachment(const std::string& instance) const {
    const auto found = attachments_.find(instance);
    return found == attachments_.end() ? nullptr : &found->second;
}

std::uint32_t AnimationNativeState::stopRequests(const std::string& instance) const {
    const auto found = stops_.find(instance);
    return found == stops_.end() ? 0 : found->second;
}

}
