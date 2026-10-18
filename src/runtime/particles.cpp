#include "runtime/particles.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace chase::runtime {

using assets::ParticleDef;
using assets::ParticleKey;

namespace {
constexpr float kUnit16 = 1.5258789e-05f;
constexpr float kToRadians = 6.2831855f;
constexpr float kDegreesToUnits = 22.222223f;
constexpr float kByteUnit = 0.003921569f;
constexpr float kUvUnit = 1.5259022e-05f;
constexpr float kRoundOne = 0.9999f;
constexpr std::uint32_t kFlagFadeSquared = 0x2, kFlagRandomGradient = 0x4, kFlagPlanar = 0x8, kFlagSpinDamping = 0x8000;
constexpr std::uint32_t kFlagTumble = 0x40, kFlagImplode = 0x20, kFlagFinishedMinusTwo = 0x100;
constexpr std::uint32_t kFlagSizeClamp = 0x200, kFlagFlatEmitterAxes = 0x1000, kFlagVelocityXz = 0x10, kFlagNoNearFade = 0x10000;
constexpr std::uint32_t kFlagSorted = 0x2000, kFlagScaleFirst = 0x4000, kFlagAlphaOnly = 0x800, kFlagAlphaTest = 0x1;
constexpr std::uint8_t kEmitterPositioned = 0x2, kEmitterBoxOverride = 0x4, kEmitterNoSpawn = 0x10;

float flt(std::uint8_t byte) { return static_cast<float>(byte); }

float turnsToRadians(float units) { return (units / kParticleTurn) * kToRadians; }

ParticleVec3 cross(const ParticleVec3& a, const ParticleVec3& b) {
    return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
}

ParticleVec3 normalised(const ParticleVec3& v) {
    float squared = v[0] * v[0];
    squared += v[1] * v[1];
    squared += v[2] * v[2];
    const float length = std::sqrt(squared);
    std::uint32_t bits;
    std::memcpy(&bits, &length, sizeof bits);
    if (bits <= 0x34000000u) return {0, 0, 1};
    const float inverse = 1.0f / length;
    return {v[0] * inverse, v[1] * inverse, v[2] * inverse};
}

// 0x249ee8 / 0x20b214: row vector times the 3x3 matrix whose rows are `m`.
ParticleVec3 rotate(const ParticleVec3& v, const std::array<ParticleVec3, 3>& m) {
    ParticleVec3 out;
    for (std::size_t c = 0; c < 3; ++c) out[c] = v[0] * m[0][c] + v[1] * m[1][c] + v[2] * m[2][c];
    return out;
}

// 0x20c7f0: the matrix times a column vector (dot products with the rows).
ParticleVec3 project(const ParticleVec3& v, const std::array<ParticleVec3, 3>& m) {
    ParticleVec3 out;
    for (std::size_t r = 0; r < 3; ++r) out[r] = v[0] * m[r][0] + v[1] * m[r][1] + v[2] * m[r][2];
    return out;
}

// The segment k with keys[k].time <= t < keys[k + 1].time among the first count - 1 keys (loops of 0x20db0c, 0x222550, 0x221e9c).
bool findSegment(const std::vector<ParticleKey>& keys, float t, std::size_t& k, float& along) {
    if (keys.size() < 2) return false;
    for (k = 0; k < keys.size() - 1; ++k) {
        if (keys[k].time <= t && keys[k + 1].time > t) {
            along = (t - keys[k].time) / (keys[k + 1].time - keys[k].time);
            return true;
        }
    }
    return false;
}

float segmentValue(const std::vector<ParticleKey>& keys, std::size_t k, float along, unsigned byteIndex) {
    const float from = flt(keys[k].value[byteIndex]), to = flt(keys[k + 1].value[byteIndex]);
    return from + (to - from) * along;
}

bool isFloatAtLeastOne(float value) {
    std::int32_t bits;
    std::memcpy(&bits, &value, sizeof bits);
    return bits > 0x3f7fffff;
}

std::uint16_t packUv(float value) {
    if (isFloatAtLeastOne(value)) value = kRoundOne;
    const float scaled = value * 65535.0f;
    return scaled > 0.0f ? static_cast<std::uint16_t>(static_cast<std::int16_t>(static_cast<int>(scaled))) : std::uint16_t(0);
}

void wrapAngle(float& angle) {
    if (angle >= 0.0f) {
        if (angle > kParticleTurn) angle -= kParticleTurn;
    } else {
        angle += kParticleTurn;
    }
}

bool ageIsLive(float age) {
    std::uint32_t bits;
    std::memcpy(&bits, &age, sizeof bits);
    return bits < 0xbf800000u;
}

// 0x2ae968: the four corners of a quad in the plane of the pivot, turned by `angleUnits` and scaled by the particle's size.
std::array<std::array<float, 2>, 4> cornerOffsets(const Particle& p, const ParticleDef& d, int angleUnits) {
    const float ax = 1.0f - d.pivotX * 2.0f, ay = d.pivotY * 2.0f - 1.0f;
    std::array<std::array<float, 2>, 4> c = {{{ax - 1.0f, ay + 1.0f}, {ax + 1.0f, ay + 1.0f}, {ax + 1.0f, ay - 1.0f}, {ax - 1.0f, ay - 1.0f}}};
    const float radians = turnsToRadians(static_cast<float>(angleUnits));
    const float s = std::sin(radians), co = std::cos(radians);
    for (auto& corner : c) {
        if ((d.flags & kFlagScaleFirst) == 0) {
            const float x = corner[0];
            corner[0] = (x * co - corner[1] * s) * p.sizeX;
            corner[1] = (x * s + corner[1] * co) * p.sizeY;
        } else {
            const float x = corner[0] * p.sizeX, y = corner[1] * p.sizeY;
            corner[0] = x * co - y * s;
            corner[1] = x * s + y * co;
        }
    }
    return c;
}
}

