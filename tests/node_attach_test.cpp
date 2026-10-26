// Scene nodes under a parent (docs/re/props-rocking.md, entry "AttachObject carries the girder guy"): 0x245008 keeps the child's world by storing
// it relative to the parent (0x258bc4: world times the rigid inverse of the parent), the child's world is local times the parent's world, and
// 0x24c118 on a child keeps its new world relative to the parent. Made-up matrices; read from the code by 20261026-crane-C on 2026-10-26.
#include "runtime/node_attach.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using chase::assets::Matrix;
using chase::runtime::NodeAttachments;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

Matrix yawAt(float radians, float x, float y, float z) {
    const float c = std::cos(radians), s = std::sin(radians);
    return {c, 0, -s, 0, 0, 1, 0, 0, s, 0, c, 0, x, y, z, 1};
}

Matrix rollAt(float radians, float x, float y, float z) {
    const float c = std::cos(radians), s = std::sin(radians);
    return {1, 0, 0, 0, 0, c, s, 0, 0, -s, c, 0, x, y, z, 1};
}

bool near(const Matrix& a, const Matrix& b, float tolerance) {
    for (int i = 0; i < 16; ++i)
        if (std::fabs(a[i] - b[i]) > tolerance) return false;
    return true;
}

void attachKeepsTheWorld() {
    const Matrix parent = yawAt(3.14159265f, 947.883f, 27.481f, 1142.455f);
    const Matrix child = yawAt(0.0f, 947.868f, 28.259f, 1142.455f);
    const Matrix local = chase::runtime::relativeToParent(child, parent);
    require(near(local, yawAt(3.14159265f, 0.015f, 0.778f, 0.0f), 2e-4f), "a parent turned by 180 degrees: the child's offset is turned back into the parent's frame");
    require(near(chase::runtime::childWorld(local, parent), child, 2e-4f), "at the attach the child keeps its world (0x245008)");
}

void childFollowsTheParent() {
    NodeAttachments links;
    const Matrix parent = yawAt(0.0f, 0.0f, 10.0f, 0.0f);
    links.attach("guy", "girder", yawAt(0.0f, 0.0f, 11.0f, 0.0f), parent);
    require(links.find("guy") && links.find("guy")->parent == "girder", "the link names the parent");
    const float roll = 0.0436332f;
    const Matrix rolled = rollAt(roll, 0.0f, 10.0f, 0.0f);
    const Matrix world = chase::runtime::childWorld(links.find("guy")->local, rolled);
    require(std::fabs(world[13] - (10.0f + std::cos(roll))) < 1e-5f && std::fabs(world[14] - std::sin(roll)) < 1e-5f && std::fabs(world[6] - std::sin(roll)) < 1e-6f,
            "a parent rolled by 2.5 degrees moves the child on its arc and turns it with it");
    const Matrix lowered = yawAt(0.0f, 0.0f, -0.629f, 0.0f);
    require(std::fabs(chase::runtime::childWorld(links.find("guy")->local, lowered)[13] - 0.371f) < 1e-5f, "a parent moved down by 10.629 carries the child");
    require(links.setWorld("guy", yawAt(0.0f, 5.0f, 2.0f, 0.0f), lowered), "0x24c118 on an attached child");
    require(near(chase::runtime::childWorld(links.find("guy")->local, lowered), yawAt(0.0f, 5.0f, 2.0f, 0.0f), 1e-5f), "the new world is kept under the parent");
    const Matrix kept = *links.follow("guy", yawAt(0.0f, 5.0f, 2.0f, 0.0f), lowered);
    require(near(kept, yawAt(0.0f, 5.0f, 2.0f, 0.0f), 1e-5f), "follow: the world of the last write stays while the parent rests");
    const Matrix up = yawAt(0.0f, 0.0f, 0.371f, 0.0f);
    require(std::fabs((*links.follow("guy", kept, up))[13] - 3.0f) < 1e-5f, "follow: the parent's move carries the child");
    const Matrix placed = yawAt(1.0f, 7.0f, 3.0f, 1.0f);
    require(near(*links.follow("guy", placed, up), placed, 1e-5f), "follow: a world written by another writer (PositionObject, the AI) is kept under the parent");
    require(std::fabs((*links.follow("guy", placed, lowered))[13] - 2.0f) < 1e-5f, "and moves with the parent from there");
    links.detach("guy");
    require(!links.find("guy") && !links.setWorld("guy", parent, parent), "a detached child has no parent");
}

}

int main() {
    try {
        attachKeepsTheWorld();
        childFollowsTheParent();
    } catch (const std::exception& error) {
        std::cerr << "node-attach: " << error.what() << '\n';
        return 1;
    }
    std::cout << "node-attach: ok\n";
    return 0;
}
