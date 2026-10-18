// The particle simulation on the shipped definitions (particles/particledefs.bprt). Arguments: fib, names csv.
// Exit code 77 when the game data is missing. Provenance of the pinned records: docs/re/particles.md (read from the
// definitions with the layout of 0x17c-byte records; record numbers are the indices 0x23de24 takes).
#include "assets/assets.hpp"
#include "runtime/particles.hpp"
#include <cmath>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
using namespace chase;

namespace {
void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

bool finite(float value) { return std::isfinite(value); }

void everyDefinitionRuns(const assets::ParticleDefs& defs) {
    // Smoke test of all 298 records: start with the shipped limits, run 3 s with the game's 2 sub-steps, draw each frame.
    runtime::GameRandom random(0x1234);
    for (std::size_t index = 0; index < defs.defs.size(); ++index) {
        runtime::ParticleSystem system(defs, random);
        system.standIns.castRay = [](const runtime::ParticleVec3& from, const runtime::ParticleVec3&, runtime::ParticleVec3& hit) {
            hit = {from[0], 0.0f, from[2]};
            return true;
        };
        const std::uint32_t handle = system.start(unsigned(index), {{5, 1, 5}, std::nullopt, 0});
        require(handle != 0, "definition " + std::to_string(index) + " refused");
        runtime::ParticleCamera camera;
        camera.position = {5, 3, -5};
        system.camera = camera;
        for (int frame = 0; frame < 90; ++frame) {
            system.update(1.0f / 30.0f);
            require(system.emitters().size() <= runtime::kParticleEmitterLimit, "emitter limit");
            for (const auto& emitter : system.emitters()) {
                require(emitter->particles.size() <= 24, "particle limit");
                for (const runtime::Particle& p : emitter->particles) {
                    require(p.age >= -2.0f && finite(p.age), "age " + std::to_string(index));
                    for (int c = 0; c < 3; ++c) require(finite(p.position[c]) && finite(p.velocity[c]), "position " + std::to_string(index));
                    require(finite(p.sizeX) && finite(p.sizeY), "size " + std::to_string(index));
                }
            }
            for (const runtime::ParticleBatch& batch : system.buildDraw(camera)) {
                require(batch.vertices.size() % 4 == 0 && batch.blendMode <= 3, "batch " + std::to_string(index));
                for (const runtime::ParticleVertex& v : batch.vertices)
                    require(finite(v.position[0]) && finite(v.position[1]) && finite(v.position[2]) && v.rgba[3] != 0, "vertex " + std::to_string(index));
            }
        }
    }
}

void characterSwapGlow(const assets::ParticleDefs& defs) {
    // Record 175 (fx_characterswapalpha): 3 particles, start speed 0.2 (+0x14), no end speed, no variation (+0x16f = 0),
    // elevation 20 degrees (+0x18), lifetime 0.45..0.75 (+0x7c, +0x80), axis (0, 0.577465, 0.816416). 0x222550: the velocity is
    // a unit direction within the cone around the axis times the speed, the life is uniform in the range.
    const assets::ParticleDef& d = defs.defs[175];
    require(defs.textures[d.texture] == "fx_characterswapalpha" && d.particles == 3 && d.rate == 0, "record 175 changed");
    runtime::GameRandom random(77);
    runtime::ParticleSystem system(defs, random);
    const std::uint32_t handle = system.start(175, {{0, 0, 0}, std::nullopt, 0});
    const runtime::ParticleEmitter* emitter = system.find(handle);
    require(emitter && emitter->particles.size() == 3, "three particles at the start");
    for (const runtime::Particle& p : emitter->particles) {
        const float speed = std::sqrt(p.velocity[0] * p.velocity[0] + p.velocity[1] * p.velocity[1] + p.velocity[2] * p.velocity[2]);
        require(std::fabs(speed - 0.2f) < 1e-4f, "speed 0.2");
        const float cosine = (p.velocity[0] * d.axis[0] + p.velocity[1] * d.axis[1] + p.velocity[2] * d.axis[2]) / speed;
        require(cosine >= std::cos(20.0f * float(M_PI) / 180.0f) - 1e-4f, "inside the 20 degree cone");
        require(p.life >= 0.45f && p.life <= 0.75f && p.age == 0.0f, "lifetime range");
        require(std::fabs(p.position[0]) <= 0.05f + 1e-6f && std::fabs(p.position[1]) <= 0.05f + 1e-6f, "box of 0.05");
    }
}

std::uint32_t bitsOf(float value) {
    std::uint32_t bits;
    std::memcpy(&bits, &value, sizeof bits);
    return bits;
}

void policeLightFlare(const assets::ParticleDefs& shipped) {
    // Oracle (Azahar, state in-vehicle, New 3DS, read through the GDB stub): the emitter at 0x08d94b20 uses record 242
    // (fx_p_lensflare, the siren light: 3 particles, spiral 60 degrees, random size +0.1 x 0.05, flip mode 22, life 100) and
    // holds three particles with these words. The emitter's LCG before its initialisations, 0x69e4f907, was found by searching
    // the recorded angle and size draws (raw 65381 / 15494 for the first particle); the angles, sizes and uv words below are the
    // recorded memory (+0x2c, +0x1c, +0x20, +0x24..+0x2a, +0x3c), the emitter's +0x114 held 4000 (three spiral steps).
    assets::ParticleDefs defs = shipped;
    require(defs.defs.size() > 242 && defs.textures[defs.defs[242].texture] == "fx_p_lensflare", "record 242 changed");
    defs.defs[242].seed = 0x69e4f907u;
    runtime::GameRandom random(1);
    runtime::ParticleSystem system(defs, random);
    const runtime::ParticleEmitter* emitter = system.find(system.start(242, {{0, 0, 0}, std::nullopt, 0}));
    require(emitter && emitter->particles.size() == 3, "three particles");
    const std::uint32_t angle[3] = {0x45f968a2, 0x454261b8, 0x44f46e38};
    const std::uint32_t sizeX[3] = {0x3f52da33, 0x3f507280, 0x3f5c9a67}, sizeY[3] = {0x3ed2da33, 0x3ed07280, 0x3edc9a67};
    for (std::size_t i = 0; i < 3; ++i) {
        const runtime::Particle& p = emitter->particles[i];
        require(bitsOf(p.angle) == angle[i], "angle of particle " + std::to_string(i));
        require(bitsOf(p.sizeX) == sizeX[i] && bitsOf(p.sizeY) == sizeY[i], "size of particle " + std::to_string(i));
        require(p.drawAngle == static_cast<std::int16_t>(static_cast<int>(p.angle)), "draw angle");
        require(p.life == 100.0f && p.age == 0.0f && p.spin == 0.0f, "life 100");
        require(p.colour[0] == 255 && p.colour[1] == 100 && p.colour[2] == 100 && p.colour[3] == 127, "colour and alpha 127");
        const bool flipped = i == 1;
        require(p.uv[0] == (flipped ? 0xfff8 : 0) && p.uv[1] == (flipped ? 0 : 0xfff8) && p.uv[2] == p.uv[0] && p.uv[3] == p.uv[1],
                "flip mode 22 turns every other particle in u and v");
        require(p.packed == (flipped ? 0xc0000000u : 0u), "flip bits");
    }
    require(std::fabs(emitter->spiral - 4000.0f) < 0.01f, "three spiral steps of 60 degrees");
}

void groundCrack(const assets::ParticleDefs& defs) {
    // Record 53 (crack2_d): flag 0x100 keeps a finished particle at age -2, flag 0x1000 draws it flat in the emitter's axes,
    // life 2.5 s, emitter duration 10 s (0x221e9c, 0x2b5f54).
    const assets::ParticleDef& d = defs.defs[53];
    require(defs.textures[d.texture] == "crack2_d" && (d.flags & 0x100) && (d.flags & 0x1000), "record 53 changed");
    runtime::GameRandom random(5);
    runtime::ParticleSystem system(defs, random);
    const std::uint32_t handle = system.start(53, {{0, 0, 0}, std::nullopt, 0});
    runtime::ParticleCamera camera;
    camera.position = {0, 5, -5};
    require(system.buildDraw(camera).size() == 1, "drawn at the start");
    for (int i = 0; i < 90; ++i) system.update(1.0f / 30.0f);
    const runtime::ParticleEmitter* emitter = system.find(handle);
    require(emitter && emitter->particles[0].age == -2.0f, "finished particle stays at -2");
    require(system.buildDraw(camera).empty(), "a finished particle is not drawn");
}
}

int main(int argc, char** argv) {
    try {
        if (argc < 3 || !std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
            std::cout << "particles-real-test: game data missing, skipped\n";
            return 77;
        }
        assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        const assets::FuseEntry* entry = archive.find("particles/particledefs.bprt");
        require(entry != nullptr, "particledefs.bprt not in the archive");
        const assets::ParticleDefs defs = assets::parseParticleDefs(archive.read(*entry));
        require(defs.defs.size() == 298, "298 records");
        everyDefinitionRuns(defs);
        characterSwapGlow(defs);
        policeLightFlare(defs);
        groundCrack(defs);
    } catch (const std::exception& error) {
        std::cerr << "particles-real-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "particles-real-test: ok\n";
    return 0;
}
