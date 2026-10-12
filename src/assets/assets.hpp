#pragma once
// Native asset runtime (plan step S4a). Mirrors the verified Python tools in
// tools/; see docs/plans/linux-port.progress.md for the recovered formats.
// No SDL or GPU dependency.
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <map>
#include <string>
#include <vector>

namespace chase::assets {
using Bytes = std::vector<std::uint8_t>;

// Codec 1 of the FUSE archive (decoder 0x12efe0): headerless RefPack variant.
// Throws on any out-of-bounds read or reference, or output beyond `limit`.
Bytes decompressRefPack(const std::uint8_t* data, std::size_t size, std::size_t limit);

struct FuseEntry {
    std::uint32_t crc;
    std::uint32_t offset;
    std::uint32_t size;   // decoded size
    std::uint32_t flags;  // bits 0-1 codec, bits 2-4 block size exponent
};

// FUSE1.00 .fib archive (open routine 0x17dea8, entry read 0x12894c).
class FuseArchive {
public:
    explicit FuseArchive(const std::string& path);
    const std::vector<FuseEntry>& entries() const { return entries_; }
    Bytes read(const FuseEntry& entry);
    // Shipped CSV index: "CRC, name, compressed size, size, offset" per line.
    void loadNames(const std::string& csvPath);
    // Case-insensitive lookup by archive path; nullptr when unknown.
    const FuseEntry* find(const std::string& name) const;
private:
    Bytes readAt(std::uint64_t offset, std::size_t size);
    std::ifstream file_;
    std::uint64_t fileSize_ = 0;
    std::vector<FuseEntry> entries_;
    std::map<std::uint32_t, std::size_t> byCrc_;
    std::map<std::string, std::uint32_t> names_;
};

// Pooled resource layout shared by every b* format (loader 0x24b740/0x253800).
enum class Pool { M14 = -14, M15 = -15, M16 = -16 };
struct Block {
    Pool pool;
    std::size_t offset;
    std::size_t size;
};
std::vector<Block> parsePooled(const Bytes& data);

// Hands out blocks per pool in table order, like 0x253800. A size-0 block is NULL.
class PoolCursor {
public:
    explicit PoolCursor(const Bytes& data);
    void push(Pool pool) { stack_.push_back(pool); }
    void pop();
    const Block* next();   // nullptr for a NULL (size-0) block
    bool exhausted() const;
private:
    std::map<Pool, std::vector<Block>> queues_;
    std::map<Pool, std::size_t> cursor_;
    std::vector<Pool> stack_;
};

// .btga/.bpng/.bdds: PICA200 texture, largest mip level, rows top-down.
// Cubemaps are returned as a vertical strip of six faces in stored order.
struct Image {
    int width = 0;
    int height = 0;      // total rows (6 x face height for cubemaps)
    int format = 0;
    int faces = 1;
    int levels = 0;
    std::vector<std::uint8_t> rgba;
};
Image decodeTexture(const Bytes& data);

// .bfnmdl geometry, parsed in the block order of deserializer 0x10ccf4.
struct Vertex {
    float position[3] = {0, 0, 0};
    float uv0[2] = {0, 0};
    float uv1[2] = {0, 0};
    float normal[3] = {0, 0, 0};
    std::uint8_t color[4] = {255, 255, 255, 255};
};
enum VertexAttribute : unsigned {
    AttrUv0 = 4, AttrUv1 = 8, AttrNormal = 0x40, AttrColor = 0x80, AttrSkin = 2
};
struct ModelPart {
    unsigned flags = 0;                 // replica of 0x12095c
    std::vector<Vertex> vertices;
    std::vector<std::uint16_t> indices; // triangle list
    std::vector<std::string> textures;  // material texture names, e.g. "Brick.tga"
};
struct ModelMesh {
    float f8[3] = {0, 0, 0};            // raw float3 at mesh+8; meaning not recovered
    std::vector<ModelPart> parts;
};
struct Model {
    std::vector<std::string> nodeNames;
    std::vector<ModelMesh> meshes;
    std::vector<std::string> textures;  // model-level texture list (-16 pool)
};
Model parseModel(const Bytes& data);
unsigned vertexFlags(const std::uint8_t* material);
// Byte stride implied by a flag mask; throws for the unrecovered flag-1 layout.
std::size_t vertexStride(unsigned flags);

std::uint32_t readU32(const Bytes& data, std::size_t offset);
std::uint16_t readU16(const Bytes& data, std::size_t offset);
} // namespace chase::assets
