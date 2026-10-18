// The class leprops/leprop_bouncer (src/runtime/prop_bouncer.*; message handler 0x1988f4, take-off enter 0x2f4e10) and the bounce flight of the player
// (src/runtime/player_vertical.cpp). Recorded values: Azahar oracle, user state m12-trampoline-built (New 3DS), the player placed on Trampoline_03 with
// `teleport 720.20 32.618 637.47`, B pressed, fixed dt 0.033 (`dt 0.033 0.033`) and `trace N NAME mem` (scratch tr8: B released after the first press,
// tr9: B held; agent r23-D): per update the node height (CSV) and ctrl+0x2b8 / +0x2b4 (the .mem controller dump). The take-off speed ctrl+0x2dc reads
// 0.3 for every bounce without B and 0.4, 0.5, 0.6, 0.7 for the bounces with B held. Exits with 77 without data in the real-data part.
#include "assets/assets.hpp"
#include "runtime/collision.hpp"
#include "runtime/player_vertical.hpp"
#include "runtime/prop_bouncer.hpp"

#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase;
using namespace chase::runtime;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

struct Row {
    int update;
    float y;
    float fallSpeed;
    float fallDistance;
};

constexpr float kGroundHeight = 33.67413f;  // the landing update's height 33.97413 minus the take-off speed 0.3 (Trampoline_03 at 32.57939 plus the collision item's top 1.09732 is 33.6767)
constexpr float kDt = 0.033f;

const Row kNoButton[] = {
    {29, 33.97413f, -0.25f, 0.0f},
    {30, 34.22413f, -0.200000003f, 0.0f},
    {31, 34.42413f, -0.150000006f, 0.0f},
    {32, 34.57413f, -0.100000009f, 0.0f},
    {33, 34.67413f, -0.0500000082f, 0.0f},
    {34, 34.72413f, -7.4505806e-09f, 0.0f},
    {35, 34.72413f, 0.0499999933f, 0.0f},
    {36, 34.62463f, 0.099499993f, 0.099499993f},
    {37, 34.62463f, 0.099499993f, 0.099499993f},
    {38, 34.47563f, 0.148999989f, 0.24849999f},
    {39, 34.27713f, 0.198499992f, 0.446999967f},
    {40, 34.02913f, 0.247999996f, 0.694999933f},
    {41, 33.73163f, 0.297499985f, 0.992499948f},
};

const Row kButtonHeld[] = {
    {29, 34.07413f, -0.349999994f, 0.0f},
    {30, 34.42413f, -0.299999982f, 0.0f},
    {31, 34.72413f, -0.249999985f, 0.0f},
    {32, 34.97413f, -0.199999988f, 0.0f},
    {33, 35.17413f, -0.149999991f, 0.0f},
    {34, 35.32413f, -0.099999994f, 0.0f},
    {35, 35.42413f, -0.0499999933f, 0.0f},
    {36, 35.47413f, 7.4505806e-09f, 0.0f},
    {37, 35.42463f, 0.0495000072f, 0.0495000072f},
    {38, 35.42463f, 0.0495000072f, 0.0495000072f},
    {39, 35.32563f, 0.0990000069f, 0.14850001f},
    {40, 35.17713f, 0.14850001f, 0.297000021f},
    {41, 34.97913f, 0.198000014f, 0.495000035f},
    {42, 34.73163f, 0.247500017f, 0.742500067f},
    {43, 34.43463f, 0.297000021f, 1.03950012f},
    {44, 34.08813f, 0.346500009f, 1.38600016f},
};

assets::CollisionItem floorItem(float height) {
    assets::CollisionItem item;
    item.name = "Collision_Test";
    const float quad[12] = {-100, height, -100, -100, height, 100, 100, height, 100, 100, height, -100};
    item.vertices.assign(quad, quad + 12);
    for (const std::array<std::uint32_t, 3>& corner : {std::array<std::uint32_t, 3>{0, 1, 2}, {0, 2, 3}}) {
        assets::CollisionTriangle triangle;
        for (int a = 0; a < 3; ++a) triangle.vertex[a] = corner[std::size_t(a)];
        triangle.normal[0] = 0;
        triangle.normal[1] = 1;
        triangle.normal[2] = 0;
        triangle.d = height;
        item.triangles.push_back(triangle);
    }
    assets::CollisionNode node;
    for (int a = 0; a < 3; ++a) node.half[a] = 200;
    node.centre[1] = height;
    node.info = 1 | (std::uint32_t(item.triangles.size()) << 1);
    item.nodes.push_back(node);
    return item;
}