std::array<ParticleVec3, 3> particleBasis(const ParticleVec3& axis) {
    std::array<ParticleVec3, 3> rows{};
    float absY = std::fabs(axis[1]);
    std::int32_t bits;
    std::memcpy(&bits, &absY, sizeof bits);
    if (bits < 0x3f34fdf5) {
        const ParticleVec3 right = normalised(cross({0, 1, 0}, axis));
        rows = {right, normalised(normalised(cross(axis, right))), axis};
    } else {
        const ParticleVec3 up = normalised(cross(axis, {1, 0, 0}));
        rows = {normalised(normalised(cross(up, axis))), up, axis};
    }
    return rows;
}

float particleKeyValue(const std::vector<ParticleKey>& keys, float t, unsigned byteIndex, float fallback, bool* found) {
    std::size_t k = 0;
    float along = 0;
    const bool hit = findSegment(keys, t, k, along);
    if (found) *found = hit;
    return hit ? segmentValue(keys, k, along, byteIndex) : fallback;
}

ParticleSystem::ParticleSystem(const assets::ParticleDefs& defs, GameRandom& random, unsigned emitterLimit)
    : defs_(defs), random_(random), emitterLimit_(emitterLimit) {
    std::memcpy(&nearFadeStart_, &defs.unknown4, sizeof nearFadeStart_);
}

float ParticleSystem::lcgRaw(ParticleEmitter& e) {
    e.lcg = e.lcg * 0x343fdu + 0x269ec3u;
    return static_cast<float>((e.lcg & 0x7fffffffu) >> 15);
}

float ParticleSystem::lcgUnit(ParticleEmitter& e) { return lcgRaw(e) * kUnit16; }

const ParticleEmitter* ParticleSystem::find(std::uint32_t handle) const {
    for (const auto& e : emitters_)
        if (e->handle == handle) return e.get();
    return nullptr;
}

std::uint32_t ParticleSystem::start(unsigned definition, const ParticleStart& request) {
    if (definition >= defs_.defs.size()) return 0;
    if (emitters_.size() >= emitterLimit_) return 0;
    ParticleDef& d = defs_.defs[definition];
    if (d.atlasColumns == 0) return 0;
    if (d.rate != 0.0f) {
        const float limit = std::ceil(d.lifetimeMin * d.rate);
        if (static_cast<std::uint32_t>(static_cast<int>(limit)) < d.particles) d.particles = static_cast<std::uint8_t>(static_cast<int>(limit) + 1);
    }
    if (defs_.maxParticles <= d.particles) d.particles = static_cast<std::uint8_t>(defs_.maxParticles - 1);

    auto owned = std::make_unique<ParticleEmitter>();
    ParticleEmitter& e = *owned;
    e.handle = nextHandle_++;
    e.def = &d;
    e.particles.assign(d.particles, Particle{});
    e.lcg = d.seed != 0 ? d.seed : random_.next();
    const ParticleVec3 axis = request.axis ? *request.axis : d.axis;
    e.basis = particleBasis(axis);
    e.position = request.position;
    if (d.flags & kFlagPlanar) e.position[2] = -request.position[2];
    e.prewarm = request.prewarm;

    unsigned initial;
    if (d.rate == 0.0f) {
        initial = d.particles;
    } else {
        initial = static_cast<unsigned>(std::floor(standIns.frameDelta * d.rate));
        if (initial == 0) initial = 1;
        if (d.particles < initial) initial = d.particles;
    }
    float scale = standIns.globalScale;
    if (d.scale != 0.0f) scale = d.scale * standIns.globalScale;
    float endSpeed = d.endSpeed, sizeX = d.sizeX, sizeY = d.sizeY, startSpeed = d.startSpeed;
    if (scale != 1.0f) {
        endSpeed *= scale;
        sizeX *= scale;
        sizeY *= scale;
        startSpeed *= scale;
    }
    e.growX = d.growX * scale;
    e.growY = d.growY * scale;
    e.gravity = d.gravity * scale;
    e.drag = d.drag;
    e.scale = scale;
    e.spinRange = d.spinMax - d.spinMin;
    e.sizeX = sizeX;
    e.sizeY = sizeY;
    e.speedStart = startSpeed;
    e.speedEnd = endSpeed;
    e.rateScale = 1.0f;
    e.alpha = 0xff;
    e.fade128 = 0x80;
    e.flags = 0;

    if (d.bounce > 0.0f) {
        const ParticleVec3 from = e.position;
        ParticleVec3 to = {e.position[0], e.position[1] - 1000.0f, e.position[2]};
        ParticleVec3 hit = to;
        if (standIns.castRay) {
            for (unsigned tries = 0; tries < 3; ++tries) {
                ParticleVec3 found;
                if (!standIns.castRay(from, to, found)) break;
                hit = found;
                to = found;
                to[1] += 1.1920929e-07f;
            }
        }
        e.ground = hit[1];
    }

    const float initialAlpha = flt(d.initialAlpha) * kByteUnit;
    for (unsigned i = 0; i < d.particles; ++i) {
        if (i < initial) initParticle(e, e.particles[i], initialAlpha);
        else e.particles[i].age = -1.0f;
    }
    const std::uint32_t handle = e.handle;
    emitters_.push_back(std::move(owned));
    return handle;
}

