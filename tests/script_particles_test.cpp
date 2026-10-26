// The script native PlayParticlesGO 0x27e8b4 with made-up scripts and objects (docs/re/props-rocking.md, read by 20261026-mission13-B with llvm-objdump
// on 2026-10-26): the definition is int(argument 0), the offset (arguments 2..4) is added to the object's node translation (0x23fb78, 0x23d39c),
// argument 5 above 0.0 asks for the node as parent.
#include "runtime/script_particles.hpp"
#include "runtime/script_runner.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using chase::assets::Level;
using chase::assets::LevelScript;
using chase::assets::ScriptCall;
using chase::runtime::ScriptParticleRequest;
using chase::runtime::ScriptRunner;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

void add(Level& level, const std::string& name, std::vector<ScriptCall> calls) {
    LevelScript script;
    script.nameHash = chase::assets::nameHash(name);
    script.calls = std::move(calls);
    level.scripts.push_back(std::move(script));
}

void requestFields() {
    const ScriptParticleRequest truncated = chase::runtime::playParticlesGoRequest(264.9f, "girder", 0, 0, 0, 0);
    require(truncated.definition == 264 && !truncated.attachToNode, "vcvt.s32.f32 truncates the definition; 0 is not above 0.0");
    require(chase::runtime::playParticlesGoRequest(-1.5f, "x", 0, 0, 0, 0).definition == -1, "truncation towards zero");
    require(chase::runtime::playParticlesGoRequest(10, "x", 0, 0, 0, 0.5f).attachToNode, "0.5 is above 0.0: the node is the parent (0x27e8f0 movgt)");
    require(!chase::runtime::playParticlesGoRequest(10, "x", 0, 0, 0, -1).attachToNode, "a negative value is not above 0.0");
    const ScriptParticleRequest offset = chase::runtime::playParticlesGoRequest(10, "exppos01", 2.3f, 0, -0.1f, 0);
    const float node[3] = {10, 20, 30};
    const auto position = chase::runtime::scriptParticlePosition(offset, node);
    require(position[0] == 12.3f && position[1] == 20 && position[2] == 29.9f, "the offset is added to the node translation (0x23d39c)");
}

void nativeQueuesStarts() {
    Level level;
    add(level, "fx", {{"PlayParticlesGO", {"264", "Crane_Girder", "0", "0", "0", "0"}},
                      {"PlayParticlesGO", {"95", "Debris", "1", "2", "3", "0"}},
                      {"PlayParticlesGO", {"56", "Unknown", "0", "0", "0", "0"}},
                      {"PlayParticlesGO", {"56", "Crane_Girder"}}});
    chase::assets::PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.setObjectPositionSource([](const std::string& name, float out[3]) {
        if (name == "crane_girder") out[0] = 947.883f, out[1] = 27.481f, out[2] = 1142.455f;
        else if (name == "debris") out[0] = 1003.56f, out[1] = 31.0769f, out[2] = 1075.84f;
        else return false;
        return true;
    });
    require(runner.start("fx"), "the script starts");
    const std::vector<ScriptParticleRequest> requests = runner.takeParticleRequests();
    require(requests.size() == 2, "the two calls on known objects start emitters; an unknown object and a short call start none, got " +
                                      std::to_string(requests.size()));
    require(requests[0].definition == 264 && requests[0].object == "crane_girder" && requests[0].position[1] == 27.481f, "the girder's position");
    require(requests[1].definition == 95 && requests[1].position[0] == 1004.56f && requests[1].position[2] == 1078.84f, "position plus offset");
    require(runner.takeParticleRequests().empty(), "taken once");
    require(runner.unhandledCalls("playparticlesgo") == 0, "PlayParticlesGO is handled");
}

}

int main() {
    try {
        requestFields();
        nativeQueuesStarts();
    } catch (const std::exception& error) {
        std::cerr << "script-particles-test: FAIL: " << error.what() << '\n';
        return 1;
    }
    std::cout << "script-particles-test: ok\n";
    return 0;
}