void flight(const BouncerParams& bouncer, bool held, const Row* rows, std::size_t count, const char* label) {
    CollisionWorld world;
    world.add(floorItem(0.0f));
    VerticalState state;
    state.y = kGroundHeight;
    BounceMemory memory;
    const BounceLaunch launch = bounceLaunch(bouncer, memory, 0.3338f, held);
    startBounceFlight(state, launch);
    for (std::size_t i = 0; i < count; ++i) {
        if (i > 0) {
            VerticalInput input;
            input.dt = kDt;
            input.ticks = stepTicks(kDt);
            input.x = 720.2f;
            input.z = 637.47f;
            verticalUpdate(state, world, input);
        }
        const Row& row = rows[i];
        const std::string where = std::string(label) + " update " + std::to_string(row.update);
        require(std::fabs(state.y - row.y) < 2e-5f, where + ": height " + std::to_string(state.y));
        require(state.fallSpeed == row.fallSpeed, where + ": ctrl+0x2b8 " + std::to_string(state.fallSpeed));
        require(std::fabs(state.fallDistance - row.fallDistance) < 1e-6f, where + ": ctrl+0x2b4 " + std::to_string(state.fallDistance));
    }
}

void rules() {
    BouncerParams bouncer;
    bouncer.instance = "trampoline_03";
    BounceMemory memory;
    BounceLaunch launch = bounceLaunch(bouncer, memory, 0.3338f, false);
    require(memory.speed == 0.3f && launch.speed == 0.3f, "first bounce without B: ctrl+0x2dc 0.3 (oracle row 29)");
    require(launch.fallSpeed == -0.25f, "ctrl+0x2b8 -0.25 after the take-off update (oracle row 29)");
    launch = bounceLaunch(bouncer, memory, 0.2975f, false);
    require(launch.speed == 0.3f, "the next bounce without B stays at 0.3 (oracle row 42)");

    BounceMemory held;
    const float expected[5] = {0.4f, 0.5f, 0.6f, 0.7f, 0.8f};
    for (float speed : expected) {
        launch = bounceLaunch(bouncer, held, 0.3f, true);
        require(std::fabs(launch.speed - speed) < 1e-5f, "B held raises ctrl+0x2dc by JumpSpeedChange: " + std::to_string(launch.speed));
    }
    BounceMemory top;
    top.speed = 0.95f;
    require(bounceLaunch(bouncer, top, 0.3f, true).speed == 1.0f, "the speed is clamped to HighestTakeOffSpeed");

    BouncerParams soft = bouncer;
    soft.fallSpeedLoss = 0.5f;
    BounceMemory fresh;
    launch = bounceLaunch(soft, fresh, 0.6f, false);
    require(fresh.field24c == 0.0f, "ctrl+0x24c is cleared by the take-off");
    require(std::fabs(launch.speed - 0.3f) < 1e-6f || launch.speed > 0.3f, "a lower FallSpeedLoss keeps part of the fall speed");
}

void realData(const char* fib, const char* names) {
    assets::FuseArchive archive(fib);
    archive.loadNames(names);
    const assets::FuseEntry* entry = archive.find("levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl");
    require(entry != nullptr, "level in archive");
    const assets::Level level = assets::parseLevel(archive.read(*entry));
    const assets::ClassLookup classOf = assets::archiveClassLookup(archive);
    BouncerSet set;
    set.load(level, classOf);
    const BouncerParams* bouncer = set.find("trampoline_03");
    require(bouncer != nullptr, "Trampoline_03 is a bouncer");
    require(bouncer->highestTakeOffSpeed == 1.0f && bouncer->lowestTakeOffSpeed == 0.3f, "take-off speeds 1.0 and 0.3 (class defaults)");
    require(std::fabs(bouncer->jumpSpeedChange - 0.1f) < 1e-7f && bouncer->fallSpeedLoss == 1.0f, "JumpSpeedChange 0.1, FallSpeedLoss 1.0 (class values)");
    require(bouncer->sound == 0x51, "ATTR_SFX 0x51 (class value)");
    require(!bouncer->autoBounce && !bouncer->trickBounce, "AutoBounce and TrickBounce off");
    flight(*bouncer, false, kNoButton, sizeof kNoButton / sizeof kNoButton[0], "B released");
    flight(*bouncer, true, kButtonHeld, sizeof kButtonHeld / sizeof kButtonHeld[0], "B held");
    require(set.size() > 3, "several bouncers in the level");
}
}

int main(int argc, char** argv) {
    try {
        rules();
        if (argc == 3) {
            if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
                std::cout << "prop-bouncer-test: data not available, skipped\n";
                return 77;
            }
            realData(argv[1], argv[2]);
        } else if (argc != 1) {
            std::cerr << "usage: prop-bouncer-test [FIB NAMES]\n";
            return 2;
        }
    } catch (const std::exception& error) {
        std::cerr << "prop-bouncer-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "prop-bouncer-test: ok\n";
    return 0;
}
