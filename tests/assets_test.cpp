// Synthetic fixtures for the native asset runtime; mirrors tests/test_extract_fib.py,
// tests/test_tt_resource.py (at 9d04c91) and tests/test_convert_textures.py.
#include "assets/assets.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
using namespace chase::assets;

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
template <typename F> void requireThrows(F f, const char* message) {
    try { f(); } catch (const std::runtime_error&) { return; }
    throw std::runtime_error(message);
}
void put32(Bytes& b, std::uint32_t v) {
    for (int i = 0; i < 4; ++i) b.push_back(std::uint8_t(v >> (8 * i)));
}
Bytes bytes(const std::string& text) { return Bytes(text.begin(), text.end()); }
Bytes padded(const char* text, std::size_t size) {
    std::string field(text);
    field.resize(size, '\0');
    return bytes(field);
}

const Bytes stream = bytes(std::string("\xe0" "abca" "\x20\x02" "\x81\x40\x09" "Y", 11) +
                           std::string("\xc0\x00\x00\x00", 4) + "\xfdX");
const std::string decoded = "abcabcabcYabcabbbbbbX";

void refpack() {
    Bytes out = decompressRefPack(stream.data(), stream.size(), decoded.size());
    require(std::string(out.begin(), out.end()) == decoded, "RefPack token forms");
    for (const std::string& bad : {std::string("\x00\x05\xfc", 3), std::string("\xe0" "ab"),
                                   std::string("\xe0" "abcd")}) {
        Bytes b = bytes(bad);
        requireThrows([&] { decompressRefPack(b.data(), b.size(), 100); }, "bad RefPack stream accepted");
    }
    requireThrows([&] { decompressRefPack(stream.data(), stream.size(), 3); }, "RefPack limit not enforced");
}

void fuse() {
    // Entries: stored "raw", codec-1 chunk, two codec-2 chunks of 0x8000.
    Bytes big(0x8000 + 5);
    for (std::size_t i = 0; i < big.size(); ++i) big[i] = std::uint8_t(i * 7);
    Bytes body, directory;
    auto add = [&](std::uint32_t crc, std::uint32_t codec, std::uint32_t size, const Bytes& payload) {
        put32(directory, crc);
        put32(directory, 20 + std::uint32_t(body.size()));
        put32(directory, size << 5 | codec);
        body.insert(body.end(), payload.begin(), payload.end());
    };
    Bytes chunk1;
    put32(chunk1, std::uint32_t(stream.size()));
    chunk1.insert(chunk1.end(), stream.begin(), stream.end());
    Bytes chunks2;
    put32(chunks2, 0x8000);
    chunks2.insert(chunks2.end(), big.begin(), big.begin() + 0x8000);
    put32(chunks2, 5);
    chunks2.insert(chunks2.end(), big.begin() + 0x8000, big.end());
    add(0x11, 0, 3, bytes("raw"));
    add(0x22, 1, std::uint32_t(decoded.size()), chunk1);
    add(0x33, 2, std::uint32_t(big.size()), chunks2);
    Bytes file = bytes("FUSE1.00");
    put32(file, 3);
    put32(file, 0);
    put32(file, 20 + std::uint32_t(body.size()));
    file.insert(file.end(), body.begin(), body.end());
    file.insert(file.end(), directory.begin(), directory.end());
    const std::string path = "assets_test_archive.fib", csv = "assets_test_archive.csv";
    std::ofstream(path, std::ios::binary).write(reinterpret_cast<const char*>(file.data()), std::streamsize(file.size()));
    std::ofstream(csv) << "Filename CRC, Filename, Compressed Size, File Size, Offset\n"
                       << "0x00000022,Models/Packed.BIN,14,21,0x17\n";
    FuseArchive archive(path);
    archive.loadNames(csv);
    require(archive.entries().size() == 3, "entry count");
    std::uint64_t stored = 0;
    Bytes raw = archive.read(archive.entries()[0], &stored);
    require(std::string(raw.begin(), raw.end()) == "raw" && stored == 3, "stored entry");
    const FuseEntry* packed = archive.find("models/packed.bin");
    require(packed && packed->crc == 0x22, "case-insensitive CSV lookup");
    Bytes unpacked = archive.read(*packed, &stored);
    require(std::string(unpacked.begin(), unpacked.end()) == decoded && stored == chunk1.size(), "codec 1 entry");
    require(archive.read(archive.entries()[2], &stored) == big && stored == chunks2.size(), "codec 2 chunks");
    require(archive.find("missing") == nullptr, "unknown name");
    std::remove(path.c_str());
    std::remove(csv.c_str());
}

Bytes pooledFile(const std::vector<std::int32_t>& table, const Bytes& stat, const Bytes& body) {
    Bytes out;
    put32(out, std::uint32_t(table.size()) << 8);
    put32(out, std::uint32_t(4 * table.size() + stat.size()));
    for (auto word : table) put32(out, std::uint32_t(word));
    out.insert(out.end(), stat.begin(), stat.end());
    out.insert(out.end(), body.begin(), body.end());
    return out;
}

void pooled() {
    Bytes data = pooledFile({-16, 0, 4, -15, 2, 0, 3, -14, 1, -14, 2}, bytes("SSSS"), bytes("AABBBCDD"));
    auto blocks = parsePooled(data);
    require(blocks.size() == 7, "pooled block count");
    require(blocks[1].pool == Pool::M16 && blocks[1].size == 4 && data[blocks[1].offset] == 'S', "-16 data in header");
    require(blocks[4].pool == Pool::M15 && data[blocks[4].offset] == 'B', "-15 data order");
    require(blocks[6].pool == Pool::M14 && data[blocks[6].offset] == 'D', "-14 data order");
    PoolCursor cursor(data);
    cursor.push(Pool::M15);
    require(cursor.next() && cursor.next() == nullptr && cursor.next(), "-15 cursor with NULL block");
    require(!cursor.exhausted(), "cursor exhausted too early");
    for (const auto& bad : {pooledFile({-14, 2}, {}, bytes("xxx")), pooledFile({5, -14}, {}, Bytes(5)),
                            pooledFile({-16, 4}, bytes("SS"), {})})
        requireThrows([&] { parsePooled(bad); }, "bad pooled resource accepted");
}

Bytes texture(int width, int height, int format, int levels, const Bytes& payload, int cube = 0) {
    Bytes out;
    for (std::uint32_t v : {0x400u, 0x10u, 0xfffffff2u, 0x20u, 0xfffffff2u, std::uint32_t(payload.size())}) put32(out, v);
    out.push_back(std::uint8_t(width)); out.push_back(std::uint8_t(width >> 8));
    out.push_back(std::uint8_t(height)); out.push_back(std::uint8_t(height >> 8));
    put32(out, std::uint32_t(payload.size()));
    out.push_back(std::uint8_t(format)); out.push_back(0); out.push_back(0); out.push_back(std::uint8_t(cube));
    put32(out, std::uint32_t(levels));
    out.resize(0x38, 0);
    out.insert(out.end(), payload.begin(), payload.end());
    return out;
}

void textures() {
    Bytes payload;
    for (int i = 0; i < 64; ++i) { payload.push_back(255); payload.push_back(0); payload.push_back(0); payload.push_back(std::uint8_t(i)); }
    Image image = decodeTexture(texture(8, 8, 0, 1, payload));
    auto red = [&](int x, int row) { return image.rgba[(std::size_t(row) * 8 + std::size_t(x)) * 4]; };
    require(red(1, 7) == 1 && red(0, 6) == 2 && red(2, 7) == 4 && red(7, 0) == 63, "Morton tiling / bottom-up rows");
    // ETC1 individual mode, colours 0x8 / 0xF, index 3 at pixel x=1,y=2.
    std::uint64_t high = 8u << 28 | 15u << 24 | 8u << 20 | 15u << 16 | 8u << 12 | 15u << 8;
    std::uint64_t word = high << 32 | 1u << 6 | 1u << 22;
    Bytes etc;
    for (int b = 0; b < 4; ++b)
        for (int i = 0; i < 8; ++i) etc.push_back(b == 0 ? std::uint8_t(word >> (8 * i)) : 0);
    Image e = decodeTexture(texture(8, 8, 12, 1, etc));
    auto px = [&](int x, int y) { return e.rgba[(std::size_t(7 - y) * 8 + std::size_t(x)) * 4]; };
    require(px(0, 0) == 138 && px(3, 3) == 255 && px(1, 2) == 128, "ETC1 individual mode");
    Image cube = decodeTexture(texture(8, 8, 7, 1, Bytes(64 * 6, 9), 1));
    require(cube.faces == 6 && cube.height == 48, "cubemap strip");
    Bytes good = texture(8, 8, 7, 1, Bytes(64));
    Bytes truncated(good.begin(), good.begin() + 20), longer = good, badFormat = good;
    longer.push_back(0);
    badFormat[0x20] = 14;
    for (const auto& bad : {truncated, longer, badFormat, texture(12, 8, 7, 1, Bytes(96)), texture(8, 8, 0, 2, Bytes(512))})
        requireThrows([&] { decodeTexture(bad); }, "bad texture accepted");
}

