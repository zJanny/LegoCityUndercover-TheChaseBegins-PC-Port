// Opt-in check against the locally extracted game data (never committed).
// Usage: assets-real-test <lego_city3ds.fib> <lego_city3ds.csv>; exits 77 (skip) when absent.
// Decodes every archive entry, parses every pooled resource, decodes every
// texture, and compares FNV-1a checksums with tools/convert_textures.py --checksum.
#include "assets/assets.hpp"
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
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
        std::size_t sounds = 0, samples = 0, loops = 0;
        std::size_t skinnedParts = 0, boneBatches = 0, jointSum = 0;
        std::size_t skeletons = 0, joints = 0, animations = 0, curves = 0;
        std::uint32_t soundSum = 0x811c9dc5;  // FNV-1a over act_bass_break's PCM, see below
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
            if (endsWith(name, ".bwav")) {
                Sound sound = decodeSound(data);
                ++sounds;
                loops += sound.loop;
                samples += std::size_t(sound.frames) * sound.channels.size();
                if (name == "music/act_bass_break.bwav")
                    for (std::uint32_t f = 0; f < sound.frames; ++f)
                        for (const auto& channel : sound.channels)
                            for (int b = 0; b < 2; ++b)
                                soundSum = (soundSum ^ std::uint8_t(std::uint16_t(channel[f]) >> (8 * b))) * 0x01000193;
            }
            if (endsWith(name, ".bfnskl")) {
                joints += parseSkeleton(data).joints.size();
                ++skeletons;
            }
            if (endsWith(name, ".bfnanm")) {
                curves += parseAnimation(data).jointCurves.size();
                ++animations;
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
                        if (!part.boneBatches.empty()) {
                            ++skinnedParts;
                            boneBatches += part.boneBatches.size();
                            for (auto index : std::set<std::uint16_t>(part.indices.begin(), part.indices.end()))
                                for (int k = 0; k < 4; ++k)
                                    if (part.vertices[index].weights[k]) jointSum += part.vertices[index].joints[k];
                        }
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
        // Reference: artifacts/audio/audio-manifest.json from tools/convert_audio.py, and FNV-1a over
        // the interleaved little-endian PCM of its music/act_bass_break.wav.
        if (sounds != 1361 || samples != 122501647 || loops != 123 || soundSum != 0xdcbf1f85)
            throw std::runtime_error("sound totals or checksum differ from the Python reference");
        // Reference: tools/convert_anims.py parse_skeleton/parse_animation over all files, and the sum of
        // |entries| of skin_matrices() for char_bear walk over all 48 frames.
        if (skeletons != 376 || joints != 5280 || animations != 1954 || curves != 30081)
            throw std::runtime_error("skeleton/animation totals differ from the Python reference");
        // Reference: convert_models.bone_batches/skin_joints over every part (drawn vertices only).
        if (skinnedParts != 1457 || boneBatches != 1465 || jointSum != 3492824)
            throw std::runtime_error("bone palette totals differ from the Python reference");
        const FuseEntry* bearSkeleton = archive.find("models/char_bear/char_bear.bfnskl");
        const FuseEntry* bearWalk = archive.find("models/char_bear/walk.bfnanm");
        if (!bearSkeleton || !bearWalk) throw std::runtime_error("char_bear reference files not found");
        Skeleton bear = parseSkeleton(archive.read(*bearSkeleton));
        Animation walk = parseAnimation(archive.read(*bearWalk));
        double poseSum = 0;
        for (unsigned frame = 0; frame < walk.frames; ++frame)
            for (const auto& m : skinMatrices(bear, walk, frame))
                for (float v : m) poseSum += std::fabs(v);
        if (walk.frames != 48 || std::fabs(poseSum - 5397.175751993695) > 1e-3)
            throw std::runtime_error("char_bear walk pose differs from the Python reference");
        // Reference: sum of |skinned position| over every vertex (preview_model.skin_vertex with
        // convert_anims.skin_matrices). The squirrel has rotated bind matrices and two bone batches;
        // the fractional frames blend neighbouring frames (47.5 wraps into frame 0).
        auto posedSum = [&](const std::string& dir, const std::string& anim, double frame) {
            auto read = [&](const std::string& name) {
                const FuseEntry* entry = archive.find(name);
                if (!entry) throw std::runtime_error("reference file not found: " + name);
                return archive.read(*entry);
            };
            std::string base = "models/" + dir + "/" + dir;
            Model model = parseModel(read(base + ".bfnmdl"));
            std::vector<Matrix> pose = skinMatrices(parseSkeleton(read(base + ".bfnskl")),
                                                    parseAnimation(read("models/" + dir + "/" + anim)), frame);
            double sum = 0;
            for (const auto& mesh : model.meshes)
                for (const auto& part : mesh.parts)
                    for (const auto& vertex : part.vertices) {
                        float position[3], normal[3];
                        skinVertex(vertex, pose, position, normal);
                        sum += std::fabs(position[0]) + std::fabs(position[1]) + std::fabs(position[2]);
                    }
            return sum;
        };
        if (std::fabs(posedSum("char_bear", "walk.bfnanm", 12) - 1754.6224097940715) > 1e-3 ||
            std::fabs(posedSum("char_squirrel", "run.bfnanm", 5) - 440.7127510475371) > 1e-3 ||
            std::fabs(posedSum("char_bear", "walk.bfnanm", 47.5) - 1741.8780890502587) > 1e-3 ||
            std::fabs(posedSum("char_squirrel", "run.bfnanm", 2.25) - 372.6377254145856) > 1e-3)
            throw std::runtime_error("skinned vertices differ from the Python reference");
        std::cout << archive.entries().size() << " entries, " << bytes << " bytes, " << pooled
                  << " pooled resources, " << textures << " textures, " << checked << " checksums match, "
                  << models << " models / " << parts << " parts / " << vertices << " vertices / "
                  << triangles << " triangles, " << levels << " levels / " << objects << " objects / " << placed
                  << " placements, " << skeletons << " skeletons / " << joints << " joints, " << animations
                  << " animations / " << curves << " joint curves, " << sounds << " sounds / " << samples << " samples / " << loops << " looping\n";
    } catch (const std::exception& error) {
        std::cerr << "assets-real-test failed: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
