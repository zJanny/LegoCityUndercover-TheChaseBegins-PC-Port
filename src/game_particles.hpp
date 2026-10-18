#pragma once
// The particle system of a chase-game level (docs/re/particles.md): the emitters of runtime::ParticleSystem started by the
// development option --particle, updated once per game update and drawn after the scene. The level objects that start
// emitters in the original are not ported (docs/plans/features/particles.md).
#include "assets/assets.hpp"
#include "game_options.hpp"
#include "render/particles.hpp"
#include "render/scene.hpp"
#include "runtime/game_random.hpp"
#include "runtime/particles.hpp"
#include <memory>
#include <optional>

namespace game_particles {

// Seeding of the original's global generator is time based; this fixed seed is a stand-in (PART-S3).
constexpr std::uint32_t kStandInSeed = 0x51a7c0deu;

// The game camera basis (row 0 screen right, row 1 up, row 2 view direction) of a view matrix of chase::render::view().
chase::runtime::ParticleCamera cameraOf(const chase::render::Mat4& view);

class LevelParticles {
public:
    LevelParticles(chase::assets::FuseArchive& archive, const std::vector<game_options::ParticleRequest>& requests);

    void update(float dt);
    void draw(const chase::render::Mat4& view, const chase::render::Mat4& viewProjection);
    void releaseGl() { renderer_.reset(); }
    chase::runtime::ParticleSystem& system() { return *system_; }
    chase::runtime::GameRandom& random() { return random_; }

private:
    chase::runtime::GameRandom random_{kStandInSeed};
    std::unique_ptr<chase::runtime::ParticleSystem> system_;
    std::optional<chase::render::ParticleRenderer> renderer_;
    chase::runtime::ParticleCamera camera_;
};

}