// Same synthetic model as tests/test_convert_models.py (at 9d04c91): one mesh, one textured triangle.
Bytes pooledBlocks(const std::vector<std::pair<std::int32_t, Bytes>>& blocks) {
    std::vector<std::int32_t> table;
    Bytes stat, body;
    std::int32_t current = 0;
    for (const auto& [marker, data] : blocks) {
        if (marker != current) table.push_back(current = marker);
        table.push_back(std::int32_t(data.size()));
        (marker == -16 ? stat : body).insert((marker == -16 ? stat : body).end(), data.begin(), data.end());
    }
    return pooledFile(table, stat, body);
}

Bytes modelFile(bool extraBlock) {
    Bytes model(0x24), mesh(0x24), material(0x60), header(0x78), vertices, slot, indices;
    model[4] = 1;                  // one mesh
    mesh[2] = 1;                   // one part
    material[10] = 0x09;           // one texture, one UV set -> flags 0x84, stride 20
    header[2] = 3;                 // vertices
    header[4] = 3;                 // indices
    header[0x1a] = 20;             // stride
    const float uvScales[4] = {1 / 32767.0f, 1 / 32767.0f, 1, 1};
    std::memcpy(&header[0x50], uvScales, sizeof uvScales);
    const float positions[3][2] = {{0, 0}, {1, 0}, {0, 1}};
    const std::int16_t uvs[3][2] = {{0, 0}, {32767, 0}, {0, 32767}};
    for (int v = 0; v < 3; ++v) {
        float xyz[3] = {positions[v][0], positions[v][1], 0};
        const auto* p = reinterpret_cast<const std::uint8_t*>(xyz);
        vertices.insert(vertices.end(), p, p + 12);
        for (auto value : uvs[v]) { vertices.push_back(std::uint8_t(value)); vertices.push_back(std::uint8_t(value >> 8)); }
        for (std::uint8_t c : {255, 0, 0, 255}) vertices.push_back(c);
    }
    indices = {0, 0, 1, 0, 2, 0, 0, 0};    // three u16 indices, padded to an even count
    put32(slot, 1); slot.push_back(0); slot.push_back(0); slot.push_back(5); slot.push_back(0);
    std::vector<std::pair<std::int32_t, Bytes>> blocks{
        {-15, model}, {-15, {}}, {-15, {}}, {-15, mesh}, {-15, Bytes(0x24)}, {-15, {}},
        {-15, material}, {-15, {}}, {-16, slot}, {-16, bytes(std::string("tex.tga", 8))},
        {-14, header}, {-14, indices}, {-14, vertices}, {-16, {}}};
    if (extraBlock) blocks.push_back({-14, bytes("xx")});
    return pooledBlocks(blocks);
}

// A model without meshes and a collision section (loader 0x114640): one item "Collision_Test" with a leaf
// node over two triangles of a 4 x 4 quad.
Bytes collisionModelFile(bool badVertexIndex) {
    Bytes model(0x24), section(0x10), items(0x2c), node(0x1c), triangles(0x40), vertices;
    model[0x1c] = 1;
    put32(section, 0);
    section.assign(0x10, 0);
    section[8] = 1;
    section[0xc] = 1;
    items[0x10] = 1;               // nodes
    items[0x14] = 2;               // triangles
    const float quad[4][3] = {{-2, 0, -2}, {2, 0, -2}, {2, 0, 2}, {-2, 0, 2}};
    for (const auto& corner : quad) {
        const auto* p = reinterpret_cast<const std::uint8_t*>(corner);
        vertices.insert(vertices.end(), p, p + 12);
    }
    const float half[3] = {2, 0, 2};
    std::memcpy(&node[12], half, sizeof half);
    node[0x18] = 1 | 2 << 1;       // leaf, two triangles, first 0
    const std::uint32_t indices[2][3] = {{0, 1, 2}, {0, 2, badVertexIndex ? 4u : 3u}};
    for (int t = 0; t < 2; ++t) {
        const float up[4] = {0, 1, 0, 0};
        std::memcpy(&triangles[std::size_t(t) * 0x20], up, sizeof up);
        std::memcpy(&triangles[std::size_t(t) * 0x20 + 16], indices[t], 12);
        triangles[std::size_t(t) * 0x20 + 0x1c] = std::uint8_t(19 + t);
    }
    return pooledBlocks({{-15, model}, {-15, {}}, {-15, {}}, {-15, {}}, {-15, {}}, {-15, section},
                         {-14, items}, {-14, bytes(std::string("Collision_Test", 15))}, {-14, {}}, {-14, Bytes(4)},
                         {-14, Bytes(4)}, {-14, Bytes(4)}, {-14, node}, {-14, triangles}, {-14, vertices}, {-16, {}}});
}

void collisionModels() {
    const Model model = parseModel(collisionModelFile(false));
    require(model.meshes.empty() && model.collision.size() == 1, "collision section parsed");
    const CollisionItem& item = model.collision[0];
    require(item.name == "Collision_Test" && item.triangles.size() == 2 && item.nodes.size() == 1 && item.vertices.size() == 12,
            "collision item counts");
    require(item.nodes[0].leaf() && item.nodes[0].count() == 2 && item.nodes[0].first() == 0 && item.nodes[0].half[0] == 2.0f,
            "collision node");
    require(item.triangles[1].vertex[2] == 3 && item.triangles[1].surface == 20 && item.triangles[0].normal[1] == 1.0f,
            "collision triangle");
    requireThrows([] { parseModel(collisionModelFile(true)); }, "vertex index outside the item accepted");
}

enum class SkinVariant { Good, Conflict, OutsidePalette };
// Two triangles in two bone batches with palettes {3, 7} and {3, 9}; vertices 1 and 2 are shared.
Bytes skinnedModelFile(SkinVariant variant) {
    Bytes model(0x24), mesh(0x24), material(0x60), header(0x78), vertices, batches(2 * 0x110);
    model[4] = 1;
    mesh[2] = 1;
    material[0] = 1;               // skin attribute -> flags 0x82, stride 24
    header[2] = 4;
    header[4] = 6;
    header[0x1a] = 24;
    header[0x20] = 2;              // two bone batches
    header[0x24] = 1;              // batch block present
    const std::uint8_t sharedIndex = variant == SkinVariant::Conflict ? 1 : 0;
    const std::uint8_t lastIndex = variant == SkinVariant::OutsidePalette ? 2 : 1;
    const std::uint8_t joints[4] = {1, sharedIndex, 0, lastIndex};
    for (int v = 0; v < 4; ++v) {
        float xyz[3] = {float(v), 0, 0};
        const auto* p = reinterpret_cast<const std::uint8_t*>(xyz);
        vertices.insert(vertices.end(), p, p + 12);
        for (std::uint8_t c : {255, 255, 255, 255}) vertices.push_back(c);
        Bytes skin = {255, 0, 0, 0, joints[v], 0, 0, 0};
        vertices.insert(vertices.end(), skin.begin(), skin.end());
    }
    const std::uint8_t palettes[2][2] = {{3, 7}, {3, 9}};
    for (std::size_t b = 0; b < 2; ++b) {
        Bytes record;
        put32(record, 1);                        // triangles
        put32(record, 2);                        // palette size
        put32(record, std::uint32_t(b));         // first vertex: batches draw vertices 0-2 and 1-3
        put32(record, 3);
        std::copy(record.begin(), record.end(), batches.begin() + std::ptrdiff_t(b * 0x110));
        std::copy_n(palettes[b], 2, batches.begin() + std::ptrdiff_t(b * 0x110 + 16));
    }
    Bytes indices = {0, 0, 1, 0, 2, 0, 1, 0, 3, 0, 2, 0};
    return pooledBlocks({{-15, model}, {-15, {}}, {-15, {}}, {-15, mesh}, {-15, Bytes(0x24)}, {-15, {}},
                         {-15, material}, {-15, {}}, {-16, {}},
                         {-14, header}, {-14, batches}, {-14, indices}, {-14, vertices}, {-16, {}}});
}

void skinnedModels() {
    const ModelPart part = parseModel(skinnedModelFile(SkinVariant::Good)).meshes[0].parts[0];
    require(part.flags == 0x82 && part.boneBatches.size() == 2 && part.boneBatches[1].firstVertex == 1,
            "bone batches");
    require(part.vertices[0].joints[0] == 7 && part.vertices[1].joints[0] == 3 && part.vertices[3].joints[0] == 9,
            "palette indices resolved to skeleton joints");
    requireThrows([] { parseModel(skinnedModelFile(SkinVariant::Conflict)); }, "conflicting shared vertex accepted");
    requireThrows([] { parseModel(skinnedModelFile(SkinVariant::OutsidePalette)); }, "index outside palette accepted");
}

