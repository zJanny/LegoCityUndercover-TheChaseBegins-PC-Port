#pragma once
// Native asset runtime (plan step S4a). The recovered formats are described in docs/re/.
// No SDL or GPU dependency.
#include <array>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <functional>
#include <map>
#include <optional>
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
    // `storedSize`, when given, receives the bytes the entry occupies in the archive.
    Bytes read(const FuseEntry& entry, std::uint64_t* storedSize = nullptr);
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
    std::vector<std::uint8_t> material;      // raw 0x60-byte material record (0x114860)
    std::vector<std::uint8_t> materialExtra; // block after the material record
    std::vector<std::uint8_t> textureSlots;  // 8-byte slot records, one per texture (byte 7 is the role)
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
    float half[3] = {0, 0, 0};   // float3 at mesh+0x14: half extents of the bounding box (0x242eb0)
    bool relative = false;
    std::vector<ModelPart> parts;
};
// Collision mesh of a model (section at model+0x1c, loader 0x114640; docs/re/collision.md). Vertices are in
// the model's coordinates, which are world coordinates for a level's master model. The triangle record is
// 0x20 bytes: plane normal, plane offset d (normal . p = d), three vertex indices, a surface byte and three
// bytes that are not understood. Nodes (0x1c bytes: box centre, box half extents, info) form a bounding-box tree: bit 0 of info marks a leaf,
// (info & 0x1f) >> 1 is the child or triangle count and info >> 5 the index of the first child or triangle.
struct CollisionTriangle {
    float normal[3] = {0, 0, 0};
    float d = 0;
    std::uint32_t vertex[3] = {0, 0, 0};
    std::uint8_t surface = 0;       // bit index in the surface exclusion masks of the queries
    std::uint8_t unknown[3] = {0, 0, 0};
};
struct CollisionNode {
    float centre[3] = {0, 0, 0};
    float half[3] = {0, 0, 0};   // half extents of the node's bounding box
    std::uint32_t info = 0;
    bool leaf() const { return info & 1; }
    std::uint32_t count() const { return (info & 0x1f) >> 1; }
    std::uint32_t first() const { return info >> 5; }
};
// One 0x10-byte cell of the floor graph (optional block set of an item with +0x28 set, loader 0x11fd20; the game keeps the
// "collision_floor" graph in 0x376c60 count, 0x376c68 cells, 0x376c64 extra triangles, 0x376c6c extra vertices; 0x28b070).
// Word 0 bit 0 set: a triangle of the item's own chunks (chunk word >> 11, index (word >> 1) & 0x3ff), the loader sets
// flag 0x2000; clear: extra triangle word >> 1, whose vertex indices count in the extra vertex block. +4 is cleared at load,
// +6 flags, +8 the cell's own index, +0xa / +0xc / +0xe the neighbour across each edge as read by 0x1c54f8 and 0x15c4a4
// (signed values -1..-3 are markers, every other value an unsigned cell index).
struct FloorCell {
    bool itemTriangle = false;
    std::uint32_t triangle = 0;   // index into CollisionItem::triangles or FloorGraph::extraTriangles
    std::uint16_t flags = 0;      // as in the file, without the loader's 0x2000
    std::uint16_t index = 0;
    std::uint16_t neighbour[3] = {0, 0, 0};
};
struct FloorGraph {
    std::uint32_t header[6] = {0, 0, 0, 0, 0, 0};  // words of the header block; 0x11fd20 replaces words 1, 3 and 5 by the block pointers
    std::vector<FloorCell> cells;
    std::vector<CollisionTriangle> extraTriangles;
    std::vector<float> extraVertices;              // x, y, z triples
};
struct CollisionItem {
    std::string name;                           // e.g. "Collision_Floor", "Collision1"
    std::vector<float> vertices;                // x, y, z triples
    std::vector<CollisionTriangle> triangles;
    std::vector<CollisionNode> nodes;           // nodes[0] is the root
    std::optional<FloorGraph> floor;
};
// Box record of the collision section (0x5c bytes at model+0x1c section +4, count at +0): a name, a float3 at +4,
// half extents at +0x10 and a 4x4 matrix at +0x1c (loader 0x28b070 reads them for "collision_vehicle" boxes,
// 0x23aff8 for the box of a prop whose model has no collision items).
struct CollisionBox {
    std::string name;
    float position[3] = {0, 0, 0};
    float half[3] = {0, 0, 0};
    float matrix[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
};
struct Model {
    std::vector<CollisionBox> collisionBoxes;
    std::vector<std::string> nodeNames;
    std::vector<int> nodeMesh;                   // mesh index of each node (i16 at node record +0x10), -1 for none
    std::vector<std::array<float, 3>> nodePivot;  // float3 at node record +4: the point a door node turns about (0x20a2f4)
    std::vector<std::uint8_t> nodeExtra;         // block at model+0x14
    std::vector<std::array<float, 16>> nodeMatrices;  // block at model+0x20: 0x40 bytes per node (row vectors, translation last), empty when the model has none
    std::vector<std::uint8_t> block08;           // block at model+0x08
    std::vector<ModelMesh> meshes;
    std::vector<std::string> textures;  // model-level texture list (-16 pool)
    std::vector<CollisionItem> collision;
};
Model parseModel(const Bytes& data);

// One PICA texture-environment stage as the game writes it (0x12e460, command record from 0x24c56c):
// the SOURCE, OPERAND, COMBINER, COLOR and SCALE register values of registers 0xc0 + 8 * stage .. + 4
// (stages 4 and 5 sit at 0xf0 and 0xf8). Sources: 0 vertex colour, 1 fragment lighting primary, 2 fragment
// lighting secondary, 3..6 texture units, 0xd previous buffer, 0xe constant, 0xf previous stage.
struct TevStage {
    std::uint32_t source = 0, operand = 0, combiner = 0, constant = 0, scale = 0;
};
// What the game's draw setup derives from a material record (0x128dc8 and 0x12e460 for the combiner,
// 0x1292fc for blending, alpha test, depth and culling).
struct MaterialState {
    bool supported = false;
    unsigned mode = 0, lightingKey = 0, textureKey = 0, alphaKey = 0;
    bool lighting = false;
    std::vector<TevStage> stages;
    std::uint32_t bufferUpdate = 0;  // bit n: the buffer takes the output of stage n - 1 before stage n
    bool reflection = false;         // texture units are bound in reverse order and the cube texture is used
    int unitSlot[3] = {-1, -1, -1};  // material texture slot bound to texture unit k, -1 when unbound
    bool alphaTest = false;
    std::uint8_t alphaReference = 0; // the test passes when alpha > reference
    unsigned blendEquation = 0, blendSource = 1, blendDestination = 0;  // PICA values
    unsigned depthFunction = 0;      // PICA comparison, 0 never .. 7 greater-equal
    bool depthTest = true, depthWrite = true;
    unsigned cullMode = 1;           // PICA: 0 none, 1 culls counter-clockwise, 2 culls clockwise
    bool fogged = true;              // 0x1292fc: fog applies unless material byte 0xb bit 6 is set or byte 9 (blend destination index) is 1
};
MaterialState materialState(const ModelPart& part);

unsigned vertexFlags(const std::uint8_t* material);
// Byte stride implied by a flag mask; throws for the unrecovered flag-1 layout.
std::size_t vertexStride(unsigned flags);

// .bwav: pooled resource with a 0x2c sound record, then one CWAV per channel part.
// PCM8/PCM16/DSP-ADPCM/IMA-ADPCM decode to signed 16-bit; parts become channels.
struct Sound {
    enum Encoding : std::uint8_t { Pcm8, Pcm16, DspAdpcm, ImaAdpcm };
    int rate = 0;
    bool loop = false;
    std::uint32_t loopStart = 0;
    std::uint32_t frames = 0;                     // samples per channel
    std::uint32_t version = 0;                    // CWAV version word of the first part
    std::vector<std::uint8_t> encodings;          // Encoding of each part
    std::vector<std::vector<std::int16_t>> channels;
};
Sound decodeSound(const Bytes& data);

// Name hash 0x24da28: CRC-32 (reflected 0xEDB88320) of the lower-cased name.
std::uint32_t nameHash(const std::string& name);

// .blvl levels, parsed in the block order of loader 0x1cfc50. Only the object
// section (+0x2c) is kept; the other sections are consumed and skipped.
struct LevelParam {
    std::uint32_t hash = 0;
    std::uint16_t flags = 0;           // record +4: bit 0 inline class value, bit 1 per-instance slot (lookup 0x217080)
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
// Section +0x30: navigation descriptor per room (used by loader 0x1971c4). Cell world
// position: origin + cellSize * cell; height origin.y + u16 * heightUnit / 256 (data fit).
struct NavRoom {
    std::string room;
    float heightUnit = 0;
    float cellSize = 0;
    float origin[3] = {0, 0, 0};
    std::uint16_t grids = 0;
    std::uint16_t paths = 0;
    std::uint16_t base = 0;  // descriptor +4: first global grid number of the room
    std::uint16_t end = 0;   // descriptor +6: one past the last global grid number
    std::uint16_t section = 0;  // index of the section record (0x28 bytes) the room belongs to; each has its own cell size and origin
};
// Section +0x24 record of a class file (0x28 bytes): a name and, at +0x10 and +0x1c, a centre and half extents.
// Lookup 0x23e798 finds the one named "CollisionBound" for the collision box of an object. The bytes +8..+0xb are the
// node index (0xff none), the shape (2 sphere, 3 box, 4 and 6 cylinder; test 0x22f650) and two switches that the
// player bound tests 0x2c185c / 0x28aef4 both require (+0xa is the one EnableBound 0x1d0ce8 writes).
struct LevelBound {
    std::string name;
    float centre[3] = {0, 0, 0};
    float half[3] = {0, 0, 0};
    std::uint8_t node = 0xff, shape = 0, enabled = 0, active = 0;
    float extent = 0;  // +0xc: read by the camera record weights 0x1d34ac as twice the length of the probe segment
    // Block at +4: the room the bound belongs to. The room setup 0x173944 sets +0xb to 1 for every bound and back to 0
    // for the bounds of a room (the room's bound list); empty for a bound outside the rooms.
    std::string room;
};
// Type 4 parameter (object reference): the block holds the name hash of the referenced instance at +4, 0 when unset.
std::uint32_t objectReferenceHash(const LevelParam& param);
// Section +0x18 script of a level: a name hash and its calls in order. A call is a native function or one of the
// script words "?" (if), "!?" (else), "?!" (else if), "??" (end if), "=" (assign), "=+" (arithmetic assign),
// "CallScript" and "SpawnScript" (executor 0x234204, docs/re/scripts.md). Arguments are the strings of the file.
struct ScriptCall {
    std::string function;
    std::vector<std::string> arguments;
};
struct LevelScript {
    std::uint32_t nameHash = 0;
    std::vector<ScriptCall> calls;
};
// Section +0x1c: event bindings (loader 0x1cfc50). A record of 0x18 bytes plus five string blocks: the object name
// (+8, empty for bound events), the script name (+0xc) and, from pool -16, the event name ("BoundEntered",
// "ObjectTriggered", ...; names of levels/letriggers.txt) and its two arguments (the bound name for bound events).
struct LevelTrigger {
    std::string objectName, scriptName, event, firstArgument, secondArgument;
    std::uint32_t nameHash = 0;  // record +0: nameHash of the trigger's name, looked up by EnableTrigger (0x1dd380)
};
// Section +0x34: room links (loader 0x1cfc50, consumer 0x173f94). A 0x10-byte record {room hash, world level hash,
// entry count, list pointer} plus one list block of 12-byte entries {room hash, world level hash, 1}. Hashes are
// nameHash of "RoomN" and of the world level instance name ("WorldLevel_01_CherryTreeHills"). Present in
// lvl_the_city (309 records), lvl_05_bluebellwood (54) and lvl_00_policestation (1).
struct RoomLinkEntry {
    std::uint32_t roomHash = 0, worldLevelHash = 0, flag = 0;
};
struct RoomLinks {
    std::uint32_t roomHash = 0, worldLevelHash = 0;
    std::vector<RoomLinkEntry> linked;
};
// Section +0x20: 0x28-byte records looked up by name hash (0x22a3b0). The path of the script functions starts at +8:
// +8 type (sampler table 0x358f9c; 0 is the Catmull-Rom spline 0x1c17c0), +9 closed, +0xa point count, then the points
// (block of 12-byte vectors). `room` is the optional block at +4; `unknownFloat` is the float at +0x20 (-1 in every record).
struct LevelPath {
    std::uint32_t nameHash = 0;
    std::uint8_t type = 0;
    bool closed = false;
    float unknownFloat = 0;
    std::string room;
    std::vector<std::array<float, 3>> points;
};
struct Level {
    std::vector<LevelPath> paths;
    std::vector<LevelTrigger> triggers;
    std::vector<RoomLinks> roomLinks;
    std::vector<LevelParam> defaults;  // section +0x10: class default parameters (char_*.blvl carry the appearance)
    std::vector<LevelParam> classParams;  // section +0x14, searched after +0x10 by lookup 0x217080
    std::vector<LevelScript> scripts;
    std::vector<LevelObject> objects;
    std::vector<LevelBound> bounds;
    // Section +0x28: 0x40-byte matrices; a bound with a node index is turned by the rows of its matrix (0x22f650, 0x229a40).
    std::vector<std::array<float, 16>> boundMatrices;
    std::vector<NavRoom> navRooms;
};
Level parseLevel(const Bytes& data);

// Objects with a "meshname" parameter, with "initialposition" and
// "initialorientation" (degrees; Euler order not recovered).
struct Placement {
    std::string instance;
    std::string room;
    std::string className;             // set by objectPlacements
    std::size_t objectIndex = 0;       // index in Level::objects, set by objectPlacements
    std::string mesh;
    float position[3] = {0, 0, 0};
    float orientation[3] = {0, 0, 0};
};
std::vector<Placement> placements(const Level& level);
// Class .blvl of an object (levels/<class name>.blvl), or nullptr when the archive has none.
using ClassLookup = std::function<const Level*(const std::string&)>;
// Parses the class files of the archive on demand and keeps them; an unknown class gives an empty Level. The lookup must not outlive `archive`.
ClassLookup archiveClassLookup(FuseArchive& archive);
// Objects that have a model, as the game resolves it (lookup 0x217080 in the model loader 0x2442b4):
// "meshname", "initialposition" and "initialorientation" exist only when the class file lists them (+0x10
// or +0x14). A listed parameter takes the instance value when the object sets it, else the class value.
// An empty meshname means no model.
std::vector<Placement> objectPlacements(const Level& level, const ClassLookup& classOf);
// Fog parameters of a level object (setup 0x1a293c): FogStart, FogEnd (floats, 0 when the class does not list them) and
// FogColour (float3 of 0..255, truncated to bytes as the game does). Only objects whose class lists FogColour are returned,
// in level order. Used by class lighting/lightambient (docs/re/rendering-fog.md).
struct FogObject {
    std::string instance, room;
    float start = 0, end = 0;
    std::array<std::uint8_t, 3> colour = {0, 0, 0};
};
std::vector<FogObject> fogObjects(const Level& level, const ClassLookup& classOf);
// Light objects (class setup 0x15dd50, docs/re/rendering-lighting.md): every object whose class lists LightType (a float the
// game converts to an integer: 0 ambient, 1 directional, 2 point, 4 specular). Colours stay 0..255 as stored; the defaults when a
// class lacks a parameter are those of 0x15dd50 (Intensity 1, MaxRange 10, SpecularColour = Colour).
struct LightObject {
    std::string instance, room;
    int type = 0;
    std::array<float, 3> colour = {0, 0, 0};
    std::array<float, 3> specular = {0, 0, 0};
    float intensity = 1, maxRange = 10;
    Placement placement;
};
std::vector<LightObject> lightObjects(const Level& level, const ClassLookup& classOf);
// BackgroundColour of the level's own parameter tables (read by 0x1bcf94 from the first scene's table, +0x10 then +0x14; bytes by
// float to int as the game does). Without the parameter the game uses black (0x1bd0a0).
std::array<std::uint8_t, 3> levelBackgroundColour(const Level& level);
// Appearance defaults of a character class (levels/chars/char_<name>.blvl). The textures named
// "*_TEST.tga" on the shared body/head models are the ones these names replace.
struct CharacterAppearance {
    std::string body, head, face, lower, upper;  // meshname, HeadModel, FaceTexture, LowerTexture, UpperTexture
    std::uint32_t characterEnum = 0;             // CharacterEnum; equals the row in tablescharacters.bxls sheet 0
};
CharacterAppearance characterAppearance(const Level& characterClass);
// A level object whose model is the shared minifig rig ("char_minifig"): the parameters the model loader
// 0x2442b4 reads (meshname, HeadModel, HatModel) and the texture parameters, each resolved like
// objectPlacements does (instance value when the class lists the parameter as a slot, else the class value).
struct CharacterObject {
    Placement placement;               // className, instance, room, mesh (body), position, orientation
    CharacterAppearance appearance;
    std::string hat;                   // HatModel, empty when the class does not list it
    std::string animationClass;        // lower-case folder under models/char_minifig/ ("class_generic" by default)
    std::string cutsceneName;          // CSCharacter: the cutscene cast folder this character plays (docs/re/cutscene-actors.md), empty when unset
};
// objectPlacements split by kind. Static geometry: models that exist in the archive. Characters: objects
// with the "char_minifig" body, except the custom characters. Left out and counted: the mission markers of
// class "props/map_*" (observed hidden at the start, see docs/re/objects.md), other skeletal "char_*"
// models (animals, they have their own rigs), objects whose model the archive does not hold (emitters,
// hearts), the studs (collectible coins: object type 0x3d with pickuptype 0..3, drawn as sprites, see
// docs/re/studs.md) and the custom characters (CharacterEnum 1..8, 0x2372e0 builds their look from the saved
// custom character, not from the object's parameters).
struct StudObject {
    std::uint32_t kind = 0;            // pickuptype: 0 silver, 1 gold, 2 blue, 3 purple
    float position[3] = {0, 0, 0};     // initialposition (the sprite centre is 0.56 above it, constructor 0x17f8e0)
    std::uint32_t group = 0;           // DisabledOnStartUp as the constructor reads it (low byte); nonzero: hidden until a script enables the group
    std::uint32_t groupOrder = 0;      // GroupOrder (low byte)
    std::string instance, room;
};
struct DrawableObjects {
    std::vector<Placement> objects;
    std::vector<CharacterObject> characters;
    std::vector<StudObject> studs;
    std::size_t missingModel = 0;
    std::size_t skeletal = 0;
    std::size_t customCharacters = 0;
    std::size_t markers = 0;
    std::vector<Placement> waypoints;  // props/map_* markers that have a model; drawn only while a script switched them on
};
DrawableObjects drawableObjects(FuseArchive& archive, const Level& level);
// The point object "Player1Start": loader 0x2e6264 finds it by name (0x23b350) and gives the
// player's scene node its transform (0x213284 builds the matrix, 0x24c118 stores it).
std::optional<Placement> playerStart(const Level& level);
// The placement (position and orientation parameters) of the level object with that instance name; names compare
// without case (the script natives resolve object arguments by name, 0x22dacc).
std::optional<Placement> objectStart(const Level& level, const std::string& instance);
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
    std::array<float, 4> orientation{0, 0, 0, 1};  // record +0x88 quaternion (x, y, z, w)
};
struct Skeleton {
    std::vector<Joint> joints;
};
Skeleton parseSkeleton(const Bytes& data);

// Keyframe of a named track: cubic Bezier with handles (time +/- delta, handle value).
// The deltas are 1/3 of the neighbouring key interval in 76% of shipped intervals.
struct CurveKey {
    float time = 0;                        // 0..1 over the animation
    float inDelta = 0, inValue = 0, value = 0, outDelta = 0, outValue = 0;
};
struct AnimTrack {
    std::uint32_t hash = 0;                // name hash; names unknown (e.g. 0x191a8df3 camera x)
    std::vector<CurveKey> keys;
};
// .bfnanm animation (loader 0x10edec); every block is consumed and checked.
struct Animation {
    unsigned frames = 0;                   // header u16 +4 & 0x3fff (0x114b58); bit 14 set in cutscenes
    std::vector<Bytes> jointCurves;        // indexed like the skeleton joints
    // Curve stored with frames + 1 samples (0x12a2b4 flag 0x10): table bit 29; the extra curve
    // (bit 28) if present, else curve 0. Inferred from data: all 30081 curves fit their blobs.
    int longCurve = -1;
    bool hasJointTable = false;            // header +8 present
    std::uint32_t jointTableWord = 0;      // first word of the joint table: bits 0..12 curves, 13..14 rate, 15..27 samples, 28 extra curve, 29 long curve
    unsigned headerRateIndex = 0;          // header u16 +4 >> 14, the rate index when there is no joint table
    std::vector<std::string> objects;      // section +0x0c names, e.g. "ExportCam"
    std::vector<std::array<Bytes, 2>> objectBlocks;  // the two data blocks that follow each name of section +0x0c
    std::vector<AnimTrack> tracks;         // section +0x18 (unused in shipped data)
    std::vector<std::vector<AnimTrack>> subAnimations;
};
// Value of a track at normalised time 0..1 (Bezier solved for time by bisection).
float evaluateTrack(const AnimTrack& track, float time);
Animation parseAnimation(const Bytes& data);

// Playback rate in frames per second (0x108190): index (jointTableWord >> 13) & 3, or the header index without a
// joint table, into the table {15, 30, 60, 0} at 0x376764.
float animationFramesPerSecond(const Animation& animation);
// Loop bit of the animation player (0x107990): joint table bit 29 (the curve stored with frames + 1 samples).
bool animationLoops(const Animation& animation);
// Length in frames the animation player uses (0x1081ec): frames, minus 1 unless animationLoops; 16 bits.
std::uint16_t animationLength(const Animation& animation);
// Time from the first to the last frame: (frames - 1) / rate.
float animationDuration(const Animation& animation);

// Translation of the extra joint curve (joint table bit 28, the last curve) relative to frame 0 for every frame,
// with the componentwise extent that 0x292ef8 accumulates over frames 1..n (minimum and maximum start at 0).
// The jump code (0x200e70) scales the Y component with height / (max.y - min.y).
struct RootMotion {
    std::vector<std::array<float, 3>> translation;
    std::array<float, 3> minimum{0, 0, 0};
    std::array<float, 3> maximum{0, 0, 0};
};
std::optional<RootMotion> rootMotion(const Animation& animation);

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
// 0x12a2b4 after sampling: rotation = sampled * joint.orientation (0x2485d8 -> 0x247f3c),
// skipped when |w| >= 1; the rotation mask bits are then set.
JointPose orientedPose(const JointChannels& channels, const Joint& joint);
// 0x252cb0 with several layers: quaternions summed with a sign flip against the first
// (0x24c924) and normalised (0x24c898); scale summed over all layers, shear and
// translation over the layers that carry them. A single layer is returned unchanged.
JointPose blendPoses(const std::vector<std::pair<JointPose, float>>& layers);
// 0x24c734 with 0x248870: local matrix from one pose.
Matrix poseMatrix(const JointPose& pose);
Matrix localMatrix(const JointChannels& channels);
// Matrix of a named object track (section +0x0c, e.g. "Bit1" of a build-it's _buildit animation) at a frame: the first block of the
// object decoded with the curve decoder 0x248910. Frame 0 is the pose the original reads as the start (0x21e96c), the last frame the end.
Matrix objectAnimationMatrix(const Animation& animation, std::size_t object, unsigned frame);
// Rigid-inverse(bind world) * animated world per joint (0x252cb0). A fractional frame
// blends floor(frame) and the next frame, wrapping, as 0x24bf80 does for looping playback.
std::vector<Matrix> skinMatrices(const Skeleton& skeleton, const Animation& animation, double frame);
// Matrix for a model attached to a joint by name (HeadModel -> "head", HatModel -> "hat",
// 0x2121e8 called from 0x2442b4): the model's vertices sit at the joint's bind position with the
// bind orientation dropped, as the baked head of csdata minifigs does, and follow the joint's skin
// matrix. Using the joint's bind world instead would carry its bind orientation (a 90 degree twist
// for "head") into the head.
Matrix jointAttachment(const Joint& joint, const Matrix& skin);
// Weighted blend of the skin matrices of the vertex's resolved joints.
void skinVertex(const Vertex& vertex, const std::vector<Matrix>& skin, float position[3], float normal[3]);

// .bfnnav navigation grids in the block order of loader 0x1971c4.
struct NavGrid {
    std::uint16_t x = 0, z = 0, height = 0, width = 0, depth = 0;
    std::uint16_t heightExtent = 0;       // record +8
    std::uint16_t flags = 0;              // record +0x34: bits 0..13 count of extras, bit 14 and 15 grid switches
    std::vector<std::uint16_t> links;     // neighbouring grid indices
    std::array<std::uint16_t, 16> linkSlots{};  // record +0x14, 0xffff = none; the slot number is what extras name
    std::vector<std::array<std::uint8_t, 4>> extras;  // extra block, 4 bytes per entry
    std::vector<std::uint8_t> cells;      // width * depth, row-major by z; bit 0 set on edges/obstacles
};
struct NavPath {
    float position[3] = {0, 0, 0};
    float extent[3] = {0, 0, 0};
    std::vector<std::uint16_t> indices;
};
struct NavMesh {
    std::vector<NavGrid> grids;
    std::vector<NavPath> paths;
};
NavMesh parseNav(const Bytes& data);

// .loc tables ("LOCA" v2): key -> Latin-1 text. Keys are the sdbm hash of the upper-case
// identifier (textKey("PAUSEMENU")).
std::uint32_t textKey(const std::string& name);
std::map<std::uint32_t, std::string> parseLocalisation(const Bytes& data);
// localisation/extended_chars_utf8.txt split into UTF-8 characters.
std::vector<std::string> extendedCharacters(const Bytes& utf8);
// Raw .loc text to UTF-8: Latin-1 bytes, and 0x01 '2' c -> extended[c - 0x30].
std::string decodeText(const std::string& raw, const std::vector<std::string>& extended);

// .tnf bitmap font: one (width, height, x, y) atlas rectangle per code first..end-1.
// Codes 0x100 and up are the extended characters in extended_chars_utf8.txt order.
struct Glyph {
    std::uint16_t width = 0, height = 0, x = 0, y = 0;
};
struct Font {
    std::uint16_t first = 0;
    std::uint8_t lineHeight = 0;
    std::vector<Glyph> glyphs;
    const Glyph* find(std::uint32_t code) const {
        return code >= first && code - first < glyphs.size() ? &glyphs[code - first] : nullptr;
    }
};
Font parseFont(const Bytes& data);
struct PlacedGlyph {
    Glyph glyph;
    int x = 0;
};
// Glyphs for UTF-8 text laid out left to right with one pixel between them; characters
// without a glyph are skipped.
std::vector<PlacedGlyph> layoutText(const Font& font, const std::string& utf8,
                                    const std::vector<std::string>& extended);

// csdata/models/<id>/manifest.bstxt: one pooled text block, one archive path per line
// relative to the folder (e.g. "player1/player1_minifig.fnmdl").
std::vector<std::string> parseManifest(const Bytes& data);
// Manifest lines grouped by their first folder into model/skeleton/animation paths
// (".fnmdl" -> ".bfnmdl" etc., lower case, prefixed with `folder`); animations in manifest order.
struct CutsceneActor {
    std::string name, model, skeleton;
    std::vector<std::string> animations;
};
std::vector<CutsceneActor> cutsceneActors(const std::vector<std::string>& manifest, const std::string& folder);
// Camera tracks of a cutscene "ExportCam" animation, as handler 0x1c1fbc stores them.
// Defaults are the handler's; fields 7-11 are kept but their meaning is unknown.
struct CutsceneCamera {
    float position[3] = {0, 0, 0};
    float rotation[3] = {0, 0, 0};         // radians, see cameraMatrix
    float fieldOfView = 35.0f;             // default 35; degrees, axis not recovered
    float field7 = 1.41732f, field8 = 0.945f, field9 = 0, field10 = 8.0f, field11 = 0.035f;
};
CutsceneCamera cutsceneCamera(const Animation& animation, float time);
// Actor transform tracks with the signs of the accumulating branch of event 0xb945fcd8 in
// 0x287ca4: position (0xdcd633e0..2) with x negated, yaw (0x4f44e168, radians) negated,
// scale (0x7e190104..6). The game adds weighted values to a base; the base is not recovered.
struct ActorPose {
    float position[3] = {0, 0, 0};
    float yaw = 0;
    float scale[3] = {1, 1, 1};
};
ActorPose actorPose(const Animation& animation, float time);
// Yaw about +y (same sense as placementMatrix), scale, then translation; the yaw sense is
// a visual choice, not traced.
Matrix actorMatrix(const ActorPose& pose);
// 0x20e850: camera world matrix (row vectors) from rotation and position; x is negated.
Matrix cameraMatrix(const CutsceneCamera& camera);

// The camera frame at which an actor's clip starts (0x2ec30c): the file name matches "<actor>_????.*" and the four
// characters after "<actor>_" are the number. Throws when the name does not match.
unsigned cutsceneClipStart(const std::string& actor, const std::string& animationPath);
struct CutsceneClip {
    unsigned startFrame = 0;
    Animation animation;
};
// Everything one cutscene folder (csdata/models/<id>) holds: the cast with model, skeleton and clips, the camera
// animation and the soundtrack path (music/csaudio/<id>.bwav, empty when the archive has none).
struct CutsceneCast {
    std::string model;
    Skeleton skeleton;
    std::vector<CutsceneClip> clips;
};
struct CutsceneContent {
    std::vector<CutsceneCast> actors;
    Animation camera;
    std::string soundPath;
};
// Throws when the folder, a listed model or skeleton or the camera animation is missing. A listed clip missing from the
// archive is skipped, as 0x2ec6c8 leaves its entry empty; actors without animation are skipped.
CutsceneContent loadCutscene(FuseArchive& archive, const std::string& id);
// Frame position in 0..1 of an animation whose playback is held at `frame` (the last frame holds).
double cutsceneNormalised(const Animation& animation, double frame);
// The clip that drives an actor at a camera frame and the frame inside it, held at its last frame (0x2ec454: a clip starts
// in the update whose frame passes its start, the clip started last drives the actor); none before the first start.
struct CutsceneClipFrame {
    const Animation* animation = nullptr;
    double frame = 0;
};
std::optional<CutsceneClipFrame> cutsceneClipAt(const CutsceneCast& actor, double cameraFrame);

// Data tables (tables/*.bxls, loader 0x10e1cc): sheets of fixed-stride rows with typed
// columns. Column names are not stored; the game addresses columns by index.
// Types: 0 not stored, 1 int32, 2 16-bit, 3 8-bit, 4 float, 5/6 string-pool offset,
// 7 32-bit (meaning unknown); 8/9/10 are 8/16/32-bit and stored only when listed. Listed
// type-8 columns end runs of eight type-8 columns and look like bit masks (unconfirmed).
struct TableColumn {
    std::uint8_t type = 0;
    bool stored = false;
    std::size_t offset = 0, size = 0;
};
struct TableSheet {
    unsigned rows = 0;
    std::size_t stride = 0;
    std::vector<TableColumn> columns;
    Bytes rowData, strings;
};
struct Table {
    std::vector<TableSheet> sheets;
};
struct TableValue {
    std::uint8_t type = 0;
    bool present = false;          // false for type 0 and unlisted optional columns
    std::uint32_t bits = 0;        // raw little-endian field, zero-extended
    std::string text;              // types 5 and 6
    float asFloat() const;
};
Table parseTable(const Bytes& data);
TableValue tableValue(const TableSheet& sheet, std::size_t row, std::size_t column);

// tables/tablessoundfx.bxls sheet 0: the row index is the sound id scripts pass to the play
// routine 0x23fa54. volumePercent is the last-but-two integer, which equals record byte +9
// of that routine; paramA and paramB equal record bytes +10 and +11 (meaning unconfirmed).
// The remaining stored columns keep their index in the name; their meaning is unknown.
struct SoundEffect {
    std::uint32_t id = 0;
    std::string name;                // "ui/brickcounter1"; empty for id 0
    std::uint32_t column2 = 0;       // 16-bit
    std::uint32_t column3 = 0;       // 8-bit
    std::uint32_t column11 = 0, column12 = 0;
    std::uint32_t volumePercent = 0; // column 13
    std::uint32_t paramA = 0;        // column 14
    std::uint32_t paramB = 0;        // column 15
    float column16 = 0;
};
std::vector<SoundEffect> parseSoundEffects(const Table& table);
// nullptr when `id` is outside the table.
const SoundEffect* findSoundEffect(const std::vector<SoundEffect>& effects, std::uint32_t id);
// Archive path of the effect's sound ("sounds/<name>.bwav"); empty for an unnamed effect.
std::string soundEffectPath(const SoundEffect& effect);

// tables/tablessoundfx.bxls sheets 1..3 (loaded by the game into the arrays at 0x376b40, 0x375d24 and 0x375f7c).
// Sheet 1 column 1: music track names (the row is the track id of OverrideMusic, 0x240ec8). Sheet 2: per music set the
// ambient and the action track (columns 1 and 2; 0 = use the other mood, 255 = no music, 0x206c40). Sheet 3: per
// character sound category the 34 sound effect ids of the character sound slots (columns 1..34, read by 0x19efbc,
// 0x1abc5c and 0x200e70 as u16 at category * 0x44 + slot * 2).
constexpr std::size_t kCharacterSoundSlots = 34;
struct MusicSet {
    std::uint8_t ambient = 0;
    std::uint8_t action = 0;
};
struct SoundTables {
    std::vector<std::string> musicNames;
    std::vector<MusicSet> musicSets;
    std::vector<std::array<std::uint16_t, kCharacterSoundSlots>> characterSounds;
};
SoundTables parseSoundTables(const Table& soundfx);
// tables/tableslevels.bxls sheet 0, column 24: index into the music sets (byte +0x39 of the 84-byte level record that
// 0x2f6c24 and 0x2e6524 read).
std::uint8_t levelMusicSet(const Table& levels, std::size_t row);
// tables/tablescharacters.bxls sheet 0, column 95: the character sound category. The row stride is 40 and the column
// sits at byte +0x21, the byte that 0x200e70 and 0x1abc5c read from the character table at 0x372e10. Row "police" is
// category 2 (the oracle landing id 410 is slot 7 of category 2).
std::uint8_t characterSoundCategory(const Table& characters, std::size_t row);
struct ConfigEntry {
    std::string key;
    std::string value;
    unsigned line = 0;
};
struct ConfigBlock {
    std::string name;
    unsigned line = 0;
    std::vector<ConfigEntry> entries;
    std::vector<ConfigBlock> blocks;
    const ConfigEntry* find(const std::string& key) const;
    const std::string& value(const std::string& key) const;
    double number(const std::string& key) const;
    std::vector<double> numbers(const std::string& key) const;
};
std::vector<ConfigBlock> parseConfigText(const std::string& text);

// pregen/collectables_level<N>.bin (N = 1..7), loader 0x23b498: a version byte 2 and 35 little-endian 16-bit values
// copied into a 70-byte record whose defaults are 0 (groups A and B) and -1 (group C). Zero in group A or B and -1 in
// group C is an empty slot; the counting reader 0x239184 totals and collects per group.
struct CollectableLists {
    std::array<std::int16_t, 20> groupA{};
    std::array<std::int16_t, 10> groupB{};
    std::array<std::int16_t, 5> groupC{};
};
CollectableLists parseCollectableLists(const std::vector<std::uint8_t>& data);

// pregen/levelobjectscount.bin: 16-bit little-endian values; the only reader (0x2e6164) reads the first 56 bytes into a
// local buffer that is never used afterwards.
std::vector<std::uint16_t> parseLevelObjectCounts(const std::vector<std::uint8_t>& data);

// Loader 0x3062e4 reads Name, Type, Location, Offset, FOV, Filters, BlendTime, Shake*, 3DOffset into a
// 0x50-byte camera record; RotationY, RotationX, Distance and Lookat have no handler (editor data).
struct CameraSetup {
    std::string kind;
    std::string name;
    int type = 0;
    double rotationY = 0, rotationX = 0, distance = 0;  // 1/256 turn; Type 0 Location derives from these
    std::array<double, 3> location{}, lookat{}, offset{};
    double fov = 0;
    std::array<double, 2> filters{};
    double blendTime = 0, shakeAmp = 0, shakeRate = 0, shakeRoll = 0, offset3d = 0;
};
struct CameraFollow {
    std::vector<CameraSetup> setups;
};
CameraFollow parseCameraFollow(const std::string& text);
// .bxaml UI layouts, parsed in the block order of loader 0x217220 (elements 0x29f7b4).
// Element record field use follows the node builder 0x1b2ec0; storyboards follow 0x1dbb80.
enum class UiElementType : std::uint32_t { Container = 1, Image = 2, Reference = 3 };
struct UiElement {
    UiElementType type = UiElementType::Container;
    std::string name;
    std::string source;          // Image: texture path; Reference: name of another .bxaml layout
    std::uint8_t unknown14 = 0;  // record +0x14, copied to node +0x147
    float opacity = 1;           // record +0x18
    float rotation = 0;          // record +0x1c, radians
    float scale[2] = {1, 1};     // record +0x20
    float position[2] = {0, 0};  // record +0x28
    float origin[2] = {0, 0};    // record +0x30
    float size[2] = {0, 0};      // record +0x38; 0 on an image means the texture size
    std::uint16_t flags = 0;     // record +0x42; bit 0x80 marks the child the parent keeps at node +0x90
    std::int16_t callback = 0;   // record +0x44; non-zero calls the hook at 0x37669c
    std::uint8_t fontSize = 0;   // record +0x46; picks one of three fonts in table 0x52ebf8
    std::uint8_t alignment = 0;  // record +0x47 & 3
    std::uint8_t colour[4] = {255, 255, 255, 255}; // record +0x48, bytes in file order
    std::vector<UiElement> children;
};
struct UiKeyframe {
    float value = 0;
    float spline[4] = {0, 0, 1, 1}; // two control points handed to 0x249030
    float time = 0;
    std::uint8_t flag = 0;           // keyframe +0x1c
};
struct UiTrack {
    std::string target;              // element name, resolved by 0x24085c
    std::uint8_t property = 0;       // 1..6; meanings are data fits, see the progress log
    std::int16_t repeat = 0;         // storyboard length uses duration * repeat (0: duration once)
    float duration = 0;
    std::uint32_t unknown1c = 0;
    std::vector<UiKeyframe> keyframes;
};
struct UiStoryboard {
    std::string name;
    std::vector<UiTrack> tracks;
};
struct UiLayout {
    UiElement root;
    std::vector<UiStoryboard> storyboards;
};
UiLayout parseLayout(const Bytes& data);
// SubRip cutscene subtitles (cutscenes/<video>_<lang>.srt). Text is decoded with decodeText.
struct SubtitleCue {
    unsigned index = 0;
    std::uint32_t startMs = 0;
    std::uint32_t endMs = 0;
    std::vector<std::string> lines;
};
std::vector<SubtitleCue> parseSubtitles(const Bytes& data, const std::vector<std::string>& extended = {});
// STAND-IN (FE-S41): the wrap width of a subtitle is 7/8 of the screen width, 280 on the 320 px bottom screen. The oracle
// breaks four intro cues at port widths 272 (fits) and 284 (breaks), so the limit lies in 272..283; the movie player's
// clip (0x2e1760, 0.05..0.95 of the width, 288) does not explain it.
constexpr int subtitleWrapWidthStandIn(int screenWidth) { return screenWidth * 7 / 8; }
// The line breaker 0x21bc2c breaks at the last space, or before the character that crosses the width when the line has
// no space; explicit line breaks stay.
std::vector<std::string> wrapSubtitleLines(const std::vector<std::string>& lines, int width,
                                           const std::function<int(const std::string&)>& measure);

// 0x1886f0: PJW/ELF hash of the ASCII-lower-cased name (bytes are signed).
std::uint32_t pjwHash(const std::string& text);

// particles/particledefs.bprt, loader 0x27d71c. Textures load as Particles/<name>.tga
// (0x271e6c). Spawners address definitions by index (e.g. ExplosionParticleDef, 0x163f00).
// Records are 0x17c bytes (0x23de24). Field use from spawner 0x216244 and particle init 0x222550.
constexpr std::size_t kParticleDefSize = 380;
// Four key lists of (f32 time, 4 value bytes), times in 0..1 of a lifetime, at most 7 keys each.
constexpr std::size_t kMaxParticleKeys = 7;
struct ParticleKey {
    float time = 0;
    std::array<std::uint8_t, 4> value{};
};
struct ParticleDef {
    std::uint32_t textureHash = 0;  // +0x2c, pjwHash of a texture name
    int texture = -1;               // index into ParticleDefs::textures, -1 if no name matches (the game uses 0)
    std::uint32_t flags = 0;        // +0x168; bit 2 random gradient colour, bit 3 planar (z = 0)
    std::uint32_t seed = 0;         // +0x84; 0 = random
    std::uint8_t particles = 0;     // +0x175, clamped below ParticleDefs::maxParticles
    std::uint8_t atlasColumns = 0;  // +0x178; the spawner refuses 0
    std::uint8_t atlasRows = 0;     // +0x179
    std::uint8_t frameMode = 0;     // +0x176: 0 whole texture, 1 random cell, 2 animated, 3 next cell
    std::uint8_t animationLoops = 0;  // +0x17b, frame mode 2: atlas cycles per particle lifetime
    std::uint8_t flipModes = 0;     // +0x17a: low nibble U, high nibble V; 1 random, 2 alternate, 3 always
    float rate = 0;                 // +0x28, particles per second
    float duration = 0;             // +0x38, seconds; above 98 the emitter runs until stopped
    float lifetimeMin = 0;          // +0x7c, seconds
    float lifetimeMax = 0;          // +0x80
    float sizeX = 0, sizeY = 0;     // +0x0c, +0x10, start size before random growth
    float growX = 0, growY = 0;     // +0x1c, +0x20, size change per second scaled by sizeKeys
    float gravity = 0;              // +0x3c, added to velocity y per second
    float drag = 0;                 // +0x24, velocity *= 1 - drag * dt
    float bounce = 0;               // +0x74, > 0: ground bounce restitution
    std::vector<ParticleKey> fadeKeys;    // +0x88, count +0x170: value[0] emitter alpha over its duration
    std::vector<ParticleKey> sizeKeys;    // +0xc0, count +0x171: (value[0] - 128) / 128 growth factor
    std::vector<ParticleKey> colourKeys;  // +0xf8, count +0x172: value[0..2] colour over particle life
    std::vector<ParticleKey> alphaKeys;   // +0x130, count +0x173: value[0] alpha over particle life
    std::array<std::uint8_t, kParticleDefSize> raw{};
};
struct ParticleDefs {
    std::uint32_t maxEmitters = 0;   // global 0x376b84, live emitter limit
    std::uint32_t maxParticles = 0;  // global 0x376b88, particles per emitter limit
    std::uint32_t unknown4 = 0;  // global 0x376bbc
    float scale = 1.0f;          // global 0x376bc0; the loader turns 0 into 1
    std::vector<std::string> textures;
    std::vector<ParticleDef> defs;
};
ParticleDefs parseParticleDefs(const Bytes& data);

std::uint32_t readU32(const Bytes& data, std::size_t offset);
std::uint16_t readU16(const Bytes& data, std::size_t offset);
} // namespace chase::assets