void ParticleSystem::stop(std::uint32_t handle, float fadeSeconds) {
    for (const auto& owned : emitters_) {
        if (owned->handle != handle) continue;
        ParticleEmitter& e = *owned;
        const float duration = e.def->duration;
        if (duration <= 98.0f) {
            e.age = duration - fadeSeconds;
        } else if (fadeSeconds == 0.0f) {
            e.age = duration + 1.0f;
        } else {
            e.endCountdown = fadeSeconds;
            e.endCountdownStart = fadeSeconds;
        }
        return;
    }
}

void ParticleSystem::setCallback(std::uint32_t handle, std::function<void(std::uint32_t, int)> callback) {
    for (const auto& e : emitters_)
        if (e->handle == handle) e->callback = std::move(callback);
}

void ParticleSystem::setAlpha(std::uint32_t handle, std::uint8_t alpha) {
    for (const auto& e : emitters_)
        if (e->handle == handle) e->alpha = alpha;
}

void ParticleSystem::setScale(std::uint32_t handle, float scale) {
    for (const auto& e : emitters_)
        if (e->handle == handle) e->scale = scale;
}

void ParticleSystem::setPosition(std::uint32_t handle, const ParticleVec3& position) {
    for (const auto& e : emitters_)
        if (e->handle == handle) e->position = position;
}

void ParticleSystem::remove(std::size_t index) {
    emitters_[index] = std::move(emitters_.back());
    emitters_.pop_back();
}

void ParticleSystem::update(float dt, unsigned subSteps) {
    run(dt, nullptr, subSteps);
    for (const auto& e : emitters_) e->previousPosition = e->localPosition;
}

void ParticleSystem::updateEmitter(std::uint32_t handle, float dt, unsigned subSteps) {
    for (const auto& e : emitters_) {
        if (e->handle != handle) continue;
        run(dt, e.get(), subSteps);
        e->previousPosition = e->localPosition;
        return;
    }
}

void ParticleSystem::run(float dt, ParticleEmitter* only, unsigned subSteps) {
    if (paused_) return;
    bool ended = false;
    std::size_t index = 0;
    while (only ? true : index < emitters_.size()) {
        ParticleEmitter* e = only ? only : emitters_[index].get();
        const float duration = e->def->duration;
        if (duration >= 98.0f) {
            if (e->endCountdown != 0.0f) {
                e->endCountdown -= dt;
                if (!(0.0f < e->endCountdown)) ended = true;
            }
        } else {
            e->age += dt;
        }
        bool keep;
        if (e->age == duration || e->age < duration) keep = !ended;
        else keep = false;
        if (keep || only) {
            if (0.0f < dt) e->delay -= dt;
            if (!(e->delay > 0.0f)) e->delay = 0.0f;
            e->flags |= 1;
            updateActive(*e, dt, subSteps);
            if (only) break;
            if (e->prewarmElapsed < e->prewarm && dt > 0.0f) {
                e->prewarmElapsed += dt;
                continue;
            }
            ++index;
        } else {
            ended = false;
            if (e->callback) {
                auto callback = std::move(e->callback);
                e->callback = nullptr;
                callback(e->handle, 2);
            }
            remove(index);
        }
    }
}