void models() {
    Model model = parseModel(modelFile(false));
    require(model.meshes.size() == 1 && model.meshes[0].parts.size() == 1, "model structure");
    const ModelPart& part = model.meshes[0].parts[0];
    require(part.textures.size() == 1 && part.textures[0] == "tex.tga", "material texture name");
    require(part.flags == 0x84 && vertexStride(part.flags) == 20, "vertex flags");
    require(part.vertices[1].position[0] == 1.0f && std::abs(part.vertices[2].uv0[1] - 1.0f) < 1e-6f, "vertex decode");
    require(part.vertices[0].color[0] == 255 && part.vertices[0].color[1] == 0, "vertex colour");
    require(part.indices == std::vector<std::uint16_t>{0, 1, 2}, "indices");
    requireThrows([] { parseModel(modelFile(true)); }, "unconsumed block accepted");
    auto mat = [](int m0, int m1, int m2, int m3, int b10) {
        std::uint8_t m[0x60] = {};
        m[0] = std::uint8_t(m0); m[1] = std::uint8_t(m1); m[2] = std::uint8_t(m2); m[3] = std::uint8_t(m3);
        m[10] = std::uint8_t(b10);
        return vertexStride(vertexFlags(m));
    };
    require(mat(0, 0, 0, 0, 0) == 16 && mat(0, 0, 1, 0, 0x09) == 24 && mat(0, 0, 2, 1, 0x11) == 20 &&
            mat(0, 1, 1, 0, 0x12) == 28 && mat(1, 0, 2, 1, 0x51) == 32 && mat(0, 0, 0, 6, 0x11) == 24,
            "strides match shipped layouts");

    // Draw state from material bytes (0x1292fc): byte 6 alpha reference, 7 blend equation, 8 and 9 blend factors,
    // 0xb bits 0-2 depth function, 3 depth write, 4 alpha test, 5 final-stage x2; 0xc bits 0-1 cull mode.
    ModelPart blended;
    blended.material.assign(0x60, 0);
    blended.material[10] = 0x09;
    blended.material[6] = 0x7f;
    blended.material[8] = 4;
    blended.material[9] = 5;
    blended.material[0xb] = 0x3b;
    blended.material[0xc] = 2;
    blended.material[0x13] = 0xff;
    blended.textureSlots.assign(8, 0);
    const MaterialState state = materialState(blended);
    require(state.supported && state.stages.size() == 2 && state.stages[0].source == 0x00e00003 && state.stages[0].constant == 0xffffffff &&
                state.stages[0].combiner == 0x00010000 && state.stages[0].scale == 0 && state.stages[1].source == 0x000f000f &&
                state.stages[1].combiner == 1 && state.stages[1].scale == 1,
            "mode 0 lighting 0 stage");
    require(state.alphaTest && state.alphaReference == 0x7f && state.blendSource == 6 && state.blendDestination == 7 &&
                state.cullMode == 0 && state.depthFunction == 4 && state.depthTest && state.depthWrite,
            "draw state from material bytes");
    blended.material[3] = 5;
    blended.material[10] = 0x11;
    blended.textureSlots.assign(16, 0);
    blended.textureSlots[7] = 0;
    blended.textureSlots[15] = 5;
    const MaterialState reflecting = materialState(blended);
    require(reflecting.reflection && reflecting.unitSlot[0] == 1 && reflecting.unitSlot[1] == 0 && reflecting.stages[0].source == 0x00e00004 &&
                reflecting.stages.back().operand == 0x200 && reflecting.stages.back().source == 0x000f04f3,
            "reflection binds units in reverse order");
    blended.material[1] = 4;
    require(!materialState(blended).supported, "unrecovered mode rejected");
}
}

namespace {
// Same synthetic level as tests/test_convert_levels.py: one object with three parameters.
Bytes levelFile(bool extraBlock) {
    Bytes head(0x38), obj(0x18), params;
    head[14] = 1;   // one object in section +0x2c
    obj[0xd] = 3;   // three parameters in the first list
    auto param = [&](const char* name, std::uint16_t kind) {
        put32(params, nameHash(name));
        params.push_back(2); params.push_back(0);
        params.push_back(std::uint8_t(kind)); params.push_back(0);
        put32(params, 1);
    };
    param("initialposition", 2);
    param("initialorientation", 2);
    param("meshname", 1);
    auto floats = [](float a, float b, float c) {
        Bytes out(12);
        float v[3] = {a, b, c};
        std::memcpy(out.data(), v, 12);
        return out;
    };
    std::vector<std::pair<std::int32_t, Bytes>> blocks{{-15, head}};
    for (int i = 0; i < 7; ++i) blocks.push_back({-15, {}});  // s10 s14 s18 s1c s20 s24 s28
    blocks.insert(blocks.end(), {{-15, obj}, {-15, bytes(std::string("Bench01", 8))}, {-15, padded("room1", 8)},
                                 {-15, params}, {-15, floats(1, 2, 3)}, {-15, floats(0, 90, 0)},
                                 {-15, padded("Prop_Bench", 12)}, {-15, {}}, {-15, {}},
                                 {-16, bytes(std::string("leprops/leprop_test", 20))}});
    if (extraBlock) blocks.push_back({-14, bytes("xx")});
    return pooledBlocks(blocks);
}

// A character class (two s10 defaults: HeadModel string, CharacterEnum 9) with one "Player1Start" point.
Bytes characterLevelFile() {
    Bytes head(0x38), obj(0x18), defaults, params;
    head[2] = 2;
    head[14] = 1;
    obj[0xd] = 2;
    auto param = [](Bytes& out, const char* name, std::uint16_t kind, std::uint32_t value) {
        put32(out, nameHash(name));
        out.push_back(2); out.push_back(0);
        out.push_back(std::uint8_t(kind)); out.push_back(0);
        put32(out, value);
    };
    param(defaults, "HeadModel", 1, 0);
    param(defaults, "CharacterEnum", 7, 9);
    param(params, "initialposition", 2, 1);
    param(params, "initialorientation", 2, 1);
    auto floats = [](float a, float b, float c) {
        Bytes out(12);
        float v[3] = {a, b, c};
        std::memcpy(out.data(), v, 12);
        return out;
    };
    std::vector<std::pair<std::int32_t, Bytes>> blocks{{-15, head}, {-15, defaults},
                                                       {-15, padded("Char_Head_Test", 16)}};
    for (int i = 0; i < 6; ++i) blocks.push_back({-15, {}});  // s14 s18 s1c s20 s24 s28
    blocks.insert(blocks.end(), {{-15, obj}, {-15, padded("Player1Start", 16)},
                                 {-15, padded("room1", 8)}, {-15, params},
                                 {-15, floats(-2, 1.5f, 17)}, {-15, floats(0, 181, 0)}, {-15, {}}, {-15, {}},
                                 {-16, padded("point", 8)}});
    return pooledBlocks(blocks);
}

void playerSpawn() {
    Level level = parseLevel(characterLevelFile());
    CharacterAppearance appearance = characterAppearance(level);
    require(appearance.head == "Char_Head_Test" && appearance.characterEnum == 9 && appearance.body.empty(),
            "character class defaults");
    auto start = playerStart(level);
    require(start && start->room == "room1" && start->position[2] == 17.0f && start->orientation[1] == 181.0f,
            "Player1Start placement");
    require(!playerStart(parseLevel(levelFile(false))), "level without Player1Start");
}

LevelParam stringParam(const std::string& name, std::uint16_t flags, const std::string& value) {
    LevelParam p;
    p.hash = nameHash(name);
    p.flags = flags;
    p.type = 1;
    p.blocks.push_back(bytes(value + std::string(1, '\0')));
    return p;
}

LevelParam float3Param(const std::string& name, std::uint16_t flags, float x, float y, float z) {
    LevelParam p;
    p.hash = nameHash(name);
    p.flags = flags;
    p.type = 2;
    float v[3] = {x, y, z};
    Bytes block(12);
    std::memcpy(block.data(), v, 12);
    p.blocks.push_back(block);
    return p;
}

void classDefaultPlacements() {
    Level crate;
    crate.classParams = {float3Param("initialposition", 6, 1, 2, 3), stringParam("meshname", 6, "Prop_Default")};
    Level bare;
    bare.defaults = {stringParam("meshname", 2, "")};
    Level level;
    auto object = [](const std::string& cls, const std::string& name) {
        LevelObject o;
        o.className = cls;
        o.instance = name;
        return o;
    };
    LevelObject inherits = object("crate", "inherits");
    LevelObject overrides = object("crate", "overrides");
    overrides.params = {stringParam("meshname", 2, "Prop_Own"), float3Param("initialposition", 2, 4, 5, 6),
                        float3Param("initialorientation", 2, 0, 90, 0)};
    LevelObject unlisted = object("bare", "unlisted");
    unlisted.params = {stringParam("meshname", 2, "Prop_Bare"), float3Param("initialposition", 2, 7, 8, 9)};
    LevelObject emptyMesh = object("bare", "empty");
    LevelObject noClass = object("missing", "noclass");
    noClass.params = {stringParam("meshname", 2, "Prop_Orphan")};
    level.objects = {inherits, overrides, unlisted, emptyMesh, noClass};
    ClassLookup lookup = [&](const std::string& name) -> const Level* {
        return name == "crate" ? &crate : name == "bare" ? &bare : nullptr;
    };
    auto found = objectPlacements(level, lookup);
    require(found.size() == 3, "objects without class or without a model are skipped");
    require(found[0].instance == "inherits" && found[0].mesh == "Prop_Default" && found[0].className == "crate" &&
            found[0].position[0] == 1.0f && found[0].position[2] == 3.0f && found[0].orientation[1] == 0.0f,
            "class default model and position");
    require(found[1].instance == "overrides" && found[1].mesh == "Prop_Own" && found[1].position[1] == 5.0f &&
            found[1].orientation[1] == 0.0f,
            "instance value replaces a slot parameter");
    require(found[2].instance == "unlisted" && found[2].mesh == "Prop_Bare" && found[2].position[0] == 0.0f,
            "a parameter the class does not list does not exist");
}

void levels() {
    playerSpawn();
    classDefaultPlacements();
    require(nameHash("meshname") == 0xacd1ebb8 && nameHash("InitialPosition") == 0x86166f20, "name hash");
    Level level = parseLevel(levelFile(false));
    require(level.objects.size() == 1 && level.objects[0].className == "leprops/leprop_test", "level object");
    auto places = placements(level);
    require(places.size() == 1 && places[0].mesh == "Prop_Bench" && places[0].room == "room1", "placement");
    require(places[0].position[2] == 3.0f && places[0].orientation[1] == 90.0f, "placement transform");
    require(meshModelPath("Prop_Plant_01") == "models/prop_plant_01/prop_plant_01.bfnmdl" &&
            meshModelPath("A/B_atlas") == "models/a/b_atlas.bfnmdl", "mesh model path");
    requireThrows([] { parseLevel(levelFile(true)); }, "unconsumed level block accepted");

    auto close = [](float a, float b) { return std::fabs(a - b) < 1e-5f; };
    Placement tilt;
    tilt.orientation[0] = 90;
    tilt.position[2] = 3;
    auto m = placementMatrix(tilt);
    require(close(m[4], 0) && close(m[5], 0) && close(m[6], 1) && m[14] == 3, "x rotation turns +y into +z");
    Placement flipped;
    flipped.orientation[0] = 180;
    flipped.orientation[1] = 30;
    flipped.orientation[2] = 180;
    auto f = placementMatrix(flipped);
    float c = std::cos(float(M_PI) * 150 / 180), sn = std::sin(float(M_PI) * 150 / 180);
    require(close(f[0], c) && close(f[2], -sn) && close(f[8], sn) && close(f[10], c) && close(f[5], 1),
            "Rz * Ry * Rx: (180, y, 180) is a yaw of 180 - y (0x237c48)");
}
}

