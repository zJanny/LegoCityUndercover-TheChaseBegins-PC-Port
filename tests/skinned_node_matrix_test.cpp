// A skinned mesh's vertices are stored in the space of their node; skinnedPartInModelSpace applies the node matrix (stand-in MOVIE-S35).
// Reference values: the wheel vertices of the dog truck of cutscene 1_2_c (csdata/models/1_2_c/policebarker) and the slat vertices of
// models/prop_furnacevent, measured by a scratch program over parseModel and parseSkeleton in round 36 (rigid vertices = weight >= 250/255 on one joint).
#include "assets/assets.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase::assets;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

struct Centroid {
    double sum[3] = {0, 0, 0};
    double count = 0;
};

Centroid rigidCentroid(const Model& model, bool placed, unsigned joint) {
    Centroid out;
    for (std::size_t meshIndex = 0; meshIndex < model.meshes.size(); ++meshIndex)
        for (const ModelPart& unplaced : model.meshes[meshIndex].parts) {
            if (!(unplaced.flags & AttrSkin)) continue;
            const ModelPart part = placed ? skinnedPartInModelSpace(model, meshIndex, unplaced) : unplaced;
            for (const Vertex& v : part.vertices) {
                unsigned strongest = 0;
                for (unsigned k = 1; k < 4; ++k)
                    if (v.weights[k] > v.weights[strongest]) strongest = k;
                if (v.weights[strongest] < 250 || v.joints[strongest] != joint) continue;
                for (int i = 0; i < 3; ++i) out.sum[i] += v.position[i];
                out.count += 1;
            }
        }
    return out;
}

void realData(const char* fibPath, const char* namesPath) {
    FuseArchive archive(fibPath);
    archive.loadNames(namesPath);
    const auto model = [&](const std::string& path) {
        const FuseEntry* entry = archive.find(path);
        require(entry != nullptr, path + " in archive");
        return parseModel(archive.read(*entry));
    };
    const auto skeleton = [&](const std::string& path) {
        const FuseEntry* entry = archive.find(path);
        require(entry != nullptr, path + " in archive");
        return parseSkeleton(archive.read(*entry));
    };

    const Model truck = model("csdata/models/1_2_c/policebarker/policebarker.bfnmdl");
    const Skeleton truckBones = skeleton("csdata/models/1_2_c/policebarker/policebarker.bfnskl");
    const Centroid raw = rigidCentroid(truck, false, 1), lifted = rigidCentroid(truck, true, 1);
    require(raw.count == 256 && lifted.count == 256, "256 rigid wheel vertices on Wheels_Front");
    require(std::fabs(raw.sum[1] / raw.count - -1.09363) < 0.001, "unplaced wheel vertices average 1.0936 below the model origin");
    const double wheelHeight = lifted.sum[1] / lifted.count, jointHeight = truckBones.joints[1].world[13];
    require(std::fabs(wheelHeight - jointHeight) < 0.01, "placed wheel vertices centre on the Wheels_Front joint height " + std::to_string(jointHeight) + ", got " + std::to_string(wheelHeight));
    require(std::fabs(lifted.sum[2] / lifted.count - truckBones.joints[1].world[14]) < 0.01, "wheel depth unchanged");

    const Model vent = model("models/prop_furnacevent/prop_furnacevent.bfnmdl");
    const Skeleton ventBones = skeleton("models/prop_furnacevent/prop_furnacevent.bfnskl");
    const Centroid ventLifted = rigidCentroid(vent, true, 1);
    require(ventLifted.count == 24, "24 rigid slat vertices");
    require(std::fabs(ventLifted.sum[0] / ventLifted.count - ventBones.joints[1].world[12]) < 0.01, "placed slat vertices centre on the Slat_2 joint x");

    const Model dog = model("models/char_dog/char_dog.bfnmdl");
    for (std::size_t meshIndex = 0; meshIndex < dog.meshes.size(); ++meshIndex)
        for (const ModelPart& part : dog.meshes[meshIndex].parts) {
            const ModelPart placed = skinnedPartInModelSpace(dog, meshIndex, part);
            for (std::size_t i = 0; i < part.vertices.size(); ++i)
                for (int axis = 0; axis < 3; ++axis) require(placed.vertices[i].position[axis] == part.vertices[i].position[axis], "a model without a placed node keeps its vertices");
        }
}
}

int main(int argc, char** argv) {
    try {
        if (argc != 3) throw std::runtime_error("usage: skinned-node-matrix-test FIB NAMES");
        realData(argv[1], argv[2]);
    } catch (const std::exception& error) {
        std::cerr << "skinned-node-matrix: " << error.what() << '\n';
        return 1;
    }
    std::cout << "skinned-node-matrix: ok\n";
    return 0;
}
