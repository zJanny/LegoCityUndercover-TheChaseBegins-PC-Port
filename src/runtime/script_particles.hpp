#pragma once
// The script native PlayParticlesGO 0x27e8b4 (docs/re/props-rocking.md): PlayParticlesGO(definition, object, x, y, z, attach) starts the particle
// definition int(definition) through 0x23de24. With attach > 0 the offset (x, y, z) is given to 0x23de24 with the object's node (entity +0x3c) as the
// parent; otherwise the offset is added to the translation of the node's world matrix (0x23fb78) and the emitter has no parent. No SDL dependency.
#include <array>
#include <cstdint>
#include <optional>
#include <string>

namespace chase::runtime {

constexpr unsigned kPlayParticlesGoArguments = 6;

struct ScriptParticleRequest {
    std::int32_t definition = 0;
    std::string object;              // lower case
    std::array<float, 3> offset{};   // arguments 2..4
    bool attachToNode = false;       // argument 5 > 0
    std::array<float, 3> position{}; // scriptParticlePosition at the call
};

// The arguments of one call, as the handler reads them: the definition truncated to an integer (vcvt.s32.f32), attach for a value above 0.0.
ScriptParticleRequest playParticlesGoRequest(float definition, const std::string& lowerObject, float x, float y, float z, float attach);

// The world position of the emitter: the node translation plus the offset (0x23fb78, then the vector add 0x23d39c).
// STAND-IN SCRIPT-S300: the port gives the object's position source (carry position or level placement), not the node's current world matrix, and an
// attached request (no shipped call has one) starts at the same world position without following the node.
std::array<float, 3> scriptParticlePosition(const ScriptParticleRequest& request, const float nodePosition[3]);

}
