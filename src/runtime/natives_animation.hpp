#pragma once
// Script natives of batch SCRIPT-32 (docs/re/script-natives-animation.md): animation and attach natives. The port has
// no animation playback for placed objects and characters (ANIM-2) and no scene node tree for placed objects, so the
// handlers' calls into those systems are recorded here and the waits complete at once (stand-in SCRIPT-S110).
// No SDL dependency.
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace chase::runtime {

enum class AnimationNativeKind {
    PlayCharAnim,
    PlayAnimation,
    LoadAnimation,
    UnloadAnimation,
    StopCharAnim,
    StopAnim,
    WaitForCurrentAnim,
    WaitForCurrentCharAnim,
    AttachObject,
    AttachToBone,
    AttachToBoneRot,
    AttachToWorldRoot,
    AttachToRoom,
};

std::optional<AnimationNativeKind> animationNativeKind(const std::string& lowerCaseName);
std::vector<std::string> animationNativeNames();
// Declared argument count of the native (the census column Args); a call with fewer arguments is ignored.
unsigned animationNativeArguments(AnimationNativeKind kind);

// What PlayCharAnim 0x29e5f0 / PlayAnimation 0x268240 asked the animation player (0x23afc0) for. A character animation
// is a file name plus a class folder (models/<model>/<class>/<name>); an object animation is models/<model>/<name>.
struct AnimationRequest {
    std::string name;
    std::string classFolder;
    bool character = false;
    bool loop = false;
    float speed = 1.0f;
    float blend = 0.1f;
    std::uint32_t requests = 0;
};

enum class AttachKind { Object, Bone, BoneRotated, WorldRoot, Room };

// 0x1bf68c, 0x1ddb90, 0x1bf718, 0x2951ec, 0x1ddc88: the scene node of `child` is moved under another node.
struct Attachment {
    AttachKind kind = AttachKind::Object;
    std::string parent;
    std::string childBone;
    std::string parentBone;
    float rotationDegrees[3] = {0, 0, 0};
};

class AnimationNativeState {
public:
    // `args` are the call's argument tokens with strings unquoted, `lowerCaseInstance` is args[0] folded to lower case.
    void apply(AnimationNativeKind kind, const std::vector<std::string>& args, const std::vector<float>& numbers);

    const AnimationRequest* current(const std::string& lowerCaseInstance) const;
    bool loaded(const std::string& lowerCaseInstance, const std::string& lowerCaseName) const;
    const Attachment* attachment(const std::string& lowerCaseInstance) const;
    std::uint32_t stopRequests(const std::string& lowerCaseInstance) const;
    std::uint32_t waitCalls() const { return waitCalls_; }

private:
    std::map<std::string, AnimationRequest> current_;
    std::map<std::string, std::vector<std::string>> loaded_;
    std::map<std::string, Attachment> attachments_;
    std::map<std::string, std::uint32_t> stops_;
    std::uint32_t waitCalls_ = 0;
};

}
