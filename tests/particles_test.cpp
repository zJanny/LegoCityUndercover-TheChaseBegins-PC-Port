// Particle simulation (chase_runtime) on made-up definitions. Every pinned number is computed by hand from the rules read in the
// original: the provenance (function and lines of docs/re/particles.md) is next to each value.
#include "runtime/particles.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
using namespace chase::runtime;
using chase::assets::ParticleDef;
using chase::assets::ParticleDefs;
using chase::assets::ParticleKey;

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

bool near(float a, float b, float tolerance = 1e-5f) { return std::fabs(a - b) <= tolerance; }

ParticleKey key(float time, std::uint8_t a, std::uint8_t b = 0, std::uint8_t c = 0, std::uint8_t d = 0) {
    ParticleKey k;
    k.time = time;
    k.value = {a, b, c, d};
    return k;
}

ParticleDef baseDef() {
    ParticleDef d;
    d.texture = 0;
    d.atlasColumns = 1;
    d.atlasRows = 1;
    d.particles = 1;
    d.duration = 100.0f;
    d.lifetimeMin = d.lifetimeMax = 1.0f;
    d.sizeX = d.sizeY = 1.0f;
    d.axis = {0, 1, 0};
    d.startSpeed = 2.0f;
    d.seed = 1;
    d.fadeKeys = {key(0, 255), key(1, 255)};
    d.sizeKeys = {key(0, 128), key(1, 128)};
    d.colourKeys = {key(0, 10, 20, 30, 255), key(1, 110, 120, 130, 255)};
    d.alphaKeys = {key(0, 200), key(1, 100)};
    return d;
}

ParticleDefs defsOf(std::vector<ParticleDef> list) {
    ParticleDefs defs;
    defs.maxEmitters = 10;
    defs.maxParticles = 25;
    defs.textures = {"fx_test"};
    defs.defs = std::move(list);
    return defs;
}

void basisOfUpAxis() {
    // 0x216244: |axis.y| >= 0.7071 takes e = axis x (1,0,0) and f = e x axis; for (0,1,0): e = (0,0,-1), f = (1,0,0).
    const auto up = particleBasis({0, 1, 0});
    require(near(up[0][0], 1) && near(up[0][1], 0) && near(up[0][2], 0), "x axis of an upward emitter");
    require(near(up[1][0], 0) && near(up[1][1], 0) && near(up[1][2], -1), "y axis of an upward emitter");
    require(near(up[2][1], 1), "z axis is the emitter axis");
    // |axis.y| < 0.7071 takes up = (0,1,0): f = up x axis, e = axis x f; for (0,0,1): f = (1,0,0), e = (0,1,0).
    const auto forward = particleBasis({0, 0, 1});
    require(near(forward[0][0], 1) && near(forward[0][1], 0) && near(forward[0][2], 0), "x axis of a forward emitter");
    require(near(forward[1][1], 1), "y axis of a forward emitter");
}

void spawnRules() {
    GameRandom random(5);
    ParticleDef counted = baseDef();
    counted.particles = 25;
    counted.rate = 10.0f;
    counted.lifetimeMin = 0.5f;
    ParticleDef refused = baseDef();
    refused.atlasColumns = 0;
    ParticleDef many = baseDef();
    many.particles = 25;
    ParticleSystem system(defsOf({counted, refused, many}), random);
    // 0x216244: rate 10 x lifetime 0.5 = ceil 5, below 25, so the count becomes 5 + 1.
    system.start(0, {});
    require(system.emitters()[0]->particles.size() == 6, "rate x lifetime shortens the particle count to ceil + 1");
    // the file header's particle limit 25 leaves 24 slots
    system.start(2, {});
    require(system.emitters()[1]->particles.size() == 24, "particle limit caps the count at 24");
    require(system.start(1, {}) == 0, "no atlas column: refused");
    require(system.start(99, {}) == 0, "unknown definition: refused");
    for (unsigned i = 2; i < kParticleEmitterLimit; ++i) require(system.start(2, {}) != 0, "emitters up to the limit");
    require(system.start(2, {}) == 0, "the 26th emitter is refused (emitter limit 25, *0x376b84 in the oracle)");
}

