#pragma once
// PlayCharAnim, StopCharAnim and WaitForCurrentCharAnim on Player1 (docs/re/script-natives-animation.md): the script clip runs in the
// player's animation slots while the player is in the "USER ANIM" state 0x50 (update 0x2a47b0, enter 0x25fbec), which runs no locomotion; once the
// slot kind (0x22f560) is 6 the state requests state 1 and locomotion takes over in the next update. No SDL dependency.
#include "assets/assets.hpp"
#include "runtime/animation_slots.hpp"
#include "runtime/natives_animation.hpp"

#include <functional>
#include <map>
#include <optional>
#include <string>

namespace chase::runtime {

constexpr unsigned kUserAnimState = 0x50;
constexpr unsigned kLocomotionState = 1;
inline constexpr const char* kPlayerInstance = "player1";

using AnimationLoader = std::function<std::optional<assets::Animation>(const std::string& path)>;
AnimationLoader archiveAnimationLoader(assets::FuseArchive& archive);

class PlayerScriptAnimation : public AnimationDriver {
public:
    PlayerScriptAnimation() = default;
    explicit PlayerScriptAnimation(AnimationLoader loader) : loader_(std::move(loader)) {}

    void play(const AnimationPlay& play) override;
    void stop(const std::string& lowerCaseInstance) override;
    std::optional<bool> currentNearEnd(const std::string& lowerCaseInstance) const override;

    // One player update, before the locomotion: advances the clock, applies a pending state request, and runs the user-animation update.
    // True while the script clip owns the player's pose in this update.
    bool update(float dt);

    // The clip and its sampled frame while update() returned true.
    const assets::Animation* animation() const;
    double frame() const;

    bool inUserAnimation() const { return state_ == kUserAnimState; }
    // models/char_minifig/<class folder>/<name>.bfnanm, lower case (0x295a8c builds "%s/%s" of the character folder and the class argument).
    static std::string clipPath(const std::string& classFolder, const std::string& name);

private:
    struct Clip {
        assets::Animation animation;
        AnimationClipInfo info;
    };
    const Clip* clipOf(const std::string& path);

    AnimationLoader loader_;
    std::map<std::string, Clip> clips_;
    std::map<std::uint32_t, const Clip*> byId_;
    AnimationSlots slots_;
    bool scriptLoaded_ = false;
    unsigned state_ = kLocomotionState;
    std::optional<unsigned> requested_;
    std::int64_t clock_ = 0;
};

}
