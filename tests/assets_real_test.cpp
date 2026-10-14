// Opt-in check against the locally extracted game data (never committed).
// Usage: assets-real-test <lego_city3ds.fib> <lego_city3ds.csv>; exits 77 (skip) when absent.
// Decodes every archive entry, parses every pooled resource, decodes every
// texture, and compares FNV-1a checksums with tools/convert_textures.py --checksum (at 9d04c91).
#include "assets/assets.hpp"
#include <algorithm>
#include <cctype>
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
    // Reference values from: python3 tools/convert_textures.py --checksum <file> (at 9d04c91)
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
        std::size_t levels = 0, objects = 0, placed = 0, drawable = 0, drawableMissing = 0, drawableSkeletal = 0, drawableMarkers = 0, drawableCharacters = 0, drawableCustom = 0, drawableStuds = 0;
        std::size_t collisionModels = 0, collisionItems = 0, collisionTriangles = 0, collisionNodes = 0;
        std::size_t unsupportedMaterials = 0;
        std::map<std::string, std::size_t> materialCensus;
        std::size_t sounds = 0, samples = 0, loops = 0;
        std::size_t locFiles = 0, locEntries = 0, fonts = 0, glyphs = 0;
        std::size_t navFiles = 0, navGrids = 0, navPaths = 0, navFlagged = 0;
        std::size_t camFiles = 0, camRoots = 0, camBlocks = 0, camEntries = 0, camSetups = 0;
        std::size_t configFiles = 0, configBlocks = 0, configEntries = 0;
        std::vector<std::string> configRejected;
        auto countBlocks = [](const auto& self, const ConfigBlock& block, std::size_t& blocks,
                              std::size_t& entries) -> void {
            ++blocks;
            entries += block.entries.size();
            for (const auto& child : block.blocks) self(self, child, blocks, entries);
        };
        std::map<std::string, std::pair<std::size_t, std::size_t>> navCounts;
        std::vector<std::pair<std::string, NavRoom>> navDescriptors;
        std::size_t skinnedParts = 0, boneBatches = 0, jointSum = 0;
        std::size_t skeletons = 0, joints = 0, animations = 0, curves = 0;
        std::size_t longCurves = 0;
        std::size_t subAnimations = 0, animTracks = 0, trackKeys = 0, cutsceneActorCount = 0;
        std::vector<std::string> manifestPaths, missingManifestFiles;
        std::size_t tableFiles = 0, tableSheets = 0, tableCells = 0, studThreshold = 0;
        std::string cutsceneVideo;
        std::size_t layouts = 0, storyboards = 0, tracks = 0, keyframes = 0, unresolvedTargets = 0;
        std::map<UiElementType, std::size_t> layoutElements;
        double layoutGeometry = 0, keyframeSum = 0;
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
                Animation animation = parseAnimation(data);
                curves += animation.jointCurves.size();
                longCurves += animation.longCurve >= 0;
                ++animations;
                subAnimations += animation.subAnimations.size();
                for (const auto& tracks : animation.subAnimations) {
                    animTracks += tracks.size();
                    for (const auto& track : tracks) trackKeys += track.keys.size();
                }
            }
            if (endsWith(name, ".bxls")) {
                Table table = parseTable(data);
                ++tableFiles;
                tableSheets += table.sheets.size();
                for (const auto& sheet : table.sheets)
                    for (unsigned row = 0; row < sheet.rows; ++row)
                        for (std::size_t column = 0; column < sheet.columns.size(); ++column)
                            tableCells += tableValue(sheet, row, column).present;
                if (name == "tables/tablescutscenes.bxls") cutsceneVideo = tableValue(table.sheets.at(0), 2, 1).text;
                if (name == "tables/tableslevels.bxls") studThreshold = tableValue(table.sheets.at(1), 1, 2).bits;
            }
            if (endsWith(name, "manifest.bstxt")) {
                manifestPaths.push_back(name.substr(0, name.rfind('/')));
                for (const auto& actor : cutsceneActors(parseManifest(data), manifestPaths.back())) {
                    ++cutsceneActorCount;
                    for (const auto& path : {actor.model, actor.skeleton, actor.animation})
                        if (!path.empty() && !archive.find(path)) missingManifestFiles.push_back(path);
                }
            }
            if (endsWith(name, ".bxaml")) {
                UiLayout layout = parseLayout(data);
                ++layouts;
                std::set<std::string> elementNames;
                auto lower = [](std::string text) {
                    for (auto& c : text) c = char(std::tolower(static_cast<unsigned char>(c)));
                    return text;
                };
                auto visit = [&](const UiElement& e, auto&& self) -> void {
                    ++layoutElements[e.type];
                    elementNames.insert(lower(e.name));
                    float fields[10] = {e.opacity, e.rotation, e.scale[0], e.scale[1], e.position[0],
                                        e.position[1], e.origin[0], e.origin[1], e.size[0], e.size[1]};
                    for (float f : fields) layoutGeometry += std::fabs(f);
                    for (const auto& child : e.children) self(child, self);
                };
                visit(layout.root, visit);
                for (const auto& storyboard : layout.storyboards) {
                    ++storyboards;
                    for (const auto& track : storyboard.tracks) {
                        ++tracks;
                        unresolvedTargets += !elementNames.count(lower(track.target));
                        for (const auto& key : track.keyframes) {
                            ++keyframes;
                            keyframeSum += std::fabs(key.value) + std::fabs(key.time);
                        }
                    }
                }
            }
            if (endsWith(name, ".tnf")) {
                glyphs += parseFont(data).glyphs.size();
                ++fonts;
            }
            if (endsWith(name, ".loc")) {
                locEntries += parseLocalisation(data).size();
                ++locFiles;
            }
            if (endsWith(name, ".cam")) {
                std::string text(data.begin(), data.end());
                for (const auto& root : parseConfigText(text)) {
                    ++camRoots;
                    countBlocks(countBlocks, root, camBlocks, camEntries);
                }
                camSetups += parseCameraFollow(text).setups.size();
                ++camFiles;
            }
            if (endsWith(name, ".txt") && !endsWith(name, "extended_chars_utf8.txt") && name.find("/maps/") == std::string::npos) {
                try {
                    for (const auto& root : parseConfigText(std::string(data.begin(), data.end())))
                        countBlocks(countBlocks, root, configBlocks, configEntries);
                    ++configFiles;
                } catch (const std::runtime_error& error) {
                    configRejected.push_back(name + ": " + error.what());
                }
            }
            if (endsWith(name, ".bfnnav")) {
                NavMesh nav = parseNav(data);
                ++navFiles;
                navGrids += nav.grids.size();
                navPaths += nav.paths.size();
                for (const auto& grid : nav.grids)
                    for (auto cell : grid.cells) navFlagged += cell & 1;
                navCounts[name] = {nav.grids.size(), nav.paths.size()};
            }
            if (endsWith(name, ".blvl")) {
                Level level = parseLevel(data);
                for (const auto& room : level.navRooms) navDescriptors.push_back({name, room});
                ++levels;
                objects += level.objects.size();
                placed += placements(level).size();
                DrawableObjects found = drawableObjects(archive, level);
                drawable += found.objects.size();
                drawableMissing += found.missingModel;
                drawableSkeletal += found.skeletal;
                drawableCharacters += found.characters.size();
                drawableCustom += found.customCharacters;
                drawableMarkers += found.markers;
                drawableStuds += found.studs.size();
            }
            if (endsWith(name, ".bfnmdl")) {
                Model model = parseModel(data);
                ++models;
                if (!model.collision.empty()) ++collisionModels;
                for (const auto& item : model.collision) {
                    ++collisionItems;
                    collisionTriangles += item.triangles.size();
                    collisionNodes += item.nodes.size();
                    // Every triangle is reachable from the root exactly once.
                    std::vector<int> seen(item.triangles.size(), 0);
                    std::vector<std::uint32_t> pending{0};
                    while (!pending.empty()) {
                        const CollisionNode& node = item.nodes.at(pending.back());
                        pending.pop_back();
                        for (std::uint32_t i = 0; i < node.count(); ++i) {
                            if (node.leaf()) ++seen.at(node.first() + i);
                            else pending.push_back(node.first() + i);
                        }
                    }
                    if (std::count(seen.begin(), seen.end(), 1) != std::ptrdiff_t(seen.size()))
                        throw std::runtime_error("collision tree does not cover every triangle once: " + name);
                }
                for (const auto& mesh : model.meshes)
                    for (const auto& part : mesh.parts) {
                        if (!part.boneBatches.empty()) {
                            ++skinnedParts;
                            boneBatches += part.boneBatches.size();
                            for (auto index : std::set<std::uint16_t>(part.indices.begin(), part.indices.end()))
                                for (int k = 0; k < 4; ++k)
                                    if (part.vertices[index].weights[k]) jointSum += part.vertices[index].joints[k];
                        }
                        const MaterialState material = materialState(part);
                        if (!material.supported) ++unsupportedMaterials;
                        ++materialCensus["mode " + std::to_string(material.mode) + " lighting " + std::to_string(material.lightingKey) +
                                         " texture " + std::to_string(material.textureKey) + " alpha " + std::to_string(material.alphaKey) +
                                         " textures " + std::to_string((part.material[10] & 0x3fu) >> 3) + " colour " +
                                         std::to_string((part.material[10] >> 6) & 1u)];
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
        // Reference totals from: python3 tools/convert_models.py --census artifacts/fib/files (at 9d04c91)
        if (models != 3113 || parts != 16122 || vertices != 7211595 || triangles != 4603391)
            throw std::runtime_error("model totals differ from the Python reference");
        // Pinned from chase-materials over every .bfnmdl, 2026-10-13: the material types of all parts, keyed by the fields
        // that select the combiner in 0x12e460 (mode = material byte 1, lighting = byte 2, texture = byte 3, alpha = byte 5,
        // textures and vertex-colour bit = byte 10). Every shipped material has a recovered combiner setup.
        {
            const std::vector<std::pair<std::string, std::size_t>> pinned = {
                {"mode 0 lighting 2 texture 1 alpha 0 textures 2 colour 1", 5012},
                {"mode 0 lighting 1 texture 0 alpha 0 textures 1 colour 0", 3186},
                {"mode 0 lighting 1 texture 0 alpha 1 textures 1 colour 0", 2094},
                {"mode 0 lighting 2 texture 1 alpha 0 textures 2 colour 0", 1181},
                {"mode 1 lighting 1 texture 0 alpha 0 textures 2 colour 0", 1164},
                {"mode 0 lighting 0 texture 0 alpha 0 textures 1 colour 0", 711},
                {"mode 0 lighting 4 texture 0 alpha 0 textures 2 colour 0", 218},
                {"mode 0 lighting 2 texture 5 alpha 1 textures 2 colour 1", 242},
                {"mode 0 lighting 2 texture 7 alpha 0 textures 2 colour 0", 44},
            };
            if (unsupportedMaterials != 0 || materialCensus.size() != 38)
                throw std::runtime_error("material census totals differ from the pinned values");
            for (const auto& [key, count] : pinned)
                if (materialCensus[key] != count) throw std::runtime_error("material census differs: " + key);
        }
        // Decoded combiner setups of named materials, 2026-10-13. Provenance: the stage generator 0x12e460 (ev_f3f2e9e9...),
        // its command record 0x24c56c (ev_304505a2...: SOURCE, header, OPERAND, COMBINER, COLOR, SCALE) and the key builder
        // 0x128dc8 (ev_0ff86efc...), hand-evaluated for each material and compared with chase-materials output.
        {
            struct Stage { std::uint32_t source, operand, combiner, constant, scale; };
            auto findPart = [&](const char* path, const std::string& first, const std::string& second) {
                const Model model = parseModel(archive.read(*archive.find(path)));
                for (const auto& mesh : model.meshes)
                    for (const auto& part : mesh.parts)
                        if (!part.textures.empty() && part.textures[0] == first && (second.empty() || (part.textures.size() > 1 && part.textures[1] == second)))
                            return part;
                throw std::runtime_error(std::string("material reference part not found: ") + first);
            };
            auto expectStages = [&](const ModelPart& part, std::uint32_t bufferUpdate, const std::vector<Stage>& want, const char* what) {
                const MaterialState state = materialState(part);
                bool same = state.supported && state.stages.size() == want.size() && state.bufferUpdate == bufferUpdate;
                for (std::size_t i = 0; same && i < want.size(); ++i)
                    same = state.stages[i].source == want[i].source && state.stages[i].operand == want[i].operand &&
                           state.stages[i].combiner == want[i].combiner && state.stages[i].constant == want[i].constant &&
                           state.stages[i].scale == want[i].scale;
                if (!same) throw std::runtime_error(std::string("combiner setup differs: ") + what);
            };
            // Baked-colour level surface: texture, then (vertex colour + lighting) x texture.
            expectStages(findPart("models/lvl_00_policestation/room3.bfnmdl", "Checker1.tga", ""), 0,
                         {{0x00e00003, 0, 0x00010000, 0xffffffff, 0}, {0x000f0f10, 0, 9, 0, 0}}, "room3 Checker1");
            // Lightmapped surface: texture, lightmap + vertex colour, (that + lighting) x the first stage's output, x2.
            expectStages(findPart("models/lvl_00_policestation/room3.bfnmdl", "MetalPlateBrushed.tga", "LIGHTMAP_lvl_00_PoliceStation_Basement000.tga"), 2,
                         {{0x00e00003, 0, 0x00010000, 0xffffffff, 0}, {0x000f0f04, 0, 2, 0, 0}, {0x000f0d1f, 0, 9, 0, 1}}, "room3 lightmap");
            // Lit prop with vertex colours and a specular mask in the alpha of the diffuse texture.
            expectStages(findPart("csdata/models/1_1_a/donut1/donut1.bfnmdl", "Brick_Top.tga", "FX_Specular.tga"), 0,
                         {{0x00e00003, 0, 0x00010000, 0xffffffff, 0}, {0x000f00f1, 0, 1, 0, 0}, {0x000f000f, 0, 1, 0, 0}, {0x000f0f32, 0x20, 8, 0, 0}},
                         "donut1");
            // Chase's face: diffuse texture only, alpha from vertex colour and material (no alpha test, no blending).
            {
                const ModelPart face = findPart("models/char_head_chase/char_head_chase.bfnmdl", "head_TEST.tga", "FX_PSP_Specular.tga");
                expectStages(face, 0, {{0x00e00003, 0, 0x00010000, 0xffffffff, 0}, {0x000f00f1, 0, 1, 0, 0}, {0x000f0f32, 0x20, 8, 0, 0}}, "head");
                const MaterialState state = materialState(face);
                if (state.alphaTest || state.blendSource != 1 || state.blendDestination != 0 || state.alphaKey != 0)
                    throw std::runtime_error("face material alpha state differs");
            }
            // Reflection: texture units are bound in reverse order, so the diffuse texture is Texture1.
            expectStages(findPart("csdata/models/1_2_a/prop_magnifyingglass/prop_magnifyingglass.bfnmdl", "Brick_Gradient.tga", "cubemap_vehicle_windows_c.tga"), 4,
                         {{0x00e00004, 0, 0x00010000, 0xffffffff, 0}, {0x000f00f1, 0, 1, 0, 0}, {0x000f0f33, 0, 1, 0, 0},
                          {0x000f00f4, 2, 1, 0, 1}, {0x000f0d0f, 0, 8, 0, 0}}, "magnifying glass reflection");
        }
        // Pinned totals recorded from chase_assets, 2026-10-13 (collision section of every .bfnmdl; each triangle
        // is reachable from the tree root exactly once, checked above).
        if (collisionModels != 776 || collisionItems != 1725 || collisionTriangles != 312483 || collisionNodes != 262633)
            throw std::runtime_error("collision totals differ from the pinned values");
        // Reference totals from tools/convert_levels.py (parse_level / placements over all .blvl) at 9d04c91
        if (levels != 856 || objects != 18469 || placed != 6563)
            throw std::runtime_error("level totals differ from the Python reference");
        // Pinned totals recorded from chase_assets drawableObjects over all .blvl, 2026-10-13: class files supply
        // meshname (and the transform) where the instance does not set it (lookup 0x217080).
        // The 886 skeletal "char_*" objects of that recording are now split: minifig characters drawn, custom
        // characters (CharacterEnum 1..8, 0x2372e0) and the other rigs.
        if (drawable != 7657 || drawableMissing != 423 || drawableStuds != 4835 || drawableMarkers != 405 ||
            drawableCharacters + drawableCustom + drawableSkeletal != 886 || drawableCharacters != 708 || drawableCustom != 8 || drawableSkeletal != 170)
            throw std::runtime_error("drawable object totals differ from the recorded values");
        // Reference: python3 tools/convert_levels.py (at 9d04c91) on lvl_00_policestation_main.blvl with parameter names from
        // code.bin (object Player1Start, class point, room1) and on chars/char_police.blvl (s10 defaults).
        {
            auto read = [&](const char* path) { return parseLevel(archive.read(*archive.find(path))); };
            auto start = playerStart(read("levels/levels/lvl_00_policestation/lvl_00_policestation_main.blvl"));
            if (!start || start->room != "room1" || start->position[0] != -2.1713f || start->position[1] != 1.4203f ||
                start->position[2] != 17.2821f || start->orientation[0] != 0 || start->orientation[1] != 180.9585f)
                throw std::runtime_error("Player1Start differs from the recorded reference");
            CharacterAppearance chase = characterAppearance(read("levels/chars/char_police.blvl"));
            if (chase.body != "char_minifig" || chase.head != "Char_Head_Chase" || chase.characterEnum != 9 ||
                chase.face != "head_ChaseMcCainDIFF64_c4x4.tga" || chase.lower != "lower_ChaseMcCainDIFF64.tga" ||
                chase.upper != "upper_ChaseMcCainDIFF64_uk.tga")
                throw std::runtime_error("char_police appearance differs from the recorded reference");
        }
        // Recorded from chase_assets drawableObjects, 2026-10-13; the class defaults come from
        // levels/leprops/leprop_wobblydestructible.blvl (s14 meshname 'Prop_Mech_CrateDefault'), levels/props/
        // parkour_zipline.blvl and levels/props/ability_freeplay_robber_atm.blvl. Instance names and positions were
        // listed with the same function and cross-checked against `tools/convert_levels.py --param-names` (the
        // instances carry no meshname parameter).
        {
            auto read = [&](const char* path) { return parseLevel(archive.read(*archive.find(path))); };
            auto find = [](const DrawableObjects& found, const std::string& instance) -> const Placement* {
                for (const auto& placement : found.objects)
                    if (placement.instance == instance) return &placement;
                return nullptr;
            };
            DrawableObjects police = drawableObjects(archive, read("levels/levels/lvl_00_policestation/lvl_00_policestation_main.blvl"));
            if (police.objects.size() != 131 || police.missingModel != 1 || police.skeletal != 0 || police.markers != 0 ||
                police.characters.size() != 32 || police.customCharacters != 8)
                throw std::runtime_error("lvl_00 drawable objects differ from the recorded values");
            const Placement* crate = find(police, "Portrait_Box01");
            if (!crate || crate->className != "leprops/leprop_wobblydestructible" || crate->mesh != "Prop_Mech_CrateDefault" ||
                meshModelPath(crate->mesh) != "models/prop_mech_cratedefault/prop_mech_cratedefault.bfnmdl" ||
                !archive.find(meshModelPath(crate->mesh)) || std::fabs(crate->position[0] - 8.7615f) > 1e-3f ||
                std::fabs(crate->position[1] - 2.5029f) > 1e-3f || std::fabs(crate->position[2] + 16.2848f) > 1e-3f)
                throw std::runtime_error("lvl_00 Portrait_Box01 (class default model) differs from the recorded values");
            DrawableObjects city = drawableObjects(archive, read("levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl"));
            if (city.objects.size() != 1198 || city.missingModel != 11 || city.skeletal != 16 || city.markers != 44 ||
                city.characters.size() != 99 || city.customCharacters != 0)
                throw std::runtime_error("lvl_01 drawable objects differ from the recorded values");
            // Studs (object type 0x3d, pickuptype 0..3 silver, gold, blue, purple). Provenance: the per-class census of
            // `tools/convert_levels.py --param-names` on both levels (silver, gold, blue, purple instances of the classes
            // pickups/pickup_stud* and lepickups/lepickup_stud*), and the oracle's pickup pool at the gameplay start
            // (docs/re/studs.md): 1038 of the 1040 lvl_01 studs matched a pool record by position, and a record is active
            // (state 1) exactly when DisabledOnStartUp is absent or 0, which is the "enabled" column here.
            auto studCounts = [](const DrawableObjects& found, bool enabledOnly) {
                std::array<std::size_t, 4> counts{};
                for (const auto& stud : found.studs)
                    if (!enabledOnly || stud.group == 0) ++counts[stud.kind];
                return counts;
            };
            using Counts = std::array<std::size_t, 4>;
            if (studCounts(police, false) != Counts{48, 46, 3, 0} || studCounts(police, true) != Counts{48, 46, 3, 0} ||
                studCounts(city, false) != Counts{495, 468, 73, 4} || studCounts(city, true) != Counts{416, 419, 42, 4})
                throw std::runtime_error("stud counts differ from the recorded values");
            const Placement* atm = find(city, "FP_Task_ROBBER_CTH_06");
            auto near = [](float a, float b) { return std::fabs(a - b) < 1e-2f; };
            if (!atm || atm->mesh != "Ability_Freeplay_Robber_ATM01" || !archive.find(meshModelPath(atm->mesh)) ||
                !near(atm->position[0], 801.858f) || !near(atm->position[1], 17.0315f) || !near(atm->position[2], 1053.8f))
                throw std::runtime_error("lvl_01 FP_Task_ROBBER_CTH_06 differs from the recorded values");
            if (find(city, "1-1_Waypoint_02")) throw std::runtime_error("map markers must not be drawable");
            // Resolved character rigs. Provenance: instance and class parameters listed with parseLevel and the
            // parameter names from the strings of code.bin (scratch dump, 2026-10-13, manual inspection). Lobby_Cop02:
            // the instance sets HeadModel and the three textures (class chars/char_ai_actor holds the TEST placeholders).
            // Dumby: only the transform is set, head and textures are the class values of chars/char_ai_deputydunby.
            // Pedestrian03: AnimationClass 'CLASS_Generic' exists only in the +0x14 table of chars/pedestrian, which the
            // game reads at 0x1913f8 ("models/%s/%s/" with char_minifig and AnimationClass).
            auto findCharacter = [](const DrawableObjects& found, const std::string& instance) -> const CharacterObject* {
                for (const auto& character : found.characters)
                    if (character.placement.instance == instance) return &character;
                return nullptr;
            };
            const CharacterObject* cop = findCharacter(police, "Lobby_Cop02");
            if (!cop || cop->placement.className != "chars/char_ai_actor" || cop->appearance.body != "char_minifig" ||
                cop->appearance.head != "Char_Head_PoliceHat" || !cop->hat.empty() ||
                cop->appearance.face != "head_MalePolice01DIFF64_c4x4.tga" || cop->appearance.lower != "lower_Lego_BlackDIFF64.tga" ||
                cop->appearance.upper != "upper_CopJacketDIFF128.tga" || cop->animationClass != "class_generic" ||
                !near(cop->placement.position[0], 14.1878f) || !near(cop->placement.position[1], 1.4209f) ||
                !near(cop->placement.position[2], 3.9843f) || !near(cop->placement.orientation[1], -71.9074f))
                throw std::runtime_error("lvl_00 Lobby_Cop02 rig differs from the recorded values");
            if (findCharacter(police, "Lineup_01_Civilian")) throw std::runtime_error("custom characters must not be drawn from their placeholder look");
            const CharacterObject* dunby = findCharacter(city, "1-3_Dumby");
            if (!dunby || dunby->appearance.head != "Char_Head_DeputyDunby" ||
                dunby->appearance.face != "head_DeputyDunbyDIFF64_c4x4.tga" || dunby->appearance.lower != "lower_Lego_BlackDIFF64.tga" ||
                dunby->appearance.upper != "upper_DeputyDunbyDIFF128.tga" || !near(dunby->placement.position[0], 916.51f))
                throw std::runtime_error("lvl_01 Dumby rig differs from the recorded values");
            const CharacterObject* walker = findCharacter(city, "Pedestrian03");
            if (!walker || walker->placement.className != "chars/pedestrian" || walker->appearance.head != "Char_Head_Cheerleader" ||
                walker->appearance.upper != "upper_CheerleaderDIFF128.tga" || walker->appearance.lower != "lower_CheerleaderDIFF128.tga" ||
                walker->appearance.face != "head_CheerleaderDIFF64_c4x4.tga" || walker->animationClass != "class_generic" ||
                !archive.find(meshModelPath(walker->appearance.head)) || !archive.find(meshModelPath(walker->appearance.body)))
                throw std::runtime_error("lvl_01 Pedestrian03 rig differs from the recorded values");
        }
        // Reference: artifacts/audio/audio-manifest.json from tools/convert_audio.py, and FNV-1a over
        // the interleaved little-endian PCM of its music/act_bass_break.wav.
        if (sounds != 1361 || samples != 122501647 || loops != 123 || soundSum != 0xdcbf1f85)
            throw std::runtime_error("sound totals or checksum differ from the Python reference");
        // Reference: tools/convert_anims.py (at 9d04c91) parse_skeleton/parse_animation over all files, and the sum of
        // |entries| of skin_matrices() for char_bear walk over all 48 frames.
        if (skeletons != 376 || joints != 5280 || animations != 1954 || curves != 30081)
            throw std::runtime_error("skeleton/animation totals differ from the Python reference");
        // Reference: recorded from this C++ parser (2026-10-13); every key block matches its u16 count.
        // Reference: count of animations with table bit 29, recorded from this C++ parser (2026-10-13).
        if (longCurves != 590) throw std::runtime_error("long-curve count differs from the recorded reference");
        if (subAnimations != 3804 || animTracks != 14564 || trackKeys != 62293)
            throw std::runtime_error("animation track totals differ from the recorded reference");
        // Reference: recorded from this C++ parser (2026-10-13) and checked with ls over
        // artifacts/fib/files/csdata/models: the only listed file absent from the archive.
        if (manifestPaths.size() != 69 || cutsceneActorCount != 397 || missingManifestFiles.size() != 1 ||
            missingManifestFiles[0] != "csdata/models/2_7_c/laundrycart/laundrycart.bfnskl")
            throw std::runtime_error("cutscene manifest totals differ from the recorded reference");
        // Reference: every sheet's computed stride equals its stored stride (checked in parseTable);
        // totals recorded from this C++ parser (2026-10-13). Cell values checked by hand against the
        // rows dumped by tools/tt_resource.py (at 9d04c91): tablescutscenes row 2 offset 1 -> "01_gameintro",
        // tableslevels sheet 1 row 1 "50c30000" = 50000.
        if (tableFiles != 24 || tableSheets != 54 || tableCells != 23512 || cutsceneVideo != "01_gameintro" ||
            studThreshold != 50000)
            throw std::runtime_error("table totals differ from the recorded reference");
        // Reference: chase-tabledump tables/tablessoundfx.bxls (2026-10-13): sheet 0 has 758 rows; row 1
        // "ui/brickcounter1" 80/20/30, row 5 "lego_sfx/legosmall1" 100/1/100 (the volume/+10/+11 values match
        // game record bytes +9/+10/+11, docs/re/audio.md). The path rule "sounds/<name>.bwav" resolves 757
        // of 757 named rows in the archive (checked with a Python scan of lego_city3ds.csv, 2026-10-13).
        {
            const FuseEntry* entry = archive.find("tables/tablessoundfx.bxls");
            if (!entry) throw std::runtime_error("tablessoundfx not found");
            std::vector<SoundEffect> effects = parseSoundEffects(parseTable(archive.read(*entry)));
            std::size_t named = 0, resolved = 0;
            for (const auto& effect : effects) {
                if (effect.name.empty()) continue;
                ++named;
                resolved += archive.find(soundEffectPath(effect)) != nullptr;
            }
            if (effects.size() != 758 || !effects[0].name.empty() || effects[1].name != "ui/brickcounter1" ||
                effects[1].volumePercent != 80 || effects[1].paramA != 20 || effects[1].paramB != 30 ||
                effects[5].name != "lego_sfx/legosmall1" || effects[5].column2 != 675 ||
                effects[5].volumePercent != 100 || effects[5].paramA != 1 || effects[5].paramB != 100 ||
                named != 757 || resolved != 757)
                throw std::runtime_error("sound effect table differs from the recorded reference");
        }
        // Reference: recorded from this C++ parser (2026-10-13); camera x is near the actors (x ~ -815 raw).
        {
            const FuseEntry* shot = archive.find("csdata/models/1_1_a/cscamera/cscamera_0000.bfnanm");
            if (!shot) throw std::runtime_error("1_1_a camera not found");
            CutsceneCamera camera = cutsceneCamera(parseAnimation(archive.read(*shot)), 0.5f);
            if (std::fabs(camera.position[0] + 880.099f) > 1e-2f || std::fabs(camera.position[2] - 1109.11f) > 1e-2f ||
                camera.fieldOfView != 43)
                throw std::runtime_error("1_1_a camera differs from the recorded reference");
        }
        // Reference: python3 tools/convert_loc.py --census artifacts/fib/files/localisation
        if (fonts != 6 || glyphs != 1521) throw std::runtime_error("font totals differ from the Python reference");
        if (locFiles != 66 || locEntries != 25207)
            throw std::runtime_error("localisation totals differ from the Python reference");
        // Reference: FNV-1a over the UTF-8 of convert_loc.decode_text (at 9d04c91) for every gametext_ru entry
        // in key order, each followed by a NUL.
        {
            auto read = [&](const char* name) { return archive.read(*archive.find(name)); };
            auto extended = extendedCharacters(read("localisation/extended_chars_utf8.txt"));
            std::uint32_t textSum = 0x811c9dc5;
            for (const auto& [key, raw] : parseLocalisation(read("localisation/gametext_ru.loc"))) {
                std::string text = decodeText(raw, extended);
                for (char c : text + std::string(1, '\0')) textSum = (textSum ^ std::uint8_t(c)) * 0x01000193;
            }
            if (textSum != 0xc0c84ff0) throw std::runtime_error("decoded Russian text differs from the Python reference");
        }
        // Reference: independent strict regex census over artifacts/fib/files/cutscenes/*.srt
        // (CRLF, "N / HH:MM:SS,mmm --> HH:MM:SS,mmm / text lines"): 204 files, 2921 cues, 3560 text
        // lines, 6486170 ms total cue duration. The decoded-text FNV-1a (each line followed by a NUL)
        // was recorded from this parser and spot-checked by reading the 01_gameintro_us cues.
        {
            std::size_t srtFiles = 0, srtCues = 0, srtLines = 0;
            std::uint64_t srtMs = 0;
            std::uint32_t srtSum = 0x811c9dc5;
            auto extended = extendedCharacters(archive.read(*archive.find("localisation/extended_chars_utf8.txt")));
            for (const auto& name : archive.names()) {
                if (name.rfind("cutscenes/", 0) != 0 || !endsWith(name, ".srt")) continue;
                ++srtFiles;
                for (const auto& cue : parseSubtitles(archive.read(*archive.find(name)), extended)) {
                    ++srtCues;
                    srtMs += cue.endMs - cue.startMs;
                    for (const auto& line : cue.lines) {
                        ++srtLines;
                        for (char c : line + std::string(1, '\0')) srtSum = (srtSum ^ std::uint8_t(c)) * 0x01000193;
                    }
                }
            }
            if (srtFiles != 204 || srtCues != 2921 || srtLines != 3560 || srtMs != 6486170 || srtSum != 0xe37a66e1)
                throw std::runtime_error("subtitle totals differ from the regex census");
        }
        // Reference: tools/tt_resource.py (at 9d04c91) parse of particles/particledefs.bprt: -15 words 10, 25, 97,
        // 298, 0, 0x401503da; 97 -16 names; one -14 block of 113240 bytes at 0x7fc whose FNV-1a is
        // 0x57544492. All 298 words at record +0x2c equal the PJW hash of a texture name. Colour key
        // counts at +0x172 (Python census): 2 x 140, 3 x 93, 4 x 45, 5 x 15, 6 x 3, 7 x 2 = 846 keys;
        // fade key counts at +0x170: 2 x 250, 3 x 44, 4 x 4 = 648; size keys (+0x171) total 718
        // and alpha keys (+0x173) 1045; 197 durations (+0x38) are 100.
        {
            ParticleDefs defs = parseParticleDefs(archive.read(*archive.find("particles/particledefs.bprt")));
            std::uint32_t sum = 0x811c9dc5;
            std::size_t resolved = 0, inLimits = 0, colourKeys = 0, fadeKeys = 0, sizeKeys = 0, alphaKeys = 0, endless = 0;
            for (const auto& def : defs.defs) {
                resolved += def.texture >= 0;
                inLimits += def.particles >= 1 && def.particles <= defs.maxParticles && def.atlasColumns != 0;
                colourKeys += def.colourKeys.size();
                fadeKeys += def.fadeKeys.size();
                sizeKeys += def.sizeKeys.size();
                alphaKeys += def.alphaKeys.size();
                endless += def.duration > 98;
                for (std::uint8_t b : def.raw) sum = (sum ^ b) * 0x01000193;
            }
            if (defs.maxEmitters != 10 || defs.maxParticles != 25 || inLimits != 298 || colourKeys != 846 || fadeKeys != 648 || sizeKeys != 718 || alphaKeys != 1045 || endless != 197 || defs.unknown4 != 0 || defs.textures.size() != 97 ||
                defs.textures[1] != "cloud" || defs.defs.size() != 298 || resolved != 298 || sum != 0x57544492 ||
                std::fabs(defs.scale - 2.32836f) > 1e-5f)
                throw std::runtime_error("particle definitions differ from the pooled-resource reference");
        }
        // Reference: python3 tools/convert_nav.py --census artifacts/fib/files (at 9d04c91)
        std::size_t navMatched = 0;
        for (const auto& [levelName, room] : navDescriptors) {
            std::string roomName = room.room;
            for (auto& c : roomName) c = char(std::tolower(static_cast<unsigned char>(c)));
            auto found = navCounts.find(levelName.substr(0, levelName.size() - 5) + "_" + roomName + ".bfnnav");
            if (found == navCounts.end()) continue;
            if (found->second != std::make_pair(std::size_t(room.grids), std::size_t(room.paths)))
                throw std::runtime_error("nav counts differ from the level descriptor: " + found->first);
            if (room.end - room.base != room.grids)
                throw std::runtime_error("nav global grid range differs from the grid count: " + found->first);
            ++navMatched;
        }
        if (navFiles != 700 || navGrids != 28646 || navPaths != 4607 || navFlagged != 429583 || navMatched != 700)
            throw std::runtime_error("navigation totals differ from the Python reference");
        // Reference: line-based Python census over the 10 .cam files: 10 CameraFollow + 1822 Bound + 253 Room
        // blocks, 2075 setups x 15 entries (Name Type RotationY RotationX Distance Location Lookat Offset FOV
        // Filters BlendTime ShakeAmp ShakeRate ShakeRoll 3DOffset) = 31125.
        if (camFiles != 10 || camRoots != 10 || camBlocks != 2085 || camEntries != 31125 || camSetups != 2075)
            throw std::runtime_error("camera totals differ from the Python census");
        // Reference: manual inspection and a line-based Python census of options3ds.txt, triggers.txt and
        // letriggers.txt (1 + 2 + 34 blocks, 11 + 13 + 33 entries). The other three files fail the strict
        // grammar: options.txt is only the header "options" without a body; scriptfns.txt line 843 lacks a
        // closing ')'; gescriptfns.txt line 264 has an extra ')'.
        if (configFiles != 3 || configBlocks != 37 || configEntries != 57 || configRejected.size() != 3)
            throw std::runtime_error("config text totals differ from the Python census");
        std::sort(configRejected.begin(), configRejected.end());
        if (configRejected[0].rfind("levels/gescriptfns.txt: config text line 264:", 0) ||
            configRejected[1].rfind("levels/scriptfns.txt: config text line 843:", 0) ||
            configRejected[2].rfind("options.txt: config text line 2:", 0))
            throw std::runtime_error("rejected config text files differ from the manual inspection");
        // Reference: convert_models.bone_batches/skin_joints (at 9d04c91) over every part (drawn vertices only).
        if (skinnedParts != 1457 || boneBatches != 1465 || jointSum != 3492824)
            throw std::runtime_error("bone palette totals differ from the Python reference");
        // Reference: a throwaway Python walk of the 0x217220/0x29f7b4 block order over all 55 .bxaml files
        // (session 2026-10-13, not kept): element types, storyboard counts, the sum of |the ten element
        // floats at +0x18|, the sum of |keyframe value| + |keyframe time|, and track targets that name no
        // element of their layout (0x24085c returns NULL for them).
        if (layouts != 55 || layoutElements[UiElementType::Container] != 383 ||
            layoutElements[UiElementType::Image] != 614 || layoutElements[UiElementType::Reference] != 110 ||
            storyboards != 161 || tracks != 1888 || keyframes != 6740 || unresolvedTargets != 2 ||
            std::fabs(layoutGeometry - 366157.4538534144) > 1e-3 || std::fabs(keyframeSum - 69052.03302517906) > 1e-3)
            throw std::runtime_error("layout totals differ from the reference walk");
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
        // Reference: sum of |skinned position| over every vertex. Bear: preview_model.skin_vertex with
        // convert_anims.skin_matrices (both at 9d04c91; its joint orientations are identity). Squirrel and minifig:
        // recorded from this C++ code (2026-10-13) after applying the joint +0x88 orientation
        // (0x2485d8, ev_6c72e2e4…a73d); manual inspection showed upright squirrel ears and an
        // assembled minifig. The squirrel has rotated bind matrices and two bone batches; the
        // fractional frames blend neighbouring frames (47.5 wraps into frame 0).
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
            std::fabs(posedSum("char_squirrel", "run.bfnanm", 5) - 422.8465882838) > 1e-3 ||
            std::fabs(posedSum("char_minifig", "class_generic/balance_walk_minifig.bfnanm", 5) - 1960.8094874918) > 1e-3 ||
            std::fabs(posedSum("char_bear", "walk.bfnanm", 47.5) - 1741.8780890502587) > 1e-3 ||
            std::fabs(posedSum("char_squirrel", "run.bfnanm", 2.25) - 350.1472583711) > 1e-3)
            throw std::runtime_error("skinned vertices differ from the Python reference");
        // Head attachment of the police idle pose (class_police/idle_minifig frame 0) at the "head" joint (index 9).
        // Recorded from this C++ code (2026-10-13); the oracle screenshot gameplay-start.png shows Chase from
        // behind with the hair centred on the back of the head, and the render with this matrix matches it.
        // With the bind world (a 90 degree twist for this joint) instead of its position, the head turned sideways.
        {
            auto read = [&](const std::string& name) {
                const FuseEntry* entry = archive.find(name);
                if (!entry) throw std::runtime_error("reference file not found: " + name);
                return archive.read(*entry);
            };
            Skeleton minifig = parseSkeleton(read("models/char_minifig/char_minifig.bfnskl"));
            Animation idle = parseAnimation(read("models/char_minifig/class_police/idle_minifig.bfnanm"));
            Matrix head = jointAttachment(minifig.joints[9], skinMatrices(minifig, idle, 0)[9]);
            if (minifig.joints[9].name != "head" || head[0] < 0.999f || head[5] < 0.999f || head[10] < 0.999f ||
                std::fabs(head[12] + 0.035988) > 1e-4 || std::fabs(head[13] - 1.309142) > 1e-4 ||
                std::fabs(head[14] + 0.008350) > 1e-4)
                throw std::runtime_error("head attachment differs from the recorded reference");
        }
        {
            // Pinned values recorded from chase_assets, 2026-10-13; the lobby floor of lvl_00 is item "Collision1"
            // (docs/re/collision.md).
            const Model police = parseModel(archive.read(*archive.find("models/lvl_00_policestation/lvl_00_policestation.bfnmdl")));
            const std::pair<const char*, std::size_t> want[] = {{"Collision2", 1616}, {"Collision1", 1201}, {"Collision3", 130}};
            if (police.collision.size() != 3) throw std::runtime_error("lvl_00 collision items differ from the pinned values");
            for (std::size_t i = 0; i < 3; ++i)
                if (police.collision[i].name != want[i].first || police.collision[i].triangles.size() != want[i].second)
                    throw std::runtime_error("lvl_00 collision item differs from the pinned values: " + police.collision[i].name);
        }
        std::cout << archive.entries().size() << " entries, " << bytes << " bytes, " << pooled
                  << " pooled resources, " << textures << " textures, " << checked << " checksums match, "
                  << models << " models / " << parts << " parts / " << vertices << " vertices / "
                  << triangles << " triangles, " << levels << " levels / " << objects << " objects / " << placed
                  << " placements, " << skeletons << " skeletons / " << joints << " joints, " << animations
                  << " animations / " << curves << " joint curves / " << animTracks << " tracks, " << manifestPaths.size() << " cutscenes / " << cutsceneActorCount << " actors, " << navFiles << " nav files / " << navGrids << " grids, " << sounds << " sounds / " << samples << " samples / " << loops << " looping, "
                  << layouts << " layouts / " << tracks << " tracks\n";
    } catch (const std::exception& error) {
        std::cerr << "assets-real-test failed: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