namespace {
// One-part .bwav with a one-channel DSP-ADPCM CWAV; mirrors tests/test_convert_audio.py.
Bytes soundFile() {
    Bytes info = bytes("INFO");
    put32(info, 0);
    info.push_back(2); info.push_back(1); info.push_back(0); info.push_back(0);  // DSP-ADPCM, loop
    put32(info, 12000); put32(info, 1); put32(info, 3); put32(info, 0);         // rate, loop start, samples
    put32(info, 1);                                                             // channel table: 1 entry
    for (std::uint32_t v : {0x7100u, 0xcu}) put32(info, v);
    for (std::uint32_t v : {0x1f00u, 0x18u, 0x0300u, 0x14u, 0u}) put32(info, v); // channel info
    std::int16_t coefs[16] = {2048};
    for (auto c : coefs) { info.push_back(std::uint8_t(c)); info.push_back(std::uint8_t(c >> 8)); }
    info.resize(info.size() + 16, 0);
    info.resize((info.size() + 31) / 32 * 32, 0);
    std::uint32_t infoSize = std::uint32_t(info.size());
    std::memcpy(&info[4], &infoSize, 4);
    Bytes frame{0x04, 0x78, 0, 0, 0, 0, 0, 0};
    Bytes data = bytes("DATA");
    put32(data, 0x20 + std::uint32_t(frame.size()));
    data.resize(data.size() + 0x18, 0);
    data.insert(data.end(), frame.begin(), frame.end());
    std::uint32_t total = 0x40 + infoSize + std::uint32_t(data.size());
    Bytes cwav = bytes("CWAV");
    cwav.push_back(0xff); cwav.push_back(0xfe); cwav.push_back(0x40); cwav.push_back(0);
    put32(cwav, 0x02010000); put32(cwav, total);
    cwav.push_back(2); cwav.push_back(0); cwav.push_back(0); cwav.push_back(0);
    for (std::uint32_t v : {0x7000u, 0x40u, infoSize, 0x7001u, 0x40u + infoSize, std::uint32_t(data.size())}) put32(cwav, v);
    cwav.resize(0x40, 0);
    cwav.insert(cwav.end(), info.begin(), info.end());
    cwav.insert(cwav.end(), data.begin(), data.end());
    Bytes record(0x2c, 0);
    std::vector<std::pair<std::int32_t, Bytes>> blocks{{-14, record}, {-14, cwav}};
    return pooledBlocks(blocks);
}

Bytes bitstream(const std::vector<std::pair<std::uint32_t, unsigned>>& fields) {
    Bytes out;
    std::uint64_t value = 0;
    unsigned used = 0;
    for (const auto& [v, n] : fields) {
        value |= std::uint64_t(v & ((1u << n) - 1)) << used;
        used += n;
        while (used >= 8) {
            out.push_back(std::uint8_t(value));
            value >>= 8;
            used -= 8;
        }
    }
    out.push_back(std::uint8_t(value));
    return out;
}

Bytes matrixBytes(const Matrix& m) {
    Bytes out;
    for (float f : m) {
        std::uint32_t bits;
        std::memcpy(&bits, &f, 4);
        put32(out, bits);
    }
    return out;
}

bool near(float a, float b) { return std::fabs(a - b) < 1e-5f; }

void subtitleWrapping() {
    const auto measure = [](const std::string& text) { return int(text.size()); };
    require(subtitleWrapWidthStandIn(320) == 280 && subtitleWrapWidthStandIn(400) == 350, "stand-in wrap width is 7/8 of the screen");
    require(wrapSubtitleLines({"aaa bbb ccc"}, 7, measure) == std::vector<std::string>{"aaa bbb", "ccc"}, "wrap at the last space");
    require(wrapSubtitleLines({"aaa bbb"}, 7, measure) == std::vector<std::string>{"aaa bbb"}, "line of exactly the width stays");
    require(wrapSubtitleLines({"abcdefghij"}, 4, measure) == std::vector<std::string>{"abcd", "efgh", "ij"}, "word wider than the box breaks inside");
    require(wrapSubtitleLines({"aa bb", "", "cc dd ee"}, 5, measure) == std::vector<std::string>{"aa bb", "", "cc dd", "ee"}, "explicit breaks stay");
    const auto glyphs = [](const std::string& text) {
        int count = 0;
        for (char c : text) count += (static_cast<unsigned char>(c) & 0xc0) != 0x80;
        return count;
    };
    require(wrapSubtitleLines({"ab \xc3\xa9\xc3\xa9"}, 4, glyphs) == std::vector<std::string>{"ab", "\xc3\xa9\xc3\xa9"}, "wrap counts characters, not bytes");
}

void subtitles() {
    subtitleWrapping();
    auto extended = extendedCharacters(bytes("\xd0\xa0"));
    auto cues = parseSubtitles(bytes("\xef\xbb\xbf" "1\r\n00:00:01,600 --> 01:02:03,004\r\nFran\xe7" "ais\r\n\x01" "20!\r\n\r\n\r\n"
                                     "7\r\n00:00:07,500 --> 00:00:12,600\r\nLast"),
                               extended);
    require(cues.size() == 2 && cues[0].index == 1 && cues[0].startMs == 1600 && cues[0].endMs == 3723004,
            "subtitle times");
    require(cues[0].lines == std::vector<std::string>{"Fran\xc3\xa7" "ais", "\xd0\xa0!"}, "subtitle text decode");
    require(cues[1].index == 7 && cues[1].lines == std::vector<std::string>{"Last"}, "subtitle without final newline");
    require(parseSubtitles(bytes("")).empty(), "empty subtitle file");
    for (const char* bad : {"1\n00:00:01.600 --> 00:00:02,000\nx\n", "1\n00:00:01,600 -> 00:00:02,000\nx\n",
                            "1\n00:61:01,600 --> 00:62:02,000\nx\n", "1\n00:00:03,000 --> 00:00:02,000\nx\n",
                            "x\n00:00:01,000 --> 00:00:02,000\nx\n", "1\n00:00:01,000 --> 00:00:02,000\n\n",
                            "1\n"})
        requireThrows([&] { parseSubtitles(bytes(bad)); }, "malformed subtitles accepted");
}

Bytes u32Bytes(std::uint32_t value) {
    return {std::uint8_t(value), std::uint8_t(value >> 8), std::uint8_t(value >> 16), std::uint8_t(value >> 24)};
}

void particles() {
    // Reference: Python port of the 0x1886f0 disassembly (lower-case, h = (h << 4) + c, fold bits 28-31).
    require(pjwHash("Cloud") == 0x6a36b4 && pjwHash("fx_characterswapalpha") == 0x75649a1, "pjw hash");
    Bytes first(kParticleDefSize), second(kParticleDefSize);
    auto hash = u32Bytes(pjwHash("smoke"));
    std::copy(hash.begin(), hash.end(), first.begin() + 0x2c);
    second[0x2c] = 0x99;
    second[0x168] = 8;
    second[0x84] = 7;
    second[0x175] = 4;
    second[0x178] = 2;
    second[0x172] = 2;
    second[0xfc] = 0x11;
    second[0x100 + 3] = 0x3f;
    second[0x104 + 1] = 0x22;
    second[0x170] = 1;
    second[0x8c] = 0x80;
    second[0x38 + 3] = 0x40;  // duration 2.0
    second[0x28 + 3] = 0x41;  // rate 8.0
    second[0x3c + 3] = 0xc0;  // gravity -2.0
    second[0x173] = 1;
    second[0x134] = 0x7f;
    Bytes data(first);
    data.insert(data.end(), second.begin(), second.end());
    auto file = [&](std::uint32_t defCount, std::uint32_t scale, const Bytes& records) {
        return pooledBlocks({{-15, u32Bytes(10)}, {-15, u32Bytes(25)}, {-15, u32Bytes(2)}, {-15, u32Bytes(defCount)},
                             {-15, u32Bytes(0)}, {-15, u32Bytes(scale)}, {-16, bytes(std::string("fire\0\x12", 6))},
                             {-16, bytes(std::string("smoke\0", 6))}, {-14, records}});
    };
    ParticleDefs defs = parseParticleDefs(file(2, 0, data));
    require(defs.maxEmitters == 10 && defs.maxParticles == 25 && defs.scale == 1.0f &&
                defs.textures == std::vector<std::string>{"fire", "smoke"},
            "particle header and texture names");
    require(defs.defs.size() == 2 && defs.defs[0].texture == 1 && defs.defs[1].texture == -1 &&
                defs.defs[1].raw[0x2c] == 0x99 && defs.defs[1].flags == 8 && defs.defs[1].seed == 7 &&
                defs.defs[1].particles == 4 && defs.defs[1].atlasColumns == 2 && defs.defs[0].colourKeys.empty(),
            "particle records");
    const auto& keys = defs.defs[1].colourKeys;
    require(keys.size() == 2 && keys[0].time == 0 && keys[0].value[0] == 0x11 && keys[1].time == 0.5f &&
                keys[1].value[1] == 0x22,
            "particle colour keys");
    require(defs.defs[1].fadeKeys.size() == 1 && defs.defs[1].fadeKeys[0].value[0] == 0x80 &&
                defs.defs[1].duration == 2.0f && defs.defs[1].rate == 8.0f && defs.defs[1].gravity == -2.0f &&
                defs.defs[1].alphaKeys.size() == 1 && defs.defs[1].alphaKeys[0].value[0] == 0x7f &&
                defs.defs[1].sizeKeys.empty(),
            "particle fade keys and timing");
    require(parseParticleDefs(file(2, 0x40000000, data)).scale == 2.0f, "particle scale");
    requireThrows([&] { parseParticleDefs(file(3, 0, data)); }, "particle count mismatch accepted");
    for (std::size_t countAt : {0x170, 0x171, 0x172, 0x173}) {
        Bytes bad = data;
        bad[kParticleDefSize + countAt] = kMaxParticleKeys + 1;
        requireThrows([&] { parseParticleDefs(file(2, 0, bad)); }, "too many particle keys accepted");
    }
}

void animations() {
    // Translation x channel 9: base 150 (16 bits), delta width 3 (5 bits), deltas [0, 5].
    Bytes curve = bitstream({{0x08, 8}, {1, 3}, {150, 16}, {3, 5}, {0, 3}, {5, 3}});
    JointChannels f0 = decodeChannels(curve, 0, 2), f1 = decodeChannels(curve, 1, 2);
    require(f0.mask == 1u << 9 && near(f0.values[9], 1.50f) && near(f1.values[9], 1.55f), "channel decode");
    require(f0.values[3] == 1 && f0.values[4] == 1 && f0.values[5] == 1, "absent scale defaults to 1");
    Bytes negative = bitstream({{0x01, 8}, {1, 3}, {(1u << 12) - 100, 12}, {0, 4}});
    require(near(decodeChannels(negative, 0, 1).values[0], -2.0f), "signed rotation base");
    require(decodeChannels(Bytes{0xfa}, 0, 1).rawMatrix, "raw matrix header");

    JointChannels pose;
    const float values[12] = {0, 0, float(M_PI / 2), 2, 1, 1, 0, 0, 0, 1, 2, 3};
    std::memcpy(pose.values, values, sizeof values);
    pose.mask = 0xe3f;
    Matrix m = localMatrix(pose);
    const Matrix expected = {0, 2, 0, 0, -1, 0, 0, 0, 0, 0, 1, 0, 1, 2, 3, 1};
    for (std::size_t i = 0; i < 16; ++i) require(near(m[i], expected[i]), "rotation, scale, translation");
    JointChannels shear;
    const float shearValues[12] = {0, 0, 0, 1, 1, 1, 0.5f, 0.25f, 2, 0, 0, 0};
    std::memcpy(shear.values, shearValues, sizeof shearValues);
    shear.mask = 0x1c0;
    Matrix sm = localMatrix(shear);
    require(sm[0] == 1 && sm[1] == 1 && sm[2] == 0.5f && sm[4] == 0.25f && sm[9] == 2, "shear rows (0x24c734)");

    Matrix bind = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1};
    Bytes joints(0x98 * 2, 0);
    auto setJoint = [&](std::size_t i, std::int32_t parent, const Matrix& world, const Matrix& local) {
        std::uint32_t p = std::uint32_t(parent);
        for (int k = 0; k < 4; ++k) joints[i * 0x98 + 4 + std::size_t(k)] = std::uint8_t(p >> (8 * k));
        Bytes w = matrixBytes(world), l = matrixBytes(local);
        std::copy(w.begin(), w.end(), joints.begin() + std::ptrdiff_t(i * 0x98 + 8));
        std::copy(l.begin(), l.end(), joints.begin() + std::ptrdiff_t(i * 0x98 + 0x48));
    };
    setJoint(0, -1, bind, bind);
    setJoint(1, 0, bind, {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1});
    Bytes head(16, 0);
    head[1] = 2;
    std::vector<std::pair<std::int32_t, Bytes>> blocks{
        {-14, head}, {-14, {}}, {-14, {}}, {-14, joints}, {-14, bytes("root")}, {-14, bytes("spine")}};
    Skeleton skeleton = parseSkeleton(pooledBlocks(blocks));
    require(skeleton.joints.size() == 2 && skeleton.joints[1].name == "spine" && skeleton.joints[1].parent == 0,
            "skeleton joints");
    blocks.push_back({-14, bytes("x")});
    requireThrows([&] { parseSkeleton(pooledBlocks(blocks)); }, "unconsumed skeleton block accepted");

