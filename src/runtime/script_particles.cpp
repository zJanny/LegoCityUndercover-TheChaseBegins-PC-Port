#include "runtime/script_particles.hpp"

namespace chase::runtime {

ScriptParticleRequest playParticlesGoRequest(float definition, const std::string& lowerObject, float x, float y, float z, float attach) {
    ScriptParticleRequest request;
    request.definition = static_cast<std::int32_t>(definition);
    request.object = lowerObject;
    request.offset = {x, y, z};
    request.attachToNode = attach > 0.0f;
    return request;
}

std::array<float, 3> scriptParticlePosition(const ScriptParticleRequest& request, const float nodePosition[3]) {
    return {nodePosition[0] + request.offset[0], nodePosition[1] + request.offset[1], nodePosition[2] + request.offset[2]};
}

}