void startValues() {
    // 0x222550 with seed 1: the LCG x -> x * 0x343fd + 0x269ec3 gives 83, 36935, 12669 (>> 15 of the low 31 bits) for
    // the three box axes: offset = extent * (draw / 65536 - 0.5) * 2, then turned by the emitter basis (x, z, -y) of an upward axis.
    ParticleDef d = baseDef();
    d.box = {1, 1, 1};
    d.lifetimeMin = d.lifetimeMax = 1.0f;
    GameRandom random(5);
    ParticleSystem system(defsOf({d}), random);
    const std::uint32_t handle = system.start(0, {{10, 20, 30}, std::nullopt, 0});
    const Particle& p = system.find(handle)->particles[0];
    require(near(p.position[0], -0.99746704f) && near(p.position[1], -0.6133728f) && near(p.position[2], -0.12716675f),
            "box offset from the emitter LCG, turned by the basis");
    // the elevation draw (no spiral) and the azimuth draw come next; elevation 0 gives sin 0, so the velocity is the axis times the start speed
    require(near(p.velocity[0], 0) && near(p.velocity[1], 2.0f) && near(p.velocity[2], 0), "start velocity along the axis");
    require(p.age == 0.0f && near(p.life, 1.0f), "age 0 and the lifetime range");
    require(p.colour[0] == 10 && p.colour[1] == 20 && p.colour[2] == 30, "first colour key");
    // 0x222550: alpha = first alpha key x the initial alpha argument, definition byte +0x8c / 255 = 0 here
    require(p.colour[3] == 0, "initial alpha argument 0 leaves the particle transparent");
}

void stepRules() {
    ParticleDef d = baseDef();
    d.gravity = -10.0f;
    d.lifetimeMin = d.lifetimeMax = 1.0f;
    d.initialAlpha = 255;
    GameRandom random(5);
    ParticleSystem system(defsOf({d}), random);
    const std::uint32_t handle = system.start(0, {{0, 5, 0}, std::nullopt, 0});
    system.update(0.1f, 1);
    const Particle& p = system.find(handle)->particles[0];
    // 0x221e9c: position += velocity x dt = 2 x 0.1, then velocity.y += gravity x dt = 2 - 10 x 0.1, then age += dt
    require(near(p.position[1], 0.2f), "position integrates the old velocity");
    require(near(p.velocity[1], 1.0f), "gravity changes the velocity after the move");
    require(near(p.age, 0.1f), "age advances");
    // 0x221e9c colour: lerp between key 0 (10, 20, 30) and key 1 (110, 120, 130) at f = 0 / 1 = 0, using the age before the step
    require(p.colour[0] == 10, "colour at the start of the life");
    system.update(0.5f, 1);
    // f = 0.1 / 1: red 10 + 100 x 0.1 = 20 (truncated); alpha 200 + (100 - 200) x 0.1 = 190 x fade 1.0
    require(p.colour[0] == 20 && p.colour[1] == 30 && p.colour[2] == 40, "colour keys interpolate with the age before the step");
    require(p.colour[3] == 190, "alpha keys interpolate");
    system.update(0.5f, 1);
    require(p.age == -1.0f, "the finished particle becomes free (-1)");
}

void bounce() {
    ParticleDef d = baseDef();
    d.bounce = 0.5f;
    d.gravity = 0.0f;
    d.startSpeed = 4.0f;
    d.axis = {0, -1, 0};
    d.lifetimeMin = d.lifetimeMax = 5.0f;
    GameRandom random(5);
    ParticleSystem system(defsOf({d}), random);
    system.standIns.castRay = [](const ParticleVec3& from, const ParticleVec3&, ParticleVec3& hit) {
        hit = {from[0], 2.0f, from[2]};
        return true;
    };
    const std::uint32_t handle = system.start(0, {{0, 3, 0}, std::nullopt, 0});
    require(near(system.find(handle)->ground, 2.0f), "ground height is the ray hit");
    // 0x221e9c: after the move the world height 3 - 4 x 0.5 = 1 is below the ground 2: velocity.y = -(-4 x 0.5), height clamped to the ground
    system.update(0.5f, 1);
    const Particle& p = system.find(handle)->particles[0];
    require(near(p.velocity[1], 2.0f), "bounce reverses and scales the speed by the restitution");
    require(near(p.position[1] + 3.0f, 2.0f), "the particle stands on the ground");
}