    Matrix turned = {0, 1, 0, 0, -1, 0, 0, 0, 0, 0, 1, 0, 2, 3, 4, 1};   // 90 degrees about z, then (2, 3, 4)
    setJoint(0, -1, turned, turned);
    Skeleton rotated = parseSkeleton(pooledBlocks({{-14, head}, {-14, {}}, {-14, {}}, {-14, joints},
                                                   {-14, bytes("root")}, {-14, bytes("spine")}}));
    Matrix rest = skinMatrices(rotated, Animation{}, 0)[0];
    for (std::size_t i = 0; i < 16; ++i)
        require(near(rest[i], i % 5 == 0 ? 1.0f : 0.0f), "unanimated joint with rotated bind skins to identity");

    Matrix attached = jointAttachment(rotated.joints[0], rest);
    for (std::size_t i = 0; i < 16; ++i) {
        const float expected[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 2, 3, 4, 1};
        require(near(attached[i], expected[i]), "unposed attachment has the bind position and no bind rotation");
    }

    Bytes animHead(0x28, 0);
    animHead[4] = 2;    // frames
    animHead[8] = 1;    // per-joint curve section present
    Bytes table;
    put32(table, 1);
    Animation anim = parseAnimation(pooledBlocks({{-14, animHead}, {-14, table}, {-14, bytes("p")}, {-14, curve}}));
    require(anim.frames == 2 && anim.jointCurves.size() == 1, "animation curves");
    {
        // Extra curve (table bit 28) with three frames at 30 frames per second: channel 10 (y) base 100, width 4, deltas 5, 10, 0.
        Bytes rootCurve = bitstream({{0x08, 8}, {2, 3}, {100, 16}, {4, 5}, {5, 4}, {10, 4}, {0, 4}});
        Bytes rootHead(0x28, 0);
        rootHead[4] = 3;
        rootHead[8] = 1;
        Bytes rootTable;
        put32(rootTable, 0x10000000u | (3u << 15) | (1u << 13));
        const Animation moving = parseAnimation(pooledBlocks({{-14, rootHead}, {-14, rootTable}, {-14, bytes("p")}, {-14, rootCurve}}));
        require(moving.jointCurves.size() == 1 && animationFramesPerSecond(moving) == 30.0f && near(animationDuration(moving), 2.0f / 30.0f),
                "joint table word: curve count, rate index 1 is 30 frames per second, duration (frames - 1) / rate");
        const std::optional<RootMotion> motion = rootMotion(moving);
        require(motion && motion->translation.size() == 3 && near(motion->translation[1][1], 0.05f) && near(motion->translation[2][1], -0.05f) &&
                    near(motion->maximum[1], 0.05f) && near(motion->minimum[1], -0.05f) && motion->maximum[0] == 0 && motion->minimum[2] == 0,
                "root motion is relative to frame 0, extent starts at 0");
        require(!rootMotion(anim), "no extra curve, no root motion");
        require(animationFramesPerSecond(anim) == 15.0f, "rate index 0 is 15 frames per second");
    }
    std::vector<Matrix> halfway = skinMatrices(skeleton, anim, 0.5);
    require(near(halfway[0][12], 1.525f), "fractional frame blends neighbouring frames");
    require(near(skinMatrices(skeleton, anim, 1.5)[0][12], 1.525f), "last frame blends into the first");
    auto turnedPose = [](float angle, unsigned mask) {
        JointChannels c;
        c.values[2] = angle;
        c.mask = mask;
        return jointPose(c);
    };
    JointPose flipped = turnedPose(float(M_PI / 2), 7);
    for (auto& q : flipped.rotation) q = -q;
    JointPose mid = blendPoses({{turnedPose(0, 7), 0.5f}, {flipped, 0.5f}});
    require(near(mid.rotation[2], float(std::sin(-M_PI / 8))), "quaternion sign flip before summing (0x24c924)");
    Joint oriented;
    oriented.orientation = {0, 0, float(std::sin(M_PI / 4)), float(std::cos(M_PI / 4))};
    JointChannels yawed;
    yawed.values[0] = float(M_PI / 2);
    JointPose combined = orientedPose(yawed, oriented);
    auto sampled = jointPose(yawed).rotation;
    const auto& o = oriented.orientation;
    require(combined.mask == 7 && near(combined.rotation[3], sampled[3] * o[3]) &&
                near(combined.rotation[0], sampled[0] * o[3]) && near(combined.rotation[1], -sampled[0] * o[2]) &&
                near(combined.rotation[2], sampled[3] * o[2]),
            "sampled rotation times joint orientation (0x247f3c)");
    require(orientedPose(yawed, Joint{}).rotation == sampled && orientedPose(yawed, Joint{}).mask == 0,
            "identity orientation leaves the pose unchanged");
    JointPose moved = turnedPose(0, 0xe00);
    moved.translation[0] = 1;
    require(near(blendPoses({{moved, 0.5f}, {turnedPose(0, 7), 0.5f}}).translation[0], 0.5f),
            "translation only from layers that carry it");
    std::vector<Matrix> skin = skinMatrices(skeleton, anim, 1);
    require(near(skin[0][12], 1.55f) && near(skin[0][13], -1.0f) && near(skin[0][0], 1.0f), "root skin matrix");
    require(near(skin[1][12], 1.55f) && near(skin[1][13], -1.0f), "child follows root (local * parent.world)");
}

