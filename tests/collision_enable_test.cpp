// Objects enabled or disabled during play: the collision world follows the script state (EnableObject bit 0, EnableCollision flag 0x200)
// without being rebuilt. Rules: docs/re/collision.md.
#include "runtime/collision_enable.hpp"
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

assets::ObjectCollision box(const std::string& instance, bool collisionDefault) {
    assets::ObjectCollision object;
    object.instance = instance;
    object.className = "props/prop_prop";
    object.collisionDefault = collisionDefault;
    object.matrix = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    object.half[0] = object.half[1] = object.half[2] = 1;
    return object;
}

int hits(const runtime::CollisionWorld& world) {
    const float low[3] = {-0.5f, -0.5f, -0.5f}, high[3] = {0.5f, 0.5f, 0.5f};
    int count = 0;
    world.query(low, high, runtime::kPlayerLayer, [&](const runtime::CollisionWorld::Triangle&) { ++count; });
    return count;
}

void run() {
    runtime::CollisionWorld world;
    world.addObject(box("Police_Car", true));
    assets::PlacedObjectState state;
    state.disabled.insert("police_car");
    runtime::applyObjectCollisionState(world, state);
    require(hits(world) == 0, "a disabled object collides");
    state.disabled.erase("police_car");
    runtime::applyObjectCollisionState(world, state);
    require(hits(world) > 0, "an object enabled during play has no collision");
    state.collisionOverride["police_car"] = false;
    runtime::applyObjectCollisionState(world, state);
    require(hits(world) == 0, "EnableCollision 0 keeps the collision");
    state.collisionOverride["police_car"] = true;
    runtime::applyObjectCollisionState(world, state);
    require(hits(world) > 0, "EnableCollision 1 does not restore the collision");

    runtime::CollisionWorld off;
    off.addObject(box("Mail_Box", false));
    assets::PlacedObjectState none;
    runtime::applyObjectCollisionState(off, none);
    require(hits(off) == 0, "BoundCollision 0 collides before EnableCollision");
    none.collisionOverride["mail_box"] = true;
    runtime::applyObjectCollisionState(off, none);
    require(hits(off) > 0, "EnableCollision 1 does not register the object");
}
}

int main() {
    try {
        run();
    } catch (const std::exception& error) {
        std::cerr << "collision-enable: " << error.what() << '\n';
        return 1;
    }
    std::cout << "collision-enable: ok\n";
    return 0;
}
