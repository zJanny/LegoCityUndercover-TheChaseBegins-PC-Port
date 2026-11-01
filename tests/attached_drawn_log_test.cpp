// AttachedDrawnLog (src/runtime/attached_drawn_log.cpp) on made-up matrices: one result per child, the first time its parent has moved more than
// a metre from where it was first seen, and the result is the child's translation in the parent's frame (turned with the parent, so a child
// that follows reports its attach position after the parent moved and turned, a child left at its placement another one).
#include "runtime/attached_drawn_log.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using chase::assets::Matrix;
using chase::runtime::AttachedDrawnLog;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

Matrix at(float yaw, float x, float y, float z) {
    const float c = std::cos(yaw), s = std::sin(yaw);
    return {c, 0, -s, 0, 0, 1, 0, 0, s, 0, c, 0, x, y, z, 1};
}

bool near(float a, float b) { return std::fabs(a - b) < 1e-4f; }

bool localIs(const std::optional<std::array<float, 3>>& local, float x, float y, float z) {
    return local && near((*local)[0], x) && near((*local)[1], y) && near((*local)[2], z);
}
}

int main() {
    try {
        const float quarter = 1.5707964f;
        const Matrix parent = at(0.0f, 10.0f, 0.0f, 20.0f), child = at(0.0f, 9.0f, 1.0f, 20.5f);
        const Matrix movedParent = at(quarter, 14.0f, 0.0f, 25.0f), followed = at(quarter, 14.5f, 1.0f, 26.0f);

        AttachedDrawnLog follower;
        require(!follower.observe("rider", child, parent), "nothing at the first sight");
        require(!follower.observe("rider", child, at(0.0f, 10.5f, 0.0f, 20.0f)), "nothing while the parent has moved half a metre");
        require(localIs(follower.observe("rider", followed, movedParent), -1.0f, 1.0f, 0.5f), "a child that followed the parent and its turn is at its attach position (-1, 1, 0.5)");
        require(!follower.observe("rider", followed, movedParent), "one result per child");

        AttachedDrawnLog stayed;
        require(!stayed.observe("rider", child, parent), "first sight");
        require(localIs(stayed.observe("rider", child, movedParent), 4.5f, 1.0f, -5.0f), "a child left at its placement is at (4.5, 1, -5) in the moved parent's frame");

        AttachedDrawnLog two;
        require(!two.observe("a", child, parent) && !two.observe("b", child, parent), "each child is seen on its own");
        require(localIs(two.observe("a", followed, movedParent), -1.0f, 1.0f, 0.5f) && two.observe("b", followed, movedParent).has_value(), "each child gets its own result");
    } catch (const std::exception& error) {
        std::cerr << "attached-drawn-log-test: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