// Same synthetic navigation file as tests/test_convert_nav.py (at 9d04c91).
Bytes navFile(bool extraBlock, std::size_t cells) {
    auto grid = [](std::uint16_t x, std::uint16_t height, std::uint16_t z, std::uint16_t w, std::uint16_t d,
                   int link, std::uint16_t extra) {
        Bytes raw(0x3c, 0);
        const std::uint16_t head[6] = {x, height, z, w, 0, d};
        std::memcpy(raw.data(), head, sizeof head);
        std::fill(raw.begin() + 0x14, raw.begin() + 0x34, 0xff);
        if (link >= 0) { raw[0x14] = std::uint8_t(link); raw[0x15] = 0; }
        raw[0x34] = std::uint8_t(extra);
        return raw;
    };
    Bytes grids = grid(10, 512, 20, 3, 2, 1, 1), second = grid(13, 0, 20, 1, 1, -1, 0);
    grids.insert(grids.end(), second.begin(), second.end());
    Bytes cellBytes = {1, 0, 0, 4, 12, 1};
    cellBytes.resize(cells + 2, 0);
    Bytes path(0x20, 0);
    const float position[6] = {1, 2, 3, 0.5f, 0.5f, 0.5f};
    std::memcpy(path.data(), position, sizeof position);
    path[0x18] = 3;
    std::vector<std::pair<std::int32_t, Bytes>> blocks{
        {-14, grids}, {-14, cellBytes}, {-14, Bytes{1, 2, 3, 4}}, {-14, Bytes{1, 0, 0, 0}},
        {-14, path}, {-14, Bytes{0, 0, 1, 0, 1, 0, 0, 0}}};
    if (extraBlock) blocks.push_back({-14, bytes("xx")});
    return pooledBlocks(blocks);
}

void navigation() {
    NavMesh nav = parseNav(navFile(false, 6));
    require(nav.grids.size() == 2 && nav.grids[0].width == 3 && nav.grids[0].depth == 2 && nav.grids[0].z == 20,
            "nav grid header");
    require(nav.grids[0].links == std::vector<std::uint16_t>{1} && nav.grids[1].links.empty(), "nav grid links");
    require(nav.grids[0].cells == std::vector<std::uint8_t>{1, 0, 0, 4, 12, 1}, "nav cells (padding dropped)");
    require(nav.paths.size() == 1 && nav.paths[0].position[2] == 3 &&
            nav.paths[0].indices == std::vector<std::uint16_t>{0, 1, 1}, "nav path");
    requireThrows([] { parseNav(navFile(true, 6)); }, "unconsumed nav block accepted");
    requireThrows([] { parseNav(navFile(false, 10)); }, "wrong nav cell count accepted");
}

void putF32(Bytes& b, std::size_t offset, float value) {
    std::uint32_t bits;
    std::memcpy(&bits, &value, sizeof bits);
    for (int i = 0; i < 4; ++i) b[offset + i] = std::uint8_t(bits >> (8 * i));
}

Bytes layoutElement(std::uint32_t type, std::uint16_t children, float x) {
    Bytes e(0x4c, 0xcd);
    for (std::size_t i = 8; i < 0x4c; ++i) e[i] = 0;
    e[8] = std::uint8_t(type);
    e[0x14] = 0xff;
    const float values[10] = {0.5f, 1.5f, -1, 1, x, 7, 0.5f, 0.5f, 64, 32};
    for (std::size_t i = 0; i < 10; ++i) putF32(e, 0x18 + 4 * i, values[i]);
    e[0x40] = std::uint8_t(children);
    e[0x42] = 0x11;
    e[0x46] = 0x0f;
    e[0x47] = 0x05;
    for (std::size_t i = 0; i < 4; ++i) e[0x48 + i] = std::uint8_t(0x10 * (i + 1));
    return e;
}

