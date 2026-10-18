// Rails of mission 1-2 in lvl_01_cherrytreehills (src/runtime/climb_bar.cpp): the bars 1-2_ClimbBar_02 / 03 and the corner rail 1-2_ClimbBar_04 from
// the level data, then replays of runs recorded in the oracle from the user state m12-12-railing (docs/re/player-traversal.md): New 3DS, OpenGL,
// no RPC, port 24731, home artifacts/oracle/r21-C, scheduler step fixed with `dt 0.033333335 0.033333335`, `trace N NAME mem` after `stick X 0`
// (railR: stick +120, railL: stick -120), `hold B` / `hold A` for 3 updates (barB1/barB2, barA1/barA2). Rows below are the frames of those traces
// (scratch output of tools/azahar_oracle.py, not kept): update index, ctrl+6 (stick heading), position, facing. Needs the extracted RomFS; exits with 77 without it.
#include "assets/assets.hpp"
#include "runtime/climb_bar.hpp"
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
constexpr const char* kLevelFile = "levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl";
constexpr float kDt = 0.033333335f;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

bool near(float a, float b, float tolerance) { return std::fabs(a - b) <= tolerance; }

struct Row {
    int update, heading;
    float x, y, z, fx, fz;
};

// railR: Idle on 1-2_ClimbBar_03 at (908.06586, 70.14743, 669.12573), stick +120 (the stick points along -x of the bar: right) to the far end (rows 57, 58).
const Row kRight[] = {
    {0, 30720, 908.12231, 70.14743, 669.09351, 0.49547, 0.86863},
    {1, 30722, 908.20404, 70.14743, 669.04688, 0.49547, 0.86863},
    {2, 30729, 908.28577, 70.14743, 669.00031, 0.49547, 0.86863},
    {3, 30737, 908.36719, 70.14743, 668.95386, 0.49547, 0.86863},
    {4, 30746, 908.44861, 70.14743, 668.90741, 0.49547, 0.86863},
    {5, 30756, 908.53033, 70.14743, 668.86078, 0.49547, 0.86863},
    {6, 30766, 908.61206, 70.14743, 668.81415, 0.49547, 0.86863},
    {7, 30776, 908.69379, 70.14743, 668.76752, 0.49547, 0.86863},
    {8, 30786, 908.77551, 70.14743, 668.72095, 0.49547, 0.86863},
    {9, 30795, 908.85693, 70.14743, 668.67450, 0.49547, 0.86863},
    {10, 30804, 908.93835, 70.14743, 668.62805, 0.49547, 0.86863},
    {11, 30812, 909.02008, 70.14743, 668.58142, 0.49547, 0.86863},
    {12, 30819, 909.10181, 70.14743, 668.53479, 0.49547, 0.86863},
    {13, 30826, 909.18353, 70.14743, 668.48816, 0.49547, 0.86863},
    {14, 30832, 909.26526, 70.14743, 668.44159, 0.49547, 0.86863},
    {15, 30837, 909.34668, 70.14743, 668.39514, 0.49547, 0.86863},
    {16, 30842, 909.42810, 70.14743, 668.34869, 0.49547, 0.86863},
    {17, 30847, 909.50983, 70.14743, 668.30206, 0.49547, 0.86863},
    {18, 30850, 909.59155, 70.14743, 668.25543, 0.49547, 0.86863},
    {19, 30850, 909.67297, 70.14743, 668.20905, 0.49547, 0.86863},
    {20, 30846, 909.75439, 70.14743, 668.16260, 0.49547, 0.86863},
    {21, 30837, 909.83612, 70.14743, 668.11597, 0.49547, 0.86863},
    {22, 30823, 909.91785, 70.14743, 668.06934, 0.49547, 0.86863},
    {23, 30802, 909.99927, 70.14743, 668.02289, 0.49547, 0.86863},
    {24, 30775, 910.08069, 70.14743, 667.97644, 0.49547, 0.86863},
    {25, 30740, 910.16241, 70.14743, 667.92981, 0.49547, 0.86863},
    {26, 30700, 910.24414, 70.14743, 667.88318, 0.49547, 0.86863},
    {27, 30652, 910.32587, 70.14743, 667.83661, 0.49547, 0.86863},
    {28, 30598, 910.40759, 70.14743, 667.78998, 0.49547, 0.86863},
    {29, 30537, 910.48901, 70.14743, 667.74353, 0.49547, 0.86863},
    {30, 30470, 910.57043, 70.14743, 667.69708, 0.49547, 0.86863},
    {31, 30398, 910.65216, 70.14743, 667.65045, 0.49547, 0.86863},
    {32, 30320, 910.73389, 70.14743, 667.60382, 0.49547, 0.86863},
    {33, 30237, 910.81561, 70.14743, 667.55725, 0.49547, 0.86863},
    {34, 30151, 910.89734, 70.14743, 667.51062, 0.49547, 0.86863},
    {35, 30060, 910.97876, 70.14743, 667.46417, 0.49547, 0.86863},
    {36, 29967, 911.06018, 70.14743, 667.41772, 0.49547, 0.86863},
    {37, 29871, 911.14191, 70.14743, 667.37109, 0.49547, 0.86863},
    {38, 29774, 911.22363, 70.14743, 667.32452, 0.49547, 0.86863},
    {39, 29675, 911.30505, 70.14743, 667.27808, 0.49547, 0.86863},
    {40, 29577, 911.38647, 70.14743, 667.23163, 0.49547, 0.86863},
    {41, 29479, 911.46820, 70.14743, 667.18500, 0.49547, 0.86863},
    {42, 29381, 911.54993, 70.14743, 667.13837, 0.49547, 0.86863},
    {43, 29283, 911.63135, 70.14743, 667.09192, 0.49547, 0.86863},
    {44, 29185, 911.71277, 70.14743, 667.04547, 0.49547, 0.86863},
    {45, 29087, 911.79449, 70.14743, 666.99884, 0.49547, 0.86863},
    {46, 28989, 911.87622, 70.14743, 666.95227, 0.49547, 0.86863},
    {47, 28892, 911.95795, 70.14743, 666.90564, 0.49547, 0.86863},
    {48, 28796, 912.03967, 70.14743, 666.85901, 0.49547, 0.86863},
    {49, 28702, 912.12109, 70.14743, 666.81256, 0.49547, 0.86863},
    {50, 28610, 912.20251, 70.14743, 666.76611, 0.49547, 0.86863},
    {51, 28521, 912.28424, 70.14743, 666.71948, 0.49547, 0.86863},
    {52, 28433, 912.36597, 70.14743, 666.67291, 0.49547, 0.86863},
    {53, 28345, 912.44769, 70.14743, 666.62628, 0.49547, 0.86863},
    {54, 28258, 912.52942, 70.14743, 666.57965, 0.49547, 0.86863},
    {55, 28170, 912.61084, 70.14743, 666.53320, 0.49547, 0.86863},
    {56, 28082, 912.69226, 70.14743, 666.48676, 0.49547, 0.86863},
    {57, 27993, 912.77258, 70.14743, 666.44098, 0.49547, 0.86863},
    {58, 27904, 912.77258, 70.14743, 666.44098, 0.49547, 0.86863},
    {59, 27810, 912.77258, 70.14743, 666.44098, 0.49547, 0.86863},
};

