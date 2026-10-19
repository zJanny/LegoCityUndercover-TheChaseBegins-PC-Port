// Detective tool rules on made-up objects (src/runtime/detective.cpp, docs/re/abilities-detective.md): footprint placement on hand-evaluated
// paths, the scan, the fade, the ways out of detective mode and the reveal prop's flight. No game data needed.
#include "runtime/detective.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace chase;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

assets::LevelPath straightPath(float x, float z) {
    assets::LevelPath path;
    path.points = {{0, 0, 0}, {x, 0, z}};
    return path;
}

// Hand-evaluated: two points with clamped neighbours give P(t) = A + (B - A) f(t), f(t) = -t^3 + 1.5t^2 + 0.5t. The length sums nine of the
// ten steps (0x293540 starts at the first point and never samples the end), 3 f(0.9) = 2.808, so ceil(2.808 x 1.6666666) = 5 footprints at
// the fractions i / 5, i.e. x = 3 f(i / 5) = 0, 0.456, 1.128, 1.872, 2.544, times 4 truncated; the side offset is -0.45 / +0.45 along
// (tangent z, tangent y, tangent x) = (0, 0, 1).
void footprints() {
    const float origin[3] = {0, 0, 0};
    const std::vector<float> lengths = runtime::detectivePathLengths(straightPath(3, 0), runtime::kPathLengthSteps);
    require(lengths.size() == 2 && std::abs(lengths.back() - 2.808f) < 1e-4f, "path length " + std::to_string(lengths.back()));
    const auto feet = runtime::detectiveFootprints(straightPath(3, 0), origin);
    const std::vector<std::array<std::int16_t, 3>> expected = {{0, 0, -1}, {1, 0, 1}, {4, 0, -1}, {7, 0, 1}, {10, 0, -1}};
    require(feet.size() == expected.size(), "footprint count " + std::to_string(feet.size()));
    for (std::size_t i = 0; i < feet.size(); ++i) require(feet[i].offset == expected[i], "footprint " + std::to_string(i));
    const auto across = runtime::detectiveFootprints(straightPath(0, 3), origin);
    require(across[0].offset[0] == -1 && across[0].offset[2] == 0 && across[1].offset[0] == 1, "a path along z puts the side offset on x");
}

// Hand-evaluated against 0x270b70 / 0x21a600: halfway between the identity and a quarter turn about y the spherical interpolation gives the
// eighth turn (0, sin 22.5, 0, cos 22.5); the position is clamped on an open path; the stored bytes are the components x127 truncated, and the
// matrix 0x248870 makes of them has the rotation rows in floats 0..2, 4..6 and 8..10 and the translation in 12..14.
void orientations() {
    assets::LevelPath path = straightPath(3, 0);
    const float turn = 0.70710678f;
    path.orientations = {{0, 0, 0, 1}, {0, turn, 0, turn}};
    float q[4];
    runtime::detectivePathOrientation(path, 0.0f, q);
    require(q[1] == 0.0f && q[3] == 1.0f, "identity at the first point");
    runtime::detectivePathOrientation(path, 0.5f, q);
    require(std::abs(q[1] - 0.38268343f) < 1e-5f && std::abs(q[3] - 0.92387953f) < 1e-5f && q[0] == 0.0f && q[2] == 0.0f, "eighth turn halfway");
    runtime::detectivePathOrientation(path, 7.0f, q);
    require(std::abs(q[1] - turn) < 1e-6f && std::abs(q[3] - turn) < 1e-6f, "clamped to the last point");
    path.orientations.clear();
    runtime::detectivePathOrientation(path, 0.5f, q);
    require(q[0] == 0.0f && q[1] == 0.0f && q[2] == 0.0f && q[3] == 1.0f, "a path without orientations gives the identity");

    path.orientations = {{0, turn, 0, turn}, {0, turn, 0, turn}};
    const float origin[3] = {0, 0, 0};
    for (const runtime::DetectiveFootprintRecord& record : runtime::detectiveFootprints(path, origin))
        require(record.rotation == std::array<std::int8_t, 4>({0, 89, 0, 89}), "bytes are the components x127 truncated");

    const float place[3] = {1, 2, 3};
    const std::array<float, 16> identity = runtime::detectiveFootprintMatrix({0, 0, 0, 127}, place);
    require(identity[0] == 1.0f && identity[5] == 1.0f && identity[10] == 1.0f && identity[1] == 0.0f && identity[12] == 1.0f && identity[13] == 2.0f &&
                identity[14] == 3.0f && identity[15] == 1.0f,
            "identity bytes give the identity rotation at the translation");
    const std::array<float, 16> quarter = runtime::detectiveFootprintMatrix({0, 89, 0, 89}, place);
    const float yy = 2.0f * (89.0f * 0.00787402f) * (89.0f * 0.00787402f);
    require(std::abs(quarter[0] - (1.0f - yy)) < 1e-5f && std::abs(quarter[2] - yy) < 1e-5f && std::abs(quarter[8] + yy) < 1e-5f &&
                std::abs(quarter[10] - (1.0f - yy)) < 1e-5f && quarter[5] == 1.0f,
            "quarter turn about y: rows (1 - yy, 0, yy) and (-yy, 0, 1 - yy)");
}

