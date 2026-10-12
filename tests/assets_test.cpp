// Synthetic fixtures for the native asset runtime; mirrors tests/test_extract_fib.py,
// tests/test_tt_resource.py and tests/test_convert_textures.py.
#include "assets/assets.hpp"
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
    Bytes raw = archive.read(archive.entries()[0]);
    require(std::string(raw.begin(), raw.end()) == "raw", "stored entry");
    const FuseEntry* packed = archive.find("models/packed.bin");
    require(packed && packed->crc == 0x22, "case-insensitive CSV lookup");
    Bytes unpacked = archive.read(*packed);
    require(std::string(unpacked.begin(), unpacked.end()) == decoded, "codec 1 entry");
    require(archive.read(archive.entries()[2]) == big, "codec 2 chunks");
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

// Same synthetic model as tests/test_convert_models.py: one mesh, one textured triangle.
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

void models() {
    Model model = parseModel(modelFile(false));
    require(model.meshes.size() == 1 && model.meshes[0].parts.size() == 1, "model structure");
    const ModelPart& part = model.meshes[0].parts[0];
    require(part.textures.size() == 1 && part.textures[0] == "tex.tga", "material texture name");
    require(part.flags == 0x84 && vertexStride(part.flags) == 20, "vertex flags");
    require(part.vertices[1].position[0] == 1.0f && part.vertices[2].uv0[1] == 1.0f, "vertex decode");
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
    blocks.insert(blocks.end(), {{-15, obj}, {-15, bytes(std::string("Bench01", 8))}, {-15, bytes(std::string("room1", 8))},
                                 {-15, params}, {-15, floats(1, 2, 3)}, {-15, floats(0, 90, 0)},
                                 {-15, bytes(std::string("Prop_Bench", 12))}, {-15, {}}, {-15, {}},
                                 {-16, bytes(std::string("leprops/leprop_test", 20))}});
    if (extraBlock) blocks.push_back({-14, bytes("xx")});
    return pooledBlocks(blocks);
}

void levels() {
    require(nameHash("meshname") == 0xacd1ebb8 && nameHash("InitialPosition") == 0x86166f20, "name hash");
    Level level = parseLevel(levelFile(false));
    require(level.objects.size() == 1 && level.objects[0].className == "leprops/leprop_test", "level object");
    auto places = placements(level);
    require(places.size() == 1 && places[0].mesh == "Prop_Bench" && places[0].room == "room1", "placement");
    require(places[0].position[2] == 3.0f && places[0].orientation[1] == 90.0f, "placement transform");
    require(meshModelPath("Prop_Plant_01") == "models/prop_plant_01/prop_plant_01.bfnmdl" &&
            meshModelPath("A/B_atlas") == "models/a/b_atlas.bfnmdl", "mesh model path");
    requireThrows([] { parseLevel(levelFile(true)); }, "unconsumed level block accepted");
}
}

int main() {
    try {
        refpack();
        fuse();
        pooled();
        textures();
        models();
        levels();
    } catch (const std::exception& error) {
        std::cerr << "assets-test failed: " << error.what() << '\n';
        return 1;
    }
    std::cout << "assets-test passed\n";
    return 0;
}
