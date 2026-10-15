#pragma once
// Skeletal animations of placed props played by PlayAnimation / StopAnim (garage shutters, the dog cage, the dumpster, the crane, lawnmowers):
// the slot player of the entity (animation_slots.hpp) over models/<model>/<name>.bfnanm and the model's skeleton. Rules and measurements:
// docs/re/script-natives-animation.md. No SDL dependency.
#include "assets/assets.hpp"
#include "runtime/animation_slots.hpp"
#include "runtime/natives_animation.hpp"

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
};

struct PropPose {
    std::string key;
    std::vector<assets::Matrix> skin;
};

class PropAnimations : public AnimationDriver {
public:
    PropAnimations() = default;
    PropAnimations(assets::FuseArchive& archive, const std::vector<AnimatedProp>& props);

    void play(const AnimationPlay& play) override;
    void stop(const std::string& lowerCaseInstance) override;
    std::optional<bool> currentNearEnd(const std::string& lowerCaseInstance) const override;

    void update(float dt);
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
