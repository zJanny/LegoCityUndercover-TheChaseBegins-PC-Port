#include "assets/object_collision.hpp"
#include "assets/object_classes.hpp"
#include "block_reader.hpp"
#include <algorithm>
#include <cctype>
#include <cstring>
#include <functional>
#include <iterator>
#include <map>

namespace chase::assets {
namespace {
constexpr float kMinimumHalf = 0.05f;  // 0x3d4ccccd clamp of the automatic box in 0x23aff8
constexpr std::int32_t kNoBoundsHitMode = 6;  // `Destructable` 6: 0x16aeb4 clears 0x200 like `NoBoundCollisions`
constexpr float kDoorMargin = 0x1.eb851ep-3f;  // *0x37713c / *0x377140 taken off the x and z half extents when 0x23aff8 gets param_6

const LevelParam* findParam(const std::vector<LevelParam>& params, std::uint32_t hash) {
    for (const auto& param : params)
        if (param.hash == hash) return &param;
    return nullptr;
}

const LevelParam* effectiveParam(const LevelObject& object, const Level& cls, std::uint32_t hash) {
    const LevelParam* classParam = findParam(cls.defaults, hash);
    if (!classParam) classParam = findParam(cls.classParams, hash);
    if (!classParam) return nullptr;
    if (classParam->flags & 2)
        if (const LevelParam* instance = findParam(object.params, hash)) return instance;
    return classParam;
}

std::int32_t intOf(const LevelParam* param) {
    if (!param) return 0;
    if (param->type != 0) return std::int32_t(param->value);
    float value = 0;
    std::memcpy(&value, &param->value, 4);
    return std::int32_t(value);
}

bool float3Of(const LevelParam* param, float* out) {
    if (!param || param->type != 2 || param->blocks.empty() || param->blocks[0].size() < 12) return false;
    std::memcpy(out, param->blocks[0].data(), 12);
    return true;
}

std::string lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return text;
}

struct ModelData {
    bool present = false;
    std::shared_ptr<const CollisionItem> firstItem;
    std::vector<CollisionBox> boxes;
    bool hasBounds = false;
    float boundsCentre[3] = {0, 0, 0};
    float boundsHalf[3] = {0, 0, 0};
};

void transformPoint(const float matrix[16], const float in[3], float out[3]) {
    for (int a = 0; a < 3; ++a)
        out[a] = in[0] * matrix[a] + in[1] * matrix[4 + a] + in[2] * matrix[8 + a] + matrix[12 + a];
}

// 0x242eb0: union of the eight corners of each mesh bounding box (centre +/- half), moved by the matrix of its node (model+0x20,
// used when its last element is not 0), over the nodes whose name does not start with "NoBounds"; a mesh counts once.
ModelData loadModel(FuseArchive& archive, const std::string& path) {
    ModelData data;
    const FuseEntry* entry = archive.find(path);
    if (!entry) return data;
    const Model model = parseModel(archive.read(*entry));
    data.present = true;
    data.boxes = model.collisionBoxes;
    if (!model.collision.empty()) data.firstItem = std::make_shared<CollisionItem>(model.collision.front());
    float low[3] = {3.4028235e+38f, 3.4028235e+38f, 3.4028235e+38f};
    float high[3] = {-3.4028235e+38f, -3.4028235e+38f, -3.4028235e+38f};
    std::vector<bool> counted(model.meshes.size(), false);
    for (std::size_t n = 0; n < model.nodeMesh.size(); ++n) {
        const int index = model.nodeMesh[n];
        if (index < 0 || std::size_t(index) >= model.meshes.size() || counted[std::size_t(index)]) continue;
        if (model.nodeNames[n].compare(0, 8, "NoBounds") == 0) continue;
        const ModelMesh& mesh = model.meshes[std::size_t(index)];
        const bool moved = n < model.nodeMatrices.size() && model.nodeMatrices[n][15] != 0.0f;
        for (int corner = 0; corner < 8; ++corner) {
            float point[3], world[3];
            for (int a = 0; a < 3; ++a) point[a] = (corner >> a & 1) ? mesh.centre[a] + mesh.half[a] : mesh.centre[a] - mesh.half[a];
            if (moved) transformPoint(model.nodeMatrices[n].data(), point, world);
            else std::copy(point, point + 3, world);
            for (int a = 0; a < 3; ++a) {
                low[a] = std::min(low[a], world[a]);
                high[a] = std::max(high[a], world[a]);
            }
        }
        counted[std::size_t(index)] = true;
    }
    if (low[0] <= high[0]) {
        data.hasBounds = true;
        for (int a = 0; a < 3; ++a) {
            data.boundsCentre[a] = (high[a] + low[a]) * 0.5f;
            data.boundsHalf[a] = (high[a] - low[a]) * 0.5f;
        }
    }
    return data;
}

void clampAutomaticBox(float centre[3], float half[3], bool margin) {
    if (margin) {
        half[0] -= kDoorMargin;
        half[2] -= kDoorMargin;
    }
    half[0] = std::max(half[0], kMinimumHalf);
    half[2] = std::max(half[2], kMinimumHalf);
    if (half[1] < kMinimumHalf) {
        centre[1] -= (half[1] - kMinimumHalf) * 0.5f;
        half[1] = kMinimumHalf;
    }
}

}

