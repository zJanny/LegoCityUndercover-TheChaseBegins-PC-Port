#include "block_reader.hpp"
#include <algorithm>
#include <array>
#include <cstring>
#include <stdexcept>

namespace chase::assets {
namespace {
using detail::cString;
using Reader = detail::BlockReader;
using detail::readF32;
// Rodata tables read by 0x12095c, as raw bytes from 0x3798c4; the u16 table there
// overlaps the byte tables at 0x3798d1, 0x3798e9 and 0x3798f8 exactly as in memory.
constexpr std::uint8_t rodata[0x44] = {
    0x80, 0x00, 0x40, 0x00, 0xc0, 0x00, 0x80, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x40, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0b, 0x0b, 0x03, 0x03, 0x0b, 0x04, 0x0b, 0x03, 0x03,
    0x0b, 0x06, 0x08, 0x09, 0x0a,
    0x00, 0x01, 0x01, 0x00, 0x01, 0x0b, 0x0b, 0x0b, 0x0b, 0x01, 0x0b, 0x0b, 0x0b, 0x00, 0x0b,
    0x00, 0x04, 0x0f, 0x00, 0x04, 0x0f, 0x0f, 0x0f, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
constexpr std::size_t byM1 = 0x0d, byM2 = 0x25, m2Extra = 0x34;

void requireSize(const Bytes& block, std::size_t size, const char* what) {
    if (block.size() < size) throw std::runtime_error(std::string("model block too small: ") + what);
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

// Block at vertex-buffer header +0x24, count at +0x20: 0x110-byte records of u32 triangles,
// u32 palette size, u32 first vertex, u32 vertex count, u8 palette[256].
void readBoneBatches(const Bytes& block, std::uint32_t count, ModelPart& part) {
    if (block.empty()) return;
    if (block.size() != std::size_t(count) * 0x110) throw std::runtime_error("bone batch block size mismatch");
    for (std::uint32_t i = 0; i < count; ++i) {
        std::size_t at = std::size_t(i) * 0x110;
        ModelPart::BoneBatch batch;
        batch.triangles = readU32(block, at);
        std::uint32_t size = readU32(block, at + 4);
        batch.firstVertex = readU32(block, at + 8);
        batch.vertices = readU32(block, at + 12);
        if (size > 0x100) throw std::runtime_error("bone palette too large");
        batch.palette.assign(block.begin() + std::ptrdiff_t(at + 16), block.begin() + std::ptrdiff_t(at + 16 + size));
        part.boneBatches.push_back(std::move(batch));
    }
}

// Rewrites palette indices to skeleton joints using the batch that draws each vertex.
// A vertex shared by two batches resolves identically in all shipped parts (checked).
void resolveSkinJoints(ModelPart& part) {
    if (part.boneBatches.empty()) return;
    std::vector<int> resolvedBy(part.vertices.size(), -1);
    std::vector<std::array<std::uint8_t, 4>> joints(part.vertices.size());
    std::size_t end = 0;
    for (const auto& batch : part.boneBatches) {
        std::size_t begin = end;
        end += std::size_t(batch.triangles) * 3;
        if (end > part.indices.size()) throw std::runtime_error("bone batches cover more triangles than the part");
        for (std::size_t i = begin; i < end; ++i) {
            std::uint16_t index = part.indices[i];
            if (index < batch.firstVertex || index - batch.firstVertex >= batch.vertices)
                throw std::runtime_error("vertex outside its bone batch range");
            const Vertex& v = part.vertices[index];
            std::array<std::uint8_t, 4> resolved{};
            for (int k = 0; k < 4; ++k) {
                if (!v.weights[k]) continue;
                if (v.joints[k] >= batch.palette.size()) throw std::runtime_error("skin index outside bone palette");
                resolved[std::size_t(k)] = batch.palette[v.joints[k]];
            }
            if (resolvedBy[index] >= 0 && joints[index] != resolved)
                throw std::runtime_error("vertex resolves to different joints in two bone batches");
            joints[index] = resolved;
            resolvedBy[index] = 0;
        }
    }
    for (std::size_t i = 0; i < part.vertices.size(); ++i)
        if (resolvedBy[i] >= 0) std::copy(joints[i].begin(), joints[i].end(), part.vertices[i].joints);
}

// 0x11528c: header, optional block, then index / vertex / optional groups (-14).
void readVertexBuffer(Reader& r, const Bytes& material, ModelPart& part) {
    Bytes header = r.next();
    requireSize(header, 0x78, "vertex buffer header");
    Bytes batches = readU32(header, 0x24) ? r.next() : Bytes{};
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
    readBoneBatches(batches, readU32(header, 0x20), part);
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
        if (part.flags & AttrSkin) {
            std::memcpy(out.weights, &vertexData[at], 4);
            std::memcpy(out.joints, &vertexData[at + 4], 4);
        }
    }
    resolveSkinJoints(part);
}

// 0x114640 (and 0x11fd20): collision section at model+0x1c, pool -14. Items of 0x2c bytes: +0x10 node
// count, +0x14 triangle count; the name, node and triangle chunks (1024 records each) and the vertex block
// follow in the order of the loader. The optional block set of an item with +0x28 set (0x11fd20) is skipped.
void readCollision(Reader& r, std::uint32_t count, std::vector<CollisionItem>& out) {
    r.cursor.push(Pool::M14);
    Bytes items = r.next();
    requireSize(items, std::size_t(count) * 0x2c, "collision items");
    std::vector<std::string> names(count);
    for (std::uint32_t i = 0; i < count; ++i) {
        names[i] = cString(r.next());
        for (int k = 0; k < 4; ++k) r.skip();
    }
    for (std::uint32_t i = 0; i < count; ++i) {
        const std::size_t base = std::size_t(i) * 0x2c;
        const std::uint32_t nodeCount = readU32(items, base + 0x10), triangleCount = readU32(items, base + 0x14);
        CollisionItem item;
        item.name = names[i];
        Bytes nodeBytes, triangleBytes;
        for (std::uint32_t n = 0; n < nodeCount; n += 0x400) {
            Bytes chunk = r.next();
            nodeBytes.insert(nodeBytes.end(), chunk.begin(), chunk.end());
        }
        for (std::uint32_t n = 0; n < triangleCount; n += 0x400) {
            Bytes chunk = r.next();
            triangleBytes.insert(triangleBytes.end(), chunk.begin(), chunk.end());
        }
        Bytes vertexBytes = r.next();
        if (nodeBytes.size() != std::size_t(nodeCount) * 0x1c) throw std::runtime_error("collision node size mismatch");
        if (triangleBytes.size() != std::size_t(triangleCount) * 0x20) throw std::runtime_error("collision triangle size mismatch");
        if (vertexBytes.size() % 12) throw std::runtime_error("collision vertex size mismatch");
        const std::uint32_t vertexCount = std::uint32_t(vertexBytes.size() / 12);
        item.vertices.resize(std::size_t(vertexCount) * 3);
        for (std::size_t k = 0; k < item.vertices.size(); ++k) item.vertices[k] = readF32(vertexBytes, k * 4);
        item.nodes.resize(nodeCount);
        for (std::uint32_t n = 0; n < nodeCount; ++n) {
            CollisionNode& node = item.nodes[n];
            for (int k = 0; k < 3; ++k) {
                node.centre[k] = readF32(nodeBytes, std::size_t(n) * 0x1c + 4 * std::size_t(k));
                node.half[k] = readF32(nodeBytes, std::size_t(n) * 0x1c + 12 + 4 * std::size_t(k));
            }
            node.info = readU32(nodeBytes, std::size_t(n) * 0x1c + 0x18);
        }
        item.triangles.resize(triangleCount);
        for (std::uint32_t t = 0; t < triangleCount; ++t) {
            CollisionTriangle& triangle = item.triangles[t];
            const std::size_t at = std::size_t(t) * 0x20;
            for (int k = 0; k < 3; ++k) triangle.normal[k] = readF32(triangleBytes, at + 4 * std::size_t(k));
            triangle.d = readF32(triangleBytes, at + 12);
            for (int k = 0; k < 3; ++k) {
                triangle.vertex[k] = readU32(triangleBytes, at + 16 + 4 * std::size_t(k));
                if (triangle.vertex[k] >= vertexCount) throw std::runtime_error("collision vertex index out of range");
            }
            triangle.surface = triangleBytes[at + 0x1c];
            std::memcpy(triangle.unknown, &triangleBytes[at + 0x1d], 3);
        }
        if (readU32(items, base + 0x28))
            for (int k = 0; k < 4; ++k) r.skip();
        out.push_back(std::move(item));
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
    model.block08 = r.next();                            // +0x08
    Bytes nodeBlock = r.next();                          // +0x0c
    requireSize(nodeBlock, nodes * 0x18, "nodes");
    for (std::size_t i = 0; i < nodes; ++i) {
        model.nodeNames.push_back(cString(r.next()));
        model.nodeMesh.push_back(std::int16_t(readU16(nodeBlock, i * 0x18 + 0x10)));
        model.nodePivot.push_back({readF32(nodeBlock, i * 0x18 + 4), readF32(nodeBlock, i * 0x18 + 8), readF32(nodeBlock, i * 0x18 + 12)});
        if (readU32(nodeBlock, i * 0x18 + 0x14)) r.skip();
    }
    Bytes meshBlock = r.next();                          // +0x10
    requireSize(meshBlock, meshes * 0x24, "meshes");
    for (std::size_t i = 0; i < meshes; ++i) r.skip(); // parts arrays
    model.nodeExtra = r.next();                          // +0x14
    if (readU32(header, 0x20)) {
        const Bytes matrices = r.next();
        requireSize(matrices, nodes * 0x40, "node matrices");
        model.nodeMatrices.resize(nodes);
        for (std::size_t i = 0; i < nodes; ++i)
            for (std::size_t k = 0; k < 16; ++k) model.nodeMatrices[i][k] = readF32(matrices, i * 0x40 + k * 4);
    }
    r.cursor.pop();
    if (readU32(header, 0x1c)) {
        r.cursor.push(Pool::M15);
        Bytes section = r.next();
        requireSize(section, 0x10, "section");
        if (readU32(section, 0)) {
            Bytes boxes = r.next();
            requireSize(boxes, std::size_t(readU32(section, 0)) * 0x5c, "collision boxes");
            for (std::uint32_t i = 0; i < readU32(section, 0); ++i) {
                CollisionBox box;
                box.name = cString(r.next());
                for (int a = 0; a < 3; ++a) {
                    box.position[a] = readF32(boxes, std::size_t(i) * 0x5c + 4 + 4 * std::size_t(a));
                    box.half[a] = readF32(boxes, std::size_t(i) * 0x5c + 0x10 + 4 * std::size_t(a));
                }
                for (int a = 0; a < 16; ++a) box.matrix[a] = readF32(boxes, std::size_t(i) * 0x5c + 0x1c + 4 * std::size_t(a));
                model.collisionBoxes.push_back(std::move(box));
            }
        }
        r.cursor.pop();
        if (readU32(section, 0xc)) readCollision(r, readU32(section, 8), model.collision);
    }
    r.cursor.push(Pool::M15);
    for (std::size_t m = 0; m < meshes; ++m) {
        ModelMesh mesh;
        for (int i = 0; i < 3; ++i) mesh.centre[i] = readF32(meshBlock, m * 0x24 + 8 + 4 * std::size_t(i));
        for (int i = 0; i < 3; ++i) mesh.half[i] = readF32(meshBlock, m * 0x24 + 0x14 + 4 * std::size_t(i));
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