void ParticleSystem::updateActive(ParticleEmitter& e, float dt, unsigned subSteps) {
    const ParticleDef& d = *e.def;
    float t = (d.duration - e.age) / d.duration;
    if (!(0.0f <= t)) t = 0.0f;
    t = 1.0f - t;
    float fade = 1.0f;
    {
        std::size_t k;
        float along;
        if (findSegment(d.fadeKeys, t, k, along)) fade = segmentValue(d.fadeKeys, k, along, 0) * kByteUnit;
    }
    if (d.flags & kFlagFadeSquared) fade = fade * fade;
    e.fade128 = static_cast<std::uint8_t>(static_cast<int>(fade * 128.0f + 0.5f));
    const float uvScale[2] = {1.0f / flt(d.atlasColumns), 1.0f / flt(d.atlasRows)};
    const int cells = d.frameMode == 2 ? int(d.atlasColumns) * int(d.atlasRows) : 0;

    float accumulator = e.accumulator + dt * d.rate * e.rateScale;
    int created = static_cast<int>(std::floor(accumulator));
    const float nextAge = e.age + dt;
    if (!(nextAge <= d.duration) || nextAge == d.duration) {
        created = 0;
    } else if (created > 0 && e.callback) {
        e.callback(e.handle, 1);
    }

    const bool simple = created < 1 || (e.flags & kEmitterNoSpawn) || subSteps < 2;
    if (simple) {
        e.accumulator = accumulator - static_cast<float>(created);
        for (std::size_t i = 0; i < e.particles.size(); ++i) {
            Particle& p = e.particles[i];
            if (created != 0 && p.age == -1.0f && !(e.flags & kEmitterNoSpawn)) {
                initParticle(e, p, fade);
                --created;
            }
            stepParticle(e, p, dt, fade, cells, uvScale);
        }
        return;
    }

    std::size_t firstFree = 0;
    bool haveFree = false;
    for (std::size_t i = 0; i < e.particles.size(); ++i) {
        stepParticle(e, e.particles[i], e.rateScale * dt, fade, cells, uvScale);
        if (!haveFree && e.particles[i].age == -1.0f) {
            haveFree = true;
            firstFree = i;
        }
    }
    const ParticleVec3 savedPosition = e.localPosition;
    float elapsed = 0.0f;
    float acc = e.accumulator;
    std::size_t slot = firstFree;
    for (unsigned s = 0; s < subSteps; ++s) {
        const float part = dt / static_cast<float>(subSteps);
        acc += part * d.rate * e.rateScale;
        int batch = static_cast<int>(std::floor(acc));
        elapsed += part;
        acc -= static_cast<float>(batch);
        if (batch != 0 && haveFree) {
            const float along = static_cast<float>(s + 1) / static_cast<float>(subSteps);
            for (std::size_t c = 0; c < 3; ++c)
                e.localPosition[c] = e.previousPosition[c] + (savedPosition[c] - e.previousPosition[c]) * along;
            while (slot < e.particles.size() && batch > 0) {
                Particle& p = e.particles[slot];
                if (p.age == -1.0f) {
                    initParticle(e, p, fade);
                    --batch;
                    stepParticle(e, p, dt - elapsed, fade, cells, uvScale);
                }
                ++slot;
            }
        }
    }
    e.accumulator = acc;
    e.localPosition = savedPosition;
}