void emitterLifetime() {
    ParticleDef timed = baseDef();
    timed.duration = 1.0f;
    timed.startSpeed = 0;
    ParticleDef endless = baseDef();
    GameRandom random(5);
    ParticleSystem system(defsOf({timed, endless}), random);
    const std::uint32_t a = system.start(0, {});
    const std::uint32_t b = system.start(1, {});
    int removed = 0;
    system.setCallback(a, [&](std::uint32_t, int message) { removed += message == 2; });
    // 0x20db0c: the emitter ages by dt per update and is removed once the age is above the duration (1.0 is still kept)
    for (int i = 0; i < 4; ++i) system.update(0.25f, 1);
    require(system.find(a) != nullptr && removed == 0, "age 1.0 is not above the duration");
    system.update(0.25f, 1);
    require(system.find(a) == nullptr && removed == 1, "removed at age 1.25 with message 2");
    for (int i = 0; i < 20; ++i) system.update(0.25f, 1);
    require(system.find(b) != nullptr, "durations of 98 and above do not age");
    // 0x23ddb4: a stop with a fade time on an endless emitter counts that time down; 0 removes it on the next update
    system.stop(b, 0.5f);
    system.update(0.25f, 1);
    require(system.find(b) != nullptr, "still fading out after 0.25 s");
    system.update(0.25f, 1);
    require(system.find(b) == nullptr, "removed when the countdown reaches 0");
    const std::uint32_t c = system.start(1, {});
    system.stop(c, 0.0f);
    system.update(0.0f, 1);
    require(system.find(c) == nullptr, "stop(0) removes at the next update");
}

void subSteps() {
    // rate 20, dt 0.1: the start creates floor(1/30 x 20) = 0 -> 1 particle. The update steps it by 0.1, then two sub-steps of
    // 0.05 s each create one (acc 1.0): the first is stepped by dt - 0.05 = 0.05, the second by dt - 0.1 = 0 (0x20db0c).
    ParticleDef d = baseDef();
    d.rate = 20.0f;
    d.particles = 25;
    d.startSpeed = 0;
    d.lifetimeMin = d.lifetimeMax = 1.0f;
    GameRandom random(5);
    ParticleSystem system(defsOf({d}), random);
    const std::uint32_t handle = system.start(0, {});
    const ParticleEmitter* e = system.find(handle);
    require(e->particles.size() == 21, "rate 20 x lifetime 1: 21 slots");
    require(e->particles[0].age == 0.0f && e->particles[1].age == -1.0f, "one particle at the start");
    system.update(0.1f, 2);
    require(near(e->particles[0].age, 0.1f), "existing particle stepped by dt");
    require(near(e->particles[1].age, 0.05f), "first sub-step particle stepped by dt - dt/2");
    require(near(e->particles[2].age, 0.0f) && e->particles[3].age == -1.0f, "second sub-step particle stepped by 0");
    // one sub-step: the new particles are initialised in slot order and stepped by the full dt
    ParticleSystem single(defsOf({d}), random);
    const std::uint32_t h2 = single.start(0, {});
    single.update(0.1f, 1);
    require(near(single.find(h2)->particles[1].age, 0.1f) && near(single.find(h2)->particles[2].age, 0.1f), "without sub-steps both are stepped by dt");
}

