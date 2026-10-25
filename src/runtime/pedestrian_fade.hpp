#pragma once
// The mesh nodes of the pedestrians and their alpha fade (docs/re/pedestrians-fade.md): the node a pedestrian entity hangs on as
// the manager's tier test and the draw test 0x2930e0 read it, the start 0x275434 after a spawn and the per-frame step 0x10eb70.
#include "runtime/node_fade.hpp"
#include "runtime/pedestrian_walk.hpp"

#include <array>
#include <cstddef>
#include <vector>

namespace chase::runtime {

// *0x37674c: the node type id that 0x103290 stores (the second registration of 0x258eb0, after slot 0 is reserved by 0x10332c).
// Read from the code, not measured at run time (PED-16).
constexpr std::uint32_t kMeshNodeType = 2;
constexpr std::uint32_t kMeshNodeDistanceFadeBit = 1u << 9;
constexpr std::uint8_t kMeshNodeFadeOn = 0x10;

// A mesh node as 0x10d2d4 creates it: the distance fade bit 9 of the word, bit 4 of +0xde, radius +0x9c.
PedestrianNode pedestrianMeshNode(float radius);

// Node +0xa0 (the bounds centre, local) and +0x9c (the radius).
struct PedestrianNodeBounds {
    PedestrianVec3 centre{};
    float radius = 0;
};

// The world position of the local bounds centre through the node rows (row vectors, translation in row 3).
PedestrianVec3 pedestrianNodeCentre(const PedestrianNodeBounds& bounds, const std::array<PedestrianVec3, 4>& rows);

// 0x2ae1ac, called by the camera manager update 0x23f174 (0x23f2b0): the fade point 0x52a1b4 is the look point of the camera pose
// C+0x4c, or its eye (C+0x70) when the pose flag 0x40000000 (C+0x5c) is set.
PedestrianVec3 pedestrianFadePoint(const PedestrianVec3& look, const PedestrianVec3& eye, bool poseNoRotation);

// The inputs of the draw test 0x2930e0 in the state step.
PedestrianDrawTest pedestrianDrawTest(const PedestrianNode& node, const PedestrianVec3& fadePoint, const DistanceFadeTable& table);

struct PedestrianFadeInputs {
    PedestrianVec3 fadePoint{};  // 0x52a1b4
    DistanceFadeTable table;
    bool jumpToTarget = false;   // 0x52a1cc
    bool forceFadeIn = false;    // 0x52a1cd
};

class PedestrianFades {
public:
    void load(std::size_t slots);
    void setBounds(std::size_t slot, const PedestrianNodeBounds& bounds);
    // 0x275434 after a spawn whose node has the mesh node type.
    void start(std::size_t slot);
    // 0x10eb70 for the node of a live pedestrian.
    void step(std::size_t slot, const PedestrianNode& node, const std::array<PedestrianVec3, 4>& rows, const PedestrianFadeInputs& inputs);

    const NodeFade& fade(std::size_t slot) const { return fades_[slot]; }
    const PedestrianNodeBounds& bounds(std::size_t slot) const { return bounds_[slot]; }

private:
    std::vector<NodeFade> fades_;
    std::vector<PedestrianNodeBounds> bounds_;
};

}