void ParticleSystem::initParticle(ParticleEmitter& e, Particle& p, float alpha) {
    const ParticleDef& d = *e.def;
    if (e.delay > 0.0f) return;
    const float uScale = 1.0f / flt(d.atlasColumns), vScale = 1.0f / flt(d.atlasRows);

    const auto offset = [&](float extent) { return extent * (lcgUnit(e) - 0.5f) * 2.0f; };
    if ((e.flags & kEmitterPositioned) == 0) {
        p.position = {0, 0, 0};
        const ParticleVec3 extents = (e.flags & kEmitterBoxOverride) ? e.boxOverride : d.box;
        for (std::size_t c = 0; c < 3; ++c)
            if (extents[c] != 0.0f) p.position[c] = offset(extents[c]);
        p.position = rotate(p.position, e.basis);
    } else {
        p.position = e.localPosition;
        const ParticleVec3 extents = (e.flags & kEmitterBoxOverride) ? e.boxOverride : d.box;
        for (std::size_t c = 0; c < 3; ++c)
            if (extents[c] != 0.0f) p.position[c] = p.position[c] + offset(extents[c]);
    }

    if (d.sway > 0.0f) {
        const float r = lcgUnit(e);
        p.packed = ((static_cast<std::uint32_t>(static_cast<int>(r * kParticleTurn)) & 0xffffffu) << 6) | (p.packed & 0xc0000000u);
    } else {
        p.packed &= 0xc0000000u;
    }

    float angleUnits = 0.0f;
    if (d.spinMode != 0) {
        angleUnits = lcgUnit(e) * kParticleTurn;
        p.angle = angleUnits;
    } else {
        p.angle = 0.0f;
    }
    p.drawAngle = static_cast<std::int16_t>(static_cast<int>(angleUnits));
    if (e.spinRange > 0.0f) {
        p.angle = static_cast<float>(static_cast<int>(angleUnits) & 0xffff);
        bool negative = d.spinMode == 2;
        if (d.spinMode == 3) negative = lcgUnit(e) < 0.5f;
        const float magnitude = d.spinMin + e.spinRange * lcgRaw(e) * kUnit16;
        p.spin = (negative ? -magnitude : magnitude) * 100.0f;
    }

    if (d.sizeRandomX > 0.0f || d.sizeRandomY > 0.0f) {
        const float r = lcgUnit(e);
        p.sizeX = e.sizeX + r * d.sizeRandomX * e.scale;
        p.sizeY = e.sizeY + r * d.sizeRandomY * e.scale;
    } else {
        p.sizeX = e.sizeX;
        p.sizeY = e.sizeY;
    }

    const auto& colourKeys = d.colourKeys;
    if ((d.flags & kFlagRandomGradient) == 0) {
        if (!colourKeys.empty()) std::memcpy(p.colour.data(), colourKeys[0].value.data(), 4);
    } else {
        const float t = lcgUnit(e);
        std::size_t k;
        float along;
        if (findSegment(colourKeys, t, k, along)) {
            for (unsigned c = 0; c < 3; ++c) p.colour[c] = static_cast<std::uint8_t>(static_cast<int>(segmentValue(colourKeys, k, along, c) * alpha));
        }
    }
    p.colour[3] = static_cast<std::uint8_t>(static_cast<int>(flt(d.alphaKeys.empty() ? 0 : d.alphaKeys[0].value[0]) * alpha));

    float azimuth, elevationUnits;
    if (d.spiralStep == 0.0f) {
        const float first = lcgUnit(e);
        const float second = lcgUnit(e);
        azimuth = first * kParticleTurn;
        elevationUnits = second * d.elevation;
    } else {
        azimuth = e.spiral + d.spiralStep * kDegreesToUnits;
        e.spiral = azimuth;
        if (azimuth <= kParticleTurn) {
            if (!(0.0f <= azimuth)) {
                azimuth += kParticleTurn;
                e.spiral = azimuth;
            }
        } else {
            azimuth -= kParticleTurn;
            e.spiral = azimuth;
        }
        elevationUnits = d.elevation;
    }
    p.life = d.lifetimeMin + (d.lifetimeMax - d.lifetimeMin) * lcgRaw(e) * kUnit16;
    const float elevation = static_cast<float>(static_cast<int>(elevationUnits * kDegreesToUnits));
    const float sinEl = std::sin(turnsToRadians(elevation)), cosEl = std::cos(turnsToRadians(elevation));
    const float azimuthUnits = static_cast<float>(static_cast<int>(azimuth));
    const float sinAz = std::sin(turnsToRadians(azimuthUnits)), cosAz = std::cos(turnsToRadians(azimuthUnits));
    const ParticleVec3 direction = {sinEl * sinAz, sinEl * cosAz, cosEl};
    p.velocity = rotate(direction, e.basis);

    float speed;
    if (d.endSpeed > 0.0f) {
        float t = (d.duration - e.age) / d.duration;
        if (!(0.0f <= t)) t = 0.0f;
        speed = e.speedEnd + (e.speedStart - e.speedEnd) * t;
    } else {
        speed = e.speedStart;
    }
    if (d.speedVariation != 0) {
        const float variation = gameRandomUnit(random_);
        speed = ((variation * 2.0f - 1.0f) * speed * flt(d.speedVariation)) / 100.0f + speed;
    }
    for (float& component : p.velocity) component *= speed;
    if (d.flags & kFlagPlanar) p.velocity[2] = 0.0f;
    if (d.flags & kFlagTumble) {
        const float r = lcgUnit(e);
        std::uint32_t sum = static_cast<std::uint32_t>(static_cast<int>(r * 500.0f + 250.0f)) + e.tumbleCounter;
        e.tumbleCounter = static_cast<std::uint16_t>(sum);
        if ((sum & 0xffffu) > 999) e.tumbleCounter = static_cast<std::uint16_t>(e.tumbleCounter - 1000);
        p.tumble = static_cast<std::int16_t>(e.tumbleCounter);
    }
    if (d.flags & kFlagImplode) {
        for (std::size_t c = 0; c < 3; ++c) p.position[c] = p.velocity[c] * p.life;
        for (float& component : p.velocity) component = -component;
    }
    if (d.spinMode == 5) {
        float x, y;
        if (d.flags & kFlagVelocityXz) {
            x = p.velocity[0];
            y = p.velocity[2];
        } else {
            const std::array<ParticleVec3, 3> axes = (d.flags & kFlagFlatEmitterAxes) ? e.basis : std::array<ParticleVec3, 3>{camera.right, camera.up, camera.forward};
            const ParticleVec3 inCamera = project(p.velocity, axes);
            x = inCamera[0];
            y = inCamera[1];
        }
        float squared = x * x;
        squared += y * y;
        const float length = std::sqrt(squared);
        std::uint32_t bits;
        std::memcpy(&bits, &length, sizeof bits);
        if (bits <= 0x34000000u) {
            x = 0.0f;
            y = 1.0f;
        } else {
            const float inverse = 1.0f / length;
            x *= inverse;
            y *= inverse;
        }
        float angle = (std::atan2(y, x) + 4.712389f) * 0.159155f * kParticleTurn;
        if (angle >= kParticleTurn) angle -= kParticleTurn;
        if (!(0.0f <= angle)) angle += kParticleTurn;
        p.angle = angle;
        p.drawAngle = static_cast<std::int16_t>(static_cast<int>(angle));
    }

    float firstU, secondU, firstV, secondV;
    if (d.frameMode == 1) {
        const float col = std::floor(lcgUnit(e) * flt(d.atlasColumns));
        const float row = std::floor(lcgUnit(e) * flt(d.atlasRows));
        firstU = col * uScale;
        secondU = uScale + col * uScale;
        firstV = row * vScale;
        secondV = vScale + firstV;
    } else if (d.frameMode == 2) {
        firstU = 0.0f;
        secondU = uScale;
        firstV = 0.0f;
        secondV = vScale;
    } else if (d.frameMode == 3) {
        const unsigned counter = e.frameCounter;
        const float col = static_cast<float>(counter % d.atlasColumns);
        const float rowIndex = static_cast<float>(counter / d.atlasColumns);
        ++e.frameCounter;
        if (static_cast<unsigned>(d.atlasColumns) * d.atlasRows <= e.frameCounter) e.frameCounter = 0;
        firstV = rowIndex * vScale;
        secondU = uScale + col * uScale;
        secondV = vScale + firstV;
        firstU = col * uScale;
    } else {
        firstU = 0.0f;
        secondU = 1.0f;
        firstV = 0.0f;
        secondV = 1.0f;
    }

    p.packed &= 0x3fffffffu;
    const std::size_t slotIndex = static_cast<std::size_t>(&p - e.particles.data());
    const unsigned flipU = d.flipModes & 0xf, flipV = d.flipModes >> 4;
    bool turnU = flipU == 3;
    if (flipU == 1) turnU = lcgUnit(e) < 0.5f;
    else if (flipU == 2) turnU = (slotIndex & 1) != 0;
    float uFirst = firstU, uSecond = secondU;
    if (turnU) {
        p.packed |= 0x40000000u;
        uFirst = secondU;
        uSecond = firstU;
    }
    bool turnV = flipV == 3;
    if (flipV == 1) turnV = lcgUnit(e) < 0.5f;
    else if (flipV == 2) turnV = (slotIndex & 1) != 0;
    float vFirst = firstV, vSecond = secondV;
    if (turnV) {
        p.packed |= 0x80000000u;
        vFirst = secondV;
        vSecond = firstV;
    }
    p.uv[0] = packUv(uFirst);
    p.uv[1] = packUv(uSecond);
    p.uv[2] = packUv(vFirst);
    p.uv[3] = packUv(vSecond);
    p.age = 0.0f;
}

