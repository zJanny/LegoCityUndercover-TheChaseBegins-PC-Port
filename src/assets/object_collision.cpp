#include "assets/object_collision.hpp"
#include "assets/object_classes.hpp"
#include "block_reader.hpp"
#include <algorithm>
#include <cctype>
#include <cstring>
#include <iterator>
#include <map>

namespace chase::assets {
namespace {
constexpr float kMinimumHalf = 0.05f;  // 0x3d4ccccd clamp of the automatic box in 0x23aff8
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

bool PlacedObjectState::isDisabled(const std::string& instance) const { return disabled.count(lower(instance)) != 0; }

bool PlacedObjectState::collisionEnabled(const std::string& instance, bool initial) const {
    const auto found = collisionOverride.find(lower(instance));
    return found == collisionOverride.end() ? initial : found->second;
}

namespace {
std::vector<ObjectCollision> collectObjectCollisions(FuseArchive& archive, const Level& level, const PlacedObjectState& state, bool keepInactive) {
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
                        hashBoundCollision = nameHash("BoundCollision"), hashCollisionBound = nameHash("CollisionBound");

    std::vector<ObjectCollision> out;
    for (const LevelObject& object : level.objects) {
        const bool prop = isPropClass(object.className);
        const bool door = isDoorClass(object.className);
        const bool dummyCollision = isDummyCollisionClass(object.className);
        const Level& cls = classOf(object.className);
        const bool vehicle = isVehicleClassFile(cls);
        const bool propFamily = isPropFamilyClass(object.className) || vehicle;
        if (!prop && !dummyCollision && !vehicle) continue;
        if (!keepInactive && state.isDisabled(object.instance)) continue;

        Placement placement;
        if (!float3Of(effectiveParam(object, cls, hashPosition), placement.position)) continue;
        float3Of(effectiveParam(object, cls, hashOrientation), placement.orientation);

        bool box = false;
        float centre[3] = {0, 0, 0}, half[3] = {0, 0, 0};
        std::shared_ptr<const CollisionItem> mesh;
        std::string modelPath;

        if (dummyCollision) {
            if (!float3Of(findParam(object.params, hashCollisionBound), half)) continue;
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
            }
        }
        const LevelParam* boundCollision = effectiveParam(object, cls, hashBoundCollision);
        const bool collisionDefault = !boundCollision || boundCollision->value != 0;
        if (!keepInactive && !state.collisionEnabled(object.instance, collisionDefault)) continue;
        if (!box && !mesh) continue;

        ObjectCollision entry;
        entry.instance = object.instance;
        entry.className = object.className;
        entry.collisionDefault = collisionDefault;
        entry.model = modelPath;
        entry.matrix = placementMatrix(placement);
        entry.mesh = mesh;
        std::memcpy(entry.centre, centre, sizeof centre);
        std::memcpy(entry.half, half, sizeof half);
        out.push_back(std::move(entry));
    }
    return out;
}
}

std::vector<ObjectCollision> objectCollisions(FuseArchive& archive, const Level& level, const PlacedObjectState& state) {
    return collectObjectCollisions(archive, level, state, false);
}

std::vector<ObjectCollision> objectCollisionCandidates(FuseArchive& archive, const Level& level) {
    return collectObjectCollisions(archive, level, PlacedObjectState{}, true);
}

}
