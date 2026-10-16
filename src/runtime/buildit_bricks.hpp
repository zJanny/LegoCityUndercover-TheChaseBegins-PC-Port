#pragma once
// The bricks of a build-it (class props/prop_buildit): one node of the parts model each, scattered at the first frame of the model's
// _buildit animation and flown to the last frame while it builds (0x18ddc0, 0x21e96c, 0x19c9e0, docs/re/buildit.md). No SDL dependency.
#include "assets/assets.hpp"
#include "runtime/buildit.hpp"
#include "runtime/prop_nodes.hpp"
#include <string>
#include <vector>

namespace chase::runtime {

struct BuilditBrick {
    std::size_t node = 0;  // node of the parts model
    NodePose start;        // record +0x28 / +0x40: the scattered pose
    NodePose end;          // record +0x34 / +0x50: the built pose
};

class BuilditBricks {
public:
    // Nodes with a mesh become bricks in node order. A node whose name the animation lacks stays where the model has it (start = end).
    static BuilditBricks of(const assets::Model& model, const assets::Animation* animation);
    // Animation of a parts model named by the object's mesh ("Dir/File"): models/<dir>/<dir>_buildit.bfnanm, lower case (0x18ddc0 builds the
    // name from the strings "models/%s/", "_buildit" and ".fnanm"; the folder name of the mesh is the %s, an inference from the shipped file).
    static std::string animationPath(const std::string& mesh);
    static BuilditBricks load(assets::FuseArchive& archive, const std::string& mesh);

    // 0x2318c8: the end matrix of every brick times `relative` (the final prop's world matrix times the inverse of the parts' own), so the bricks
    // fly to the final prop's place; the start poses stay.
    void buildAt(const NodeMatrix& relative);

    const std::vector<BuilditBrick>& bricks() const { return bricks_; }
    // Node matrix of brick `index` at flight progress `t` (0 scattered, 1 built).
    NodeMatrix matrix(std::size_t index, float t) const;

private:
    std::vector<BuilditBrick> bricks_;
};

}