void ParticleSystem::stepParticle(ParticleEmitter& e, Particle& p, float dt, float fade, int cells, const float uvScale[2]) {
    if (!ageIsLive(p.age)) return;
    const ParticleDef& d = *e.def;
    for (std::size_t c = 0; c < 3; ++c) p.position[c] += p.velocity[c] * dt;
    if (d.bounce > 0.0f) {
        const float worldY = p.position[1] + e.position[1];
        if (worldY <= e.ground && p.velocity[1] < 0.0f) {
            p.velocity[1] = -(p.velocity[1] * d.bounce);
            p.position[1] = e.ground - e.position[1];
        }
    }
    std::uint32_t dragBits;
    const float dragAbs = std::fabs(e.drag);
    std::memcpy(&dragBits, &dragAbs, sizeof dragBits);
    if (dragBits > 0x34000000u) {
        float factor = e.drag * dt;
        if (isFloatAtLeastOne(factor)) factor = 1.0f;
        const float scaleBy = 1.0f - factor;
        for (float& component : p.velocity) component *= scaleBy;
        if (d.flags & kFlagSpinDamping) p.spin = p.spin * (1.0f - factor);
    }
    if (e.spinRange > 0.0f) {
        p.angle = p.angle + p.spin;
        wrapAngle(p.angle);
        p.drawAngle = static_cast<std::int16_t>(static_cast<int>(p.angle));
    }
    if (d.sway > 0.0f) {
        std::uint32_t phase = static_cast<std::uint32_t>(static_cast<int>(d.swayRate * 6400.0f)) + (p.packed & 0x3fffffffu);
        if (phase > 0x7cfffu) phase = 0;
        p.packed = (phase & 0x3fffffffu) | (p.packed & 0xc0000000u);
    }
    const float f = p.age / p.life;
    {
        std::size_t k;
        float along;
        if (findSegment(d.sizeKeys, f, k, along)) {
            const float g = ((segmentValue(d.sizeKeys, k, along, 0)) - 128.0f) * 0.0078125f * dt;
            p.sizeX = p.sizeX + e.growX * g;
            p.sizeY = p.sizeY + e.growY * g;
            if (d.flags & kFlagSizeClamp) {
                if (p.sizeX <= 0.0f) p.sizeX = 0.0f;
                if (!(p.sizeY > 0.0f)) p.sizeY = 0.0f;
            }
        }
    }
    p.velocity[1] = p.velocity[1] + e.gravity * dt;
    if (d.frameMode == 2) {
        const float loops = flt(d.animationLoops);
        const float whole = std::floor(loops * f);
        const float cell = (loops * f - whole) * static_cast<float>(cells);
        const unsigned index = static_cast<unsigned>(static_cast<int>(cell));
        const unsigned columns = d.atlasColumns;
        const float col = static_cast<float>(index % columns) * uvScale[0];
        const float row = static_cast<float>(static_cast<unsigned>(static_cast<int>(cell / flt(d.atlasColumns)))) * uvScale[1];
        float uA = col, uB = uvScale[0] + col, vA = row, vB = uvScale[1] + row;
        float first = uA, second = uB;
        if (p.packed & 0x40000000u) {
            first = uB;
            second = uA;
        }
        float vFirst = vA, vSecond = vB;
        if (p.packed & 0x80000000u) {
            vFirst = vB;
            vSecond = vA;
        }
        p.uv[0] = packUv(first);
        p.uv[1] = packUv(second);
        p.uv[2] = packUv(vFirst);
        p.uv[3] = packUv(vSecond);
    }
    p.age = p.age + dt;
    if (p.life <= p.age) p.age = (d.flags & kFlagFinishedMinusTwo) ? -2.0f : -1.0f;
    if ((d.flags & kFlagRandomGradient) == 0) {
        std::size_t k;
        float along;
        if (findSegment(d.colourKeys, f, k, along)) {
            for (unsigned c = 0; c < 3; ++c) p.colour[c] = static_cast<std::uint8_t>(static_cast<int>(segmentValue(d.colourKeys, k, along, c)));
        }
    }
    {
        std::size_t k;
        float along;
        if (findSegment(d.alphaKeys, f, k, along)) p.colour[3] = static_cast<std::uint8_t>(static_cast<int>(segmentValue(d.alphaKeys, k, along, 0) * fade));
    }
}