// railL: stick -120: left along 03 into the corner rail 04 (updates 1..16), onto 02 (grab in update 16, arrival in the same update), idle placement in
// update 17, then left along 02.
const Row kLeft[] = {
    {0, -2048, 908.00940, 70.14743, 669.15796, 0.49547, 0.86863},
    {1, -2051, 908.00940, 70.14743, 669.15796, 0.49547, 0.86863},
    {2, -2053, 907.99463, 70.21967, 669.23376, 0.58355, 0.81208},
    {3, -2060, 907.75360, 70.21967, 669.37140, 0.66524, 0.74663},
    {4, -2076, 907.49762, 70.21967, 669.53564, 0.73964, 0.67300},
    {5, -2101, 907.27917, 70.21967, 669.71899, 0.80594, 0.59200},
    {6, -2131, 907.15118, 70.21967, 669.91382, 0.86340, 0.50452},
    {7, -2163, 907.14148, 70.21967, 670.14136, 0.91141, 0.41150},
    {8, -2195, 907.21521, 70.21967, 670.40601, 0.94943, 0.31398},
    {9, -2226, 907.33118, 70.21967, 670.67517, 0.97705, 0.21302},
    {10, -2255, 907.44800, 70.21967, 670.91583, 0.99396, 0.10972},
    {11, -2265, 907.52423, 70.21967, 671.09552, 0.99999, 0.00522},
    {12, -2248, 907.52600, 70.21967, 671.13348, 0.99505, -0.09933},
    {13, -2212, 907.53168, 70.21967, 671.17114, 0.97922, -0.20280},
    {14, -2158, 907.54138, 70.21967, 671.20801, 0.95266, -0.30405},
    {15, -2089, 907.55481, 70.21967, 671.24359, 0.91566, -0.40196},
    {16, -2009, 907.52576, 70.14743, 671.26013, 0.91566, -0.40196},
    {17, -1916, 907.52576, 70.14743, 671.26013, 0.86863, -0.49547},
    {18, -1812, 907.55798, 70.14743, 671.31659, 0.86863, -0.49547},
    {19, -1701, 907.60461, 70.14743, 671.39832, 0.86863, -0.49547},
    {20, -1585, 907.65125, 70.14743, 671.48004, 0.86863, -0.49547},
    {21, -1465, 907.69769, 70.14743, 671.56146, 0.86863, -0.49547},
    {22, -1339, 907.74414, 70.14743, 671.64288, 0.86863, -0.49547},
    {23, -1208, 907.79077, 70.14743, 671.72461, 0.86863, -0.49547},
    {24, -1073, 907.83734, 70.14743, 671.80634, 0.86863, -0.49547},
    {25, -933, 907.88397, 70.14743, 671.88806, 0.86863, -0.49547},
    {26, -788, 907.93060, 70.14743, 671.96979, 0.86863, -0.49547},
    {27, -639, 907.97705, 70.14743, 672.05121, 0.86863, -0.49547},
    {28, -487, 908.02350, 70.14743, 672.13263, 0.86863, -0.49547},
    {29, -331, 908.07013, 70.14743, 672.21436, 0.86863, -0.49547},
    {30, -172, 908.11670, 70.14743, 672.29608, 0.86863, -0.49547},
};

