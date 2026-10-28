#pragma once
// Script natives of batch SCRIPT-32 (docs/re/script-natives-animation.md): animation and attach natives. Requests are
// recorded here and handed to an AnimationDriver (the level characters' animation players); objects the driver does not
// animate and the attach natives stay records (stand-in SCRIPT-S110). No SDL dependency.
#include "runtime/animation_slots.hpp"

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <utility>
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
    unsigned mode = kAnimationOnce;
    std::uint32_t offset = 0;
    float speed = 1.0f;
    float blend = 0.1f;
    std::uint32_t requests = 0;
};

// A play handed to the driver: PlayCharAnim (`character`, folder models/<model>/<class>/) or PlayAnimation of an animation
// LoadAnimation loaded (`classFolder` as LoadAnimation gave it, empty for models/<model>/<name>).
struct AnimationPlay {
    std::string instance;
    AnimationRequest request;
};

class AnimationDriver {
public:
    virtual ~AnimationDriver() = default;
    virtual void play(const AnimationPlay& play) = 0;
    virtual void stop(const std::string& lowerCaseInstance) = 0;
    // The test of WaitForCurrentAnim / WaitForCurrentCharAnim; nullopt when the driver does not animate the instance.
    virtual std::optional<bool> currentNearEnd(const std::string& lowerCaseInstance) const = 0;
};

// The rules of PlayCharAnim's player call 0x295a8c on an entity's animation player: `loaded` tells whether the entity had
// the animation loaded before (0x21e8b4), `scriptLoaded` is the entity's flag ctrl+0x317 bit 0x40 (the current animation
// was loaded by this call). The same loaded animation playing under that flag only gets the new mode and starts again
// at frame 0 (0x22c2fc); otherwise it starts in a new slot with speed 1.0 and blend 0.1, from frame 0 to its length.
void playCharacterAnimation(AnimationSlots& slots, bool& scriptLoaded, const AnimationClipInfo& clip, bool loaded, bool loop, std::int64_t now);
// PlayAnimation 0x268240 after its loaded check: start with the script's speed, mode and blend, then the object's
// AnimationOffset (frames) when it is not 0.
void playObjectAnimation(AnimationSlots& slots, const AnimationClipInfo& clip, const AnimationRequest& request, std::int64_t now);

enum class AttachKind { Object, Bone, BoneRotated, WorldRoot, Room };

// 0x1bf68c, 0x1ddb90, 0x1bf718, 0x2951ec, 0x1ddc88, keyed by the attached instance: AttachObject(parent, child) detaches the
// child's node and attaches it under the parent's node keeping its world transform (0x245048, 0x245008); AttachToBone /
// AttachToBoneRot(parent, child, parent bone, child bone or rotation) attach it to a bone of the parent (0x20f33c);
// AttachToWorldRoot / AttachToRoom(child) move it under the world root or its room node. The scene does not follow yet.
struct Attachment {
    AttachKind kind = AttachKind::Object;
    std::string parent;
    std::string childBone;
    std::string parentBone;
    float rotationDegrees[3] = {0, 0, 0};
    std::uint32_t sequence = 0;
};

class AnimationNativeState {
public:
    // `args` are the call's argument tokens with strings unquoted, `lowerCaseInstance` is args[0] folded to lower case.
    // Returns false while a WaitForCurrent native blocks. `driver` may be null (record only).
    bool apply(AnimationNativeKind kind, const std::vector<std::string>& args, const std::vector<float>& numbers, AnimationDriver* driver);
    // The AnimationOffset parameter of placed objects (lookup 0x2451d0), lower-case instance names.
    void setAnimationOffsets(std::map<std::string, std::uint32_t> offsets) { offsets_ = std::move(offsets); }

    const AnimationRequest* current(const std::string& lowerCaseInstance) const;
    bool loaded(const std::string& lowerCaseInstance, const std::string& lowerCaseName) const;
    const Attachment* attachment(const std::string& lowerCaseInstance) const;
    const std::map<std::string, Attachment>& attachments() const { return attachments_; }
    std::uint32_t stopRequests(const std::string& lowerCaseInstance) const;
    std::uint32_t waitCalls() const { return waitCalls_; }

private:
    std::map<std::string, AnimationRequest> current_;
    std::map<std::string, std::map<std::string, std::string>> loaded_;
    std::map<std::string, std::uint32_t> offsets_;
    std::map<std::string, Attachment> attachments_;
    std::map<std::string, std::uint32_t> stops_;
    std::uint32_t waitCalls_ = 0;
    std::uint32_t attachCalls_ = 0;
};

}
