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

void textureOfKindAndAbility() {
    using namespace chase::runtime;
    // Provenance: llvm-objdump of 0x292388 on artifacts/game/game.arm.elf (first switch: entity kind to ability, second switch: ability to texture index).
    struct Row { unsigned kind; int ability; };
    const Row kinds[] = {{144, 2}, {145, 2}, {146, 2}, {127, 21}, {182, 25}, {195, 25}, {130, 28}, {150, 28}, {200, 28}, {209, 29}, {124, 30},
                         {233, 30}, {133, 31}, {210, 35}, {190, 38}, {191, 55}, {186, 56}};
    for (const Row& row : kinds) require(useMarkerKindAbility(row.kind) == row.ability, "ability of kind " + std::to_string(row.kind));
    for (unsigned kind : {13u, 36u, 73u, 77u, 110u, 15u, 153u, 165u, 94u, 169u, 171u, 173u, 181u, 231u, 232u, 227u, 58u, 185u, 201u})
        require(useMarkerKindAbility(kind) == kUseMarkerNoAbility, "kind " + std::to_string(kind) + " asks for no ability");
    require(kUseMarkerKinds.size() == 36 && useMarkerKindListed(182) && useMarkerKindListed(165) && !useMarkerKindListed(238) && !useMarkerKindListed(74),
            "the list 0x374640 holds 36 kinds with the detective spot and the use spot, not the reveal spot or the reveal prop");
    const struct { int ability, texture; } textures[] = {{2, 1}, {25, 1}, {26, 2}, {55, 2}, {56, 2}, {27, 3}, {38, 3}, {28, 4}, {29, 5}, {58, 5},
                                                          {21, 6}, {35, 7}, {30, 8}, {60, 8}, {31, 9}, {57, 9}, {3, 0}};
    for (const auto& row : textures) require(useMarkerAbilityTexture(row.ability) == row.texture, "texture of ability " + std::to_string(row.ability));
    require(useMarkerTexture(25, false, true) == 1 && std::string(kUseMarkerTextures[1]) == "usepolice", "the detective spot shows the police disc");
    require(useMarkerTexture(25, true, true) == kUseMarkerLockedTexture && useMarkerTexture(25, false, false) == kUseMarkerLockedTexture,
            "locked in the save or nobody with the ability: the locked disc");
    require(useMarkerTexture(kUseMarkerNoAbility, true, false) == kUseMarkerGenericTexture, "a kind without an ability skips the lock test");
}

void slotKeepsGroundAndFollowsTexture() {
    UseMarkers markers;
    const float camera[3] = {10, 8, 30};
    UseMarkerCandidate first = spot(true);
    first.texture = 1;
    markers.update({first}, camera, camera, 0.05f);
    UseMarkerCandidate later = spot(true);
    later.ground = {50, 50, 50};
    later.texture = 10;
    markers.update({later}, camera, camera, 0.05f);
    const auto quads = markers.quads();
    require(quads.size() == 1 && quads[0].texture == 10, "the texture is read again every update (0x292388 runs in the draw)");
    require(near(quads[0].vertices[0].position[1], 2.1f), "the placement is made once, when the slot is taken (0x172254 marks the entry new)");
    later.usable = false;
    markers.update({later}, camera, camera, 0.01f);
    require(markers.quads()[0].texture == 10, "a fading marker keeps being drawn");
}
}

int main() {
    try {
        fadesInWhileUsableAndOutAfterTheHold();
        rangeOfCameraOrPlayer();
        quadOnTheGroundSpins();
        textureOfKindAndAbility();
        slotKeepsGroundAndFollowsTexture();
    } catch (const std::exception& error) {
        std::cerr << "use-marker-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "use-marker-test: ok\n";
    return 0;
}