bool modelBounds(FuseArchive& archive, const std::string& modelPath, float centre[3], float half[3]) {
    const ModelData data = loadModel(archive, modelPath);
    if (!data.hasBounds) return false;
    std::copy(data.boundsCentre, data.boundsCentre + 3, centre);
    std::copy(data.boundsHalf, data.boundsHalf + 3, half);
    return true;
}

bool modelCollisionBox(FuseArchive& archive, const std::string& modelPath, float centre[3], float half[3]) {
    const ModelData data = loadModel(archive, modelPath);
    if (!data.firstItem || data.firstItem->nodes.empty()) return false;
    const CollisionNode& root = data.firstItem->nodes.front();
    std::copy(root.centre, root.centre + 3, centre);
    std::copy(root.half, root.half + 3, half);
    return true;
}

bool PlacedObjectState::isDisabled(const std::string& instance) const {
    const std::string key = lower(instance);
    return disabled.count(key) != 0 || destroyed.count(key) != 0;
}

bool PlacedObjectState::isHidden(const std::string& instance) const { return disabled.count(lower(instance)) != 0; }

bool PlacedObjectState::collisionEnabled(const std::string& instance, bool initial) const {
    const auto found = collisionOverride.find(lower(instance));
    return found == collisionOverride.end() ? initial : found->second;
}

