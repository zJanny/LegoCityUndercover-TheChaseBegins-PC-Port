#include "assets/object_collision.hpp"
#include "block_reader.hpp"
#include <algorithm>
#include <cctype>
#include <cstring>
#include <iterator>
#include <map>

namespace chase::assets {
namespace {
constexpr float kMinimumHalf = 0.05f;  // 0x3d4ccccd clamp of the automatic box in 0x23aff8
constexpr float kDoorCentreXStandIn = 0.0f;  // stand-in: Police_Door_02 registers x 0.0 although its mesh record says -0.0698; the source of the 0 is not found
constexpr float kDoorMargin = 0x1.eb851ep-3f;  // *0x37713c / *0x377140 taken off the x and z half extents when 0x23aff8 gets param_6

// Stand-in: instances of lvl_01_cherrytreehills that the original did not offer to the player and camera queries at gameplay-start:
// owners with flag bit 0 set (descriptor word 7 = 3 rejects them) or without any grid entry, read from the grid in the oracle. What
// sets the bit and decides the registration (level scripts, mission state) is not recovered, so the recorded names stand in for it.
constexpr const char* kStateDisabledStandIn[] = {
    "1-1_BI_DonutVanParts", "1-1_BI_DonutVanParts_Prop", "1-1_BrokenVan", "1-1_Broken_Lampost", "1-1_Crate_Trolley",
    "1-1_Donut_Van_Body", "1-1_Donut_Van_Collision", "1-1_DonutsDEMO", "1-1_DonutsDEMO01", "1-1_Mail_Box_Collision",
    "1-1_Mail_Box_Collision01", "1-2_Dog", "1-2_Dog_Bone", "1-2_Dog_Cage", "1-2_Dog_House", "1-2_Dtv_DogPath_XSpot",
    "1-2_Temp_Trampoline", "1-2_TrapDoor_01", "1-3_Camera_chopper", "1-3_Camera_park", "1-3_Cart_crate",
    "1-3_Chopper", "1-3_CraneHook_Collision", "1-3_CraneHook_Down_Collision", "1-3_Crane_Collision",
    "1-3_Crane_Girder", "1-3_Crane_bits", "1-3_Crow", "1-3_Dtv_BrickSpot_XSpot_02", "1-3_Garage_Key",
    "1-3_Gardner_Prop", "1-3_Gardner_Prop_02", "1-3_Generator_Pull_Switch", "1-3_Lawnmower_01", "1-3_Lawnmower_02",
    "1-3_Lawnmower_03", "1-3_Lawnmower_Final", "1-3_Ledge_Platform_Switch", "1-3_Ledge_Platform_Switch_Bits",
    "1-3_Ledge_Platform_Switch_Prop", "1-3_Lift_Crate_02", "1-3_Lift_Crate_03", "1-3_Push_Cart_bits",
    "1-3_Push_Platform", "1-3_Push_cart", "1-3_Roundabout", "1-3_Seesaw", "1-3_Swing", "1-3_TrapDoor_02",
    "1-3_Wheelbarrow02", "1-3_crane_built", "1-3_grab_bits", "1-4_Roadblock_12", "1-4_Roadblock_13",
    "1-4_Roadblock_18", "1-4_Roadblock_19", "1-4_Showroom_Ladder", "1-5_Generic_Prop_10", "1-8_BossGate_01",
    "1-8_BossGate_02", "1-8_Cage_01", "1-8_Goon_Collision_01", "1-8_Goon_Collision_02", "1-9_Ferry",
    "6-1_PlaceDonutsWaypoint", "9_Generic_Prop_081", "Bouncer_01", "CTG_Tshirt", "Donut_Box_01",
    "FP_ElectricRepair_PoliceGarageDoor02", "FP_Platform_TrainStation_MineCart", "FP_Sky_Dive_01", "FP_Spang_01",
    "FP_Task_ASTRONAUT_CTH_01", "FP_Task_ASTRONAUT_CTH_02", "FP_Task_ASTRONAUT_CTH_03", "FP_Task_ASTRONAUT_CTH_04",
    "Fast_Travel_Ferry", "GrappleGun", "Police_Plaque_Floor", "Race01EndBanner", "Race01StartBanner", "Trampoline_03",
    "Trampoline_06", "Trampoline_07",
};

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

bool startsWith(const std::string& text, const char* prefix) { return text.rfind(prefix, 0) == 0; }

struct ModelData {
    bool present = false;
    std::shared_ptr<const CollisionItem> firstItem;
    std::vector<CollisionBox> boxes;
    bool hasBounds = false;
    float boundsCentre[3] = {0, 0, 0};
    float boundsHalf[3] = {0, 0, 0};
};

// 0x242eb0: union of the mesh bounding boxes (centre +/- half) of the nodes whose name does not start with "NoBounds".
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
    for (std::size_t n = 0; n < model.nodeMesh.size(); ++n) {
        const int index = model.nodeMesh[n];
        if (index < 0 || std::size_t(index) >= model.meshes.size()) continue;
        if (model.nodeNames[n].compare(0, 8, "NoBounds") == 0) continue;
        const ModelMesh& mesh = model.meshes[std::size_t(index)];
        for (int a = 0; a < 3; ++a) {
            low[a] = std::min(low[a], mesh.centre[a] - mesh.half[a]);
            high[a] = std::max(high[a], mesh.centre[a] + mesh.half[a]);
        }
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

void transformPoint(const float matrix[16], const float in[3], float out[3]) {
    for (int a = 0; a < 3; ++a)
        out[a] = in[0] * matrix[a] + in[1] * matrix[4 + a] + in[2] * matrix[8 + a] + matrix[12 + a];
}
}

std::vector<ObjectCollision> objectCollisions(FuseArchive& archive, const Level& level) {
    std::map<std::string, Level> classes;
    const auto classOf = [&](const std::string& name) -> const Level& {
        auto found = classes.find(name);
        if (found == classes.end()) {
            const FuseEntry* entry = archive.find("levels/" + name + ".blvl");
            found = classes.emplace(name, entry ? parseLevel(archive.read(*entry)) : Level{}).first;
        }
        return found->second;
    };
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
        const std::string className = lower(object.className);
        const bool prop = startsWith(className, "props/") || startsWith(className, "leprops/");
        const bool door = className == "props/prop_door" || className == "leprops/leprop_door";
        const bool propFamily = startsWith(className, "leprops/leprop_") || className == "props/prop_respawnabledestructible" || door;
        const bool dummyCollision = className == "dummy/dummycollision";
        if (!prop && !dummyCollision) continue;
        if (std::find(std::begin(kStateDisabledStandIn), std::end(kStateDisabledStandIn), object.instance) != std::end(kStateDisabledStandIn)) continue;
        const Level& cls = classOf(object.className);

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
                    if (className == "props/prop_door") centre[0] = kDoorCentreXStandIn;
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
        if (const LevelParam* boundCollision = effectiveParam(object, cls, hashBoundCollision))
            if (boundCollision->value == 0) box = false;
        if (!box && !mesh) continue;

        ObjectCollision entry;
        entry.instance = object.instance;
        entry.className = object.className;
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
