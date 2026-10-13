#include "assets/assets.hpp"
#include <cstdio>
#include <exception>
#include <iostream>
#include <map>
#include <string>

using namespace chase::assets;

namespace {
std::string describe(const ModelPart& part) {
    const MaterialState s = materialState(part);
    char text[160];
    std::snprintf(text, sizeof text, "mode %u lighting %u texture %u alpha %u textures %u colour %u%s", s.mode, s.lightingKey,
                  s.textureKey, s.alphaKey, (part.material[10] & 0x3fu) >> 3, (part.material[10] >> 6) & 1u,
                  s.supported ? "" : " UNSUPPORTED");
    return text;
}

void printStages(const ModelPart& part) {
    const MaterialState s = materialState(part);
    std::printf("  %s | alpha test %d ref %u | blend %u %u %u | depth %u test %d write %d | cull %u | buffer %x | units %d %d %d%s\n",
                describe(part).c_str(), s.alphaTest, s.alphaReference, s.blendEquation, s.blendSource, s.blendDestination,
                s.depthFunction, s.depthTest, s.depthWrite, s.cullMode, s.bufferUpdate, s.unitSlot[0], s.unitSlot[1],
                s.unitSlot[2], s.reflection ? " reflection" : "");
    for (std::size_t i = 0; i < s.stages.size(); ++i) {
        const TevStage& t = s.stages[i];
        std::printf("    stage %zu source %08x operand %08x combiner %08x constant %08x scale %x\n", i, t.source, t.operand,
                    t.combiner, t.constant, t.scale);
    }
}
}

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "usage: chase-materials <fib> <csv> [model path ...]\n";
        return 2;
    }
    try {
        FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        if (argc > 3) {
            for (int i = 3; i < argc; ++i) {
                const FuseEntry* entry = archive.find(argv[i]);
                if (!entry) {
                    std::cerr << "not in archive: " << argv[i] << "\n";
                    return 1;
                }
                const Model model = parseModel(archive.read(*entry));
                std::printf("%s\n", argv[i]);
                for (const auto& mesh : model.meshes)
                    for (const auto& part : mesh.parts) {
                        std::printf(" part textures:");
                        for (const auto& name : part.textures) std::printf(" %s", name.c_str());
                        std::printf("\n");
                        printStages(part);
                    }
            }
            return 0;
        }
        std::map<std::string, std::size_t> census;
        std::size_t parts = 0;
        for (const auto& name : archive.names()) {
            if (name.size() < 7 || name.compare(name.size() - 7, 7, ".bfnmdl") != 0) continue;
            const Model model = parseModel(archive.read(*archive.find(name)));
            for (const auto& mesh : model.meshes)
                for (const auto& part : mesh.parts) {
                    ++census[describe(part)];
                    ++parts;
                }
        }
        std::printf("parts %zu\n", parts);
        for (const auto& [key, count] : census) std::printf("%7zu %s\n", count, key.c_str());
    } catch (const std::exception& error) {
        std::cerr << error.what() << "\n";
        return 1;
    }
    return 0;
}
