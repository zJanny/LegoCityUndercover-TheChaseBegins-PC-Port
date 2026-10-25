// Ladders of lvl_01_cherrytreehills (src/runtime/ladder.cpp): the kind 56 objects from the level data and replays of the oracle runs of
// mission 1-3 (docs/re/player-traversal-ladders.md). All runs: New 3DS save state in front of the crane ladder of mission 1-3 (Chase in front of 1-4_Ladder01), Azahar
// without RPC on port 40401, scheduler step fixed to 1/30 (`dt 0.033333335 0.033333335`), rows are `trace N NAME mem` output of
// round 40 agent A (x, y, z of the player node after each update; the three loads of the state agreed). The clip ladder_dismount
// comes from the extracted RomFS; exits with 77 without it.
#include "assets/assets.hpp"
#include "runtime/ladder.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace chase;
using runtime::LadderExit;
using runtime::LadderPhase;

constexpr const char* kLevelFile = "levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl";
constexpr const char* kClamberClip = "models/char_minifig/class_generic/ladder_dismount_minifig.bfnanm";
constexpr float kDt = 0.033333335f;
constexpr float kRoof = 30.71885f;      // lad-up rows 138..152: the roof under the clamber end
// The roof edge: rows 129..132 pass below the roof height beside it, row 133 (0.10 along the facing from the snap point) is over it.
constexpr float kRoofEdge = 0.05f;
constexpr float kSnap[2] = {894.38904f, 1134.82788f};
constexpr float kPlatform = 23.62416f;  // adrop-end rows 16..: the platform Chase lands on in front of the ladder

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
bool near(float a, float b, float tolerance) { return std::fabs(a - b) <= tolerance; }

struct Row { int frame; float x, y, z; };

// lad-up rows 120..152 (stick (0, 120) from row 16): the top at row 121 and the clamber; row 152 is state 1.
const Row kClamber[] = {
    {120, 894.38904f, 28.82253f, 1134.82788f}, {121, 894.38904f, 28.82253f, 1134.82788f}, {122, 894.38904f, 28.82253f, 1134.82788f},
    {123, 894.38904f, 28.82253f, 1134.82788f}, {124, 894.38904f, 28.82253f, 1134.82788f}, {125, 894.38904f, 28.82253f, 1134.82788f},
    {126, 894.38904f, 28.82253f, 1134.82788f}, {127, 894.38306f, 29.01159f, 1134.82495f}, {128, 894.37708f, 29.20065f, 1134.82202f},
    {129, 894.36804f, 29.57315f, 1134.81738f}, {130, 894.35901f, 29.94565f, 1134.81274f}, {131, 894.35809f, 30.26721f, 1134.81360f},
    {132, 894.37354f, 30.58877f, 1134.83081f}, {133, 894.45001f, 30.80534f, 1134.91077f}, {134, 894.52649f, 31.02190f, 1134.99072f},
    {135, 894.60474f, 31.00159f, 1135.07202f}, {136, 894.68298f, 30.98127f, 1135.15332f}, {137, 894.72144f, 30.89627f, 1135.19702f},
    {138, 894.75989f, 30.71885f, 1135.24072f}, {139, 894.78113f, 30.71885f, 1135.26550f}, {140, 894.80237f, 30.71885f, 1135.29028f},
    {141, 894.80237f, 30.71885f, 1135.29028f}, {142, 894.80237f, 30.71885f, 1135.29028f}, {143, 894.80237f, 30.71885f, 1135.29028f},
    {144, 894.80237f, 30.71885f, 1135.29028f}, {145, 894.80237f, 30.71885f, 1135.29028f}, {146, 894.80237f, 30.71885f, 1135.29028f},
    {147, 894.80237f, 30.71885f, 1135.29028f}, {148, 894.80237f, 30.71885f, 1135.29028f}, {149, 894.80237f, 30.71885f, 1135.29028f},
    {150, 894.80237f, 30.71885f, 1135.29028f}, {151, 894.80237f, 30.71885f, 1135.29028f}, {152, 894.80237f, 30.71885f, 1135.29028f}};