// barB1 + barB2: y after each update of the jump off the bar (B pressed in update 0; the same x and z); the bar is grabbed again in update 21.
const float kJumpY[] = {70.14743f, 70.42049f, 70.69354f, 70.97359f, 71.25365f, 71.49927f, 71.74489f, 71.91895f, 72.09300f, 72.15407f, 72.21513f, 72.22832f,
                        72.24150f, 72.23289f, 72.22429f, 72.14977f, 72.07526f, 71.85977f, 71.64429f, 71.28003f, 70.91577f, 70.26198f, 70.14743f};
// barA1 + barA2: y after each update of the drop (A pressed in update 0).
const float kDropY[] = {70.14743f, 70.09743f, 69.99743f, 69.84743f, 69.64743f, 69.39743f, 69.09743f, 68.74743f};


int find(const chase::runtime::ClimbBarSet& set, const char* name) {
    for (std::size_t i = 0; i < set.bars.size(); ++i)
        if (set.bars[i].instance == name) return int(i);
    throw std::runtime_error(std::string("no bar ") + name);
}

chase::assets::RootMotion rootMotionOf(chase::assets::FuseArchive& archive, const char* clip) {
    using namespace chase;
    const assets::FuseEntry* entry = archive.find(std::string("models/char_minifig/class_generic/") + clip + "_minifig.bfnanm");
    require(entry != nullptr, std::string("clip missing ") + clip);
    const assets::Animation animation = assets::parseAnimation(archive.read(*entry));
    const std::optional<assets::RootMotion> motion = assets::rootMotion(animation);
    require(motion.has_value() && motion->translation.size() > 1, "root motion");
    return *motion;
}
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: climb-bar-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "climb-bar-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        using namespace chase;
        using runtime::ClimbPhase;
        assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        const assets::FuseEntry* entry = archive.find(kLevelFile);
        require(entry != nullptr, std::string("not in archive: ") + kLevelFile);
        const assets::Level level = assets::parseLevel(archive.read(*entry));
        const runtime::ClimbBarSet set = runtime::loadClimbBars(level, assets::archiveClassLookup(archive));
        const int bar02 = find(set, "1-2_climbbar_02"), bar03 = find(set, "1-2_climbbar_03");
        require(set.corners.size() >= 1 && set.corners[0].path.size() == 3, "corner rail path");
        // dump of the oracle (m12-12): GrabBound centre (0.00228, 0.12063, 0.08835), half (2.77179, 0.2, 0.3), flags byte 0x04 (FaceJumpDir only).
        const runtime::ClimbBar& bar = set.bars[std::size_t(bar03)];
        require(near(bar.grabHalf[0], 2.77179f, 1e-4f) && near(bar.grabCentre[1], 0.12063f, 1e-4f), "grab bound");
        require(bar.faceJumpDir && !bar.narrowBar && !bar.acrobatPole && !bar.ledge && !bar.immediateJump, "bar flags");
        require(near(set.bars[std::size_t(bar02)].yawDegrees, -60.2993f, 1e-3f) && near(bar.yawDegrees, 209.7007f, 1e-3f), "bar yaw");

        runtime::ClimbBarInput input;
        input.dt = kDt;
        input.stickHeld = true;
        const assets::RootMotion leftMotion = rootMotionOf(archive, "ledge_left"), rightMotion = rootMotionOf(archive, "ledge_right");
        input.leftMotion = &leftMotion;
        input.rightMotion = &rightMotion;

        // Right along 03.
        {
            runtime::ClimbBarRider rider;
            rider.phase = ClimbPhase::Idle;
            rider.bar = bar03;
            float position[3] = {908.06586f, 70.14743f, 669.12573f};
            std::int16_t facing = 5406;
            for (const Row& row : kRight) {
                input.stickHeading = std::int16_t(row.heading);
                std::copy(position, position + 3, input.position);
                input.facing = facing;
                const runtime::ClimbBarStep step = runtime::climbBarUpdate(rider, set, input);
                require(step.riding, "right: riding");
                std::copy(step.position, step.position + 3, position);
                facing = step.facing;
                const std::string where = "right update " + std::to_string(row.update);
                require(near(position[0], row.x, 3e-4f) && near(position[1], row.y, 3e-4f) && near(position[2], row.z, 3e-4f), where + " position " + std::to_string(position[0]) + " " + std::to_string(position[1]) + " " + std::to_string(position[2]) + " expected " + std::to_string(row.x) + " " + std::to_string(row.y) + " " + std::to_string(row.z));
            }
            require(rider.phase == ClimbPhase::Idle, "right: the end of the bar blocks and the state goes back to IDLE");
        }

        // Left along 03, round the corner rail, onto 02 and left along it.
        {
            runtime::ClimbBarRider rider;
            rider.phase = ClimbPhase::Idle;
            rider.bar = bar03;
            float position[3] = {908.06586f, 70.14743f, 669.12573f};
            std::int16_t facing = 5406;
            bool corner = false;
            for (const Row& row : kLeft) {
                input.stickHeading = std::int16_t(row.heading);
                std::copy(position, position + 3, input.position);
                input.facing = facing;
                const runtime::ClimbBarStep step = runtime::climbBarUpdate(rider, set, input);
                require(step.riding, "left: riding");
                std::copy(step.position, step.position + 3, position);
                facing = step.facing;
                if (rider.phase == ClimbPhase::CornerLeft) corner = true;
                const std::string where = "left update " + std::to_string(row.update);
                require(near(position[0], row.x, 3e-4f) && near(position[1], row.y, 3e-4f) && near(position[2], row.z, 3e-4f), where + " position " + std::to_string(position[0]) + " " + std::to_string(position[1]) + " " + std::to_string(position[2]) + " expected " + std::to_string(row.x) + " " + std::to_string(row.y) + " " + std::to_string(row.z));
                if (row.update >= 1)
                    require(near(std::sin(float(std::uint16_t(facing)) / 10430.378f), row.fx, 4e-4f) && near(std::cos(float(std::uint16_t(facing)) / 10430.378f), row.fz, 4e-4f),
                            where + " facing");
            }
            require(corner, "left: corner state entered");
            require(rider.bar == bar02 && rider.phase == ClimbPhase::Left, "left: on 02 moving left");
        }

        // Jump off 03 (B press, update 0) and the grab of it in update 21.
        {
            runtime::ClimbBarRider rider;
            rider.bar = bar03;
            runtime::climbBarLeave(rider);
            rider.jumpCount = 0;
            require(rider.ignoreBar == bar03, "ignore set on leaving");
            int grabbedAt = -1;
            for (int k = 1; k <= 22 && grabbedAt < 0; ++k) {
                if (!runtime::climbBarJumpTick(rider, false, true)) {
                    require(k < 6, "tests run from update 6");
                    require(rider.ignoreBar == bar03, "ignore kept before the tests");
                    continue;
                }
                require(k >= 6, "no test before update 6");
                const float position[3] = {908.06586f, kJumpY[k], 669.12573f};
                const int found = runtime::climbBarGrabTest(set, rider, position, 5406, {});
                if (k == 6) require(rider.ignoreBar < 0, "ignore forgotten when the box leaves the bar (update 6)");
                if (found >= 0) grabbedAt = k;
            }
            require(grabbedAt == 21, "grabbed again in update " + std::to_string(grabbedAt));
            // The arrival: the grab update only starts it, the next update snaps to the hang point.
            input.position[0] = 908.06586f;
            input.position[1] = kJumpY[21];
            input.position[2] = 669.12573f;
            input.facing = 5406;
            input.stickHeld = false;
            runtime::climbBarGrab(rider, set, bar03, input);
            require(rider.phase == ClimbPhase::Arrive, "arrive phase");
            const runtime::ClimbBarStep arrived = runtime::climbBarUpdate(rider, set, input);
            require(near(arrived.position[1], kJumpY[22], 3e-4f) && rider.phase == ClimbPhase::Idle && rider.enteredThisUpdate, "arrival snaps to the hang point");
        }


        // Grab of 1-2_FlagPole_01 from the bounce of the built trampoline (docs/re/player-traversal-rails.md, TRAV-36). Oracle: user state
        // m12-trampoline-built, `teleport 720.20 32.618 637.47`, B held, `dt 0.033333335`, `trace 340 t2 mem` (scratch output, not kept):
        // after the take-off at speed 0.9 the node rises to 42.21911 (state 26), the fall update (state 5, y 42.06911) grabs the pole and the
        // arrival runs in the same update: rows (x, y, z) 720.26990 41.81580 637.51532; 720.33978 41.56249 637.56067; 720.36835 41.45909
        // 637.57922 with the request for state 93 (acrobat pole); the first update of state 93: 721.36157 41.45909 637.57922, facing (0, -1).
        {
            const int pole01 = find(set, "1-2_flagpole_01");
            const runtime::ClimbBar& pole = set.bars[std::size_t(pole01)];
            require(pole.acrobatPole && near(pole.yawDegrees, 180.0f, 1e-3f), "flag pole is an acrobat pole turned by 180 degrees");
            require(near(pole.position[0], 723.11157f, 1e-3f) && near(pole.position[1], 43.4695f, 1e-3f) && near(pole.position[2], 637.55219f, 1e-3f), "flag pole position");
            runtime::ClimbBarRider rider;
            rider.jumpLatch = true;
            const std::int16_t facing = -30488;
            const float fallen[3] = {720.20001f, 42.06911f, 637.46997f};
            require(runtime::climbBarGrabTest(set, rider, fallen, facing, {}) == pole01, "fall update at y 42.06911 grabs 1-2_FlagPole_01");
            const float apex[3] = {720.20001f, 38.0f, 637.46997f};
            require(runtime::climbBarGrabTest(set, rider, apex, facing, {}) < 0, "no grab below the pole");
            input.position[0] = fallen[0];
            input.position[1] = fallen[1];
            input.position[2] = fallen[2];
            input.facing = facing;
            input.stickHeld = false;
            input.aPressed = false;
            input.bHeld = true;
            runtime::climbBarGrab(rider, set, pole01, input);
            const float expected[3][3] = {{720.26990f, 41.81580f, 637.51532f}, {720.33978f, 41.56249f, 637.56067f}, {720.36835f, 41.45909f, 637.57922f}};
            for (int k = 0; k < 3; ++k) {
                const runtime::ClimbBarStep step = runtime::climbBarUpdate(rider, set, input);
                require(step.riding, "acrobat arrival: riding");
                for (int axis = 0; axis < 3; ++axis)
                    require(near(step.position[axis], expected[k][axis], 2e-3f), "acrobat arrival row " + std::to_string(k) + " axis " + std::to_string(axis));
                std::copy(step.position, step.position + 3, input.position);
                require(step.facing == facing, "arrival keeps the facing");
            }
            require(rider.phase == ClimbPhase::Idle && rider.enteredThisUpdate, "arrival ends in the third update");
            const runtime::ClimbBarStep entered = runtime::climbBarUpdate(rider, set, input);
            require(near(entered.position[0], 721.36157f, 2e-3f) && near(entered.position[1], 41.45909f, 2e-3f) && near(entered.position[2], 637.57922f, 2e-3f),
                    "state 93 stands at the pole centre");
            require(std::abs(int(entered.facing) - 32768) <= 1 || std::abs(int(entered.facing) + 32768) <= 1, "facing -z for a pole grabbed from the front");
            input.bHeld = false;
        }

        // Acrobat pole jumps (0x211b88, docs/re/player-traversal-rails.md §2026-10-18T21:33:08). Oracle traces of r24-C (scratch output, not
        // kept): t1 from the user state m12-trampoline-pole (hanging on 01 facing (0, 1), no stick, B held, `trace 160 t1 mem`): states
        // 93 -> 95 (ctrl+0x140 = 1-2_FlagPole_02) -> 96 (03) -> 95 (04) -> 98 -> 5 -> 1; t2 (bounce grab of 01, facing (0, -1) after the
        // arrival, B held, `trace 340 t2 mem`): 93 -> 97 (ctrl+0x140 = 0) -> 5 -> 6 -> 1.
        {
            const int poles[4] = {find(set, "1-2_flagpole_01"), find(set, "1-2_flagpole_02"), find(set, "1-2_flagpole_03"), find(set, "1-2_flagpole_04")};
            const std::int16_t towardsPlusZ = 0, towardsMinusZ = -32768;
            const runtime::ClimbBar& pole01 = set.bars[std::size_t(poles[0])];
            require(runtime::acrobatPoleFacing(pole01, towardsPlusZ) == towardsPlusZ, "t1: facing (0, 1) stays on the pole");
            require(runtime::acrobatPoleFacing(pole01, -30488) == towardsMinusZ, "t2: arrival facing (-0.217, -0.976) turns to (0, -1)");
            const int states[4] = {runtime::kAcrobatPoleIdleState, runtime::kAcrobatPoleJumpState, runtime::kAcrobatPoleJumpAgainState,
                                   runtime::kAcrobatPoleJumpState};
            const int expected[4] = {runtime::kAcrobatPoleJumpState, runtime::kAcrobatPoleJumpAgainState, runtime::kAcrobatPoleJumpState,
                                     runtime::kAcrobatPoleJumpToLandState};
            for (int k = 0; k < 4; ++k) {
                const runtime::ClimbBar& pole = set.bars[std::size_t(poles[k])];
                const runtime::AcrobatPoleJump jump = runtime::acrobatPoleJump(pole, towardsPlusZ, states[k], false, 0, {});
                require(jump.state == expected[k], "t1: jump state off pole " + std::to_string(k + 1));
                require(jump.link >= 0, "t1: a target ahead of pole " + std::to_string(k + 1));
                const runtime::ClimbBarLink& target = pole.links[std::size_t(jump.link)];
                if (k < 3) require(target.bar == poles[k + 1], "t1: the next pole after pole " + std::to_string(k + 1));
                else require(target.bar < 0 && target.instance == "1-2_flagpole_landing_point", "t1: the last jump goes to the landing point");
            }
            const runtime::AcrobatPoleJump away = runtime::acrobatPoleJump(pole01, towardsMinusZ, runtime::kAcrobatPoleIdleState, false, 0, {});
            require(away.state == runtime::kAcrobatPoleJumpToFallState && away.link < 0, "t2: no pole ahead, jump to the fall");
        }

        // Drop (A press) and the first fall update: y 70.14743 in the press update, then the fall of the port takes over.
        {
            runtime::ClimbBarRider rider;
            rider.phase = ClimbPhase::Idle;
            rider.bar = bar03;
            input.stickHeld = false;
            input.aPressed = true;
            const float position[3] = {908.06586f, kDropY[0], 669.12573f};
            std::copy(position, position + 3, input.position);
            input.facing = 5406;
            const runtime::ClimbBarStep step = runtime::climbBarUpdate(rider, set, input);
            require(!step.riding && step.release == runtime::ClimbBarRelease::Fall && rider.ignoreBar == bar03, "A press drops the player and remembers the bar");
        }
    } catch (const std::exception& error) {
        std::cerr << "climb-bar-real-test FAILED: " << error.what() << '\n';
        return 1;
    }
    std::cout << "climb-bar-real-test: ok\n";
    return 0;
}
