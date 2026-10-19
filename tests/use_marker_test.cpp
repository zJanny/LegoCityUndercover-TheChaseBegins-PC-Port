// Use markers (src/runtime/use_marker.cpp, docs/re/use-spot.md): the fader slot of 0x23cb90 / 0x23cc00 / 0x235e2c with
// fade in and out of 0.15 s and a hold of 0.1 s, the range 25 m of 0x1c015c / 0x172254 and the quad of 0x1b29f0 (corners
// +-1 at 0.1 above the ground, spinning at clock / 240 radians). The candidates are made up.
#include "runtime/use_marker.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using chase::runtime::UseMarkerCandidate;
using chase::runtime::UseMarkers;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

bool near(float a, float b) { return std::fabs(a - b) < 1e-4f; }

UseMarkerCandidate spot(bool usable) {
    UseMarkerCandidate candidate;
    candidate.instance = "spot";
    candidate.matrix = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 10, 2, 20, 1};
    candidate.ground = {10, 2, 20};
    candidate.usable = usable;
    return candidate;
}

void fadesInWhileUsableAndOutAfterTheHold() {
    UseMarkers markers;
    const float camera[3] = {10, 8, 30}, player[3] = {10, 2, 21};
    markers.update({spot(true)}, camera, player, 0.05f);
    require(near(markers.alpha("spot"), 0.0f), "the first show starts the fade in");
    markers.update({spot(true)}, camera, player, 0.05f);
    require(near(markers.alpha("spot"), 0.05f / 0.15f), "fade in over 0.15 s");
    for (int i = 0; i < 3; ++i) markers.update({spot(true)}, camera, player, 0.05f);
    require(markers.alpha("spot") == 1.0f, "shown after the fade in");
    markers.update({spot(false)}, camera, player, 0.06f);
    require(markers.alpha("spot") == 1.0f, "the hold of 0.1 s keeps it");
    markers.update({spot(false)}, camera, player, 0.06f);
    require(markers.alpha("spot") == 1.0f, "the fade out starts when the hold runs out");
    markers.update({spot(false)}, camera, player, 0.075f);
    require(near(markers.alpha("spot"), 0.5f), "half way out after 0.075 s");
    markers.update({spot(false)}, camera, player, 0.075f);
    require(markers.alpha("spot") == 0.0f && markers.quads().empty(), "the slot is freed below alpha 4 / 255");
}

void rangeOfCameraOrPlayer() {
    UseMarkers markers;
    const float far[3] = {100, 2, 100}, nearPoint[3] = {10, 2, 40};
    markers.update({spot(true)}, far, far, 0.2f);
    markers.update({spot(true)}, far, far, 0.2f);
    require(markers.quads().empty(), "no marker beyond 25 m of camera and player");
    markers.update({spot(true)}, nearPoint, far, 0.2f);
    require(markers.quads().size() == 1, "the camera within 25 m is enough");
    UseMarkers byPlayer;
    byPlayer.update({spot(true)}, far, nearPoint, 0.2f);
    require(byPlayer.quads().size() == 1, "the player within 25 m is enough");
}

void quadOnTheGroundSpins() {
    UseMarkers markers;
    const float camera[3] = {10, 8, 30};
    markers.update({spot(true)}, camera, camera, 0.0f);
    auto quads = markers.quads();
    require(quads.size() == 1 && quads[0].texture == 0, "one quad with useGeneric");
    const auto& v = quads[0].vertices;
    require(near(v[0].position[0], 9) && near(v[0].position[1], 2.1f) && near(v[0].position[2], 19) && v[0].uv[0] == 1 && v[0].uv[1] == 0,
            "corner (-1, 0.1, -1) with uv (1, 0) at clock 0");
    require(near(v[2].position[0], 11) && near(v[2].position[2], 21) && v[2].uv[0] == 0 && v[2].uv[1] == 1, "corner (1, 0.1, 1) with uv (0, 1)");
    const float quarterTurnSeconds = float(M_PI) / 2 * chase::runtime::kUseMarkerSpinDivisor / chase::runtime::kUseMarkerClockUnitsPerSecondStandIn;
    markers.update({spot(true)}, camera, camera, quarterTurnSeconds);
    quads = markers.quads();
    const auto& turned = quads[0].vertices;
    require(near(turned[0].position[0], 9) && near(turned[0].position[2], 21), "a quarter turn moves corner 0 from (-1, -1) to (-1, 1)");
}
}

int main() {
    try {
        fadesInWhileUsableAndOutAfterTheHold();
        rangeOfCameraOrPlayer();
        quadOnTheGroundSpins();
    } catch (const std::exception& error) {
        std::cerr << "use-marker-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "use-marker-test: ok\n";
    return 0;
}
