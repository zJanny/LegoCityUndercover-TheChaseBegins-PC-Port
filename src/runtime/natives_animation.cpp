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

void AnimationNativeState::apply(AnimationNativeKind kind, const std::vector<std::string>& args, const std::vector<float>& numbers) {
    if (args.size() < animationNativeArguments(kind)) return;
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
        request.requests = requests;
        break;
    }
    case AnimationNativeKind::PlayAnimation: {
        AnimationRequest& request = current_[instance];
        const std::uint32_t requests = request.requests + 1;
        request = AnimationRequest{};
        request.name = args[1];
        request.speed = numbers[2];
        request.loop = (static_cast<std::int32_t>(numbers[3]) & 0xff) != 0;
        request.blend = numbers[4];
        request.requests = requests;
        break;
    }
    case AnimationNativeKind::LoadAnimation: {
        std::vector<std::string>& names = loaded_[instance];
        const std::string name = lower(args[1]);
        if (std::find(names.begin(), names.end(), name) == names.end()) names.push_back(name);
        break;
    }
    case AnimationNativeKind::UnloadAnimation: {
        std::vector<std::string>& names = loaded_[instance];
        names.erase(std::remove(names.begin(), names.end(), lower(args[1])), names.end());
        break;
    }
    case AnimationNativeKind::StopCharAnim:
    case AnimationNativeKind::StopAnim:
        current_.erase(instance);
        ++stops_[instance];
        break;
    case AnimationNativeKind::WaitForCurrentAnim:
    case AnimationNativeKind::WaitForCurrentCharAnim: ++waitCalls_; break;
    case AnimationNativeKind::AttachObject: {
        Attachment& attachment = attachments_[instance];
        attachment = Attachment{};
        attachment.kind = AttachKind::Object;
        attachment.parent = lower(args[1]);
        break;
    }
    case AnimationNativeKind::AttachToBone: {
        Attachment& attachment = attachments_[instance];
        attachment = Attachment{};
        attachment.kind = AttachKind::Bone;
        attachment.parent = lower(args[1]);
        attachment.childBone = lower(args[2]);
        attachment.parentBone = lower(args[3]);
        break;
    }
    case AnimationNativeKind::AttachToBoneRot: {
        Attachment& attachment = attachments_[instance];
        attachment = Attachment{};
        attachment.kind = AttachKind::BoneRotated;
        attachment.parent = lower(args[1]);
        attachment.childBone = lower(args[2]);
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
}

const AnimationRequest* AnimationNativeState::current(const std::string& instance) const {
    const auto found = current_.find(instance);
    return found == current_.end() ? nullptr : &found->second;
}

bool AnimationNativeState::loaded(const std::string& instance, const std::string& name) const {
    const auto found = loaded_.find(instance);
    return found != loaded_.end() && std::find(found->second.begin(), found->second.end(), name) != found->second.end();
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