void billboardQuad() {
    // 0x2b5970 / 0x2ae968: centred pivot, size 1, angle 0: corners (-1, 1), (1, 1), (1, -1), (-1, -1) along camera right / up;
    // the whole texture is u 0..0.9999 x 65535 (0x222550 clamps 1.0 to 0.9999), and the top vertices take the second v.
    ParticleDef d = baseDef();
    d.startSpeed = 0;
    d.initialAlpha = 255;
    d.blendMode = 1;
    GameRandom random(5);
    ParticleSystem system(defsOf({d}), random);
    system.start(0, {{1, 2, 3}, std::nullopt, 0});
    ParticleCamera camera;
    camera.position = {1, 2, -7};
    const auto batches = system.buildDraw(camera);
    require(batches.size() == 1 && batches[0].vertices.size() == 4 && batches[0].blendMode == 1, "one quad");
    const auto& v = batches[0].vertices;
    require(near(v[0].position[0], 0) && near(v[0].position[1], 3) && near(v[0].position[2], 3), "top left corner");
    require(near(v[2].position[0], 2) && near(v[2].position[1], 1), "bottom right corner");
    const float top = 65528.0f * 1.5259022e-05f;
    require(near(v[0].uv[0], 0) && near(v[0].uv[1], top, 1e-4f) && near(v[1].uv[0], top, 1e-4f) && near(v[2].uv[1], 0), "texture coordinates");
    // alpha: key 0 = 200 x initial argument 1.0, near fade 1 (depth 10 x 2.328 is above 1), emitter alpha 255 -> 200
    require(v[0].rgba[3] == 200 && v[0].rgba[0] == 10, "vertex colour");
}

void nearFade() {
    // 0x2b5970: factor (depth - word4) x scale, 0 at the camera; depth 0.2 x scale 2 = 0.4 -> alpha 200 x 0.4 + 0.5 = 80
    ParticleDef d = baseDef();
    d.startSpeed = 0;
    d.initialAlpha = 255;
    ParticleDefs defs = defsOf({d});
    defs.scale = 2.0f;
    GameRandom random(5);
    ParticleSystem system(defs, random);
    system.start(0, {{0, 0, 0.2f}, std::nullopt, 0});
    ParticleCamera camera;
    const auto batches = system.buildDraw(camera);
    require(batches.size() == 1 && batches[0].vertices[0].rgba[3] == 80, "near fade");
    camera.position = {0, 0, 0.3f};
    require(system.buildDraw(camera).empty(), "behind the camera the quad is invisible");
}

void atlas() {
    // 0x222550 frame mode 3: the counter E+0x153 walks the cells row by row and wraps at columns x rows
    ParticleDef d = baseDef();
    d.atlasColumns = 2;
    d.atlasRows = 2;
    d.frameMode = 3;
    d.particles = 5;
    d.startSpeed = 0;
    GameRandom random(5);
    ParticleSystem system(defsOf({d}), random);
    const auto* e = system.find(system.start(0, {}));
    const std::uint16_t half = static_cast<std::uint16_t>(0.5f * 65535.0f);
    require(e->particles[0].uv[0] == 0 && e->particles[0].uv[1] == half && e->particles[0].uv[2] == 0, "cell 0");
    require(e->particles[1].uv[0] == half && e->particles[1].uv[1] == 65528 && e->particles[1].uv[2] == 0, "cell 1 (second column)");
    require(e->particles[2].uv[0] == 0 && e->particles[2].uv[2] == half, "cell 2 (second row)");
    require(e->particles[4].uv[0] == 0 && e->particles[4].uv[2] == 0, "the counter wrapped after 4 cells");
}

void pausedAndFlush() {
    ParticleDef d = baseDef();
    GameRandom random(5);
    ParticleSystem system(defsOf({d}), random);
    const std::uint32_t handle = system.start(0, {});
    system.setPaused(true);
    system.update(0.1f, 2);
    require(system.find(handle)->particles[0].age == 0.0f, "paused (DAT_00376b69): nothing changes");
}
}

int main() {
    try {
        basisOfUpAxis();
        spawnRules();
        startValues();
        stepRules();
        bounce();
        emitterLifetime();
        subSteps();
        billboardQuad();
        nearFade();
        atlas();
        pausedAndFlush();
    } catch (const std::exception& error) {
        std::cerr << "particles-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "particles-test: ok\n";
    return 0;
}
