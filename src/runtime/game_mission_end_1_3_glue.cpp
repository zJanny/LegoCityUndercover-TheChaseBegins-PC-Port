#include "runtime/script_runner.hpp"

#include <algorithm>
#include <cctype>

namespace chase::runtime {
namespace {

std::string lowerName(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return text;
}

}

void ScriptRunner::playParticlesGoNative(const assets::ScriptCall& call) {
    const auto& args = call.arguments;
    if (args.size() < kPlayParticlesGoArguments) return;
    ScriptParticleRequest request = playParticlesGoRequest(read(args[0]), lowerName(args[1]), read(args[2]), read(args[3]), read(args[4]), read(args[5]));
    float node[3];
    if (!objectPosition_ || !objectPosition_(request.object, node)) return;
    request.position = scriptParticlePosition(request, node);
    particleRequests_.push_back(std::move(request));
}

std::vector<ScriptParticleRequest> ScriptRunner::takeParticleRequests() {
    std::vector<ScriptParticleRequest> taken;
    taken.swap(particleRequests_);
    return taken;
}

}
