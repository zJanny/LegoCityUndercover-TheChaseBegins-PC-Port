// The fence vault (src/runtime/fence_vault.cpp) with the fences of lvl_01_cherrytreehills and the clip vault_fence (docs/re/player-traversal-fence-vault.md).
// The player position and forward are those of the original in front of the fence that has to be climbed in the mission 1-3 lawn mower rescue, recorded
// from a save state (read by the lead, 2026-10-27, Azahar New 3DS, OpenGL); every other value comes from the level and clip data. Exits with 77 without
// the extracted RomFS.
#include "assets/assets.hpp"
#include "runtime/fence_vault.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace chase;

constexpr const char* kLevelFile = "levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl";
constexpr const char* kClip = "models/char_minifig/class_generic/vault_fence_minifig.bfnanm";
constexpr float kPlayer[3] = {1021.043f, 32.849f, 1019.872f};
constexpr std::int16_t kFacing = 11384;  // int(atan2(0.887, 0.461) x 10430.378), the recorded forward

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
bool near(float a, float b, float tolerance) { return std::fabs(a - b) <= tolerance; }

}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: fence-vault-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "fence-vault-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        const assets::FuseEntry* entry = archive.find(kLevelFile);
        require(entry != nullptr, std::string("not in archive: ") + kLevelFile);
        const assets::Level level = assets::parseLevel(archive.read(*entry));
        const std::vector<runtime::FenceVaultObject> fences = runtime::loadFenceVaults(level, assets::archiveClassLookup(archive));
        require(fences.size() == 24, "24 kind 189 fences in lvl_01, got " + std::to_string(fences.size()));
        const auto none = [](const runtime::FenceVaultObject&) { return false; };
        const int taken = runtime::fenceVaultOffer(fences, kPlayer, kFacing, none);
        require(taken >= 0 && fences[std::size_t(taken)].instance == "1-3_parkour_fence_climb", "the B press goes to 1-3_Parkour_fence_climb");
        const runtime::FenceVaultObject& fence = fences[std::size_t(taken)];
        require(near(fence.position()[0], 1021.4955f, 1e-3f) && near(fence.position()[1], 36.3025f, 1e-3f) && near(fence.position()[2], 1021.0831f, 1e-3f),
                "fence origin (its top)");
        require(near(fence.forward()[0], 0.95212f, 1e-4f) && near(fence.forward()[2], 0.30571f, 1e-4f), "yaw 72.1989");
        require(near(fence.useCentre[1], -3.3209f, 1e-4f) && near(fence.useHalf[0], 3.0848f, 1e-4f) && near(fence.useHalf[1], 3.3269f, 1e-4f) &&
                    near(fence.useHalf[2], 3.0f, 1e-4f),
                "class UseBound");
        require(fence.vaultType == 0 && fence.faceTowards == 0 && runtime::fenceVaultClip(fence) == runtime::kAnimationVaultFence, "VaultType 0, no facing test");

        const runtime::FenceVaultApproach approach = runtime::fenceVaultApproach(fence, kPlayer, kFacing);
        require(near(approach.position[0], 1020.3299f, 2e-3f) && near(approach.position[1], 32.849f, 1e-4f) && near(approach.position[2], 1019.6430f, 2e-3f),
                "use point 1.55 in front of the fence line");
        require(approach.heading == 13143 && near(approach.seconds, 0.1f, 1e-7f), "heading across the fence, 0.1 s (the player faces it)");

        const assets::FuseEntry* clipEntry = archive.find(kClip);
        require(clipEntry != nullptr, std::string("not in archive: ") + kClip);
        const assets::Animation clip = assets::parseAnimation(archive.read(*clipEntry));
        const std::optional<assets::RootMotion> motion = assets::rootMotion(clip);
        require(motion && assets::animationLength(clip) == 29 && near(assets::animationFramesPerSecond(clip), 15.0f, 1e-4f), "vault_fence: 29 frames at 15");
        require(near(motion->maximum[1] - motion->minimum[1], 4.58f, 1e-4f) && near(motion->translation.back()[2], 3.49f, 1e-4f), "root rise and travel");
        require(near(runtime::fenceVaultRiseScale(fence, kPlayer[1], *motion), 0.75404f, 1e-4f), "rise scale (36.3025 - 32.849) / 4.58");

        // Oracle replay, RPC 20261030-qolstores-F-1 (servant 20261030-qolstores-servant, 2026-10-30, Azahar New 3DS, OpenGL, RPC; recorded from a save state in
        // front of the fence that has to be climbed in the mission 1-3 lawn mower rescue; scheduler step dt 0.033333335, B held for one update u0; two loads
        // agreed). Rows: the player node position after update u, 3 decimals. The lerp's ground contact (y 32.799 at u0) is not ported (TRAV-S38).
        constexpr float dt = 0.033333335f;
        const runtime::FenceVaultClipData data{&*motion, 15.0f, 28.0f};
        runtime::FenceVaulter vaulter;
        runtime::FenceVaultStep step = vaulter.start(fences, std::size_t(taken), kPlayer, kFacing, dt);
        require(near(step.position[0], 1020.806f, 1.5e-3f) && near(step.position[2], 1019.796f, 1.5e-3f) && step.facing == 13143, "u0: a third of the way, facing turned");
        struct Row { int u; float x, y, z; };
        const Row lerpRows[] = {{1, 1020.568f, 32.849f, 1019.720f}, {2, 1020.330f, 32.849f, 1019.643f}, {3, 1020.330f, 32.849f, 1019.643f}};
        for (const Row& row : lerpRows) {
            step = vaulter.update(fences, data, dt);
            require(near(step.position[0], row.x, 1.5e-3f) && near(step.position[1], row.y, 1.5e-3f) && near(step.position[2], row.z, 1.5e-3f) &&
                        vaulter.state() == runtime::kPlayerStateFenceVaultLerp,
                    "USE LERP row u" + std::to_string(row.u));
        }
        const float climbY[] = {32.849f, 32.930f, 33.011f, 33.188f, 33.366f, 33.545f, 33.724f, 33.805f, 33.886f, 33.886f, 33.886f, 33.888f,
                                33.890f, 33.892f, 33.893f, 33.893f, 33.893f, 34.133f, 34.372f, 34.862f, 35.352f, 35.760f, 36.167f};
        std::vector<Row> rows;
        for (int i = 0; i < 23; ++i) rows.push_back({4 + i, 1020.330f, climbY[i], 1019.643f});
        rows.push_back({27, 1020.337f, 36.235f, 1019.646f});
        rows.push_back({28, 1020.344f, 36.302f, 1019.648f});
        const float across[][2] = {{1020.404f, 1019.667f}, {1020.463f, 1019.686f}, {1020.570f, 1019.720f}, {1020.677f, 1019.755f}, {1020.823f, 1019.802f},
                                   {1020.968f, 1019.848f}, {1021.139f, 1019.903f}, {1021.311f, 1019.958f}, {1021.503f, 1020.020f}, {1021.696f, 1020.082f},
                                   {1021.894f, 1020.145f}, {1022.091f, 1020.209f}, {1022.289f, 1020.272f}, {1022.487f, 1020.336f}, {1022.670f, 1020.395f},
                                   {1022.853f, 1020.453f}, {1023.013f, 1020.505f}, {1023.172f, 1020.556f}, {1023.301f, 1020.597f}, {1023.429f, 1020.638f},
                                   {1023.512f, 1020.665f}, {1023.596f, 1020.692f}, {1023.624f, 1020.701f}, {1023.653f, 1020.710f}};
        for (int i = 0; i < 24; ++i) rows.push_back({29 + i, across[i][0], 36.302f, across[i][1]});
        const float downY[] = {36.201f, 36.099f, 35.844f, 35.590f, 35.247f, 34.904f, 34.542f};
        for (int i = 0; i < 7; ++i) rows.push_back({53 + i, 1023.653f, downY[i], 1020.710f});
        for (const Row& row : rows) {
            step = vaulter.update(fences, data, dt);
            require(!step.ended && vaulter.state() == runtime::kPlayerStateFenceVault, "state 326 at u" + std::to_string(row.u));
            require(row.u != 4 || (step.entered && *step.entered == runtime::kPlayerStateFenceVault && step.facing == 13143), "state 326 starts at u4");
            require(near(step.position[0], row.x, 1.5e-3f) && near(step.position[1], row.y, 1.5e-3f) && near(step.position[2], row.z, 1.5e-3f),
                    "state 326 row u" + std::to_string(row.u) + ": " + std::to_string(step.position[0]) + ' ' + std::to_string(step.position[1]) + ' ' +
                        std::to_string(step.position[2]));
        }
        step = vaulter.update(fences, data, dt);
        require(step.ended && !vaulter.active(), "u60: state 5");
        require(near(step.fallSpeed, 0.3619f, 1e-3f) && near(step.position[1], 33.818f, 1.5e-3f), "u60: fall speed 0.3619 and its first fall step");
        std::cout << "fence-vault-real-data: ok\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "fence-vault-real-data: " << error.what() << '\n';
        return 1;
    }
}
