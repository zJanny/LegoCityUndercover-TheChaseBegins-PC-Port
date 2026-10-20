// Poles and drain pipes of lvl_01_cherrytreehills (src/runtime/pole.cpp): the pole objects from the level data and replays of the
// oracle runs of mission 1-2 (docs/re/player-traversal.md, entry of 2026-10-18 on the pole). All runs: New 3DS states
// `m12-10-pole-climb` (on the pipe 1-5_Drain_Pipe05) and `m12-10a-pole-foot` (standing 0.77 m from it), scheduler step fixed to 1/30
// (`dt 0.033333335 0.033333335`), stick (0, 120) = up, rows are `trace N NAME mem` output of round 21 agent A (x, y, z of the
// player node after each update). The clips vpole_up / vpole_down / vpole_dismount come from the extracted RomFS; exits with 77 without it.
#include "assets/assets.hpp"
#include "runtime/pole.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace chase;
using runtime::PoleExit;
using runtime::PolePhase;

constexpr const char* kLevelFile = "levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl";
constexpr const char* kClipDirectory = "models/char_minifig/class_generic/vpole_";
constexpr float kDt = 0.033333335f;
constexpr float kCameraYaw = -0.048126574605703354f;  // *0x376894 = 0xbd45205f, read in m12-10

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
bool near(float a, float b, float tolerance) { return std::fabs(a - b) <= tolerance; }

struct Row { int frame; float x, y, z; };