Bytes layoutFile(std::uint32_t childType, bool extraBlock) {
    Bytes storyboards(8, 0), storyboard(0x10, 0), track(0x20, 0);
    storyboards[4] = 1;
    storyboard[0xc] = 1;
    track[8] = 2;
    track[0xa] = 3;
    track[0x14] = 6;
    putF32(track, 0x18, 0.25f);
    std::vector<Bytes> keys;
    for (float time : {0.0f, 0.25f}) {
        Bytes key(0x20, 0);
        putF32(key, 4, 1 - 4 * time);
        putF32(key, 0x10, 1);
        putF32(key, 0x14, 1);
        putF32(key, 0x18, time);
        keys.push_back(key);
    }
    keys[1][0x1c] = 1;
    std::vector<std::pair<std::int32_t, Bytes>> blocks{
        {-14, Bytes{1, 0, 0, 0}}, {-14, layoutElement(1, 1, 0)}, {-14, bytes(std::string("LayoutRoot\0\xde", 12))},
        {-14, layoutElement(childType, 0, 20)}, {-14, bytes(std::string("img\0", 4))},
        {-14, bytes(std::string("images/a.png\0\0\0\0", 16))}, {-14, storyboards}, {-14, storyboard},
        {-14, bytes(std::string("Show\0\0\0\0", 8))}, {-14, track}, {-14, bytes(std::string("img\0", 4))},
        {-14, keys[0]}, {-14, keys[1]}};
    if (extraBlock) blocks.push_back({-14, Bytes(4)});
    return pooledBlocks(blocks);
}

void layouts() {
    UiLayout layout = parseLayout(layoutFile(2, false));
    const UiElement& root = layout.root;
    require(root.type == UiElementType::Container && root.name == "LayoutRoot" && root.source.empty() &&
            root.children.size() == 1, "layout root");
    const UiElement& image = root.children[0];
    require(image.type == UiElementType::Image && image.name == "img" && image.source == "images/a.png",
            "layout image element");
    require(image.unknown14 == 0xff && image.opacity == 0.5f && image.rotation == 1.5f && image.scale[0] == -1 &&
            image.position[0] == 20 && image.position[1] == 7 && image.origin[1] == 0.5f && image.size[0] == 64 &&
            image.size[1] == 32, "layout element floats");
    require(image.flags == 0x11 && image.callback == 0 && image.fontSize == 0x0f && image.alignment == 1 &&
            image.colour[0] == 0x10 && image.colour[3] == 0x40, "layout element trailer");
    require(layout.storyboards.size() == 1 && layout.storyboards[0].name == "Show" &&
            layout.storyboards[0].tracks.size() == 1, "layout storyboard");
    const UiTrack& track = layout.storyboards[0].tracks[0];
    require(track.target == "img" && track.property == 6 && track.repeat == 3 && track.duration == 0.25f &&
            track.keyframes.size() == 2, "layout track");
    require(track.keyframes[0].value == 1 && track.keyframes[1].value == 0 && track.keyframes[1].time == 0.25f &&
            track.keyframes[1].spline[2] == 1 && track.keyframes[1].flag == 1, "layout keyframes in file order");
    requireThrows([] { parseLayout(layoutFile(2, true)); }, "unconsumed layout block accepted");
    requireThrows([] { parseLayout(layoutFile(4, false)); }, "unknown layout element type accepted");
    requireThrows([] { parseLayout(layoutFile(1, false)); }, "container read a source block");
}

void localisation() {
    require(textKey("No") == 78u * 65599u + 79u && textKey("ok") == 0x4f13bcu, "text key hash (sdbm, upper case)");
    Bytes loc = bytes("LOCA");
    for (std::uint32_t v : {2u, 2u, 16u, 12u}) put32(loc, v);
    put32(loc, textKey("OK")); put32(loc, 0);
    put32(loc, textKey("ON")); put32(loc, 3);
    for (char c : std::string("OK\0On\0Fran\xe7\0", 12)) loc.push_back(std::uint8_t(c));
    auto entries = parseLocalisation(loc);
    require(entries.size() == 2 && entries[textKey("on")] == "On", "LOCA entries");
    auto extended = extendedCharacters(bytes("\xd0\xa0\xd1\x83x\n"));
    require(extended == std::vector<std::string>{"\xd0\xa0", "\xd1\x83", "x"}, "extended character list");
    require(decodeText("\x01" "20\x01" "21 \xe7\x01" "2;", extended) == "\xd0\xa0\xd1\x83 \xc3\xa7\x01" "2;",
            "text decode: escapes, Latin-1 to UTF-8, unknown escape kept");
    Bytes tnf = {0x23, 0, 0x20, 2};
    for (std::uint16_t i = 0; i < 3; ++i)
        for (std::uint16_t v : {std::uint16_t(1 + i), std::uint16_t(2), std::uint16_t(3 * i), std::uint16_t(0)}) {
            tnf.push_back(std::uint8_t(v));
            tnf.push_back(std::uint8_t(v >> 8));
        }
    Font font = parseFont(tnf);
    require(font.find(0x22) && font.find(0x22)->x == 6 && !font.find(0x23), "font glyph table");
    auto placed = layoutText(font, "!\"\xd1\x83", extended);
    require(placed.size() == 2 && placed[1].x == 3, "text layout (missing glyphs skipped)");
    Bytes badFont = tnf;
    badFont.push_back(0);
    requireThrows([&] { parseFont(badFont); }, "font with wrong size accepted");
    Bytes cut(loc.begin(), loc.end() - 1);
    requireThrows([&] { parseLocalisation(cut); }, "truncated LOCA accepted");
}

void putF32(Bytes& b, float v) {
    std::uint32_t bits;
    std::memcpy(&bits, &v, 4);
    put32(b, bits);
}

void cutscenes() {
    AnimTrack linear{0, {{0, 0, 0, 0, 1.0f / 3, 1}, {1, 1.0f / 3, 2, 3, 0, 0}}};
    require(near(evaluateTrack(linear, 0.5f), 1.5f) && near(evaluateTrack(linear, 0.25f), 0.75f),
            "Bezier with one-third handles is linear");
    require(evaluateTrack(linear, -1) == 0 && evaluateTrack(linear, 2) == 3, "track clamps outside its keys");
    AnimTrack eased{0, {{0, 0, 0, 0, 0.5f, 0}, {1, 0.5f, 1, 1, 0, 0}}};
    require(near(evaluateTrack(eased, 0.5f), 0.5f) && evaluateTrack(eased, 0.1f) < 0.1f, "eased track");

    Bytes head(0x28, 0);
    head[4] = 3;
    head[5] = 0x40;   // cutscene bit 14
    head[0x20] = 1;   // one sub-animation
    Bytes fixups, count, records(0x18, 0), name, times, values;
    put32(fixups, 0);
    put32(count, 1);
    records[4] = 2;
    put32(name, 0x191a8df3);
    putF32(times, 0);
    putF32(times, 1);
    for (float v : {0.0f, 0.0f, 10.0f, 1.0f / 3, 20.0f, 1.0f / 3, 30.0f, 40.0f, 0.0f, 0.0f}) putF32(values, v);
    std::vector<std::pair<std::int32_t, Bytes>> blocks{{-14, head}, {-14, {}}, {-14, fixups}, {-14, count},
                                                       {-14, records}, {-14, name}, {-14, {}}, {-14, times},
                                                       {-14, values}, {-14, {}}};
    Animation camera = parseAnimation(pooledBlocks(blocks));
    require(camera.frames == 3 && camera.subAnimations.size() == 1 && camera.subAnimations[0].size() == 1 &&
                camera.subAnimations[0][0].keys.size() == 2 && camera.subAnimations[0][0].keys[1].value == 40,
            "sub-animation track keys");
    CutsceneCamera shot = cutsceneCamera(camera, 0.5f);
    require(near(shot.position[0], 25) && shot.fieldOfView == 35 && near(shot.field11, 0.035f),
            "camera track and handler defaults (0x1c1fbc)");
    Matrix view = cameraMatrix(shot);
    require(view[0] == -1 && view[5] == 1 && view[10] == -1 && near(view[12], -25), "camera matrix (0x20e850)");
    records[4] = 3;
    blocks[4].second = records;
    requireThrows([&] { parseAnimation(pooledBlocks(blocks)); }, "key count larger than the key blocks accepted");

    ActorPose pose;
    pose.yaw = float(M_PI / 2);
    pose.position[0] = 5;
    Matrix actor = actorMatrix(pose);
    require(near(actor[2], -1) && near(actor[8], 1) && actor[12] == 5, "actor yaw and translation");

    Bytes text = bytes("CSCamera/cscamera_0000.fnanm\nDonutMan/donutman_minifig.fnmdl\r\nDonutMan/donutman_minifig.fnskl\n"
                       "DonutMan/donutman_0000_minifig.fnanm");
    text.push_back(0);
    auto manifest = parseManifest(pooledBlocks({{-14, text}}));
    require(manifest.size() == 4 && manifest[1] == "DonutMan/donutman_minifig.fnmdl", "manifest lines");
    auto actors = cutsceneActors(manifest, "csdata/models/1_1_a");
    require(actors.size() == 2 && actors[0].name == "cscamera" && actors[0].model.empty() &&
                actors[1].model == "csdata/models/1_1_a/donutman/donutman_minifig.bfnmdl" &&
                actors[1].skeleton == "csdata/models/1_1_a/donutman/donutman_minifig.bfnskl" &&
                actors[1].animation == "csdata/models/1_1_a/donutman/donutman_0000_minifig.bfnanm",
            "cutscene actors");
}