runtime::LadderInput baseInput(const assets::RootMotion& clamber, float fps, unsigned frames) {
    runtime::LadderInput input;
    input.dt = kDt;
    input.clamber = &clamber;
    input.clamberRise = clamber.translation.back()[1];
    input.clamberFramesPerSecond = fps;
    input.clamberFrames = frames;
    input.floorBelow = [](const float position[3], float length) { return position[1] - length <= kPlatform && position[1] >= kPlatform; };
    input.floorHeight = [](const float position[3], float reach) -> std::optional<float> {
        const float along = ((position[0] - kSnap[0]) + (position[2] - kSnap[1])) * 0.70710678f;
        if (along > kRoofEdge && std::fabs(position[1] - kRoof) <= reach) return kRoof;
        return std::nullopt;
    };
    return input;
}

void follow(runtime::LadderInput& input, const runtime::LadderStep& step) {
    for (int a = 0; a < 3; ++a) input.position[a] = step.position[a];
    input.facing = step.facing;
}

// The A press of row 3 and the mount rows 3..6 (lad-a, lad-mount): the player at (894.07965, 23.62261, 1134.04700) facing 9922.
void mountByUse(const std::vector<runtime::LadderObject>& ladders, int index, runtime::LadderClimber& climber, runtime::LadderInput& input) {
    input.position[0] = 894.07965f;
    input.position[1] = 23.62261f;
    input.position[2] = 1134.04700f;
    input.facing = 9922;
    require(runtime::ladderUseTest(ladders, input.position, input.facing, input.clamberRise, nullptr) == index, "the A press uses 1-4_Ladder01");
    const Row mount[] = {{3, 894.15698f, 23.62261f, 1134.24219f}, {4, 894.23438f, 23.62261f, 1134.43750f}, {5, 894.31171f, 23.62261f, 1134.63269f},
                         {6, 894.38904f, 23.62261f, 1134.82788f}};
    runtime::LadderStep step = runtime::ladderMount(climber, ladders, index, input, true);
    for (const Row& row : mount) {
        if (row.frame > 3) step = runtime::ladderUpdate(climber, ladders, input);
        require(near(step.position[0], row.x, 2e-4f) && near(step.position[1], row.y, 2e-4f) && near(step.position[2], row.z, 2e-4f),
                "mount row " + std::to_string(row.frame));
        require(step.facing == (row.frame < 6 ? 9922 : 8192), "facing in mount row " + std::to_string(row.frame));
        follow(input, step);
    }
    require(climber.phase == LadderPhase::Idle && near(climber.height, 5.10641f, 1e-4f), "idle at ctrl+0x24c 5.10641 (row 7)");
}
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: ladder-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "ladder-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        const assets::FuseEntry* entry = archive.find(kLevelFile);
        require(entry != nullptr, std::string("not in archive: ") + kLevelFile);
        const assets::Level level = assets::parseLevel(archive.read(*entry));
        const std::vector<runtime::LadderObject> ladders = runtime::loadLadders(level, assets::archiveClassLookup(archive));
        // Level scan (C++ loaders): 10 placed objects of kind 56 in lvl_01 (1-4_Ladder01..05, 1-4_Showroom_Ladder, 1-6_Ladder_01, FP_Ladder_01..03).
        require(ladders.size() == 10, "ladders: " + std::to_string(ladders.size()));
        int index = -1;
        for (std::size_t i = 0; i < ladders.size(); ++i)
            if (ladders[i].instance == "1-4_ladder01") index = int(i);
        require(index >= 0, "1-4_Ladder01 is a ladder");
        const runtime::LadderObject& ladder = ladders[std::size_t(index)];
        // Class props/prop_crane_ladderlarge: snap offset 0.45, CollisionBound centre y 5.98798 half 6.18749 (top 12.17547 over the origin,
        // the roof at 30.69), UseBound centre (0, 5.99124, 0.54) half (0.8, 6.19124, 0.8); placed at (894.707, 18.5162, 1135.15) yaw 225.
        require(near(ladder.snapOffset, 0.45f, 1e-6f) && ladder.canDismount && !ladder.furled && ladder.faceTowards == 0, "class parameters");
        require(near(ladder.top(), 12.17547f, 1e-4f) && near(ladder.useCentre[2], 0.54f, 1e-5f), "bounds");
        require(ladder.heading == -24576, "heading of yaw 225: " + std::to_string(ladder.heading) + " forward " + std::to_string(ladder.forward[0]) + " " + std::to_string(ladder.forward[2]));

        const assets::FuseEntry* clipEntry = archive.find(kClamberClip);
        require(clipEntry != nullptr, std::string("not in archive: ") + kClamberClip);
        const assets::Animation clip = assets::parseAnimation(archive.read(*clipEntry));
        const std::optional<assets::RootMotion> clamber = assets::rootMotion(clip);
        require(clamber && !clamber->translation.empty(), "ladder_dismount has a root track");
        // ctrl+0x250 = 0x3fea8f5c (1.8325) in every recorded row: the root y of ladder_dismount at its last frame (0x221a54).
        require(near(clamber->translation.back()[1], 1.8325f, 1e-5f), "clamber rise 1.8325");
        const float fps = assets::animationFramesPerSecond(clip);
        const unsigned frames = assets::animationLength(clip);

        // Run lad-up: mount, idle rows 7..15, stick (0, 120) from row 16 (intent -4936, camera yaw -0.47174: quadrant 0).
        {
            runtime::LadderClimber climber;
            runtime::LadderInput input = baseInput(*clamber, fps, frames);
            mountByUse(ladders, index, climber, input);
            for (int frame = 7; frame <= 15; ++frame) follow(input, runtime::ladderUpdate(climber, ladders, input));
            input.stickActive = true;
            input.stickHeading = -4936;
            input.cameraYaw = -0.47174f;
            runtime::LadderStep step = runtime::ladderUpdate(climber, ladders, input);
            require(climber.phase == LadderPhase::Up && near(step.position[1], 23.62261f, 1e-5f), "row 16: the climb is asked, no move");
            follow(input, step);
            const Row climb[] = {{17, 0, 23.67261f, 0}, {18, 0, 23.72260f, 0}, {65, 0, 26.07257f, 0}, {119, 0, 28.77253f, 0}};
            std::size_t next = 0;
            for (int frame = 17; frame <= 119; ++frame) {
                step = runtime::ladderUpdate(climber, ladders, input);
                follow(input, step);
                if (next < std::size(climb) && climb[next].frame == frame) {
                    require(near(step.position[1], climb[next].y, 1e-4f) && near(step.position[0], 894.38904f, 2e-4f), "climb row " + std::to_string(frame));
                    ++next;
                }
            }
            for (const Row& row : kClamber) {
                step = runtime::ladderUpdate(climber, ladders, input);
                follow(input, step);
                // TRAV-S35: in row 131 the original's move 0x215804 stops the forward motion at the roof edge (y below the roof), the port
                // has no wall contact in the clamber, so from there x and z run one update's motion (0.023) ahead.
                const float flat = row.frame < 131 ? 2e-4f : 0.025f;
                require(near(step.position[0], row.x, flat) && near(step.position[1], row.y, 2e-4f) && near(step.position[2], row.z, flat),
                        "clamber row " + std::to_string(row.frame) + ": " + std::to_string(step.position[0]) + " " + std::to_string(step.position[1]) +
                            " " + std::to_string(step.position[2]));
                if (row.frame == 121) require(climber.phase == LadderPhase::Clamber && near(climber.height, 10.35643f, 1e-4f), "top in row 121");
                require((step.exit == LadderExit::Ground) == (row.frame == 152), "state 1 in row 152, row " + std::to_string(row.frame));
            }
        }

        // Run down (the mount takes rows 1..4 of this run): stick (0, 120) from row 7, stick (0, -120) from row 17 (intent 27871): row 17
        // still climbs, rows 18..24 go down 0.05, the ray of 0x22f134 meets the platform in row 25 (no move, facing 8192 - 0x3000), state 5 from row 26.
        {
            runtime::LadderClimber climber;
            runtime::LadderInput input = baseInput(*clamber, fps, frames);
            mountByUse(ladders, index, climber, input);
            input.stickActive = true;
            input.stickHeading = -5206;
            input.cameraYaw = -0.49293f;
            for (int frame = 7; frame <= 16; ++frame) follow(input, runtime::ladderUpdate(climber, ladders, input));
            require(near(input.position[1], 24.07260f, 1e-4f), "row 16 at 24.07260");
            input.stickHeading = 27871;
            input.cameraYaw = -0.46866f;
            runtime::LadderStep step = runtime::ladderUpdate(climber, ladders, input);
            require(near(step.position[1], 24.12260f, 1e-4f) && climber.phase == LadderPhase::Down, "row 17 climbs once more");
            follow(input, step);
            for (int frame = 18; frame <= 24; ++frame) {
                step = runtime::ladderUpdate(climber, ladders, input);
                follow(input, step);
                require(near(step.position[1], 24.12260f - 0.05f * float(frame - 17), 1e-4f), "down row " + std::to_string(frame));
            }
            step = runtime::ladderUpdate(climber, ladders, input);
            require(step.exit == LadderExit::Fall && near(step.position[1], 23.77260f, 1e-4f) && step.facing == -4096 && step.targetHeading == -24576,
                    "row 25: off at the platform, ctrl+0xa -24576");
        }

        // Run adrop (mount rows 1..4, stick (0, 120) from row 7): A in row 13 while climbing (rows 8..12 up): state 5 in that update without
        // the step, facing 8192 - 0x3000.
        {
            runtime::LadderClimber climber;
            runtime::LadderInput input = baseInput(*clamber, fps, frames);
            mountByUse(ladders, index, climber, input);
            input.stickActive = true;
            input.stickHeading = -5206;
            input.cameraYaw = -0.49293f;
            for (int frame = 7; frame <= 12; ++frame) follow(input, runtime::ladderUpdate(climber, ladders, input));
            require(near(input.position[1], 23.87260f, 1e-4f), "row 12 at 23.87260");
            input.aPressed = true;
            const runtime::LadderStep step = runtime::ladderUpdate(climber, ladders, input);
            require(step.exit == LadderExit::Fall && near(step.position[1], 23.87260f, 1e-4f) && step.facing == -4096 && step.targetHeading == -24576,
                    "row 13: A drop, ctrl+0xa -24576");
        }

        // Run bjump (mount rows 1..4, idle rows 5..9): B in row 10: state 4 in that update, no move, facing kept (the jump rises 0.27306 in row 11).
        {
            runtime::LadderClimber climber;
            runtime::LadderInput input = baseInput(*clamber, fps, frames);
            mountByUse(ladders, index, climber, input);
            for (int frame = 7; frame <= 9; ++frame) follow(input, runtime::ladderUpdate(climber, ladders, input));
            input.bPressed = true;
            const runtime::LadderStep step = runtime::ladderUpdate(climber, ladders, input);
            require(step.exit == LadderExit::Jump && near(step.position[1], 23.62261f, 1e-5f) && step.facing == 8192, "row 10: B jump");
        }
    } catch (const std::exception& error) {
        std::cerr << "ladder-real-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "ladder-real-test: ok\n";
    return 0;
}
