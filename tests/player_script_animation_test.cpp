// PlayCharAnim, StopCharAnim and WaitForCurrentCharAnim on Player1 (docs/re/script-natives-animation.md, entry on the user-animation state).
// Provenance of the recorded values:
//  - state table words at 0x3577f4 (read from code.bin, llvm-objdump of game.arm.elf): state 0x50 = {update 0x2a47b0, enter 0x25fbec}; PlayCharAnim 0x29e5f0
//    requests 0x50 (0x51 for PlayCutscene) after the play 0x295a8c; the update 0x2a47b0 requests state 1 when the slot kind (0x22f560) is 6;
//    StopCharAnim 0x1bfb84 stops the entity's slot (0x225c5c: mode 6).
//  - the clip is models/char_minifig/class_generic/idle_minifig.bfnanm: 19 frames at 15 fps with the loop bit (oracle m11-1-donut-shop, docs/re/npc-animation.md).
// With FIB and NAMES arguments the same sequence runs on the real clip; exits with 77 (skipped) when the data is missing.
#include "assets/assets.hpp"
#include "runtime/player_script_animation.hpp"

#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase::runtime;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr float kStep = 1.0f / 30.0f;
constexpr const char* kIdlePath = "models/char_minifig/class_generic/idle_minifig.bfnanm";

chase::assets::Animation idleClip() {
    chase::assets::Animation clip;
    clip.frames = 19;
    clip.hasJointTable = true;
    clip.jointTableWord = 0x20000000u;
    return clip;
}

AnimationPlay playOf(const std::string& instance, const std::string& name, bool loop, bool character = true) {
    AnimationPlay play;
    play.instance = instance;
    play.request.name = name;
    play.request.classFolder = "class_generic";
    play.request.character = character;
    play.request.loop = loop;
    play.request.mode = loop ? kAnimationLoop : kAnimationOnce;
    return play;
}

void run(const AnimationLoader& loader) {
    {
        PlayerScriptAnimation animation(loader);
        require(!animation.update(kStep) && !animation.inUserAnimation(), "no user animation before a play");
        require(!animation.currentNearEnd("player1"), "the wait completes at once without a script clip");
        animation.play(playOf("player1", "Idle_Minifig", false));
        require(!animation.inUserAnimation() && animation.currentNearEnd("player1") == std::optional<bool>(false),
                "the request is pending until the next update; the wait already blocks");
        require(!animation.update(kStep) && animation.inUserAnimation() && animation.enteredState() == std::optional<unsigned>(kUserAnimState),
                "the update after the request runs the previous state's update; state 0x50 is entered at its end");
        require(animation.posesScriptClip() && animation.animation() && animation.animation()->frames == 19, "the slot holds the script clip from the play on (SCRIPT-S272)");
        require(animation.update(kStep) && !animation.enteredState() && animation.frame() > 0.0, "the update of state 0x50 starts one update later and owns the pose");
        for (int update = 3; update < 38; ++update) {
            require(animation.update(kStep), "the clip owns the pose until it ends");
            require(animation.currentNearEnd("player1") == std::optional<bool>(false), "play-once clip blocks the wait");
        }
        require(animation.update(kStep) && animation.currentNearEnd("player1") != std::optional<bool>(false),
                "update 38 (3800 ticks, 19 frames at 15 fps) sees kind 6: the pose is the last frame and the wait is done");
        require(animation.frame() == 18.0, "the pose is clamped to the last sample");
        require(!animation.inUserAnimation() && animation.enteredState() == std::optional<unsigned>(kLocomotionState),
                "the request for state 1 is applied at the end of the update that saw kind 6");
        require(!animation.update(kStep) && !animation.enteredState() && !animation.posesScriptClip(), "locomotion runs in the next update");
        require(!animation.currentNearEnd("player1"), "the wait completes at once after the hand-back");
    }
    {
        PlayerScriptAnimation animation(loader);
        animation.play(playOf("player1", "idle_minifig", true));
        require(!animation.update(kStep) && animation.update(kStep), "looping clip entered one update after the request");
        for (int update = 0; update < 200; ++update) require(animation.update(kStep), "a looping clip keeps the state");
        animation.stop("player1");
        require(animation.update(kStep) && animation.currentNearEnd("player1") != std::optional<bool>(false), "StopCharAnim makes the slot kind 6");
        require(!animation.update(kStep), "locomotion resumes after the stop");
    }
    {
        PlayerScriptAnimation animation(loader);
        animation.play(playOf("player1", "idle_minifig", true));
        animation.update(kStep);
        animation.update(kStep);
        animation.play(playOf("player1", "idle_minifig", true));
        require(animation.update(kStep) && !animation.enteredState(), "a request for the state the player is in does not run the enter function");
        VerticalState vertical;
        vertical.fallSpeed = 0.3f;
        vertical.fallDistance = 1.5f;
        applyScriptStateEnter(kUserAnimState, vertical);
        require(vertical.fallSpeed == 0.0f && vertical.fallDistance == 1.5f, "the enter of state 0x50 clears ctrl+0x2b8 only (0x25fbec)");
        applyScriptStateEnter(kLocomotionState, vertical);
        require(vertical.fallDistance == 0.0f, "the enter of state 1 clears ctrl+0x2b4 and ctrl+0x2b8 (0x1abbb0)");
    }
    {
        PlayerScriptAnimation animation(loader);
        animation.play(playOf("1-1_ai_donutlady", "idle_minifig", false));
        animation.play(playOf("player1", "idle_minifig", false, false));
        animation.play(playOf("player1", "missing_minifig", false));
        require(!animation.update(kStep) && !animation.currentNearEnd("player1"), "other instances, object animations and missing clips do not start the state");
        animation.stop("player1");
    }
    require(PlayerScriptAnimation::clipPath("class_generic", "Idle_Minifig") == kIdlePath, "clip path");
}

