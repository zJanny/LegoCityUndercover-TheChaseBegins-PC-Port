#pragma once
// PlayCharAnim, StopCharAnim and WaitForCurrentCharAnim on Player1 (docs/re/script-natives-animation.md): the script clip runs in the
// player's animation slots while the player is in the "USER ANIM" state 0x50 (update 0x2a47b0, enter 0x25fbec), which runs no locomotion; once the
// slot kind (0x22f560) is 6 the state requests state 1 and locomotion takes over in the next update. The player's update (0x1835d0 -> 0x26a19c)
// runs the current state's update only while no request is pending, then applies a request (0x226100: exit, enter). The script runner runs before
// the player's update, so a request of a native is applied in the same update without the old state's update. No SDL dependency.
#include "assets/assets.hpp"
#include "runtime/animation_slots.hpp"
#include "runtime/natives_animation.hpp"
#include "runtime/player_motion.hpp"
#include "runtime/prop_root_motion.hpp"

#include <functional>
#include <map>
#include <optional>
#include <string>

namespace chase::runtime {

// The root motion of the script clip for one update (0x2a47b0 -> 0x215804 -> 0x20cd30 -> 0x2c9770, docs/re/script-natives-animation.md): the world
// displacement of the player and, when the clip turns, the new facing (0x22f03c: trunc(atan2 of the node's third row * 10430.378)).
struct ScriptRootStep {
    float dx = 0, dy = 0, dz = 0;
    std::optional<std::int16_t> facing;
};

constexpr unsigned kUserAnimState = 0x50;
// What the player's state machine ran in one update: the locomotion state's update, no state update (a request was pending), or 0x2a47b0.
enum class PlayerStateStep { Locomotion, Skipped, UserAnimation };
constexpr unsigned kLocomotionState = 1;
inline constexpr const char* kPlayerInstance = "player1";

using AnimationLoader = std::function<std::optional<assets::Animation>(const std::string& path)>;
AnimationLoader archiveAnimationLoader(assets::FuseArchive& archive);

class PlayerScriptAnimation : public AnimationDriver {
public:
    PlayerScriptAnimation() = default;
    explicit PlayerScriptAnimation(AnimationLoader loader) : loader_(std::move(loader)) {}

    // STAND-IN SCRIPT-S262: a clip that is not found requests no state; 0x29e5f0 requests 0x50 without testing the play.
    void play(const AnimationPlay& play) override;
    void stop(const std::string& lowerCaseInstance) override;
    std::optional<bool> currentNearEnd(const std::string& lowerCaseInstance) const override;

    // One player update, after the script runner and before the locomotion: runs the current state's update unless a request is pending (state
    // 0x50: the kind test), applies the request (its enter function runs at the end of the update, the new state's update starts in the next one),
    // then advances the clock, so the state step sees the clock of the runner and the pose the advanced one (oracle the save state at the dog cage trigger of mission 1-2).
    PlayerStateStep update(float dt);

    // The state whose enter function ran at the end of the last update(), if a request was applied.
    std::optional<unsigned> enteredState() const { return entered_; }
    // STAND-IN ANIM-S13: the slot's script clip is shown at full weight from the play on; the original blends it in over 0.1 s.
    bool posesScriptClip() const { return state_ == kUserAnimState; }

    // The clip and its sampled frame (the pose, at the advanced clock) while posesScriptClip().
    const assets::Animation* animation() const;
    double frame() const;

    // After update() returned UserAnimation: the clip's root displacement for this update given the player's facing. Nothing in the first update of a
    // clip (it only stores the sample) and for clips without an extra curve. STAND-IN SCRIPT-S266: all four axes are applied.
    std::optional<ScriptRootStep> rootMotion(std::int16_t facing);

    bool inUserAnimation() const { return state_ == kUserAnimState; }
    // models/char_minifig/<class folder>/<name>.bfnanm, lower case (0x295a8c builds "%s/%s" of the character folder and the class argument).
    static std::string clipPath(const std::string& classFolder, const std::string& name);

private:
    struct Clip {
        assets::Animation animation;
        AnimationClipInfo info;
    };
    const Clip* clipOf(const std::string& path);
    double frameAt(std::int64_t now) const;

    AnimationLoader loader_;
    std::map<std::string, Clip> clips_;
    std::map<std::uint32_t, const Clip*> byId_;
    AnimationSlots slots_;
    bool scriptLoaded_ = false;
    unsigned state_ = kLocomotionState;
    std::optional<unsigned> requested_;
    std::optional<unsigned> entered_;
    std::int64_t clock_ = 0;
    std::int64_t stateClock_ = 0;
    std::optional<RootSample> lastRoot_;
    std::uint32_t lastRootStart_ = 0;
    float nodeHeading_ = 0;
    std::optional<std::int16_t> lastFacing_;
};

// Moves the player by a root step: wall slide and the vertical step run as for the attack root step (playerRootStepUpdate); the turn sets the facing
// and the intent heading (0x22f03c writes ctrl+8 and ctrl+10).
// The enter functions' writes to the vertical state: state 0x50 (0x25fbec) sets ctrl+0x2b8 = 0, state 1 (0x1abbb0) sets ctrl+0x2b8 and ctrl+0x2b4 = 0.
void applyScriptStateEnter(unsigned state, VerticalState& vertical);

void applyScriptRootStep(const ScriptRootStep& step, LocomotionState& motion, VerticalState& vertical, const CollisionWorld& world, float dt);

}