std::vector<ParticleBatch> ParticleSystem::buildDraw(const ParticleCamera& camera) const {
    std::vector<ParticleBatch> batches;
    for (const auto& owned : emitters_) {
        const ParticleEmitter& e = *owned;
        const ParticleDef& d = *e.def;
        if (d.particles == 0 || (d.flags & (kFlagAlphaOnly | kFlagPlanar))) continue;
        ParticleBatch batch;
        batch.texture = d.texture;
        batch.blendMode = d.blendMode;
        batch.alphaTest = (d.flags & kFlagAlphaTest) != 0;
        batch.alphaReference = flt(e.fade128);
        const bool billboard = d.spinMode == 4 || (d.flags & 0x1050u) == 0;
        std::vector<std::size_t> order(e.particles.size());
        for (std::size_t i = 0; i < order.size(); ++i) order[i] = order.size() - 1 - i;
        const auto depthOf = [&](const Particle& p) {
            float depth = 0;
            for (std::size_t c = 0; c < 3; ++c) depth += (p.position[c] + e.position[c] - camera.position[c]) * camera.forward[c];
            return depth;
        };
        if (billboard && (d.flags & kFlagSorted)) {
            order.assign(e.particles.size(), 0);
            for (std::size_t i = 0; i < order.size(); ++i) order[i] = i;
            std::stable_sort(order.begin(), order.end(), [&](std::size_t a, std::size_t b) { return depthOf(e.particles[a]) > depthOf(e.particles[b]); });
        } else if (!billboard) {
            for (std::size_t i = 0; i < order.size(); ++i) order[i] = i;
        }
        const float nearStart = nearFadeStart_, nearScale = defs_.scale;
        for (const std::size_t slot : order) {
            const Particle& p = e.particles[slot];
            if (!ageIsLive(p.age)) continue;
            std::array<std::array<float, 3>, 4> corner{};
            std::uint8_t alphaOut;
            if (billboard) {
                float fadeNear = 1.0f;
                if ((d.flags & kFlagNoNearFade) == 0) {
                    const float z = depthOf(p);
                    const float scaled = (z - nearStart) * nearScale;
                    std::int32_t bits;
                    std::memcpy(&bits, &scaled, sizeof bits);
                    if (bits < 0x3f800000) fadeNear = scaled <= 0.0f ? 0.0f : scaled;
                }
                const int alphaInt = static_cast<int>((flt(p.colour[3]) * fadeNear * flt(e.alpha)) / 255.0f + 0.5f);
                if (alphaInt == 0) continue;
                alphaOut = static_cast<std::uint8_t>(alphaInt);
                int angleUnits = static_cast<std::uint16_t>(p.drawAngle);
                if (d.spinMode == 4) {
                    const ParticleVec3 inCamera = project(p.velocity, {camera.right, camera.up, camera.forward});
                    float x = inCamera[0], y = inCamera[1];
                    float squared = x * x;
                    squared += y * y;
                    const float length = std::sqrt(squared);
                    std::uint32_t bits;
                    std::memcpy(&bits, &length, sizeof bits);
                    if (bits <= 0x34000000u) {
                        x = 0.0f;
                        y = 1.0f;
                    } else {
                        const float inverse = 1.0f / length;
                        x *= inverse;
                        y *= inverse;
                    }
                    const float angle = (std::atan2(y, x) + 4.712389f) * 0.159155f * kParticleTurn;
                    unsigned units = angle > 0.0f ? static_cast<unsigned>(angle) : 0u;
                    if (units > 7999u) units -= 8000u;
                    angleUnits = static_cast<int>(units);
                }
                const auto offsets = cornerOffsets(p, d, angleUnits);
                float cx = p.position[0], cy = p.position[1], cz = p.position[2];
                if (d.sway > 0.0f) {
                    const float phase = static_cast<float>((p.packed & 0x3fffffffu) >> 6);
                    const float swayOffset = std::sin((phase / kParticleTurn) * kToRadians);
                    cx = cx + d.sway * swayOffset * camera.right[0];
                    cz = cz + d.sway * swayOffset * camera.right[2];
                }
                for (std::size_t k = 0; k < 4; ++k) {
                    const float a = offsets[k][0], b = offsets[k][1];
                    corner[k][0] = cx + a * camera.right[0] + b * camera.up[0];
                    corner[k][1] = cy + a * camera.right[1] + b * camera.up[1];
                    corner[k][2] = cz + a * camera.right[2] + b * camera.up[2];
                }
            } else {
                alphaOut = static_cast<std::uint8_t>((static_cast<int>(p.colour[3]) * static_cast<int>(e.alpha)) / 255);
                if (p.colour[3] == 0) continue;
                const auto offsets = cornerOffsets(p, d, p.drawAngle);
                for (std::size_t k = 0; k < 4; ++k) {
                    ParticleVec3 v = {offsets[k][0], 0.0f, offsets[k][1]};
                    if (d.flags & kFlagTumble) {
                        const float tumble = static_cast<float>(p.tumble) * 0.001f * 3.1415927f;
                        const float s = std::sin(tumble), c = std::cos(tumble);
                        const std::array<ParticleVec3, 3> matrix = {{{c, s, 0.0f}, {-(s * c), c * c, s}, {s * s, -(s * c), c}}};
                        v = rotate(v, matrix);
                    } else if (d.flags & kFlagFlatEmitterAxes) {
                        v = rotate({offsets[k][0], offsets[k][1], 0.0f}, e.basis);
                    }
                    for (std::size_t c = 0; c < 3; ++c) corner[k][c] = v[c] + p.position[c];
                }
            }
            if (alphaOut == 0) continue;
            const float u0 = static_cast<float>(p.uv[0]) * kUvUnit, u1 = static_cast<float>(p.uv[1]) * kUvUnit;
            const float v0 = static_cast<float>(p.uv[2]) * kUvUnit, v1 = static_cast<float>(p.uv[3]) * kUvUnit;
            const float us[4] = {u0, u1, u1, u0}, vs[4] = {v1, v1, v0, v0};
            for (std::size_t k = 0; k < 4; ++k) {
                ParticleVertex vertex{};
                for (std::size_t c = 0; c < 3; ++c) vertex.position[c] = corner[k][c] + e.position[c];
                vertex.uv[0] = us[k];
                vertex.uv[1] = vs[k];
                vertex.rgba[0] = p.colour[0];
                vertex.rgba[1] = p.colour[1];
                vertex.rgba[2] = p.colour[2];
                vertex.rgba[3] = alphaOut;
                batch.vertices.push_back(vertex);
            }
        }
        if (!batch.vertices.empty()) batches.push_back(std::move(batch));
    }
    return batches;
}

}
