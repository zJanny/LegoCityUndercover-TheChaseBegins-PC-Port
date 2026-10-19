// The poles of the 24 zipline anchors of lvl_01_cherrytreehills: each anchor is a drawable placement of the model
// models/ability_parkour_zip_line/ability_parkour_zip_line.bfnmdl (class meshname `Ability_Parkour_Zip_Line`; the create 0x1a999c loads it unless
// "NoMesh" is set), and with the node 0 matrix of the model (translation y -27.432, found with a parseModel dump) the mesh lies below the anchor
// node: y from -4.05 to 0.16, x / z within 0.8. Without the matrix the mesh hangs 23 to 28 above the anchor. Needs the extracted RomFS; exits 77
// without it.
#include "assets/assets.hpp"
#include "runtime/zipline_pole.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

std::string lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return text;
}
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: zipline-pole-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "zipline-pole-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        using namespace chase;
        assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        const assets::FuseEntry* levelEntry = archive.find("levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl");
        require(levelEntry != nullptr, "lvl_01 is in the archive");
        const assets::Level level = assets::parseLevel(archive.read(*levelEntry));
        const std::vector<runtime::ZiplineAnchor> anchors = runtime::loadZiplineAnchors(level, assets::archiveClassLookup(archive));
        require(anchors.size() == 24, "zipline anchors: " + std::to_string(anchors.size()));
        const assets::DrawableObjects drawable = assets::drawableObjects(archive, level);
        const assets::FuseEntry* modelEntry = archive.find("models/ability_parkour_zip_line/ability_parkour_zip_line.bfnmdl");
        require(modelEntry != nullptr, "the pole model is in the archive");
        const assets::Model model = assets::parseModel(archive.read(*modelEntry));
        const auto node = runtime::modelRootNodeMatrix(model);
        require(node.has_value(), "the pole model has a root node matrix");
        std::size_t checked = 0;
        for (const runtime::ZiplineAnchor& anchor : anchors) {
            const assets::Placement* placement = nullptr;
            for (const assets::Placement& candidate : drawable.objects)
                if (lower(candidate.instance) == anchor.instance) placement = &candidate;
            require(placement != nullptr, anchor.instance + " is a drawable placement");
            require(assets::meshModelPath(placement->mesh) == "models/ability_parkour_zip_line/ability_parkour_zip_line.bfnmdl", anchor.instance + " has the pole model");
            const assets::Matrix world = runtime::ziplinePoleMatrix(*node, anchor, assets::placementMatrix(*placement));
            for (const assets::ModelPart& part : model.meshes.at(std::size_t(model.nodeMesh[0])).parts)
                for (const assets::Vertex& vertex : part.vertices) {
                    float out[3];
                    for (int c = 0; c < 3; ++c)
                        out[c] = vertex.position[0] * world[c] + vertex.position[1] * world[4 + c] + vertex.position[2] * world[8 + c] + world[12 + c];
                    const float dx = out[0] - anchor.position[0], dy = out[1] - anchor.position[1], dz = out[2] - anchor.position[2];
                    require(dy > -4.1f && dy < 0.2f && dx > -0.9f && dx < 0.9f && dz > -0.9f && dz < 0.9f,
                            anchor.instance + " pole vertex off the anchor: " + std::to_string(dx) + " " + std::to_string(dy) + " " + std::to_string(dz));
                }
            ++checked;
        }
        require(checked == 24, "all anchors checked");
    } catch (const std::exception& error) {
        std::cerr << "zipline-pole-real-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "zipline-pole-real-test: ok\n";
    return 0;
}