void tables() {
    Bytes head(4, 0), records(0x1c, 0), types{0, 6, 3, 8, 2, 1, 9, 0xcd, 0xcd}, rows(24, 0), stored{3},
        strings = bytes(std::string("\0walk\0", 6));
    head[0] = 1;
    records[0] = 7;          // columns
    records[2] = 2;          // rows
    records[4] = 12;         // stride: string 0, u8 4, listed u8 5, u16 6, int 8
    records[0x10] = 1;       // string pool present
    records[0x14] = 1;       // stored-column list present
    records[0x18] = 1;
    rows[12] = 1;
    rows[16] = 9;
    rows[17] = 1;
    rows[18] = 0x34;
    rows[19] = 0x12;
    rows[20] = 0x78;
    std::vector<std::pair<std::int32_t, Bytes>> blocks{{-14, head}, {-14, records}, {-14, types},
                                                       {-14, rows}, {-14, stored}, {-14, strings}};
    Table table = parseTable(pooledBlocks(blocks));
    const TableSheet& sheet = table.sheets.at(0);
    require(sheet.rows == 2 && sheet.stride == 12 && sheet.columns[4].offset == 6 && !sheet.columns[6].stored,
            "table layout (0x10e1cc)");
    require(!tableValue(sheet, 1, 0).present && tableValue(sheet, 0, 1).text.empty() &&
                tableValue(sheet, 1, 1).text == "walk" && tableValue(sheet, 1, 2).bits == 9 &&
                tableValue(sheet, 1, 3).bits == 1 && tableValue(sheet, 1, 4).bits == 0x1234 &&
                tableValue(sheet, 1, 5).bits == 0x78 && !tableValue(sheet, 1, 6).present,
            "table cells");
    records[4] = 16;
    blocks[1].second = records;
    requireThrows([&] { parseTable(pooledBlocks(blocks)); }, "table stride mismatch accepted");
}

void soundEffects() {
    const std::uint8_t types[]{0, 6, 2, 8, 8, 8, 8, 8, 8, 8, 8, 3, 3, 3, 3, 3, 4};
    const std::size_t sizes[]{0, 4, 2, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 4};
    TableSheet sheet;
    sheet.rows = 3;
    std::size_t offset = 0;
    for (std::size_t i = 0; i < 17; ++i) {
        TableColumn column;
        column.type = types[i];
        column.stored = sizes[i] != 0;
        column.size = sizes[i];
        column.offset = offset;
        offset += sizes[i];
        sheet.columns.push_back(column);
    }
    sheet.stride = offset;
    sheet.rowData.assign(3 * offset, 0);
    auto setByte = [&](std::size_t row, std::size_t column, std::uint8_t value) {
        sheet.rowData[row * sheet.stride + sheet.columns[column].offset] = value;
    };
    sheet.strings = bytes(std::string("\0ui/click\0", 10));
    sheet.rowData[1 * sheet.stride + sheet.columns[1].offset] = 1;
    setByte(1, 13, 80);
    setByte(1, 14, 20);
    setByte(1, 15, 30);
    sheet.rowData[2 * sheet.stride + sheet.columns[1].offset] = 1;
    setByte(2, 13, 55);
    Table table;
    table.sheets.push_back(sheet);
    std::vector<SoundEffect> effects = parseSoundEffects(table);
    require(effects.size() == 3 && effects[0].name.empty() && soundEffectPath(effects[0]).empty() &&
                effects[1].id == 1 && effects[1].name == "ui/click" && effects[1].volumePercent == 80 &&
                effects[1].paramA == 20 && effects[1].paramB == 30 && effects[2].volumePercent == 55 &&
                soundEffectPath(effects[1]) == "sounds/ui/click.bwav",
            "sound effect rows");
    require(findSoundEffect(effects, 2) == &effects[2] && !findSoundEffect(effects, 3), "sound effect lookup");
    table.sheets[0].columns.pop_back();
    requireThrows([&] { parseSoundEffects(table); }, "narrow sound effect sheet accepted");
}

void sounds() {
    Sound sound = decodeSound(soundFile());
    require(sound.rate == 12000 && sound.loop && sound.loopStart == 1 && sound.frames == 3, "sound header");
    require(sound.channels.size() == 1 && sound.channels[0] == std::vector<std::int16_t>{112, -16, -16},
            "DSP-ADPCM decode (floor shift)");
    require(sound.version == 0x02010000 && sound.encodings == std::vector<std::uint8_t>{Sound::DspAdpcm},
            "sound version and encodings");
    Bytes bad = soundFile();
    bad[12] = 0x30;   // sound-record block size 0x2c -> 0x30: blocks no longer close
    requireThrows([&] { decodeSound(bad); }, "bad .bwav accepted");
}
}

static void configText() {
    auto roots = parseConfigText("// header\r\nRoot \r\n{ \r\n\tChild\r\n\t{\r\n\t\tName(a b)\r\n\t\tNote(x, {y (z)} )\r\n\t\tEmpty()\r\n\t}\r\n\tLevel(3)\r\n\tList(1, 2.5,-3)\r\n}\r\nSecond{}");
    require(roots.size() == 2 && roots[0].name == "Root" && roots[1].name == "Second", "config roots");
    const ConfigBlock& root = roots[0];
    require(root.line == 2 && root.entries.size() == 2 && root.blocks.size() == 1, "config structure");
    require(root.number("Level") == 3 && root.numbers("List") == std::vector<double>({1, 2.5, -3}), "config numbers");
    const ConfigBlock& child = root.blocks[0];
    require(child.line == 4 && child.entries.size() == 3 && child.value("Name") == "a b", "config child");
    require(child.value("Note") == "x, {y (z)}" && child.value("Empty").empty(), "config nested parentheses");
    require(child.entries[2].key == "Empty" && child.entries[2].line == 8, "config entry order and line");
    require(!root.find("Name") && roots[1].blocks.empty(), "config lookup");
    requireThrows([&] { root.value("Missing"); }, "missing config key accepted");
    requireThrows([&] { child.number("Name"); }, "non-numeric config value accepted");
    for (const char* bad : {"Root\n{\n", "Root\n{\n A(1\n}\n", "Root\n{\n A(1)\n}\n}", "Root\n", "Root\n{\n A\n}\n",
                            "{\n}\n", "Root\n{\n A(1\n)\n}\n", "A(1)\n"})
        requireThrows([&] { parseConfigText(bad); }, bad);

    const std::string setup =
        "\tBound\r\n\t{\r\n\t\tName(Camera)\r\n\t\tType(2)\r\n\t\tRotationY(10)\r\n\t\tRotationX(15)\r\n\t\tDistance(8.5)\r\n"
        "\t\tLocation(0.5,5,13)\r\n\t\tLookat(0,2,0)\r\n\t\tOffset(0,2,1)\r\n\t\tFOV(43)\r\n\t\tFilters(32,28)\r\n"
        "\t\tBlendTime(0.85)\r\n\t\tShakeAmp(1)\r\n\t\tShakeRate(2)\r\n\t\tShakeRoll(3)\r\n\t\t3DOffset(5)\r\n\t}\r\n";
    CameraFollow camera = parseCameraFollow("CameraFollow\r\n{\r\n" + setup + "}\r\n");
    require(camera.setups.size() == 1, "camera setup count");
    const CameraSetup& s = camera.setups[0];
    require(s.kind == "Bound" && s.name == "Camera" && s.type == 2 && s.distance == 8.5 && s.rotationY == 10 &&
                s.location[0] == 0.5 && s.location[2] == 13 && s.offset[2] == 1 && s.filters[1] == 28 &&
                s.fov == 43 && s.blendTime == 0.85 && s.shakeRoll == 3 && s.offset3d == 5,
            "camera fields");
    requireThrows([&] { parseCameraFollow("Other\n{\n}\n"); }, "wrong camera root accepted");
    requireThrows([&] { parseCameraFollow("CameraFollow\n{\nBound\n{\nName(x)\n}\n}\n"); }, "incomplete camera accepted");
    std::string shortList = setup;
    shortList.replace(shortList.find("0.5,5,13"), 8, "0.5,5");
    requireThrows([&] { parseCameraFollow("CameraFollow\n{\n" + shortList + "}\n"); }, "short camera vector accepted");
}

int main() {
    try {
        configText();
        soundEffects();
        sounds();
        refpack();
        fuse();
        pooled();
        textures();
        models();
        skinnedModels();
        collisionModels();
        levels();
        navigation();
        layouts();
        localisation();
        subtitles();
        particles();
        animations();
        cutscenes();
        tables();
    } catch (const std::exception& error) {
        std::cerr << "assets-test failed: " << error.what() << '\n';
        return 1;
    }
    std::cout << "assets-test passed\n";
    return 0;
}