// pole-up.csv rows 96..129 (oracle run `stick 0 120`, `trace 200 pole-up mem` from m12-10-pole-climb): the dismount from the pipe top.
const Row kDismount[] = {
    {96, 940.54224f, 65.32635f, 683.41003f},
    {97, 940.54224f, 65.34510f, 683.41003f},
    {98, 940.54224f, 65.34510f, 683.41003f},
    {99, 940.54224f, 65.34510f, 683.41003f},
    {100, 940.55505f, 65.53323f, 683.39990f},
    {101, 940.56787f, 65.72135f, 683.38977f},
    {102, 940.58759f, 66.09198f, 683.37354f},
    {103, 940.60730f, 66.46261f, 683.35730f},
    {104, 940.58936f, 66.78323f, 683.36694f},
    {105, 940.57141f, 67.10386f, 683.37659f},
    {106, 940.47601f, 67.32073f, 683.43268f},
    {107, 940.38062f, 67.53761f, 683.48877f},
    {108, 940.30878f, 67.46495f, 683.53070f},
    {109, 940.23694f, 67.46495f, 683.57263f},
    {110, 940.10504f, 67.45404f, 683.66827f},
    {111, 939.97314f, 67.45404f, 683.76392f},
    {112, 939.88324f, 67.45404f, 683.83356f},
    {113, 939.79333f, 67.45404f, 683.90320f},
    {114, 939.74921f, 67.46576f, 683.92688f},
    {115, 939.70508f, 67.46576f, 683.95056f},
    {116, 939.66370f, 67.46576f, 683.96918f},
    {117, 939.62231f, 67.46576f, 683.98779f},
    {118, 939.58539f, 67.46576f, 684.00293f},
    {119, 939.54846f, 67.46576f, 684.01807f},
    {120, 939.51801f, 67.46576f, 684.02881f},
    {121, 939.48755f, 67.46576f, 684.03955f},
    {122, 939.46521f, 67.46576f, 684.04675f},
    {123, 939.44287f, 67.46576f, 684.05396f},
    {124, 939.43097f, 67.46576f, 684.05731f},
    {125, 939.41907f, 67.46576f, 684.06067f},
    {126, 939.41742f, 67.46576f, 684.06097f},
    {127, 939.41577f, 67.46576f, 684.06128f},
    {128, 939.41577f, 67.46576f, 684.06128f},
    {129, 939.41577f, 67.46576f, 684.06128f}
};
// pole-up2.csv rows 16..38 (idle for 12 updates, then stick up for 24): the climb up, first update after the idle state.
const Row kUp[] = {
    {16, 940.54224f, 60.23748f, 683.41003f},
    {17, 940.54224f, 60.29373f, 683.41003f},
    {18, 940.54224f, 60.34967f, 683.41003f},
    {19, 940.54224f, 60.40561f, 683.41003f},
    {20, 940.54224f, 60.46186f, 683.41003f},
    {21, 940.54224f, 60.51811f, 683.41003f},
    {22, 940.54224f, 60.57436f, 683.41003f},
    {23, 940.54224f, 60.63061f, 683.41003f},
    {24, 940.54224f, 60.68655f, 683.41003f},
    {25, 940.54224f, 60.74249f, 683.41003f},
    {26, 940.54224f, 60.79874f, 683.41003f},
    {27, 940.54224f, 60.85499f, 683.41003f},
    {28, 940.54224f, 60.91124f, 683.41003f},
    {29, 940.54224f, 60.96749f, 683.41003f},
    {30, 940.54224f, 61.02343f, 683.41003f},
    {31, 940.54224f, 61.07937f, 683.41003f},
    {32, 940.54224f, 61.13562f, 683.41003f},
    {33, 940.54224f, 61.19188f, 683.41003f},
    {34, 940.54224f, 61.24781f, 683.41003f},
    {35, 940.54224f, 61.30375f, 683.41003f},
    {36, 940.54224f, 61.36000f, 683.41003f},
    {37, 940.54224f, 61.41626f, 683.41003f},
    {38, 940.54224f, 61.47219f, 683.41003f}
};
// pole-down.csv rows 52..71 (stick (0, -120) held from 61.47219 down): the climb down and the exit in row 71.
const Row kDown[] = {
    {52, 940.54224f, 61.31376f, 683.41003f},
    {53, 940.54224f, 61.15532f, 683.41003f},
    {54, 940.54224f, 60.99689f, 683.41003f},
    {55, 940.54224f, 60.83845f, 683.41003f},
    {56, 940.54224f, 60.68002f, 683.41003f},
    {57, 940.54224f, 60.52158f, 683.41003f},
    {58, 940.54224f, 60.36314f, 683.41003f},
    {59, 940.54224f, 60.20471f, 683.41003f},
    {60, 940.54224f, 60.04596f, 683.41003f},
    {61, 940.54224f, 59.88721f, 683.41003f},
    {62, 940.54224f, 59.72878f, 683.41003f},
    {63, 940.54224f, 59.57034f, 683.41003f},
    {64, 940.54224f, 59.41190f, 683.41003f},
    {65, 940.54224f, 59.25347f, 683.41003f},
    {66, 940.54224f, 59.09503f, 683.41003f},
    {67, 940.54224f, 58.93660f, 683.41003f},
    {68, 940.54224f, 58.77816f, 683.41003f},
    {69, 940.54224f, 58.61972f, 683.41003f},
    {70, 940.54224f, 58.46129f, 683.41003f},
    {71, 941.11188f, 58.46129f, 683.05731f}
};
// foot-walk.csv rows 3..10 (m12-10a-pole-foot, stick (-100, 60) held, no button): the walk-in and the mount.
const Row kFoot[] = {
    {3, 941.10773f, 58.33629f, 683.05060f},
    {4, 941.10321f, 58.33629f, 683.04327f},
    {5, 941.09833f, 58.33629f, 683.03540f},
    {6, 940.97473f, 58.33629f, 683.11865f},
    {7, 940.85120f, 58.33629f, 683.20190f},
    {8, 940.72760f, 58.33629f, 683.28516f},
    {9, 940.60400f, 58.33629f, 683.36841f},
    {10, 940.54224f, 58.33629f, 683.41003f}
};

struct Clip {
    assets::Animation animation;
    std::optional<assets::RootMotion> motion;
};

