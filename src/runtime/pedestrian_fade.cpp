#include "runtime/pedestrian_fade.hpp"

namespace chase::runtime {

PedestrianNode pedestrianMeshNode(float radius) {
    PedestrianNode node;
    node.word = kMeshNodeType | kMeshNodeDistanceFadeBit;
    node.fadeFlags = kMeshNodeFadeOn;
    node.radius = radius;
    return node;
}

PedestrianVec3 pedestrianNodeCentre(const PedestrianNodeBounds& bounds, const std::array<PedestrianVec3, 4>& rows) {
    PedestrianVec3 world{};
    for (std::size_t axis = 0; axis < 3; ++axis) {
        float sum = bounds.centre[0] * rows[0][axis];
        sum = sum + bounds.centre[1] * rows[1][axis];
        sum = sum + bounds.centre[2] * rows[2][axis];
        world[axis] = sum + rows[3][axis];
    }
    return world;
}

PedestrianDrawTest pedestrianDrawTest(const PedestrianNode& node, const PedestrianVec3& fadePoint, const DistanceFadeTable& table) {
    PedestrianDrawTest test;
    test.node = node;
    test.meshNodeType = kMeshNodeType;
    test.fade = table;
    test.fadePoint = fadePoint;
    return test;
}

void PedestrianFades::load(std::size_t slots) {
    fades_.assign(slots, NodeFade{});
    bounds_.assign(slots, PedestrianNodeBounds{});
}

void PedestrianFades::setBounds(std::size_t slot, const PedestrianNodeBounds& bounds) { bounds_.at(slot) = bounds; }

void PedestrianFades::start(std::size_t slot) { nodeFadeStart(fades_.at(slot)); }

void PedestrianFades::step(std::size_t slot, const PedestrianNode& node, const std::array<PedestrianVec3, 4>& rows,
                           const PedestrianFadeInputs& inputs) {
    NodeFadeFrame frame;
    frame.tier = pedestrianFadeTier(node, inputs.table);
    frame.radius = node.radius;
    frame.distanceSquared = pedestrianDistanceSquared(pedestrianNodeCentre(bounds_.at(slot), rows), inputs.fadePoint);
    frame.jumpToTarget = inputs.jumpToTarget;
    frame.forceFadeIn = inputs.forceFadeIn;
    nodeFadeStep(fades_.at(slot), frame, inputs.table);
}

}
