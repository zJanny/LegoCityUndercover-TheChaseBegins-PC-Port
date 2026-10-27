// Reflection cube maps (src/runtime/reflection.cpp, docs/re/rendering-reflections.md). Rules: llvm-objdump of the material setup
// 0x129560 (reflection flag 0x37796d), the texture binding 0x12df94 (cube type bit 0x10000000, face addresses to registers 0x85..0x8a)
// and a PICA200 disassembly of the shader blob at code.bin 0x2779a0 (subroutine 0x39..0x48), 20261027-graphics-C, 2026-10-27. With
// (FIB NAMES) the census of all shipped models is checked; exits with 77 (skipped) when the data is missing.
#include "assets/assets.hpp"
#include "runtime/reflection.hpp"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>

namespace {
using namespace chase::runtime;
using namespace chase::assets;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

void sampling() {
    require(unitSampling(0, true, true, true) == UnitSampling::Cube, "reflection material: the cube texture of unit 0 is a cube map");
    require(unitSampling(1, false, true, true) == UnitSampling::Disabled, "0x12df94 skips a cube texture while the reflection flag is clear");
    require(unitSampling(0, false, true, true) == UnitSampling::Disabled, "no cube map without the reflection flag");
    require(unitSampling(1, true, true, true) == UnitSampling::Disabled, "only unit 0 has a cube type");
    require(unitSampling(1, true, true, false) == UnitSampling::Texture, "the diffuse texture of a reflection material is a 2D texture");
    require(unitSampling(0, true, false, true) == UnitSampling::None, "no texture, nothing bound");
}

void coordinates() {
    require(unitUvSet(1, true, true) == 0, "subroutine 0x39: texture coordinate 1 is uv0");
    require(unitUvSet(2, true, true) == 1, "subroutine 0x39: texture coordinate 2 is uv1");
    require(unitUvSet(0, false, true) == 0 && unitUvSet(1, false, true) == 1 && unitUvSet(1, false, false) == 0,
            "without the reflection flag the mapping of RENDER-S4 stays");
}

void faces() {
    const CubeFace expected[6] = {CubeFace::PositiveX, CubeFace::NegativeX, CubeFace::PositiveY,
                                  CubeFace::NegativeY, CubeFace::PositiveZ, CubeFace::NegativeZ};
    for (int i = 0; i < 6; ++i) require(cubeFace(i) == expected[i], "stored face " + std::to_string(i) + " goes to register 0x85 + i");
    bool threw = false;
    try {
        cubeFace(6);
    } catch (const std::out_of_range&) {
        threw = true;
    }
    require(threw, "a seventh face is rejected");
}

std::string stem(const std::string& path) {
    const auto slash = path.find_last_of("/\\");
    std::string name = slash == std::string::npos ? path : path.substr(slash + 1);
    const auto dot = name.rfind('.');
    name = dot == std::string::npos ? name : name.substr(0, dot);
    std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return name;
}

int realData(const std::string& fibPath, const std::string& namesPath) {
    if (!std::filesystem::exists(fibPath) || !std::filesystem::exists(namesPath)) return 77;
    FuseArchive archive(fibPath);
    archive.loadNames(namesPath);
    std::map<std::string, std::string> textures;
    for (const auto& name : archive.names()) {
        const auto dot = name.rfind('.');
        const std::string ext = dot == std::string::npos ? "" : name.substr(dot);
        if (ext == ".btga" || ext == ".bpng" || ext == ".bdds") textures.emplace(stem(name), name);
    }
    std::map<std::string, bool> cubeCache;
    const auto isCube = [&](const std::string& texture) {
        const std::string key = stem(texture);
        const auto known = cubeCache.find(key);
        if (known != cubeCache.end()) return known->second;
        const auto path = textures.find(key);
        require(path != textures.end(), "texture " + texture + " is in the archive");
        return cubeCache[key] = decodeTexture(archive.read(*archive.find(path->second))).faces == 6;
    };
    std::size_t reflective = 0, cubeOnUnit0 = 0, disabledCubes = 0;
    std::map<unsigned, std::size_t> byTextureKey;
    std::set<std::string> cubeTextures;
    for (const auto& name : archive.names()) {
        if (name.size() < 7 || name.compare(name.size() - 7, 7, ".bfnmdl") != 0) continue;
        const Model model = parseModel(archive.read(*archive.find(name)));
        for (const auto& mesh : model.meshes)
            for (const auto& part : mesh.parts) {
                if (part.material.size() < 0x60) continue;
                const MaterialState state = materialState(part);
                if (state.reflection) {
                    ++reflective;
                    ++byTextureKey[state.textureKey];
                    require(part.flags & AttrNormal, name + ": a reflection part has normals");
                }
                for (int unit = 0; unit < 3; ++unit) {
                    const int slot = state.unitSlot[unit];
                    if (slot < 0 || std::size_t(slot) >= part.textures.size()) continue;
                    const bool cube = isCube(part.textures[std::size_t(slot)]);
                    const UnitSampling use = unitSampling(unit, state.reflection, true, cube);
                    if (use == UnitSampling::Cube) {
                        ++cubeOnUnit0;
                        require(part.textureSlots.at(std::size_t(slot) * 8 + 7) == 5, name + ": the cube map has slot role 5");
                        cubeTextures.insert(stem(part.textures[std::size_t(slot)]));
                    }
                    if (use == UnitSampling::Disabled) {
                        ++disabledCubes;
                        for (const TevStage& stage : state.stages)
                            for (int field = 0; field < 7; ++field)
                                require(((stage.source >> (4 * field)) & 0xf) != std::uint32_t(3 + unit),
                                        name + ": no stage reads a switched-off cube unit");
                    }
                    if (state.reflection && unit == 1) require(!cube, name + ": the diffuse unit of a reflection part is 2D");
                }
            }
    }
    require(reflective == 354, "354 reflection parts, got " + std::to_string(reflective));
    require(byTextureKey[5] == 279 && byTextureKey[6] == 6 && byTextureKey[7] == 69, "texture bytes 5 / 6 / 7: 279 / 6 / 69");
    require(cubeOnUnit0 == reflective, "every reflection part has a cube map on unit 0, so no part needs a stand-in colour");
    require(disabledCubes == 8, "8 parts outside reflection materials bind a cube texture (second slot), switched off, got " +
                                    std::to_string(disabledCubes));
    require(cubeTextures == std::set<std::string>{"cubemap_abstract_c", "cubemap_showroom_c", "cubemap_vehicle_windows_c"},
            "three cube maps are used");
    for (const auto& texture : cubeTextures) {
        const Image image = decodeTexture(archive.read(*archive.find(textures.at(texture))));
        require(image.faces == 6 && image.width == 128 && image.height == 6 * 128, texture + ": six faces of 128 x 128");
    }
    std::cout << "reflection-real-data: " << reflective << " reflection parts, " << cubeTextures.size() << " cube maps\n";
    return 0;
}
}

int main(int argc, char** argv) {
    try {
        if (argc == 3) return realData(argv[1], argv[2]);
        sampling();
        coordinates();
        faces();
        std::cout << "reflection-core: ok\n";
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << "\n";
        return 1;
    }
    return 0;
}