Clip load(assets::FuseArchive& archive, const char* name) {
    const std::string path = std::string(kClipDirectory) + name + "_minifig.bfnanm";
    const assets::FuseEntry* entry = archive.find(path);
    require(entry != nullptr, "not in archive: " + path);
    Clip clip;
    clip.animation = assets::parseAnimation(archive.read(*entry));
    clip.motion = assets::rootMotion(clip.animation);
    require(clip.motion.has_value(), path + " has a root track");
    return clip;
}
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: pole-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "pole-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        const assets::FuseEntry* entry = archive.find(kLevelFile);
        require(entry != nullptr, std::string("not in archive: ") + kLevelFile);
        const assets::Level level = assets::parseLevel(archive.read(*entry));
        const std::vector<runtime::PoleObject> poles = runtime::loadPoles(level, assets::archiveClassLookup(archive));
        // Level scan: 12 objects of ObjectType 187 in lvl_01.
        require(poles.size() == 12, "poles: " + std::to_string(poles.size()));
        int index = -1;
        for (std::size_t i = 0; i < poles.size(); ++i)
            if (poles[i].instance == "1-5_drain_pipe05") index = int(i);
        require(index >= 0, "1-5_Drain_Pipe05 is a pole");
        const runtime::PoleObject& pole = poles[std::size_t(index)];
        // Oracle read of the object at 0x08b9a720 in m12-10: +0x64 = +0x70 = 0x412e6666 (10.9), snap offset 0.1 (class param 0x730232e3),
        // position (940.457, 45.3713, 683.463) from the level start data, orientation 121.766 degrees.
        require(near(pole.collisionCentre[1], 10.9f, 1e-5f) && near(pole.collisionHalf[1], 10.9f, 1e-5f), "pole bounds +0x64 / +0x70");
        require(near(pole.bottom(), 0.0f, 1e-5f) && near(pole.top(), 21.8f, 1e-5f), "pole bottom and top");
        require(near(pole.snapOffset, 0.1f, 1e-6f) && pole.canDismount, "snap offset 0.1, CanDismount");
        require(near(pole.position[1], 45.3713f, 1e-3f), "pole y");

        const Clip up = load(archive, "up"), down = load(archive, "down"), dismount = load(archive, "dismount");
        runtime::PoleInput input;
        input.dt = kDt;
        input.cameraYaw = kCameraYaw;
        input.clips.up = &*up.motion;
        input.clips.down = &*down.motion;
        input.clips.dismount = &*dismount.motion;
        input.clips.upFramesPerSecond = assets::animationFramesPerSecond(up.animation);
        input.clips.downFramesPerSecond = assets::animationFramesPerSecond(down.animation);
        input.clips.dismountFramesPerSecond = assets::animationFramesPerSecond(dismount.animation);
        input.clips.upFrames = assets::animationLength(up.animation);
        input.clips.downFrames = assets::animationLength(down.animation);
        input.clips.dismountFrames = assets::animationLength(dismount.animation);
        require(input.clips.upFrames == 10 && input.clips.downFrames == 15 && input.clips.dismountFrames == 17, "clip lengths 10, 15, 17");
        require(near(input.clips.upFramesPerSecond, 15.0f, 0.0f), "15 frames per second");

        // Walk-in (foot-walk): the mount happened in the update of row 5, with the facing after that update's turn within a quarter turn of the
        // pole (row 4 facing -31627 and the 6400 turn step: -25227). Rows 3 and 4 (facing 27509 and -31627 after their turns) did not mount.
        {
            const float rowThree[3] = {941.10773f, 58.33629f, 683.05060f};
            const float rowFour[3] = {941.10321f, 58.33629f, 683.04327f};
            const float rowFive[3] = {941.09833f, 58.33629f, 683.03540f};
            const auto none = [](const runtime::PoleObject&) { return false; };
            require(runtime::poleWalkInTest(poles, rowThree, 27509, true, -11773, none) == -1, "no mount in row 3 (facing away)");
            require(runtime::poleWalkInTest(poles, rowFour, -31627, true, -11869, none) == -1, "no mount in row 4 (facing away)");
            require(runtime::poleWalkInTest(poles, rowFive, -25227, true, -11869, none) == index, "mount in row 5");
            require(runtime::poleWalkInTest(poles, rowFive, -25227, true, 20000, none) == -1, "a stick away from the pole does not mount");
            require(runtime::poleWalkInTest(poles, rowFive, -25227, true, -11869, [](const runtime::PoleObject&) { return true; }) == -1, "disabled");

            runtime::PoleClimber climber;
            runtime::PoleInput mountInput = input;
            std::copy_n(rowFive, 3, mountInput.position);
            mountInput.facing = -25227;
            runtime::PoleStep step = runtime::poleMount(climber, poles, index, mountInput, false);
            require(step.riding && climber.phase == PolePhase::Mount, "mount phase");
            // ctrl+8 = ctrl+0xa = -10602 in row 5 (recorded); the handler computes atan2(pole forward) - 0x8000.
            require(std::abs(int(step.facing) - (-10602)) <= 2, "facing after the walk-in " + std::to_string(step.facing));
            const int expectedFacing[4] = {-10389, -10414, -10447, -10491};  // recorded ctrl+8 of rows 6..9
            for (int i = 0; i < 5; ++i) {
                std::copy_n(step.position, 3, mountInput.position);
                mountInput.facing = step.facing;
                step = runtime::poleUpdate(climber, poles, mountInput);
                if (i < 4) {
                    const Row& row = kFoot[3 + i];
                    require(near(step.position[0], row.x, 2e-4f) && near(step.position[2], row.z, 2e-4f),
                            "mount lerp row " + std::to_string(row.frame) + ": " + std::to_string(step.position[0]) + " " + std::to_string(step.position[2]));
                    require(std::abs(int(step.facing) - expectedFacing[i]) <= 3, "mount facing row " + std::to_string(row.frame) + ": " + std::to_string(step.facing));
                }
            }
            const Row& last = kFoot[7];
            require(climber.phase == PolePhase::Idle, "idle after 5 mount updates");
            require(near(step.position[0], last.x, 2e-4f) && near(step.position[2], last.z, 2e-4f) && near(step.position[1], last.y, 1e-4f), "snap point");
            require(near(climber.height, 12.965f, 1e-3f), "ctrl+0x24c at the snap point (recorded 12.965)");
        }

        // Climb up from the idle state at 60.18122 (pole-up2): recorded y per update.
        {
            runtime::PoleClimber climber;
            climber.phase = PolePhase::Idle;
            climber.pole = index;
            climber.height = 60.18122f - pole.position[1];
            runtime::PoleInput climb = input;
            climb.position[0] = 940.54224f; climb.position[1] = 60.18122f; climb.position[2] = 683.41003f;
            climb.facing = -10600;
            climb.stickActive = true;
            climb.stickHeading = -501;
            runtime::PoleStep step = runtime::poleUpdate(climber, poles, climb);
            require(climber.phase == PolePhase::Up && near(step.position[1], 60.18122f, 1e-5f), "idle -> up in one update, no move (row 15)");
            for (const Row& row : kUp) {
                std::copy_n(step.position, 3, climb.position);
                step = runtime::poleUpdate(climber, poles, climb);
                require(near(step.position[1], row.y, 2e-5f), "climb up row " + std::to_string(row.frame) + ": " + std::to_string(step.position[1]));
            }
            require(near(climber.height, 61.47219f - pole.position[1], 2e-4f), "ctrl+0x24c after 23 climb updates (recorded 16.1009)");
        }

        // Climb down (pole-down) and the exit at the bottom of the pipe: the roof under it ends the descent in row 71.
        {
            runtime::PoleClimber climber;
            climber.phase = PolePhase::Idle;
            climber.pole = index;
            climber.height = 61.47219f - pole.position[1];
            runtime::PoleInput climb = input;
            climb.position[0] = 940.54224f; climb.position[1] = 61.47219f; climb.position[2] = 683.41003f;
            climb.facing = -10600;
            climb.stickActive = true;
            climb.stickHeading = 32266;  // stick pulled back: the camera yaw (-502) plus a half turn
            climb.floorBelow = [](const float position[3], float length) { return position[1] - length <= 58.337f; };
            runtime::PoleStep step = runtime::poleUpdate(climber, poles, climb);
            require(climber.phase == PolePhase::Down, "idle -> down");
            for (const Row& row : kDown) {
                std::copy_n(step.position, 3, climb.position);
                step = runtime::poleUpdate(climber, poles, climb);
                if (row.frame < 71) {
                    require(near(step.position[1], row.y, 3e-5f), "climb down row " + std::to_string(row.frame) + ": " + std::to_string(step.position[1]));
                } else {
                    require(step.exit == PoleExit::Fall && !climber.active(), "fall at the bottom (row 71)");
                    require(near(step.position[0], row.x, 2e-3f) && near(step.position[2], row.z, 2e-3f) && near(step.position[1], row.y, 1e-5f),
                            "standing point after the exit: " + std::to_string(step.position[0]) + " " + std::to_string(step.position[2]));
                }
            }
        }

        // A and B on the pole (pole-A1, pole-B1): state 5 / state 4 in the update of the press, at the standing point (941.11188, 683.05731).
        for (const bool jump : {false, true}) {
            runtime::PoleClimber climber;
            climber.phase = PolePhase::Idle;
            climber.pole = index;
            climber.height = 60.18122f - pole.position[1];
            runtime::PoleInput press = input;
            press.position[0] = 940.54224f; press.position[1] = 60.18122f; press.position[2] = 683.41003f;
            press.facing = -10600;
            press.aPressed = !jump;
            press.bPressed = jump;
            const runtime::PoleStep step = runtime::poleUpdate(climber, poles, press);
            require(step.exit == (jump ? PoleExit::Jump : PoleExit::Fall) && !climber.active(), "A falls, B jumps");
            require(near(step.position[0], 941.11188f, 2e-3f) && near(step.position[2], 683.05731f, 2e-3f) && near(step.position[1], 60.18122f, 1e-5f),
                    "standing point after the press");
        }

        // Top of the pipe and the dismount (pole-up, rows 94..129). The up clip is at update 91 of the clip: its state began at row 3, the first step is row 4.
        {
            runtime::PoleClimber climber;
            climber.phase = PolePhase::Up;
            climber.pole = index;
            climber.height = 65.28885f - pole.position[1];
            climber.phaseUpdates = 91;
            runtime::PoleInput climb = input;
            climb.position[0] = 940.54224f; climb.position[1] = 65.28885f; climb.position[2] = 683.41003f;
            climb.facing = -10600;
            climb.stickActive = true;
            climb.stickHeading = -6826;
            // The roof under the pipe top (stand-in floor for the replay: TRAV-S8), read from the recorded heights.
            climb.floorHeight = [](const float position[3], float) -> std::optional<float> {
                if (position[0] > 940.35f) return 65.3451f;
                if (position[0] > 940.2f) return 67.46495f;
                if (position[0] > 939.77f) return 67.45404f;
                return 67.46576f;
            };
            runtime::PoleStep step = runtime::poleUpdate(climber, poles, climb);
            require(climber.phase == PolePhase::Dismount && near(climber.height, 19.97364f, 2e-4f), "dismount at the top (row 95), ctrl+0x24c 19.97364");
            require(near(step.position[1], 65.28885f, 1e-5f), "no move in the update that starts the dismount");
            for (const Row& row : kDismount) {
                std::copy_n(step.position, 3, climb.position);
                step = runtime::poleUpdate(climber, poles, climb);
                require(near(step.position[0], row.x, 6e-3f) && near(step.position[2], row.z, 6e-3f),
                        "dismount x z row " + std::to_string(row.frame) + ": " + std::to_string(step.position[0]) + " " + std::to_string(step.position[2]));
                if (row.frame >= 100)
                    require(near(step.position[1], row.y, 6e-3f), "dismount y row " + std::to_string(row.frame) + ": " + std::to_string(step.position[1]));
                if (row.frame < 129) require(step.exit == PoleExit::None, "dismount runs until row 128");
                else require(step.exit == PoleExit::Ground, "state 1 again in row 129");
            }
        }
        std::cout << "pole-real-test: ok\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "pole-real-test: " << error.what() << '\n';
        return 1;
    }
}
