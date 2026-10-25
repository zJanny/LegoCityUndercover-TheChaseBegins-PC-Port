// The side test of the wall candidate filter 0x209a98 (flag 0x20, entries of placed objects): a wall triangle of a placed object only blocks while the
// player box centre at the start of the update lies on the front side of its plane. Recorded values: Azahar oracle, save state at the built trampoline of mission 1-2,
// RPC `sample 3.5 0.004` of a full-speed run (`freestick 75 -93`) at the built Trampoline_03 (origin 720.20 637.47), scratch r26B_run.csv (agent r26-B):
// from (718.843, 641.299), 4.06 m out, one update steps 0.23 m; the run stops on the circle of 2.448 m around the origin in the seventh update and
// then only slides. The earlier walk over the pad (round 23) started 1.83 m from the origin, inside the frame, and crossed it.
// Exits with 77 without data in the real-data part.
#include "assets/assets.hpp"
#include "assets/object_collision.hpp"
#include "runtime/collision.hpp"

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

assets::CollisionItem wallItem() {
    assets::CollisionItem item;
    item.name = "Collision_Wall";
    const float quad[12] = {-100, 0, 0, 100, 0, 0, 100, 10, 0, -100, 10, 0};
    item.vertices.assign(quad, quad + 12);
    for (const std::array<std::uint32_t, 3>& corner : {std::array<std::uint32_t, 3>{0, 1, 2}, {0, 2, 3}}) {
        assets::CollisionTriangle triangle;
        for (int a = 0; a < 3; ++a) triangle.vertex[a] = corner[std::size_t(a)];
        triangle.normal[0] = 0;
        triangle.normal[1] = 0;
        triangle.normal[2] = 1;
        triangle.d = 0;
        item.triangles.push_back(triangle);
    }
    assets::CollisionNode node;
    node.half[0] = 100;
    node.half[1] = 5;
    node.half[2] = 1;
    node.centre[1] = 5;
    node.info = 1 | (std::uint32_t(item.triangles.size()) << 1);
    item.nodes.push_back(node);
    return item;
}

float walkIntoWall(std::uint32_t mask, float startZ) {
    CollisionWorld world;
    world.add(wallItem(), mask);
    float dx = 0, dz = -0.2f;
    slideAlongWalls(world, 0, 0.1f, startZ, 0, -1, dx, dz);
    return dz;
}

void rules() {
    require(walkIntoWall(6, 0.3f) > -0.2f, "a placed object's wall blocks a box centre in front of the plane");
    require(walkIntoWall(6, -0.1f) == -0.2f, "a placed object's wall lets a box centre behind its plane move on");
    require(walkIntoWall(kStaticMask, 0.3f) > -0.2f, "a static wall blocks from the front");
    require(walkIntoWall(kStaticMask, -0.1f) > -0.2f, "a static wall has no side test");
}

constexpr float kOriginX = 720.20f, kOriginZ = 637.47f;
constexpr float kStep = 0.23f;

float distanceToOrigin(float x, float z) { return std::hypot(x - kOriginX, z - kOriginZ); }

void realData(const char* fib, const char* names) {
    assets::FuseArchive archive(fib);
    archive.loadNames(names);
    const assets::FuseEntry* entry = archive.find("levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl");
    require(entry != nullptr, "level in archive");
    const assets::Level level = assets::parseLevel(archive.read(*entry));
    CollisionWorld world;
    for (const assets::ObjectCollision& object : assets::objectCollisionCandidates(archive, level))
        if (object.instance == "Trampoline_03") world.addObject(object);
    require(!world.empty(), "Trampoline_03 has collision");

    float x = 718.843f, z = 641.299f;
    float forwardX = 0.386f, forwardZ = -0.9225f;
    const float length = std::hypot(forwardX, forwardZ);
    forwardX /= length;
    forwardZ /= length;
    require(std::fabs(distanceToOrigin(x, z) - 4.06f) < 0.01f, "the run starts 4.06 m from the origin (oracle sample at 0.741 s)");
    for (int update = 1; update <= 13; ++update) {
        float dx = forwardX * kStep, dz = forwardZ * kStep;
        slideAlongWalls(world, x, 32.65f, z, forwardX, forwardZ, dx, dz);
        x += dx;
        z += dz;
        const float distance = distanceToOrigin(x, z);
        if (update <= 6) require(distance > 2.6f, "the run is not stopped before the sixth update");
        if (update >= 7) require(distance > 2.44f && distance < 2.50f, "from the seventh update the run stays on the circle of 2.448 m (oracle)");
    }

    x = 720.20f;
    z = 639.30f;
    for (int update = 1; update <= 8; ++update) {
        float dx = 0, dz = -kStep;
        const int overlaps = slideAlongWalls(world, x, 32.618f, z, 0, -1, dx, dz);
        require(overlaps == 0 && dz == -kStep, "a walk that starts inside the frame crosses it (round 23 oracle walk)");
        z += dz;
    }
}
}

int main(int argc, char** argv) {
    try {
        rules();
        if (argc == 3) {
            if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
                std::cout << "prop-wall-side-test: data not available, skipped\n";
                return 77;
            }
            realData(argv[1], argv[2]);
        } else if (argc != 1) {
            std::cerr << "usage: prop-wall-side-test [FIB NAMES]\n";
            return 2;
        }
    } catch (const std::exception& error) {
        std::cerr << "prop-wall-side-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "prop-wall-side-test: ok\n";
    return 0;
}
