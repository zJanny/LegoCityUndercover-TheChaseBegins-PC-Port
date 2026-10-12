#include "assets.hpp"
#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace chase::assets {
namespace {
// Rodata tables read by 0x12095c, as raw bytes from 0x3798c4; the u16 table there
// overlaps the byte tables at 0x3798d1, 0x3798e9 and 0x3798f8 exactly as in memory.
constexpr std::uint8_t rodata[0x44] = {
    0x80, 0x00, 0x40, 0x00, 0xc0, 0x00, 0x80, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x40, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0b, 0x0b, 0x03, 0x03, 0x0b, 0x04, 0x0b, 0x03, 0x03,
    0x0b, 0x06, 0x08, 0x09, 0x0a,
    0x00, 0x01, 0x01, 0x00, 0x01, 0x0b, 0x0b, 0x0b, 0x0b, 0x01, 0x0b, 0x0b, 0x0b, 0x00, 0x0b,
    0x00, 0x04, 0x0f, 0x00, 0x04, 0x0f, 0x0f, 0x0f, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
constexpr std::size_t byM1 = 0x0d, byM2 = 0x25, m2Extra = 0x34;

struct Reader {
    const Bytes& data;
    PoolCursor cursor;
    explicit Reader(const Bytes& bytes) : data(bytes), cursor(bytes) {}
    // Next block of the current pool as a view; empty for NULL.
    Bytes next() {
        const Block* block = cursor.next();
        if (!block) return {};
        return Bytes(data.begin() + std::ptrdiff_t(block->offset),
                     data.begin() + std::ptrdiff_t(block->offset + block->size));
    }
    void skip() { cursor.next(); }
};

void requireSize(const Bytes& block, std::size_t size, const char* what) {
    if (block.size() < size) throw std::runtime_error(std::string("model block too small: ") + what);
}
std::string cString(const Bytes& block) {
    auto end = std::find(block.begin(), block.end(), std::uint8_t(0));
    return std::string(block.begin(), end);
}
float readF32(const Bytes& data, std::size_t offset) {
    std::uint32_t bits = readU32(data, offset);
    float value;
    std::memcpy(&value, &bits, sizeof value);
    return value;
}

// 0x114860: material record, extra pointer, texture slots in the -16 pool.
std::vector<std::string> readMaterial(Reader& r, Bytes& material) {
    material = r.next();
    requireSize(material, 0x60, "material");
    r.skip();
    unsigned count = (material[10] & 0x3fu) >> 3;
    r.cursor.push(Pool::M16);
    r.skip();
    std::vector<std::string> names;
    for (unsigned i = 0; i < count; ++i) {
        Bytes name = r.next();
        if (!name.empty()) names.push_back(cString(name));
    }
    r.cursor.pop();
    return names;
}

// 0x11528c: header, optional block, then index / vertex / optional groups (-14).
void readVertexBuffer(Reader& r, const Bytes& material, ModelPart& part) {
    Bytes header = r.next();
    requireSize(header, 0x78, "vertex buffer header");
    if (readU32(header, 0x24)) r.skip();
    std::size_t vertices = readU16(header, 2), indices = readU32(header, 4), stride = readU16(header, 0x1a);
    r.cursor.push(Pool::M14);
    Bytes indexData = r.next(), vertexData = r.next();
    if (readU32(header, 0x48)) r.skip();
    r.cursor.pop();
    part.flags = vertexFlags(material.data());
    if (vertexStride(part.flags) != stride) throw std::runtime_error("vertex stride disagrees with material");
    std::size_t expected = stride * vertices;
    if (readU32(header, 0x44)) expected += vertices * (readU32(header, 0x40) - 1) * 12;
    if (vertexData.size() != expected) throw std::runtime_error("vertex block size mismatch");
    if (indexData.size() != (indices + 1) / 2 * 4 || indices % 3) throw std::runtime_error("index block size mismatch");
    for (std::size_t i = 0; i < indices; ++i) {
        std::uint16_t index = readU16(indexData, i * 2);
        if (index >= vertices) throw std::runtime_error("vertex index out of range");
        part.indices.push_back(index);
    }
    // UV scales at header +0x50 (uv0.xy, uv1.xy), the "UVScales" shader uniform.
    const float scales[4] = {readF32(header, 0x50), readF32(header, 0x54), readF32(header, 0x58), readF32(header, 0x5c)};
    part.vertices.resize(vertices);
    for (std::size_t v = 0; v < vertices; ++v) {
        std::size_t at = v * stride;
        Vertex& out = part.vertices[v];
        for (int i = 0; i < 3; ++i) out.position[i] = readF32(vertexData, at + 4 * std::size_t(i));
        at += 12;
        auto uv = [&](float* dst, const float* scale) {
            dst[0] = std::int16_t(readU16(vertexData, at)) * scale[0];
            dst[1] = std::int16_t(readU16(vertexData, at + 2)) * scale[1];
            at += 4;
        };
        if (part.flags & AttrUv0) uv(out.uv0, scales);
        if (part.flags & AttrUv1) uv(out.uv1, scales + 2);
        if (part.flags & AttrNormal) {
            for (int i = 0; i < 3; ++i) out.normal[i] = std::int8_t(vertexData[at + std::size_t(i)]) / 127.0f;
            at += 4;
        }
        if (part.flags & AttrColor) {
            std::memcpy(out.color, &vertexData[at], 4);
            at += 4;
        }
    }
}

// 0x114640 (and 0x11fd20): optional section at model+0x1c, pool -14. Skipped, not interpreted.
void skipSection1c(Reader& r, std::uint32_t count) {
    r.cursor.push(Pool::M14);
    Bytes items = r.next();
    for (std::uint32_t i = 0; i < count; ++i)
        for (int k = 0; k < 5; ++k) r.skip();
    for (std::uint32_t i = 0; i < count; ++i) {
        requireSize(items, (i + 1) * 0x2c, "section item");
        std::size_t base = i * 0x2c;
        for (std::uint32_t n = 0; n < readU32(items, base + 0x10); n += 0x400) r.skip();
        for (std::uint32_t n = 0; n < readU32(items, base + 0x14); n += 0x400) r.skip();
        r.skip();
        if (readU32(items, base + 0x28))
            for (int k = 0; k < 4; ++k) r.skip();
    }
    r.cursor.pop();
}
}

unsigned vertexFlags(const std::uint8_t* m) {
    unsigned index;
    if (m[1] < 9) {
        if (m[2] >= m2Extra - byM2) throw std::runtime_error("material byte 2 outside table");
        index = rodata[byM2 + m[2]];
        if (index == 0 && (m[3] == 5 || m[3] == 6 || m[3] == 7)) index = 2;
    } else {
        if (m[1] >= byM2 - byM1) throw std::runtime_error("material byte 1 outside table");
        index = rodata[byM1 + m[1]];
    }
    if (index * 2 + 1 >= sizeof rodata) throw std::runtime_error("vertex base index outside table");
    unsigned flags = unsigned(rodata[index * 2] | rodata[index * 2 + 1] << 8);
    static const unsigned byM0[4] = {0, 2, 1, 3};
    flags |= byM0[m[0] & 3];
    if (m[10] & 7) flags |= (m[10] & 7) == 1 ? 4u : 0xcu;
    if ((m[10] & 0x40) || rodata[m2Extra + std::min<std::size_t>(m[2], 0x0f)] == 4 || m[3] == 7) flags |= 0x80;
    return flags;
}

std::size_t vertexStride(unsigned flags) {
    if (flags & 1) throw std::runtime_error("vertex flag 1 layout not recovered");
    std::size_t stride = 12;
    if (flags & AttrUv0) stride += 4;
    if (flags & AttrUv1) stride += 4;
    if (flags & AttrNormal) stride += 4;
    if (flags & AttrColor) stride += 4;
    if (flags & AttrSkin) stride += 8;
    return stride;
}

Model parseModel(const Bytes& data) {
    Reader r(data);
    Model model;
    r.cursor.push(Pool::M15);
    Bytes header = r.next();
    requireSize(header, 0x24, "model");
    std::size_t nodes = readU16(header, 2), meshes = readU16(header, 4), textures = header[6];
    r.skip();                                            // +0x08
    Bytes nodeBlock = r.next();                          // +0x0c
    requireSize(nodeBlock, nodes * 0x18, "nodes");
    for (std::size_t i = 0; i < nodes; ++i) {
        model.nodeNames.push_back(cString(r.next()));
        if (readU32(nodeBlock, i * 0x18 + 0x14)) r.skip();
    }
    Bytes meshBlock = r.next();                          // +0x10
    requireSize(meshBlock, meshes * 0x24, "meshes");
    for (std::size_t i = 0; i < meshes; ++i) r.skip(); // parts arrays
    r.skip();                                            // +0x14
    if (readU32(header, 0x20)) r.skip();
    r.cursor.pop();
    if (readU32(header, 0x1c)) {
        r.cursor.push(Pool::M15);
        Bytes section = r.next();
        requireSize(section, 0x10, "section");
        if (readU32(section, 0)) {
            r.skip();
            for (std::uint32_t i = 0; i < readU32(section, 0); ++i) r.skip();
        }
        r.cursor.pop();
        if (readU32(section, 0xc)) skipSection1c(r, readU32(section, 8));
    }
    r.cursor.push(Pool::M15);
    for (std::size_t m = 0; m < meshes; ++m) {
        ModelMesh mesh;
        for (int i = 0; i < 3; ++i) mesh.centre[i] = readF32(meshBlock, m * 0x24 + 8 + 4 * std::size_t(i));
        mesh.relative = meshBlock[m * 0x24] & 1;
        std::size_t parts = meshBlock[m * 0x24 + 2];
        std::vector<Bytes> materials(parts);
        mesh.parts.resize(parts);
        for (std::size_t p = 0; p < parts; ++p) mesh.parts[p].textures = readMaterial(r, materials[p]);
        r.cursor.push(Pool::M14);
        for (std::size_t p = 0; p < parts; ++p) {
            readVertexBuffer(r, materials[p], mesh.parts[p]);
            if (mesh.relative)
                for (auto& v : mesh.parts[p].vertices)
                    for (int i = 0; i < 3; ++i) v.position[i] += mesh.centre[i];
        }
        r.cursor.pop();
        model.meshes.push_back(std::move(mesh));
    }
    r.cursor.pop();
    r.cursor.push(Pool::M16);
    r.skip();                                            // texture records
    for (std::size_t i = 0; i < textures; ++i) {
        r.skip();
        model.textures.push_back(cString(r.next()));
    }
    r.cursor.pop();
    if (!r.cursor.exhausted()) throw std::runtime_error("unconsumed model blocks");
    return model;
}
} // namespace chase::assets
