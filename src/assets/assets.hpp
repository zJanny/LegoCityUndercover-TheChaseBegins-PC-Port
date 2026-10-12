#pragma once
// Native asset runtime (plan step S4a). Mirrors the verified Python tools in
// tools/; see docs/plans/linux-port.progress.md for the recovered formats.
// No SDL or GPU dependency.
#include <array>
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
    // Lower-case archive paths known from the CSV index.
    std::vector<std::string> names() const;
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
    // Skin attribute (flag 2): u8 weights summing to 255, then u8 palette indices, which
    // parseModel resolves to skeleton joint indices through the part's bone batches.
    std::uint8_t weights[4] = {0, 0, 0, 0};
    std::uint8_t joints[4] = {0, 0, 0, 0};
};
enum VertexAttribute : unsigned {
    AttrUv0 = 4, AttrUv1 = 8, AttrNormal = 0x40, AttrColor = 0x80, AttrSkin = 2
};
struct ModelPart {
    unsigned flags = 0;                 // replica of 0x12095c
    std::vector<Vertex> vertices;
    std::vector<std::uint16_t> indices; // triangle list
    std::vector<std::string> textures;  // material texture names, e.g. "Brick.tga"
    // Skinned parts: consecutive triangle runs with up to 26 joints each (the "Bones"
    // uniform), each drawing only vertices firstVertex .. firstVertex + vertices - 1.
    struct BoneBatch {
        std::uint32_t triangles = 0;
        std::uint32_t firstVertex = 0;
        std::uint32_t vertices = 0;
        std::vector<std::uint8_t> palette;
    };
    std::vector<BoneBatch> boneBatches;
};
struct ModelMesh {
    // float3 at mesh+8: the bounding-box centre of the mesh. When mesh byte 0 bit 0
    // is set, vertices are stored relative to it; parseModel already adds it back,
    // so ModelPart positions are always in model space (checked on all 6648 meshes).
    float centre[3] = {0, 0, 0};
    bool relative = false;
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

// .bwav: pooled resource with a 0x2c sound record, then one CWAV per channel part.
// PCM8/PCM16/DSP-ADPCM/IMA-ADPCM decode to signed 16-bit; parts become channels.
struct Sound {
    int rate = 0;
    bool loop = false;
    std::uint32_t loopStart = 0;
    std::uint32_t frames = 0;                     // samples per channel
    std::vector<std::vector<std::int16_t>> channels;
};
Sound decodeSound(const Bytes& data);

// Name hash 0x24da28: CRC-32 (reflected 0xEDB88320) of the lower-cased name.
std::uint32_t nameHash(const std::string& name);

// .blvl levels, parsed in the block order of loader 0x1cfc50. Only the object
// section (+0x2c) is kept; the other sections are consumed and skipped.
struct LevelParam {
    std::uint32_t hash = 0;
    std::uint16_t type = 0;
    std::uint32_t value = 0;           // inline value (record +8)
    std::vector<Bytes> blocks;         // out-of-line data (0x208ae0)
};
struct LevelObject {
    std::string className;             // e.g. "leprops/leprop_wobblydestructible"
    std::string instance;
    std::string room;
    std::vector<LevelParam> params;
};
struct Level {
    std::vector<LevelObject> objects;
};
Level parseLevel(const Bytes& data);

// Objects with a "meshname" parameter, with "initialposition" and
// "initialorientation" (degrees; Euler order not recovered).
struct Placement {
    std::string instance;
    std::string room;
    std::string mesh;
    float position[3] = {0, 0, 0};
    float orientation[3] = {0, 0, 0};
};
std::vector<Placement> placements(const Level& level);
// Object matrix: rotation of 0x237c48 (Rz * Ry * Rx for row vectors, i.e. x applied
// first) from "initialorientation" degrees (x 0.0174533, as at 0x1a48b0), with
// "initialposition" as the translation row.
std::array<float, 16> placementMatrix(const Placement& placement);
// "Dir/File" -> models/dir/file.bfnmdl, "Name" -> models/name/name.bfnmdl (lower case).
std::string meshModelPath(const std::string& mesh);

// Row-major 4x4 for row vectors (translation in the last row), as stored by the game.
using Matrix = std::array<float, 16>;
Matrix multiply(const Matrix& a, const Matrix& b);

// .bfnskl skeleton (loader 0x10f26c). Parents precede children.
struct Joint {
    std::string name;
    int parent = -1;
    Matrix world{};   // bind pose, record +0x08
    Matrix local{};   // relative to the parent, record +0x48
};
struct Skeleton {
    std::vector<Joint> joints;
};
Skeleton parseSkeleton(const Bytes& data);

// .bfnanm animation (loader 0x10edec); every block is consumed and checked, but
// only the per-joint curves (+0x08 section) are kept.
struct Animation {
    unsigned frames = 0;                   // header u16 +4
    std::vector<Bytes> jointCurves;        // indexed like the skeleton joints
};
Animation parseAnimation(const Bytes& data);

// Curve decoder 0x248910: 12 channels (rotation radians, scale, shear, translation).
struct JointChannels {
    float values[12] = {0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0};
    unsigned mask = 0;                     // bit per present channel
    bool rawMatrix = false;                // header 0xfa: stored 4x4 per frame, not decoded
};
JointChannels decodeChannels(const Bytes& curve, unsigned frame, unsigned frames);
// 0x248248: Euler radians to quaternion (x, y, z, w).
std::array<float, 4> eulerToQuaternion(float x, float y, float z);
// Pose entry built by 0x12d4b0: quaternion (x, y, z, w), scale, shear, translation, mask.
struct JointPose {
    std::array<float, 4> rotation{0, 0, 0, 1};
    float scale[3] = {1, 1, 1};
    float shear[3] = {0, 0, 0};
    float translation[3] = {0, 0, 0};
    unsigned mask = 0;
};
JointPose jointPose(const JointChannels& channels);
// 0x252cb0 with several layers: quaternions summed with a sign flip against the first
// (0x24c924) and normalised (0x24c898); scale summed over all layers, shear and
// translation over the layers that carry them. A single layer is returned unchanged.
JointPose blendPoses(const std::vector<std::pair<JointPose, float>>& layers);
// 0x24c734 with 0x248870: local matrix from one pose.
Matrix poseMatrix(const JointPose& pose);
Matrix localMatrix(const JointChannels& channels);
// Rigid-inverse(bind world) * animated world per joint (0x252cb0). A fractional frame
// blends floor(frame) and the next frame, wrapping, as 0x24bf80 does for looping playback.
std::vector<Matrix> skinMatrices(const Skeleton& skeleton, const Animation& animation, double frame);
// Weighted blend of the skin matrices of the vertex's resolved joints.
void skinVertex(const Vertex& vertex, const std::vector<Matrix>& skin, float position[3], float normal[3]);

std::uint32_t readU32(const Bytes& data, std::size_t offset);
std::uint16_t readU16(const Bytes& data, std::size_t offset);
} // namespace chase::assets