void rootMotion(const AnimationLoader& loader) {
    {
        PlayerScriptAnimation animation(loader);
        animation.play(playOf("player1", "combo_1_minifig", false));
        require(!animation.update(kStep) && animation.update(kStep), "state entered at the end of the first update, its update runs in the second");
        require(!animation.rootMotion(0), "the first update of a clip only stores the root sample");
        float dx = 0, dy = 0, dz = 0;
        int steps = 0;
        while (animation.update(kStep)) {
            const std::optional<ScriptRootStep> step = animation.rootMotion(0);
            if (!step) continue;
            dx += step->dx;
            dy += step->dy;
            dz += step->dz;
            require(!step->facing, "a clip without a turn leaves the facing alone");
            ++steps;
        }
        require(steps > 5, "one step per update of the clip");
        require(std::fabs(dx) < 1e-4f && std::fabs(dy) < 1e-4f, "combo_1 moves along z only");
        require(dz > 0.88f && dz < 0.93f, "the steps add up to the root z travel of the clip from frame 1, where the first 0x50 update samples (1.05684 from frame 0 to 14, archive scan; 0.90586 is this replica's own value, pinned as a regression bound)");
    }
    {
        PlayerScriptAnimation animation(loader);
        animation.play(playOf("player1", "bluebell_interrogated_intro_minifig", false));
        animation.update(kStep);
        std::int16_t facing = 0;
        float dz = 0;
        while (animation.update(kStep)) {
            const std::optional<ScriptRootStep> step = animation.rootMotion(facing);
            if (!step) continue;
            if (step->facing) facing = *step->facing;
            dz += step->dz;
        }
        require(facing > 16300 && facing < 16400, "the turn of 1.57125 rad (53 frames, archive scan) ends near 16389");
        require(dz > 0.0f && dz < 10.7f, "the walk bends with the turn, so z stays below the straight 10.63");
    }
    {
        LocomotionState motion;
        VerticalState vertical;
        CollisionWorld world;
        ScriptRootStep step;
        step.dz = 0.5f;
        step.facing = 16384;
        applyScriptRootStep(step, motion, vertical, world, kStep);
        require(motion.facing == 16384 && motion.lastIntent == 16384, "the turn writes ctrl+8 and ctrl+10");
    }
}
}

int main(int argc, char** argv) {
    try {
        if (argc == 1) {
            run([](const std::string& path) -> std::optional<chase::assets::Animation> {
                if (path == kIdlePath) return idleClip();
                return std::nullopt;
            });
            return 0;
        }
        if (argc != 3) {
            std::cerr << "usage: player-script-animation-test [FIB NAMES]\n";
            return 2;
        }
        if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
            std::cout << "player-script-animation-test: data not available, skipped\n";
            return 77;
        }
        chase::assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        run(archiveAnimationLoader(archive));
        rootMotion(archiveAnimationLoader(archive));
    } catch (const std::exception& error) {
        std::cerr << "player-script-animation-test: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
