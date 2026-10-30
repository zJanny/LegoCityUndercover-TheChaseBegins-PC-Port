// The fence vault (src/runtime/fence_vault.cpp) with a made-up fence and clip: the zone test 0x221570, the B press gate 0x1fee94, the use point of the
// handler 0x19de38, the rise scale of 0x2769dc and the USE LERP / state 326 sequence with hand-computed values (docs/re/player-traversal-fence-vault.md).
#include "runtime/fence_vault.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase;
using namespace chase::runtime;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
bool near(float a, float b, float tolerance) { return std::fabs(a - b) <= tolerance; }

FenceVaultObject makeFence() {
    FenceVaultObject fence;
    fence.instance = "test_fence";
    fence.matrix = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    fence.hasUseBound = true;
    fence.useCentre[1] = -1.0f;
    fence.useHalf[0] = 2.0f;
    fence.useHalf[1] = 1.0f;
    fence.useHalf[2] = 1.0f;
    return fence;
}

assets::RootMotion makeClip() {
    assets::RootMotion motion;
    motion.translation = {{0, 0, 0}, {0, 1, 0}, {0, 2, 1}, {0, 2, 2}, {0, 1, 3}, {0, 0.5f, 3}};
    motion.minimum = {0, 0, 0};
    motion.maximum = {0, 2, 3};
    return motion;
}

}

int main() {
    try {
        std::vector<FenceVaultObject> fences = {makeFence()};
        const auto none = [](const FenceVaultObject&) { return false; };
        const float behind[3] = {0.5f, -0.5f, -0.8f}, outside[3] = {0.5f, -0.5f, -1.1f}, above[3] = {0.5f, 0.1f, -0.8f};
        require(fenceVaultZoneHolds(fences[0], behind, 0) && fenceVaultZoneHolds(fences[0], behind, std::int16_t(0x8000)), "zone holds, FaceTowards 0");
        require(!fenceVaultZoneHolds(fences[0], outside, 0) && !fenceVaultZoneHolds(fences[0], above, 0), "zone fails beyond its half and above the origin");
        fences[0].faceTowards = 1;
        require(fenceVaultZoneHolds(fences[0], behind, 0) && !fenceVaultZoneHolds(fences[0], behind, std::int16_t(0x8000)), "FaceTowards 1: face the fence");
        fences[0].faceTowards = 0;
        require(fenceVaultOffer(fences, behind, 0, none) == 0, "the B press is taken in the zone");
        require(fenceVaultOffer(fences, outside, 0, none) == -1, "no fence outside the zones");
        require(fenceVaultOffer(fences, behind, 0, [](const FenceVaultObject&) { return true; }) == -1, "a disabled fence is not asked");

        // Use point: the foot on the fence line (x 0.5, the player's y), 1.55 to the player's side; facing the fence the lerp takes 0.1 s, else 0.2 s.
        const FenceVaultApproach facing = fenceVaultApproach(fences[0], behind, 0);
        require(near(facing.position[0], 0.5f, 1e-6f) && near(facing.position[1], -0.5f, 1e-6f) && near(facing.position[2], -1.55f, 1e-6f),
                "use point on the -z side");
        require(facing.heading == 0 && near(facing.seconds, 0.1f, 1e-7f), "heading across the fence, 0.1 s when facing it");
        require(near(fenceVaultApproach(fences[0], behind, std::int16_t(0x8000)).seconds, 0.2f, 1e-7f), "0.2 s when facing away");
        const float front[3] = {0.5f, -0.5f, 0.8f};
        const FenceVaultApproach other = fenceVaultApproach(fences[0], front, 0);
        require(near(other.position[2], 1.55f, 1e-6f) && std::abs(int(other.heading)) >= 32767, "use point and heading on the +z side");
        fences[0].vaultType = 1;
        require(near(fenceVaultApproach(fences[0], behind, 0).position[2], -1.0f, 1e-6f) && fenceVaultClip(fences[0]) == kAnimationVault, "VaultType 1");
        fences[0].vaultType = 0;
        require(fenceVaultClip(fences[0]) == kAnimationVaultFence, "other VaultType: vault_fence");

        const assets::RootMotion motion = makeClip();
        require(near(fenceVaultRiseScale(fences[0], -0.5f, motion), 0.25f, 1e-7f), "rise scale (0 - -0.5) / 2");

        // Sequence at dt 1/16 (exact sums): USE LERP from the press (0.625 of the way per update), the heading set when the lerp is at 1, state 326 in
        // the update after it; its k-th update shows clip frame k / 2 (8 per second), the update at frame 4 (length 5 minus 1) ends it and takes the first
        // fall step of the height lost in that update (the order of RPC 20261030-qolstores-F-1).
        constexpr float dt = 0.0625f;
        const FenceVaultClipData clip{&motion, 8.0f, 4.0f};
        FenceVaulter vaulter;
        const float start[3] = {0.5f, -0.5f, -0.8f};
        FenceVaultStep step = vaulter.start(fences, 0, start, std::int16_t(-3400), dt);
        require(step.entered && *step.entered == kPlayerStateFenceVaultLerp && vaulter.active(), "USE LERP starts in the press update");
        require(step.facing == std::int16_t(-200) && step.animation == 0, "turn by at most 3200");
        require(near(step.position[2], -0.8f - 0.75f * 0.625f, 1e-5f), "first lerp step");
        int lerpUpdates = 0;
        for (; lerpUpdates < 10; ++lerpUpdates) {
            step = vaulter.update(fences, clip, dt);
            if (step.entered) break;
        }
        require(step.entered && *step.entered == kPlayerStateFenceVault && lerpUpdates == 2, "state 326 after the lerp");
        require(step.facing == 0 && step.animation == kAnimationVaultFence, "facing the heading, clip vault_fence");
        require(near(step.position[1], -0.5f, 1e-6f) && near(step.position[2], -1.55f, 1e-6f), "the first update of state 326 does not move");
        step = vaulter.update(fences, clip, dt);
        require(near(step.position[1], -0.5f + 0.5f * 0.25f, 1e-5f) && near(step.position[2], -1.55f, 1e-5f), "update 1: frame 0.5");
        for (int k = 2; k <= 7; ++k) {
            step = vaulter.update(fences, clip, dt);
            require(!step.ended, "clip still running at update " + std::to_string(k));
        }
        require(near(step.position[1], -0.125f, 1e-5f) && near(step.position[2], 0.95f, 1e-5f), "update 7: frame 3.5");
        step = vaulter.update(fences, clip, dt);
        require(step.ended && !vaulter.active(), "state 5 after the clip");
        require(near(step.position[1], -0.375f, 1e-5f) && near(step.position[2], 1.45f, 1e-5f) && near(step.fallSpeed, 0.125f, 1e-5f),
                "update 8: frame 4, then one fall step of 0.125");
        std::cout << "fence-vault: ok\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "fence-vault: " << error.what() << '\n';
        return 1;
    }
}
