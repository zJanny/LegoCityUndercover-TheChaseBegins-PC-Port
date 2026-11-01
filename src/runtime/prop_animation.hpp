#pragma once
// Skeletal animations of placed props played by PlayAnimation / StopAnim (garage shutters, the dog cage, the dumpster, the crane, lawnmowers):
// the slot player of the entity (animation_slots.hpp) over models/<model>/<name>.bfnanm and the model's skeleton. Rules and measurements:
// docs/re/script-natives-animation.md. No SDL dependency.
#include "assets/assets.hpp"
#include "runtime/animation_slots.hpp"
#include "runtime/natives_animation.hpp"
#include "runtime/prop_root_motion.hpp"

#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace chase::runtime {

// Lower-case first arguments of the PlayAnimation and LoadAnimation calls of the level's scripts: the objects that may play an animation.
std::set<std::string> animationTargets(const assets::Level& level);

// Scene key of the instance of a model that is skinned on its own.
std::string propSceneKey(const std::string& modelPath, const std::string& lowerCaseInstance);

struct AnimatedProp {
    std::string instance;   // lower case
    std::string modelPath;  // models/<dir>/<dir>.bfnmdl
    std::string mesh;       // placement mesh "Dir/File": the animation folder is models/<dir lower case>/
    assets::Matrix node = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};  // node matrix at the level start (placementMatrix)
};

struct PropPose {
    std::string key;
    std::vector<assets::Matrix> skin;
    std::string instance;   // lower case
    bool moved = false;     // the clip's root motion moved the node (0x16b06c)
    assets::Matrix node{};  // node matrix after the root motion, accumulated from the placement or the last place()
    std::optional<RootSample> delta;  // the root delta of this update only: the node it applies to is the object's current one (attached and carried props)
};

class PropAnimations : public AnimationDriver {
public:
    PropAnimations() = default;
    PropAnimations(assets::FuseArchive& archive, const std::vector<AnimatedProp>& props);

    void play(const AnimationPlay& play) override;
    void stop(const std::string& lowerCaseInstance) override;
    std::optional<bool> currentNearEnd(const std::string& lowerCaseInstance) const override;

    // Advances the clock and applies the root delta of each playing slot to its node (prop update 0x15aa70 -> 0x16b06c).
    void update(float dt);
    // PositionObject 0x1d9e6c: the node takes the target's matrix and later root motion starts from it. False when the instance is not an animated prop.
    bool place(const std::string& lowerCaseInstance, const assets::Matrix& node);
    // Skin matrices of the props that started an animation, at the frame of the current slot (the last frame once a play-once clip ended).
    std::vector<PropPose> poses() const;
    std::size_t started() const;
    // Animation folder: models/<mesh folder lower case>/<name>.bfnanm (0x18ddc0 builds object animation paths the same way).
    static std::string animationPath(const std::string& mesh, const std::string& name);

private:
    struct Clip {
        assets::Animation animation;
        AnimationClipInfo info;
    };
    struct Entry {
        AnimatedProp prop;
        std::string key;
        assets::Skeleton skeleton;
        AnimationSlots slots;
        assets::Matrix node{};
        bool moved = false;
        std::optional<RootSample> delta;
        std::optional<RootSample> lastRoot;
        std::uint32_t lastRootStart = 0;
    };
    std::optional<std::uint32_t> clipOf(const std::string& path);
    Entry* find(const std::string& lowerCaseInstance);
    const Entry* find(const std::string& lowerCaseInstance) const;

    assets::FuseArchive* archive_ = nullptr;
    std::vector<Clip> clips_;
    std::map<std::string, std::uint32_t> clipIndex_;
    std::vector<Entry> entries_;
    std::int64_t clock_ = 0;
};

}
