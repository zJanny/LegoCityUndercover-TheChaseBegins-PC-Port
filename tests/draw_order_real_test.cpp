// The part boxes the draw list sorts by (part record +0 / +0xc, read by 0x10bbe8; docs/re/rendering-draw-order.md), checked on every
// shipped model: each part's box equals the box of its vertices (parseModel adds the mesh centre of a relative mesh to both).
// Provenance of the pinned counts: this check run over the archive by 20261027-graphics-D on 2026-10-27 (3113 models, 16122 parts with
// vertices, 38 meshes with flag bit 8, the bit that makes 0x10bbe8 sort a blended part first).
#include "assets/assets.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void require(bool ok, const std::string& what) {
    if (!ok) throw std::runtime_error(what);
}

}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: draw-order-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "draw-order-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        chase::assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        std::size_t parts = 0, sortFirst = 0;
        for (const std::string& name : archive.names()) {
            if (name.size() < 7 || name.compare(name.size() - 7, 7, ".bfnmdl") != 0) continue;
            const chase::assets::Model model = chase::assets::parseModel(archive.read(*archive.find(name)));
            for (const chase::assets::ModelMesh& mesh : model.meshes) {
                if (mesh.flags & 0x100) ++sortFirst;
                for (const chase::assets::ModelPart& part : mesh.parts) {
                    if (part.vertices.empty()) continue;
                    ++parts;
                    for (int axis = 0; axis < 3; ++axis) {
                        float lo = part.vertices.front().position[axis], hi = lo;
                        for (const chase::assets::Vertex& v : part.vertices) {
                            lo = std::min(lo, v.position[axis]);
                            hi = std::max(hi, v.position[axis]);
                        }
                        const float centre = (lo + hi) / 2, half = (hi - lo) / 2, tolerance = 0.01f + 0.01f * std::fabs(centre);
                        require(std::fabs(part.centre[axis] - centre) <= tolerance && std::fabs(part.half[axis] - half) <= tolerance,
                                "part box differs from its vertices in " + name);
                    }
                }
            }
        }
        require(parts == 16122, "part count " + std::to_string(parts));
        require(sortFirst == 38, "meshes with flag bit 8: " + std::to_string(sortFirst));
    } catch (const std::exception& error) {
        std::cerr << "draw-order-real-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "draw-order-real-test: ok\n";
    return 0;
}
