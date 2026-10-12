// Opt-in check against the locally extracted game data (never committed).
// Usage: assets-real-test <lego_city3ds.fib> <lego_city3ds.csv>; exits 77 (skip) when absent.
// Decodes every archive entry, parses every pooled resource, decodes every
// texture, and compares FNV-1a checksums with tools/convert_textures.py --checksum.
#include "assets/assets.hpp"
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
using namespace chase::assets;

namespace {
std::uint32_t fnv1a(const std::vector<std::uint8_t>& data) {
    std::uint32_t value = 0x811c9dc5;
    for (auto byte : data) value = (value ^ byte) * 0x01000193;
    return value;
}
bool endsWith(const std::string& text, const std::string& suffix) {
    return text.size() >= suffix.size() && text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0;
}
}

int main(int argc, char** argv) {
    if (argc != 3 || !std::ifstream(argv[1]) || !std::ifstream(argv[2])) {
        std::cout << "game data not extracted; skipping\n";
        return 77;
    }
    // Reference values from: python3 tools/convert_textures.py --checksum <file>
    const std::map<std::string, std::uint32_t> expected{
        {"sprites/game/saveicon.bpng", 0x7d3a8f9f},
        {"models/textures/atlas_lvl_00_policestation_main.bdds", 0x8267ad3e},
        {"models/textures/cubemap_showroom_c.btga", 0x1e43400a},
        {"particles/cloud.btga", 0xec37160c},
        {"sprites/ui_loadingscreens/ui_ls_generic1.btga", 0xf832a49c},
    };
    try {
        FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        std::map<std::uint32_t, std::string> names;
        {
            std::ifstream csv(argv[2]);
            std::string line;
            std::getline(csv, line);
            while (std::getline(csv, line)) {
                auto comma = line.find(','), second = line.find(',', comma + 1);
                names[std::uint32_t(std::stoul(line.substr(0, comma), nullptr, 16))] =
                    line.substr(comma + 1, second - comma - 1);
            }
        }
        std::size_t bytes = 0, pooled = 0, textures = 0, checked = 0;
        std::size_t models = 0, parts = 0, vertices = 0, triangles = 0;
        std::size_t levels = 0, objects = 0, placed = 0;
        for (const auto& entry : archive.entries()) {
            Bytes data = archive.read(entry);
            if (data.size() != entry.size) throw std::runtime_error("entry size mismatch");
            bytes += data.size();
            auto name = names.count(entry.crc) ? names[entry.crc] : std::string();
            auto dot = name.rfind('.');
            std::string ext = dot == std::string::npos ? "" : name.substr(dot);
            if (ext.size() > 2 && ext[1] == 'b' && ext != ".bin") {
                parsePooled(data);
                ++pooled;
            }
            if (endsWith(name, ".blvl")) {
                Level level = parseLevel(data);
                ++levels;
                objects += level.objects.size();
                placed += placements(level).size();
            }
            if (endsWith(name, ".bfnmdl")) {
                Model model = parseModel(data);
                ++models;
                for (const auto& mesh : model.meshes)
                    for (const auto& part : mesh.parts) {
                        ++parts;
                        vertices += part.vertices.size();
                        triangles += part.indices.size() / 3;
                    }
            }
            if (endsWith(name, ".btga") || endsWith(name, ".bpng") || endsWith(name, ".bdds")) {
                Image image = decodeTexture(data);
                ++textures;
                auto want = expected.find(name);
                if (want != expected.end()) {
                    if (fnv1a(image.rgba) != want->second) throw std::runtime_error("checksum mismatch: " + name);
                    ++checked;
                }
            }
        }
        if (checked != expected.size()) throw std::runtime_error("reference textures not found");
        // Reference totals from: python3 tools/convert_models.py --census artifacts/fib/files
        if (models != 3113 || parts != 16122 || vertices != 7211595 || triangles != 4603391)
            throw std::runtime_error("model totals differ from the Python reference");
        // Reference totals from tools/convert_levels.py (parse_level / placements over all .blvl)
        if (levels != 856 || objects != 18469 || placed != 6563)
            throw std::runtime_error("level totals differ from the Python reference");
        std::cout << archive.entries().size() << " entries, " << bytes << " bytes, " << pooled
                  << " pooled resources, " << textures << " textures, " << checked << " checksums match, "
                  << models << " models / " << parts << " parts / " << vertices << " vertices / "
                  << triangles << " triangles, " << levels << " levels / " << objects << " objects / " << placed
                  << " placements\n";
    } catch (const std::exception& error) {
        std::cerr << "assets-real-test failed: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
