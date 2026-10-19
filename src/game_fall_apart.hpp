#pragma once
// The fall-apart bodies of dead minifig characters in chase-game (runtime/fall_apart.hpp): the part objects BrokenHead0N..BrokenHips0N of
// levels/charbits.blvl drawn with the dead character's textures, and its head (and hat) on the head part.
#include "assets/assets.hpp"
#include "render/scene.hpp"
#include "runtime/fall_apart.hpp"

#include <array>
#include <cstddef>
#include <map>
#include <optional>
#include <string>

namespace game_fall_apart {

// 0x15ce3c finds the part objects by name in the object of 0x23b370 (levels/charbits.blvl, as the debris objects).
constexpr const char* kPartLevel = "levels/charbits.blvl";

struct DeadCharacter {
    std::string instance;
    chase::assets::CharacterAppearance look;
    std::string hat;
    chase::runtime::FallApartMatrix world{};
    std::optional<chase::runtime::FallApartMatrix> head, hatWorld;
};

class LevelFallApart {
public:
    // The part meshes of slot 1 (the four slots name the same meshes) and their bounds; the base look names the head model's placeholder face.
    void load(chase::assets::FuseArchive& archive, chase::runtime::GameRandom& random, const chase::assets::CharacterAppearance& base);
    // 0x1d3b50 for a dead character; false when every slot is in use. STAND-IN SCRIPT-S265: the parts take the class's UpperTexture / LowerTexture
    // (0x270ae0 / 0x22ad0c not read), the head part the character's head model with its FaceTexture and hat; the white node colour is not applied.
    bool start(chase::render::Scene& scene, chase::assets::FuseArchive& archive, const DeadCharacter& dead);
    // STAND-IN SCRIPT-S265: the caller passes the game update count as the clock 0x241098 and the wheel ray as the line test 0x21ab20.
    void update(const chase::runtime::DebrisLineTest& test, std::uint32_t clock);
    void apply(chase::render::Scene& scene) const;
    const chase::runtime::FallApartPool* pool() const { return pool_ ? &*pool_ : nullptr; }

private:
    struct Shown {
        std::array<std::optional<std::size_t>, chase::runtime::kFallApartParts> parts;
        std::optional<std::size_t> hat;
        chase::runtime::FallApartMatrix headAtDeath{}, hatAtDeath{};
        chase::runtime::FallApartMatrix characterWorld{};
    };
    // One scene instance per slot and model key (the key of Scene::addModel, which shares the model's data between the slots).
    std::optional<std::size_t> instance(chase::render::Scene& scene, unsigned slot, const std::string& modelKey, const std::string& path,
                                        const std::map<std::string, std::string>& textures);

    std::optional<chase::runtime::FallApartPool> pool_;
    std::array<std::string, chase::runtime::kFallApartParts> meshes_;
    chase::assets::CharacterAppearance base_;
    std::map<std::string, std::size_t> instances_;
    std::array<Shown, chase::runtime::kFallApartSlots> shown_{};
};

}
