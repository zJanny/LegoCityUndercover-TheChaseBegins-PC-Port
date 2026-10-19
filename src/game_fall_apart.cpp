#include "game_fall_apart.hpp"
#include "assets/object_collision.hpp"

#include <algorithm>
#include <cctype>

namespace game_fall_apart {
namespace assets = chase::assets;
namespace runtime = chase::runtime;

namespace {

constexpr const char* kPartNames[runtime::kFallApartParts] = {"BrokenHead01", "BrokenBody01", "BrokenArm01", "BrokenLegs01", "BrokenHips01"};
// The placeholder textures 0x1d3d10 / 0x1d3dac replace on the parts ("upper_TEST.tga", "lower_TEST.tga"; the leg and hip models name
// "lower_Test.tga").
constexpr const char* kUpperPlaceholder = "upper_test";
constexpr const char* kLowerPlaceholder = "lower_test";

std::string lowerStem(const std::string& file) {
    std::string name = file;
    std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return name.substr(0, name.rfind('.'));
}

std::optional<runtime::FallApartBounds> boundsOf(assets::FuseArchive& archive, const std::string& mesh) {
    if (mesh.empty()) return std::nullopt;
    float centre[3], half[3];
    if (!assets::modelBounds(archive, assets::meshModelPath(mesh), centre, half)) return std::nullopt;
    return runtime::FallApartBounds{{centre[0], centre[1], centre[2]}, {half[0], half[1], half[2]}};
}

}

void LevelFallApart::load(assets::FuseArchive& archive, runtime::GameRandom& random, const assets::CharacterAppearance& base) {
    base_ = base;
    std::array<runtime::FallApartBounds, runtime::kFallApartParts> bounds{};
    if (const assets::FuseEntry* entry = archive.find(kPartLevel)) {
        const assets::Level level = assets::parseLevel(archive.read(*entry));
        const std::uint32_t meshHash = assets::nameHash("meshname");
        for (unsigned part = 0; part < runtime::kFallApartParts; ++part)
            for (const assets::LevelObject& object : level.objects) {
                if (object.instance != kPartNames[part]) continue;
                for (const assets::LevelParam& param : object.params)
                    if (param.hash == meshHash && param.type == 1 && !param.blocks.empty())
                        meshes_[part] = reinterpret_cast<const char*>(param.blocks[0].data());
                bounds[part] = boundsOf(archive, meshes_[part]).value_or(runtime::FallApartBounds{});
            }
    }
    pool_.emplace(random, bounds);
}

std::optional<std::size_t> LevelFallApart::instance(chase::render::Scene& scene, unsigned slot, const std::string& modelKey, const std::string& path,
                                                    const std::map<std::string, std::string>& textures) {
    const std::string key = std::to_string(slot) + "|" + modelKey;
    const auto found = instances_.find(key);
    if (found != instances_.end()) return found->second;
    const std::size_t index = scene.instances();
    if (!scene.addModel(path, chase::render::identity(), textures, modelKey)) return std::nullopt;
    scene.setInstanceVisible(index, false);
    instances_[key] = index;
    return index;
}

bool LevelFallApart::start(chase::render::Scene& scene, assets::FuseArchive& archive, const DeadCharacter& dead) {
    if (!pool_) return false;
    const auto slot = pool_->start(dead.instance, dead.world, boundsOf(archive, dead.look.head));
    if (!slot) return false;
    Shown& shown = shown_[*slot];
    shown = Shown{};
    for (unsigned part = 1; part < runtime::kFallApartParts; ++part) {
        if (meshes_[part].empty()) continue;
        const bool upper = part == unsigned(runtime::FallApartPart::Body) || part == unsigned(runtime::FallApartPart::Arm);
        const std::string& texture = upper ? dead.look.upper : dead.look.lower;
        shown.parts[part] = instance(scene, *slot, "fallapart|" + meshes_[part] + "|" + texture, assets::meshModelPath(meshes_[part]),
                                     {{upper ? kUpperPlaceholder : kLowerPlaceholder, texture}});
    }
    if (!dead.look.head.empty()) {
        const std::string headPath = assets::meshModelPath(dead.look.head);
        shown.parts[unsigned(runtime::FallApartPart::Head)] =
            instance(scene, *slot, "head|" + headPath + "|" + dead.look.face, headPath, {{lowerStem(base_.face), dead.look.face}});
    }
    if (!dead.hat.empty()) {
        const std::string hatPath = assets::meshModelPath(dead.hat);
        shown.hat = instance(scene, *slot, hatPath, hatPath, {});
    }
    shown.characterWorld = dead.world;
    shown.headAtDeath = dead.head.value_or(dead.world);
    shown.hatAtDeath = dead.hatWorld.value_or(dead.world);
    return true;
}

void LevelFallApart::update(const runtime::DebrisLineTest& test, std::uint32_t clock) {
    if (pool_) pool_->update(test, clock);
}

void LevelFallApart::apply(chase::render::Scene& scene) const {
    for (const auto& [key, index] : instances_) scene.setInstanceVisible(index, false);
    if (!pool_) return;
    for (unsigned slot = 0; slot < runtime::kFallApartSlots; ++slot) {
        const runtime::FallApartSlot& state = pool_->slots()[slot];
        if (!state.inUse) continue;
        const Shown& shown = shown_[slot];
        for (unsigned part = 0; part < runtime::kFallApartParts; ++part) {
            const runtime::FallApartBody& body = state.parts[part];
            if (!runtime::FallApartPool::partUsed(state, part) || !body.enabled) continue;
            const bool head = part == unsigned(runtime::FallApartPart::Head);
            if (shown.parts[part]) {
                scene.setTransform(*shown.parts[part], head ? runtime::fallApartCarried(shown.headAtDeath, shown.characterWorld, body.matrix) : body.matrix);
                scene.setInstanceVisible(*shown.parts[part], true);
            }
            if (head && shown.hat) {
                scene.setTransform(*shown.hat, runtime::fallApartCarried(shown.hatAtDeath, shown.characterWorld, body.matrix));
                scene.setInstanceVisible(*shown.hat, true);
            }
        }
    }
}

}
