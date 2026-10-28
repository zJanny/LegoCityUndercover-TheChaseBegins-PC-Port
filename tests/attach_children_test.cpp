// Placed objects under a placed parent (docs/re/props-rocking.md, entry "Attached children take the parent's rotation"): the child's world is its
// local matrix times the parent's world every update, parents before their children, and a repeated AttachObject call takes the local matrix
// again (0x245008 per call). Made-up matrices; transcribed from the code by 20261027-mission13-E on 2026-10-27.
#include "runtime/attach_children.hpp"

#include <cmath>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>

namespace {
using chase::assets::Matrix;
using chase::runtime::AttachedChildren;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
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

struct World {
    std::map<std::string, Matrix> nodes;
    AttachedChildren::WorldOf lookup() {
        return [this](const std::string& name) -> std::optional<Matrix> {
            const auto found = nodes.find(name);
            if (found == nodes.end()) return std::nullopt;
            return found->second;
        };
    }
    void apply(const std::vector<std::pair<std::string, Matrix>>& changed) {
        for (const auto& [name, matrix] : changed) nodes[name] = matrix;
    }
};

void childTurnsWithTheParent() {
    World world;
    world.nodes["girder"] = rollAt(0.0f, 0.0f, 10.0f, 0.0f);
    world.nodes["rope"] = rollAt(0.0f, 0.0f, 12.0f, 0.0f);
    AttachedChildren children;
    require(children.attach("rope", "girder", 1, world.nodes["rope"], world.nodes["girder"]), "the first call is taken");
    require(!children.attach("rope", "girder", 1, world.nodes["rope"], world.nodes["girder"]), "the same call is not taken twice");
    world.apply(children.follow(world.lookup()));
    require(children.follow(world.lookup()).empty(), "nothing moved: nothing changes");
    const float roll = 0.0872665f;
    world.nodes["girder"] = rollAt(roll, 0.0f, 10.0f, 0.0f);
    const auto changed = children.follow(world.lookup());
    require(changed.size() == 1 && changed[0].first == "rope", "the child follows the parent");
    const Matrix expected = chase::runtime::childWorld(rollAt(0.0f, 0.0f, 2.0f, 0.0f), world.nodes["girder"]);
    require(near(changed[0].second, expected, 1e-4f), "the child takes the parent's rotation and its offset turns with it");
    require(std::fabs(changed[0].second[6] - std::sin(roll)) < 1e-4f && std::fabs(changed[0].second[14] - 2.0f * std::sin(roll)) < 1e-4f,
            "the rotation rows and the swung offset are the parent's");
}

void parentsGoFirst() {
    World world;
    world.nodes["platform"] = rollAt(0.0f, 0.0f, 20.0f, 0.0f);
    world.nodes["girder"] = rollAt(0.0f, 0.0f, 10.0f, 0.0f);
    world.nodes["crow"] = rollAt(0.0f, 0.0f, 11.0f, 0.0f);
    AttachedChildren children;
    children.attach("crow", "girder", 2, world.nodes["crow"], world.nodes["girder"]);
    children.attach("girder", "platform", 1, world.nodes["girder"], world.nodes["platform"]);
    world.nodes["platform"] = rollAt(0.0f, 0.0f, 15.0f, 0.0f);
    const auto changed = children.follow(world.lookup());
    require(changed.size() == 2 && changed[0].first == "girder" && changed[1].first == "crow", "the girder moves before the crow");
    require(near(changed[1].second, rollAt(0.0f, 0.0f, 6.0f, 0.0f), 1e-4f), "the crow follows the girder's new place in the same update");
}

void repeatedAttachTakesTheLocalMatrixAgain() {
    World world;
    world.nodes["girder"] = rollAt(0.0f, 0.0f, 10.0f, 0.0f);
    world.nodes["crow"] = rollAt(0.0f, 0.0f, 11.0f, 0.0f);
    AttachedChildren children;
    children.attach("crow", "girder", 1, world.nodes["crow"], world.nodes["girder"]);
    world.nodes["crow"] = rollAt(0.0f, 0.0f, 13.0f, 0.0f);
    require(children.attach("crow", "girder", 2, world.nodes["crow"], world.nodes["girder"]), "a new call to the same parent is taken");
    require(near(children.find("crow")->local, rollAt(0.0f, 0.0f, 3.0f, 0.0f), 1e-4f), "the local matrix is the child's world at the second call");
}

void otherWriterIsKeptRelative() {
    World world;
    world.nodes["girder"] = rollAt(0.0f, 0.0f, 10.0f, 0.0f);
    world.nodes["crow"] = rollAt(0.0f, 0.0f, 11.0f, 0.0f);
    AttachedChildren children;
    children.attach("crow", "girder", 1, world.nodes["crow"], world.nodes["girder"]);
    world.nodes["crow"] = rollAt(0.0f, 0.0f, 14.0f, 0.0f);
    world.apply(children.follow(world.lookup()));
    require(near(world.nodes["crow"], rollAt(0.0f, 0.0f, 14.0f, 0.0f), 1e-4f), "a world written by something else stays");
    world.nodes["girder"] = rollAt(0.0f, 0.0f, 20.0f, 0.0f);
    world.apply(children.follow(world.lookup()));
    require(near(world.nodes["crow"], rollAt(0.0f, 0.0f, 24.0f, 0.0f), 1e-4f), "and then follows the parent from there");
}

void ownMoveIsInTheParentsFrame() {
    World world;
    const float roll = 1.5707963f;
    world.nodes["girder"] = rollAt(roll, 0.0f, 10.0f, 0.0f);
    world.nodes["crow"] = rollAt(roll, 0.0f, 10.0f, 0.0f);
    AttachedChildren children;
    children.attach("crow", "girder", 1, world.nodes["crow"], world.nodes["girder"]);
    const float delta[3] = {0.0f, 0.0f, 5.0f};
    require(children.translate("crow", delta, world.nodes["girder"]), "a moved child is translated");
    world.apply(children.follow(world.lookup()));
    require(near(world.nodes["crow"], rollAt(roll, 0.0f, 10.0f, 5.0f), 1e-4f), "the world shifts by the world-space delta whatever the parent's rotation");
    require(!children.translate("rope", delta, world.nodes["girder"]), "an unknown child is not translated");
}

void detachStops() {
    World world;
    world.nodes["girder"] = rollAt(0.0f, 0.0f, 10.0f, 0.0f);
    world.nodes["crow"] = rollAt(0.0f, 0.0f, 11.0f, 0.0f);
    AttachedChildren children;
    children.attach("crow", "girder", 1, world.nodes["crow"], world.nodes["girder"]);
    children.detach("crow");
    world.nodes["girder"] = rollAt(0.0f, 0.0f, 20.0f, 0.0f);
    require(!children.has("crow") && children.follow(world.lookup()).empty(), "a detached child stays");
}
}

int main() {
    try {
        childTurnsWithTheParent();
        parentsGoFirst();
        repeatedAttachTakesTheLocalMatrixAgain();
        otherWriterIsKeptRelative();
        ownMoveIsInTheParentsFrame();
        detachStops();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::cout << "attach-children ok\n";
    return 0;
}