runtime::DetectiveSet madeUpSet() {
    runtime::DetectiveSet set;
    runtime::DetectiveSpot spot;
    spot.instance = "spot";
    spot.objectIndex = 0;
    spot.useZone.present = true;
    spot.useZone.centre[1] = 1.0f;
    spot.useZone.half[0] = spot.useZone.half[1] = spot.useZone.half[2] = 1.5f;
    spot.footprintType = 0;
    for (int i = 0; i < 20; ++i) spot.footprints.push_back({{std::int16_t(-4 * i), 0, 0}, {0, 0, 0, 127}, -1, 0});
    set.spots().push_back(spot);
    return set;
}

void pressOnSpot(runtime::DetectiveSet& set, assets::PlacedObjectState& state, runtime::DetectiveInput& input) {
    input.actionPressed = true;
    set.update(state, input);
    input.actionPressed = false;
    for (unsigned i = 0; i < runtime::kDetectiveOutUpdates; ++i) set.update(state, input);
    require(set.playerState() == runtime::DetectivePlayerState::Normal && set.modeActive(), "detective mode after detect_out");
}

void scanAndFade() {
    runtime::DetectiveSet set = madeUpSet();
    assets::PlacedObjectState state;
    runtime::DetectiveInput input;
    input.dt = 0.033f;
    input.forward[0] = -1.0f, input.forward[2] = 0.0f;
    pressOnSpot(set, state, input);
    const runtime::DetectiveSpot& spot = set.spots()[0];
    require(spot.footprints[0].slot == -1, "the footprint under the player is not ahead (dot 0 is not below 0)");
    require(spot.footprints[1].slot == 0 && spot.footprints[1].alpha == 247, "a footprint 1 unit ahead is revealed and faded once in the update");
    require(spot.footprints[2].slot == 1 && spot.footprints[3].slot == -1, "2 ahead is revealed, 3 is outside 2.5");
    input.forward[0] = 1.0f;
    for (int i = 0; i < 30; ++i) set.update(state, input);
    require(spot.footprints[1].slot == 0 && spot.footprints[1].alpha == 7, "alpha 255 - 8 x 31 after 30 more updates");
    set.update(state, input);
    require(spot.footprints[1].slot == -1 && spot.footprints[1].alpha == 0, "freed below 10");
}

void waysOut() {
    {
        runtime::DetectiveSet set = madeUpSet();
        assets::PlacedObjectState state;
        runtime::DetectiveInput input;
        pressOnSpot(set, state, input);
        input.player[0] = 20.0f;
        set.update(state, input);
        require(set.spots()[0].requested == 0, "8 units away from every footprint the spot asks for state 0");
        set.update(state, input);
        require(set.playerState() == runtime::DetectivePlayerState::In && !set.modeActive(), "leaving state 1 puts the tool away");
    }
    {
        runtime::DetectiveSet set = madeUpSet();
        assets::PlacedObjectState state;
        runtime::DetectiveInput input;
        pressOnSpot(set, state, input);
        input.actionPressed = true;
        set.update(state, input);
        require(set.playerState() == runtime::DetectivePlayerState::In && !set.modeActive(), "A with no object taking it puts the tool away");
        input.actionPressed = false;
        set.update(state, input);
        require(set.spots()[0].requested == 0, "the spot sees the cleared bit and asks for state 0");
    }
    {
        runtime::DetectiveSet set = madeUpSet();
        assets::PlacedObjectState state;
        state.disabled.insert("spot");
        runtime::DetectiveInput input;
        input.actionPressed = true;
        set.update(state, input);
        require(set.playerState() == runtime::DetectivePlayerState::Normal, "a disabled spot does not take the press");
    }
}

void revealProp() {
    runtime::DetectiveSet set;
    runtime::RevealProp prop;
    prop.instance = "prop";
    prop.revealObject = "bone";
    prop.triggerObject = "script";
    prop.delayUpdates = 30;
    prop.flightUpdates = 15;
    set.revealProps().push_back(prop);
    assets::PlacedObjectState state;
    runtime::DetectiveInput input;
    set.triggerMessage("prop");
    std::vector<int> triggers;
    int launch = -1;
    for (int update = 1; update <= 80; ++update) {
        for (const runtime::DetectiveAction& action : set.update(state, input))
            if (action.kind == runtime::DetectiveActionKind::Activate && action.instance == "script") triggers.push_back(update);
        if (launch < 0 && set.revealProps()[0].flightMode == 2) launch = update;
    }
    require(launch == 31, "state 1 at 1, launch 30 updates later: " + std::to_string(launch));
    require(triggers.size() == 2 && triggers[0] == 46, "landing trigger 15 updates after the launch");
    require(triggers[1] == 46 + 15, "second trigger after three bounce phases of 5 updates (STAND-IN DETECT-S5)");
}
}

int main() {
    try {
        footprints();
        orientations();
        scanAndFade();
        waysOut();
        revealProp();
    } catch (const std::exception& error) {
        std::cerr << "detective-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "detective-test: ok\n";
    return 0;
}
