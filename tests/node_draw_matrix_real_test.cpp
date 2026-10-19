// Every model node's mesh is drawn with assets::nodeDrawMatrix before the node world (model draw 0x205160 -> 0x10e028); skinned vertices are
// skinned first. Reference values: parseModel / parseSkeleton dumps by scratch programs (round 36: the policebarker wheels and the furnace vent
// slats; round 37: Prop_Plant_04 and Prop_Construction_Elevator_Runners). The drawn bounds equal the object box of modelBounds, the replica of
// 0x242eb0 whose boxes matched 405 of 406 entity boxes of the oracle grid read at gameplay-start (docs/re/collision.md §2026-10-13T20:53:44).
#include "assets/assets.hpp"
#include "assets/object_collision.hpp"
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase::assets;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

void transform(const Matrix& m, const float in[3], float out[3]) {
    for (int c = 0; c < 3; ++c) out[c] = in[0] * m[c] + in[1] * m[4 + c] + in[2] * m[8 + c] + m[12 + c];
}

struct Centroid {
    double sum[3] = {0, 0, 0};
    double count = 0;
};

// Bind pose: every skin matrix is the identity, so the skinned vertex is the stored vertex and the node matrix follows.
Centroid rigidCentroid(const Model& model, bool drawn, unsigned joint) {
    Centroid out;
    for (std::size_t node = 0; node < model.nodeMesh.size(); ++node) {
        if (model.nodeMesh[node] < 0) continue;
        const Matrix m = drawn ? nodeDrawMatrix(model, node) : Matrix{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
        for (const ModelPart& part : model.meshes[std::size_t(model.nodeMesh[node])].parts) {
            if (!(part.flags & AttrSkin)) continue;
            for (const Vertex& v : part.vertices) {
                unsigned strongest = 0;
                for (unsigned k = 1; k < 4; ++k)
                    if (v.weights[k] > v.weights[strongest]) strongest = k;
                if (v.weights[strongest] < 250 || v.joints[strongest] != joint) continue;
                float p[3];
                transform(m, v.position, p);
                for (int i = 0; i < 3; ++i) out.sum[i] += p[i];
                out.count += 1;
            }
        }
    }
    return out;
}

struct Bounds {
    float lo[3] = {1e30f, 1e30f, 1e30f}, hi[3] = {-1e30f, -1e30f, -1e30f};
};

Bounds drawnBounds(const Model& model) {
    Bounds b;
    for (std::size_t node = 0; node < model.nodeMesh.size(); ++node) {
        if (model.nodeMesh[node] < 0) continue;
        const Matrix m = nodeDrawMatrix(model, node);
        for (const ModelPart& part : model.meshes[std::size_t(model.nodeMesh[node])].parts)
            for (const Vertex& v : part.vertices) {
                float p[3];
                transform(m, v.position, p);
                for (int i = 0; i < 3; ++i) {
                    b.lo[i] = std::min(b.lo[i], p[i]);
                    b.hi[i] = std::max(b.hi[i], p[i]);
                }
            }
    }
    return b;
}

bool near(float a, float b, float tolerance) { return std::fabs(a - b) < tolerance; }

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
    require(std::fabs(raw.sum[1] / raw.count - -1.09363) < 0.001, "stored wheel vertices average 1.0936 below the model origin");
    const double wheelHeight = lifted.sum[1] / lifted.count, jointHeight = truckBones.joints[1].world[13];
    require(std::fabs(wheelHeight - jointHeight) < 0.01, "drawn wheel vertices centre on the Wheels_Front joint height " + std::to_string(jointHeight) + ", got " + std::to_string(wheelHeight));
    require(std::fabs(lifted.sum[2] / lifted.count - truckBones.joints[1].world[14]) < 0.01, "wheel depth unchanged");

    const Model vent = model("models/prop_furnacevent/prop_furnacevent.bfnmdl");
    const Skeleton ventBones = skeleton("models/prop_furnacevent/prop_furnacevent.bfnskl");
    const Centroid ventLifted = rigidCentroid(vent, true, 1);
    require(ventLifted.count == 24, "24 rigid slat vertices");
    require(std::fabs(ventLifted.sum[0] / ventLifted.count - ventBones.joints[1].world[12]) < 0.01, "drawn slat vertices centre on the Slat_2 joint x");

    const Model dog = model("models/char_dog/char_dog.bfnmdl");
    const Matrix identity = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    for (std::size_t node = 0; node < dog.nodeMesh.size(); ++node) require(nodeDrawMatrix(dog, node) == identity, "the dog's nodes draw with the identity");

    const std::string plantPath = "models/prop_plant_04/prop_plant_04_atlas_lvl_01_cherrytreehills_main.bfnmdl";
    const Bounds plant = drawnBounds(model(plantPath));
    require(near(plant.lo[1], 0.0f, 1e-3f) && near(plant.hi[1], 2.385f, 2e-3f), "Prop_Plant_04 stands on its node: y " + std::to_string(plant.lo[1]) + " .. " + std::to_string(plant.hi[1]));

    const std::string runnersPath = "models/prop_construction_elevator_runners/prop_construction_elevator_runners.bfnmdl";
    const Bounds runners = drawnBounds(model(runnersPath));
    require(near(runners.lo[0], -3.519f, 2e-3f) && near(runners.hi[0], 3.519f, 2e-3f) && near(runners.lo[2], -1.066f, 2e-3f) && near(runners.hi[2], 1.225f, 2e-3f),
            "the elevator runners are turned 90 degrees: x " + std::to_string(runners.lo[0]) + " .. " + std::to_string(runners.hi[0]) + ", z " +
                std::to_string(runners.lo[2]) + " .. " + std::to_string(runners.hi[2]));

    for (const std::string& path : {plantPath, runnersPath}) {
        const Bounds drawn = drawnBounds(model(path));
        float centre[3], half[3];
        require(modelBounds(archive, path, centre, half), path + " has an object box");
        for (int a = 0; a < 3; ++a)
            require(near(drawn.lo[a], centre[a] - half[a], 2e-3f) && near(drawn.hi[a], centre[a] + half[a], 2e-3f), path + " drawn bounds equal the object box on axis " + std::to_string(a));
    }
}
}

int main(int argc, char** argv) {
    try {
        if (argc != 3) throw std::runtime_error("usage: node-draw-matrix-real-test FIB NAMES");
        if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
            std::cout << "node-draw-matrix-real-data: data not available, skipped\n";
            return 77;
        }
        realData(argv[1], argv[2]);
    } catch (const std::exception& error) {
        std::cerr << "node-draw-matrix-real-data: " << error.what() << '\n';
        return 1;
    }
    std::cout << "node-draw-matrix-real-data: ok\n";
    return 0;
}
