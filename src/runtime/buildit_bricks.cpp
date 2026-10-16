#include "runtime/buildit_bricks.hpp"

#include <algorithm>
#include <cctype>

namespace chase::runtime {
namespace {

std::string lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return text;
}

}

BuilditBricks BuilditBricks::of(const assets::Model& model, const assets::Animation* animation) {
    BuilditBricks out;
    const PropNodes nodes = PropNodes::of(model);
    for (std::size_t node = 0; node < nodes.mesh.size(); ++node) {
        if (nodes.mesh[node] < 0) continue;
        BuilditBrick brick;
        brick.node = node;
        brick.start = brick.end = nodePoseOf(nodes.local[node]);
        if (animation && animation->frames > 0) {
            for (std::size_t object = 0; object < animation->objects.size() && object < animation->objectBlocks.size(); ++object) {
                if (lower(animation->objects[object]) != lower(model.nodeNames[node])) continue;
                brick.start = nodePoseOf(assets::objectAnimationMatrix(*animation, object, 0));
                brick.end = nodePoseOf(assets::objectAnimationMatrix(*animation, object, animation->frames - 1));
                break;
            }
        }
        out.bricks_.push_back(brick);
    }
    return out;
}

std::string BuilditBricks::animationPath(const std::string& mesh) {
    const std::string name = lower(mesh.substr(0, mesh.find('/')));
    return "models/" + name + "/" + name + "_buildit.bfnanm";
}

BuilditBricks BuilditBricks::load(assets::FuseArchive& archive, const std::string& mesh) {
    const assets::FuseEntry* modelEntry = archive.find(assets::meshModelPath(mesh));
    if (!modelEntry) return {};
    const assets::Model model = assets::parseModel(archive.read(*modelEntry));
    const assets::FuseEntry* animationEntry = archive.find(animationPath(mesh));
    if (!animationEntry) return of(model, nullptr);
    const assets::Animation animation = assets::parseAnimation(archive.read(*animationEntry));
    return of(model, &animation);
}

void BuilditBricks::buildAt(const NodeMatrix& relative) {
    for (BuilditBrick& brick : bricks_) brick.end = nodePoseOf(assets::multiply(nodeMatrixOf(brick.end), relative));
}

NodeMatrix BuilditBricks::matrix(std::size_t index, float t) const {
    const BuilditBrick& brick = bricks_.at(index);
    return nodeFlight(brick.start, brick.end, t);
}

}