namespace {
constexpr std::uint8_t kNeverCreated = 1;      // createFlags bit 0 (0x1a4130)
constexpr std::uint8_t kDefaultSceneOnly = 2;  // createFlags bit 1: created only in the scene at 0x589a1c, not in the game scene
constexpr int kMaxNesting = 8;  // STAND-IN COLL-S7: a guard of the port's recursion, the original's nesting bound was not read
constexpr std::uint8_t kPlayerObjectKind = 65;  // class "players": its children are the hand items, which hang on the bones and are hidden (docs/re/hand-items.md)

bool hasStartupScript(const Level& classFile) {
    const std::uint32_t hash = nameHash("startup");
    return std::any_of(classFile.scripts.begin(), classFile.scripts.end(), [hash](const LevelScript& script) { return script.nameHash == hash; });
}

enum class Scope { Placed, Nested };

std::vector<ObjectCollision> collectObjectCollisions(FuseArchive& archive, const Level& level, const PlacedObjectState& state, bool keepInactive, Scope scope) {
    const ClassLookup classLookup = archiveClassLookup(archive);
    const auto classOf = [&](const std::string& name) -> const Level& { return *classLookup(name); };
    std::map<std::string, ModelData> models;
    const auto modelAt = [&](const std::string& path) -> const ModelData& {
        auto found = models.find(path);
        if (found == models.end()) found = models.emplace(path, loadModel(archive, path)).first;
        return found->second;
    };
    const std::uint32_t hashMesh = nameHash("meshname"), hashPosition = nameHash("initialposition"),
                        hashOrientation = nameHash("initialorientation"), hashMeshCollision = nameHash("MeshCollision"),
                        hashBoundCollision = nameHash("BoundCollision"), hashCollisionBound = nameHash("CollisionBound"),
                        hashHasCollisionMesh = nameHash("HasCollisionMesh"), hashDestructable = nameHash("Destructable"),
                        hashNoBoundCollisions = nameHash("NoBoundCollisions");

    std::vector<ObjectCollision> out;
    const auto consider = [&](const LevelObject& object, const std::string& instance, const Matrix* parent) {
        const bool prop = isPropClass(object.className);
        const bool door = isDoorClass(object.className);
        const bool dummyCollision = isDummyCollisionClass(object.className);
        const Level& cls = classOf(object.className);
        const bool vehicle = isVehicleClassFile(cls);
        const std::uint8_t kind = objectKind(object, cls);
        const bool platformKind = kind == kPlatformObjectKind;
        const bool propKind = kind == kPropObjectKind || platformKind;
        const bool superBuildit = kind == kSuperBuilditObjectKind;
        const bool propFamily = isPropFamilyClass(object.className) || vehicle || propKind || superBuildit;
        if (!prop && !dummyCollision && !vehicle && !propKind) return;
        if (!keepInactive && state.isDisabled(instance)) return;

        Placement placement;
        if (!float3Of(effectiveParam(object, cls, hashPosition), placement.position) && !parent) return;
        float3Of(effectiveParam(object, cls, hashOrientation), placement.orientation);

        bool box = false;
        float centre[3] = {0, 0, 0}, half[3] = {0, 0, 0};
        std::shared_ptr<const CollisionItem> mesh;
        std::string modelPath;

        if (dummyCollision) {
            if (!float3Of(findParam(object.params, hashCollisionBound), half)) return;
            box = true;
        }
        const auto bound = std::find_if(cls.bounds.begin(), cls.bounds.end(), [](const LevelBound& b) { return lower(b.name) == "collisionbound"; });
        if (!dummyCollision && bound != cls.bounds.end()) {
            std::memcpy(centre, bound->centre, sizeof centre);
            std::memcpy(half, bound->half, sizeof half);
            box = true;
        }

        if (propFamily) {
            std::string meshName;
            if (const LevelParam* param = effectiveParam(object, cls, hashMesh))
                if (param->type == 1 && !param->blocks.empty()) meshName = detail::cString(param->blocks[0]);
            if (!meshName.empty()) modelPath = meshModelPath(meshName);
            const ModelData* model = meshName.empty() ? nullptr : &modelAt(modelPath);
            if (model && model->present) {
                const auto takeModelCollision = [&] {
                    if (model->firstItem && !model->firstItem->nodes.empty()) {
                        mesh = model->firstItem;
                        std::memcpy(centre, mesh->nodes.front().centre, sizeof centre);
                        std::memcpy(half, mesh->nodes.front().half, sizeof half);
                    } else if (!model->boxes.empty()) {
                        const CollisionBox& record = model->boxes.front();
                        transformPoint(record.matrix, record.position, centre);
                        std::memcpy(half, record.half, sizeof half);
                    }
                };
                if (!box && model->hasBounds) {
                    std::memcpy(centre, model->boundsCentre, sizeof centre);
                    std::memcpy(half, model->boundsHalf, sizeof half);
                    clampAutomaticBox(centre, half, door);
                    box = true;
                }
                const LevelParam* meshCollision = effectiveParam(object, cls, hashMeshCollision);
                if (meshCollision && meshCollision->value != 0) {
                    if (model->firstItem) mesh = model->firstItem;
                    else if (!model->boxes.empty()) {
                        const CollisionBox& record = model->boxes.front();
                        transformPoint(record.matrix, record.position, centre);
                        std::memcpy(half, record.half, sizeof half);
                    }
                }
                if (superBuildit) takeModelCollision();
                else if (propKind && !mesh && intOf(effectiveParam(object, cls, hashHasCollisionMesh)) != 0) takeModelCollision();
            }
        }
        const LevelParam* boundCollision = effectiveParam(object, cls, hashBoundCollision);
        bool collisionDefault = !boundCollision || boundCollision->value != 0;
        if (propKind) {
            if ((!platformKind && intOf(effectiveParam(object, cls, hashDestructable)) == kNoBoundsHitMode) ||
                intOf(effectiveParam(object, cls, hashNoBoundCollisions)) != 0)
                collisionDefault = false;
        }
        if ((propKind || superBuildit) && !collisionDefault && !mesh) return;
        if (!keepInactive && !state.collisionEnabled(instance, collisionDefault)) return;
        if (!box && !mesh) return;

        ObjectCollision entry;
        entry.instance = instance;
        entry.className = object.className;
        entry.collisionDefault = collisionDefault;
        entry.model = modelPath;
        entry.matrix = parent ? multiply(placementMatrix(placement), *parent) : placementMatrix(placement);
        entry.mesh = mesh;
        std::memcpy(entry.centre, centre, sizeof centre);
        std::memcpy(entry.half, half, sizeof half);
        out.push_back(std::move(entry));
    };

    std::function<void(const LevelObject&, const std::string&, const Matrix&, int)> descend = [&](const LevelObject& parent, const std::string& name,
                                                                                               const Matrix& parentMatrix, int depth) {
        const Level& parentClass = classOf(parent.className);
        if (hasStartupScript(parentClass) || objectKind(parent, parentClass) == kPlayerObjectKind) return;
        for (const LevelObject& child : parentClass.objects) {
            if ((child.createFlags & kNeverCreated) || (child.createFlags & kDefaultSceneOnly)) continue;
            const std::string childName = name + "." + child.instance;
            const Level& childClass = classOf(child.className);
            Placement local;
            float3Of(effectiveParam(child, childClass, hashPosition), local.position);
            float3Of(effectiveParam(child, childClass, hashOrientation), local.orientation);
            const Matrix matrix = multiply(placementMatrix(local), parentMatrix);
            consider(child, childName, &matrix);
            if (depth < kMaxNesting) descend(child, childName, matrix, depth + 1);
        }
    };

    for (const LevelObject& object : level.objects) {
        if (scope == Scope::Placed) {
            consider(object, object.instance, nullptr);
            continue;
        }
        if (object.createFlags & kNeverCreated) continue;
        Placement placed;
        const Level& cls = classOf(object.className);
        if (!float3Of(effectiveParam(object, cls, hashPosition), placed.position)) continue;
        float3Of(effectiveParam(object, cls, hashOrientation), placed.orientation);
        descend(object, object.instance, placementMatrix(placed), 0);
    }
    return out;
}
}

std::vector<ObjectCollision> objectCollisions(FuseArchive& archive, const Level& level, const PlacedObjectState& state) {
    return collectObjectCollisions(archive, level, state, false, Scope::Placed);
}

std::vector<ObjectCollision> objectCollisionCandidates(FuseArchive& archive, const Level& level) {
    return collectObjectCollisions(archive, level, PlacedObjectState{}, true, Scope::Placed);
}

std::vector<ObjectCollision> nestedObjectCollisionCandidates(FuseArchive& archive, const Level& level) {
    return collectObjectCollisions(archive, level, PlacedObjectState{}, true, Scope::Nested);
}

}
