#pragma once
// The brick debris and the smoke emitters of hits in chase-game (docs/re/vehicles-damage.md): runtime::DebrisField drawn with the level objects
// Debris01..Debris20 of levels/charbits.blvl, and the vehicle smoke emitters of runtime::ParticleSystem.
#include "assets/assets.hpp"
#include "game_debris_super.hpp"
#include "render/scene.hpp"
#include "runtime/debris.hpp"
#include "runtime/particles.hpp"

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <vector>

namespace game_effects {

// 0x2ecf30 looks the debris objects up by name in the object of 0x23b370; levels/charbits.blvl holds Debris01..Debris20.
constexpr const char* kDebrisLevel = "levels/charbits.blvl";

class LevelEffects {
public:
    // One hidden scene instance per debris object, with the model of its meshname, and the SBDebris objects (game_debris_super.hpp).
    void load(chase::render::Scene& scene, chase::assets::FuseArchive& archive, const chase::assets::ClassLookup& classOf, chase::runtime::GameRandom& random,
              chase::runtime::DebrisPlaneRing& planes);
    chase::runtime::DebrisField* debris() { return field_ ? &*field_ : nullptr; }
    std::size_t debrisInstances() const;
    std::size_t superDebrisInstances() const { return super_.instances(); }
    void update(float frameDelta, const chase::runtime::DebrisVec3& camera);
    // STAND-IN VEH-S101: a debris object is drawn while it is enabled and its alpha is not 0, without the alpha itself; the spin clip is not played.
    void apply(chase::render::Scene& scene) const;

    // 0x22a124: stops the vehicle's previous smoke emitter (fade 0.1 s) and starts `definition` at `position`; the emitter's removal clears the
    // handle (0x2ad53c, message 2).
    void startSmoke(std::size_t vehicle, chase::runtime::ParticleSystem& particles, unsigned definition, const chase::runtime::ParticleVec3& position);
    void stopSmoke(std::size_t vehicle, chase::runtime::ParticleSystem& particles, float fade);
    std::optional<std::uint32_t> smoke(std::size_t vehicle) const;

private:
    std::optional<chase::runtime::DebrisField> field_;
    game_debris_super::SuperDebrisObjects super_;
    std::vector<std::ptrdiff_t> instances_;
    std::map<std::size_t, std::uint32_t> smoke_;
};

}
