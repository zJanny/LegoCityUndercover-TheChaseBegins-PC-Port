#pragma once
// Particle emitters and particles of the original (docs/re/particles.md): the spawner 0x216244 behind the start entry
// 0x23de24, the emitter update 0x20db0c (called with 2 sub-steps by the game loops 0x2e5458 and 0x2faf0c), the per-particle
// init 0x222550 and step 0x221e9c, the stop 0x23ddb4 and the quad builders 0x2b5970 / 0x2b5f54 of the draw 0x2722cc.
// No SDL and no GL: drawing takes the batches built here (render/particles.hpp). Emitters are placed in the world
// (parent node, room lookup and attached modes of the spawner are not modelled, see docs/plans/features/particles.md).
#include "assets/assets.hpp"
#include "runtime/game_random.hpp"
#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace chase::runtime {

using ParticleVec3 = std::array<float, 3>;

inline constexpr unsigned kParticleSubSteps = 2;
inline constexpr unsigned kParticleEmitterLimit = 25;
inline constexpr float kParticleTurn = 8000.0f;

struct Particle {
    ParticleVec3 velocity{};                // +0x00
    ParticleVec3 position{};                // +0x0c, relative to the emitter
    std::array<std::uint8_t, 4> colour{};   // +0x18, red, green, blue, alpha
    float sizeX = 0, sizeY = 0;             // +0x1c, +0x20, half extents of the quad
    std::uint16_t uv[4]{};                  // +0x24 first u, +0x26 second u, +0x28 first v, +0x2a second v (x 65535)
    float angle = 0;                        // +0x2c, 8000 per turn
    float spin = 0;                         // +0x30, added to the angle every update
    float life = 0;                         // +0x34
    float age = -1.0f;                      // +0x38; -1 free, -2 finished (definition flag 0x100)
    std::uint32_t packed = 0;               // +0x3c; bits 0..29 sway phase x 64, bit 30 flips u, bit 31 flips v
    std::int16_t drawAngle = 0;             // +0x40
    std::int16_t tumble = 0;                // +0x42, 0..999
};

struct ParticleCamera {
    ParticleVec3 position{}, right{1, 0, 0}, up{0, 1, 0}, forward{0, 0, 1};
};

struct ParticleEmitter {
    std::uint32_t handle = 0;
    const assets::ParticleDef* def = nullptr;           // +0x144
    std::array<ParticleVec3, 3> basis{};                // rows of the emitter node's orientation
    ParticleVec3 position{};                            // node translation
    ParticleVec3 localPosition{}, previousPosition{};   // +0xbc, +0xc8
    ParticleVec3 boxOverride{};                         // +0xd4..+0xdc, used with flag 4
    float age = 0;                                      // +0xe0
    float accumulator = 0;                              // +0xe4
    float rateScale = 1.0f;                             // +0xe8
    float growX = 0, growY = 0;                         // +0xec, +0xf0
    float drag = 0;                                     // +0xf4
    float speedStart = 0, speedEnd = 0;                 // +0xf8, +0xfc
    float sizeX = 0, sizeY = 0;                         // +0x100, +0x104
    float scale = 1.0f;                                 // +0x108
    float gravity = 0;                                  // +0x10c
    float spinRange = 0;                                // +0x110
    float spiral = 0;                                   // +0x114
    float ground = 0;                                   // +0x11c, world height particles bounce on
    float endCountdown = 0, endCountdownStart = 0;      // +0x120, +0x124
    float prewarm = 0, prewarmElapsed = 0;              // +0x128, +0x12c
    float delay = 0;                                    // +0x130, no particle is created while above 0
    std::uint32_t lcg = 0;                              // +0x134
    std::uint8_t flags = 0;                             // +0x150: 1 active, 2 positioned inside the node, 4 box override, 0x10 no new particles
    std::uint8_t frameCounter = 0;                      // +0x153
    std::uint8_t alpha = 255;                           // +0x154
    std::uint8_t fade128 = 128;                         // +0x155, emitter fade x 128
    std::uint16_t tumbleCounter = 0;                    // +0x156
    std::function<void(std::uint32_t handle, int message)> callback;  // +0x140: message 1 particles are created, 2 the emitter is removed
    std::vector<Particle> particles;
};

struct ParticleStart {
    ParticleVec3 position{};
    std::optional<ParticleVec3> axis;   // default: the definition's axis (+0x00)
    float prewarm = 0;                  // seconds simulated at once on the first updates (E+0x128)
};

