#include "game_particles.hpp"
#include <stdexcept>
#include <string>

namespace game_particles {
namespace render = chase::render;
namespace runtime = chase::runtime;

runtime::ParticleCamera cameraOf(const render::Mat4& view) {
    const render::Mat4 world = render::rigidInverse(view);
    runtime::ParticleCamera camera;
    camera.position = {world[12], world[13], world[14]};
    camera.right = {-view[0], -view[4], -view[8]};
    camera.up = {view[1], view[5], view[9]};
    camera.forward = {-view[2], -view[6], -view[10]};
    return camera;
}

LevelParticles::LevelParticles(chase::assets::FuseArchive& archive, const std::vector<game_options::ParticleRequest>& requests) {
    const chase::assets::FuseEntry* entry = archive.find("particles/particledefs.bprt");
    if (!entry) throw std::runtime_error("not in archive: particles/particledefs.bprt");
    const chase::assets::ParticleDefs defs = chase::assets::parseParticleDefs(archive.read(*entry));
    system_ = std::make_unique<runtime::ParticleSystem>(defs, random_);
    renderer_.emplace(archive, defs.textures);
    for (const game_options::ParticleRequest& request : requests) {
        if (request.definition >= defs.defs.size())
            throw std::runtime_error("--particle: definition " + std::to_string(request.definition) + " is outside the " +
                                     std::to_string(defs.defs.size()) + " definitions of the file");
        if (system_->start(request.definition, {request.position, std::nullopt, 0}) == 0)
            throw std::runtime_error("--particle: definition " + std::to_string(request.definition) + " was refused (emitter limit or no atlas column)");
    }
}

void LevelParticles::update(float dt) {
    system_->camera = camera_;
    system_->update(dt);
}

void LevelParticles::draw(const render::Mat4& view, const render::Mat4& viewProjection) {
    camera_ = cameraOf(view);
    if (renderer_) renderer_->draw(viewProjection, system_->buildDraw(camera_));
}

}
