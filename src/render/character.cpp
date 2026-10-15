#include "render/character.hpp"
#include <algorithm>
#include <map>
#include <stdexcept>

namespace chase::render {
using namespace chase::assets;

namespace {
std::string lowerStem(const std::string& file) {
    std::string name = lower(file);
    return name.substr(0, name.rfind('.'));
}

std::optional<std::size_t> jointIndex(const Skeleton& skeleton, const std::string& name) {
    const auto found = std::find_if(skeleton.joints.begin(), skeleton.joints.end(),
                                    [&](const Joint& joint) { return lower(joint.name) == name; });
    if (found == skeleton.joints.end()) return std::nullopt;
    return std::size_t(found - skeleton.joints.begin());
}

const FuseEntry& require(FuseArchive& archive, const std::string& path) {
    const FuseEntry* entry = archive.find(path);
    if (!entry) throw std::runtime_error("not in archive: " + path);
    return *entry;
}

constexpr const char* defaultAnimationClass = "class_generic";

std::string idleAnimationPath(const std::string& animationClass, const char* name) {
    return "models/char_minifig/" + animationClass + "/" + name + "_minifig.bfnanm";
}

Mat4 onJoint(const CharacterRig& rig, std::size_t joint, const std::vector<Matrix>& skin, const Mat4& world) {
    return chase::assets::multiply(jointAttachment(rig.skeleton.joints[joint], skin[joint]), world);
}
}

CharacterRig loadCharacterRig(FuseArchive& archive, const std::string& bodyModelPath) {
    CharacterRig rig;
    const std::string skeletonPath = bodyModelPath.substr(0, bodyModelPath.rfind('.')) + ".bfnskl";
    rig.skeleton = parseSkeleton(archive.read(require(archive, skeletonPath)));
    rig.headJoint = jointIndex(rig.skeleton, "head");
    rig.hatJoint = jointIndex(rig.skeleton, "hat");
    return rig;
}

Animation loadAnimation(FuseArchive& archive, const std::string& path) {
    return parseAnimation(archive.read(require(archive, path)));
}

CharacterInstance addCharacter(Scene& scene, const CharacterAppearance& base, const CharacterAppearance& look,
                               const std::string& hat, const std::string& poseOwner, const Mat4& world) {
    CharacterInstance instance;
    const std::string bodyPath = meshModelPath(look.body), headPath = meshModelPath(look.head);
    instance.bodyKey = poseOwner + "|" + bodyPath + "|" + look.upper + "|" + look.lower;
    const std::map<std::string, std::string> bodyTextures = {{lowerStem(base.lower), look.lower},
                                                              {lowerStem(base.upper), look.upper}};
    const std::map<std::string, std::string> headTextures = {{lowerStem(base.face), look.face}};
    if (!scene.addModel(bodyPath, world, bodyTextures, instance.bodyKey)) throw std::runtime_error("not in archive: " + bodyPath);
    instance.bodyInstance = scene.instances() - 1;
    if (!scene.addModel(headPath, world, headTextures, "head|" + headPath + "|" + look.face))
        throw std::runtime_error("not in archive: " + headPath);
    instance.headInstance = scene.instances() - 1;
    if (!hat.empty()) {
        const std::string hatPath = meshModelPath(hat);
        if (!scene.addModel(hatPath, world)) throw std::runtime_error("not in archive: " + hatPath);
        instance.hatInstance = scene.instances() - 1;
    }
    return instance;
}

CharacterAppearance readBaseAppearance(FuseArchive& archive) {
    return characterAppearance(parseLevel(archive.read(require(archive, "levels/chars/char_player.blvl"))));
}

std::string levelCharacterAnimationPath(FuseArchive& archive, const std::string& animationClass, const char* name) {
    const std::string path = idleAnimationPath(animationClass, name);
    return archive.find(path) ? path : idleAnimationPath(defaultAnimationClass, name);
}

std::vector<std::optional<CharacterInstance>> placeLevelCharacters(Scene& scene, FuseArchive& archive, const std::vector<CharacterObject>& characters) {
    std::vector<std::optional<CharacterInstance>> placed(characters.size());
    if (characters.empty()) return placed;
    const CharacterAppearance base = readBaseAppearance(archive);
    std::map<std::string, CharacterRig> rigs;
    std::map<std::string, Animation> animations;
    for (std::size_t index = 0; index < characters.size(); ++index) {
        const CharacterObject& character = characters[index];
        const std::string bodyPath = meshModelPath(character.appearance.body);
        if (!archive.find(bodyPath)) continue;
        const std::string animationPath = levelCharacterAnimationPath(archive, character.animationClass, "idle");
        auto rig = rigs.find(bodyPath);
        if (rig == rigs.end()) rig = rigs.emplace(bodyPath, loadCharacterRig(archive, bodyPath)).first;
        auto animation = animations.find(animationPath);
        if (animation == animations.end()) animation = animations.emplace(animationPath, loadAnimation(archive, animationPath)).first;
        const Mat4 world = placementMatrix(character.placement);
        const CharacterInstance instance = addCharacter(scene, base, character.appearance, character.hat, character.placement.instance, world);
        placeCharacter(scene, rig->second, instance, poseBody(scene, rig->second, instance, animation->second, 0), world);
        placed[index] = instance;
    }
    return placed;
}

std::size_t addLevelCharacters(Scene& scene, FuseArchive& archive, const std::vector<CharacterObject>& characters) {
    const auto placed = placeLevelCharacters(scene, archive, characters);
    return std::size_t(std::count_if(placed.begin(), placed.end(), [](const auto& instance) { return instance.has_value(); }));
}

std::vector<Matrix> poseBody(Scene& scene, const CharacterRig& rig, const CharacterInstance& instance,
                             const Animation& animation, double frame) {
    std::vector<Matrix> skin = skinMatrices(rig.skeleton, animation, frame);
    scene.pose(instance.bodyKey, skin);
    return skin;
}

void placeCharacter(Scene& scene, const CharacterRig& rig, const CharacterInstance& instance,
                    const std::vector<Matrix>& skin, const Mat4& world) {
    scene.setTransform(instance.bodyInstance, world);
    scene.setTransform(instance.headInstance, rig.headJoint ? onJoint(rig, *rig.headJoint, skin, world) : world);
    if (instance.hatInstance)
        scene.setTransform(*instance.hatInstance, rig.hatJoint ? onJoint(rig, *rig.hatJoint, skin, world) : world);
}

}