struct ParticleVertex {
    float position[3];
    float uv[2];
    std::uint8_t rgba[4];
};

struct ParticleBatch {
    int texture = -1;               // index into ParticleDefs::textures
    unsigned blendMode = 0;         // definition byte +0x177
    bool alphaTest = false;         // definition flag 1: fragments pass when alpha x 255 is above alphaReference
    float alphaReference = 0;       // emitter fade x 128 (0..128)
    std::vector<ParticleVertex> vertices;  // four per quad: top left, top right, bottom right, bottom left
};

struct ParticleStandIns {
    float frameDelta = 1.0f / 30.0f;        // 0x23602c, the frame time of the game clock
    float globalScale = 1.0f;               // 0x376b94
    std::function<bool(const ParticleVec3& from, const ParticleVec3& to, ParticleVec3& hit)> castRay;  // 0x22f978, null = no world
};

class ParticleSystem {
public:
    // `defs` is copied: the spawner shortens the particle count of a definition it uses. The game's init 0x1ed440 replaces the
    // file header's emitter limit (10) with 25 (*0x376b84 in the oracle, 12 emitters alive in state in-vehicle).
    ParticleSystem(const assets::ParticleDefs& defs, GameRandom& random, unsigned emitterLimit = kParticleEmitterLimit);

    ParticleStandIns standIns;
    ParticleCamera camera;   // DAT_00529fcc: the camera matrix of the current frame; set it before update()

    // 0x23de24 / 0x216244 (mode 0, no parent, no room check): returns the emitter handle, 0 when refused.
    std::uint32_t start(unsigned definition, const ParticleStart& request);
    // 0x23ddb4: ends the emitter; with `fadeSeconds` > 0 it runs that long first.
    void stop(std::uint32_t handle, float fadeSeconds);
    // 0x23de14
    void setCallback(std::uint32_t handle, std::function<void(std::uint32_t, int)> callback);
    // 0x171e08 keys 0 and 2
    void setAlpha(std::uint32_t handle, std::uint8_t alpha);
    void setScale(std::uint32_t handle, float scale);
    void setPosition(std::uint32_t handle, const ParticleVec3& position);
    void setPaused(bool paused) { paused_ = paused; }

    // 0x1ece0c: advances every emitter by `dt` seconds with `subSteps` creation steps, then remembers the positions.
    void update(float dt, unsigned subSteps = kParticleSubSteps);
    // 0x20db0c with one emitter (the catch-up 0x21539c); the other emitters are not touched.
    void updateEmitter(std::uint32_t handle, float dt, unsigned subSteps);

    // 0x2722cc: the quads of every emitter in list order, in world coordinates.
    std::vector<ParticleBatch> buildDraw(const ParticleCamera& camera) const;

    const ParticleEmitter* find(std::uint32_t handle) const;
    const std::vector<std::unique_ptr<ParticleEmitter>>& emitters() const { return emitters_; }
    const assets::ParticleDefs& defs() const { return defs_; }
    unsigned emitterLimit() const { return emitterLimit_; }

private:
    void run(float dt, ParticleEmitter* only, unsigned subSteps);
    void updateActive(ParticleEmitter& e, float dt, unsigned subSteps);
    void initParticle(ParticleEmitter& e, Particle& p, float alpha);
    void stepParticle(ParticleEmitter& e, Particle& p, float dt, float fade, int cells, const float uvScale[2]);
    void remove(std::size_t index);
    float lcgUnit(ParticleEmitter& e);
    float lcgRaw(ParticleEmitter& e);

    assets::ParticleDefs defs_;
    GameRandom& random_;
    unsigned emitterLimit_;
    std::vector<std::unique_ptr<ParticleEmitter>> emitters_;
    std::uint32_t nextHandle_ = 1;
    bool paused_ = false;
    float nearFadeStart_ = 0;
};

// 0x216244 lines 0x29..0x7d: the orthonormal basis (rows x, y, z) of an emitter whose z axis is `axis`.
std::array<ParticleVec3, 3> particleBasis(const ParticleVec3& axis);
// The key lists: linear value between two keys (0x20db0c fade, 0x222550, 0x221e9c). `found` is false when no segment contains t.
float particleKeyValue(const std::vector<assets::ParticleKey>& keys, float t, unsigned byteIndex, float fallback, bool* found = nullptr);

}
