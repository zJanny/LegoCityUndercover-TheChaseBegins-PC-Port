// Build-it state machine (src/runtime/buildit.cpp). The expected rows are the record fields of the object 1-1_BI_DonutVanParts read in
// the oracle after every update with the action button held from the state m11-4-van-parts (command and result: docs/re/buildit.md,
// entry "Build of 1-1_BI_DonutVanParts measured"): row i is the state after i updates, each update being the object update followed
// by the use message. Six bricks. turnedZone: the use zone is tested in the node's frame (0x221570 turns the player with the node's world matrix,
// 0x2536c0, before the box test 0x22f650); read from the code, entry "The use zone is turned with the placement" of docs/re/buildit.md.
#include "assets/assets.hpp"
#include "runtime/buildit.hpp"
#include "runtime/buildit_set.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>

namespace {
using namespace chase::runtime;

int failures = 0;

void require(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

struct Row {
    int update;
    int state, target, bit;
    float progressOfBit;
    float rate;
    float spin;
};

const Row kRows[] = {
    {0, 2, 2, -1, 0.0f, 0.1f, 0.0f},      {1, 2, 8, -1, 0.0f, 0.1f, 0.0f},      {2, 8, 8, 0, 0.0f, 0.1f, 0.0f},
    {3, 8, 8, 0, 0.1f, 0.106f, 0.0f},     {4, 8, 8, 0, 0.21f, 0.112f, 0.0f},    {10, 8, 8, 0, 0.97f, 0.148f, 0.0f},
    {11, 8, 8, 0, 1.12f, 0.154f, 0.0f},   {12, 9, 9, 0, 1.0f, 0.154f, 0.0f},    {13, 2, 8, 0, 1.0f, 0.1f, 0.0f},
    {14, 8, 8, 1, 0.0f, 0.1f, 0.0f},      {25, 2, 8, 1, 1.0f, 0.1f, 0.0f},      {38, 8, 8, 3, 0.0f, 0.1f, 0.0f},
    {72, 9, 9, 5, 1.0f, 0.154f, 0.0f},    {73, 10, 10, 5, 1.0f, 0.154f, 0.0f},  {74, 10, 10, 5, 1.0f, 0.154f, 12.0f},
    {81, 10, 10, 5, 1.0f, 0.154f, 96.0f}, {82, 10, 10, 5, 1.0f, 0.154f, 120.0f}, {84, 10, 10, 5, 1.0f, 0.154f, 168.0f},
    {85, 2, 2, -1, 0.0f, 0.1f, 0.0f},
};

bool near(float a, float b) { return std::fabs(a - b) < 0.0006f; }

void replay() {
    BuilditState build(6);
    std::size_t next = 0;
    int brickStarts = 0, brickDones = 0, reveals = 0, builts = 0, revealAt = -1, builtAt = -1;
    for (int update = 0; update <= 90; ++update) {
        if (update > 0) {
            for (const BuilditEvent event : builditUpdate(build)) {
                if (event == BuilditEvent::BrickStarted) ++brickStarts;
                if (event == BuilditEvent::BrickDone) ++brickDones;
                if (event == BuilditEvent::FinalPropShown) ++reveals, revealAt = update;
                if (event == BuilditEvent::Built) ++builts, builtAt = update;
            }
            if (!build.built) builditUse(build, true);
        }
        if (next < sizeof kRows / sizeof kRows[0] && kRows[next].update == update) {
            const Row& row = kRows[next++];
            const std::string where = "update " + std::to_string(update);
            require(build.state == row.state, where + " state " + std::to_string(build.state));
            require(build.target == row.target, where + " target " + std::to_string(build.target));
            require(build.bit == row.bit, where + " bit " + std::to_string(build.bit));
            const float progress = build.bit >= 0 ? build.progress[std::size_t(build.bit)] : 0.0f;
            require(std::fabs(progress - row.progressOfBit) < 0.006f, where + " progress " + std::to_string(progress));
            require(near(build.rate, row.rate), where + " rate " + std::to_string(build.rate));
            require(near(build.spin, row.spin), where + " spin " + std::to_string(build.spin));
        }
        if (update == 85) break;
    }
    require(next == sizeof kRows / sizeof kRows[0], "all rows checked");
    require(brickStarts == 6 && brickDones == 6, "six bricks");
    require(reveals == 1 && revealAt == 73, "final prop shown at update 73");
    require(builts == 1 && builtAt == 85, "built at update 85");
    require(build.built, "built flag");
}

void releasedButton() {
    BuilditState build(6);
    require(builditUse(build, false) == 1 && build.target == 2, "no button, no start");
    require(builditUse(build, true) == 1 && build.target == 8, "button starts the build");
    builditUpdate(build);
    builditUse(build, false);
    for (int i = 0; i < 12; ++i) builditUpdate(build);
    require(build.state == 2 && build.bit == 0, "the brick in progress is finished without the button");
    require(build.target == 2, "the next brick waits for the button");
    BuilditState done(6);
    done.state = done.target = 10;
    require(builditUse(done, true) == 2, "not usable while the final prop rises");
}

void propRise() {
    require(near(builditPropRise(0.0f), 0.0f), "rise at 0");
    require(near(builditPropRise(90.0f), 1.0f), "rise at 90");
    require(near(builditPropRise(180.0f), 0.0f), "rise at 180");
}

void flightAndRise() {
    BuilditState build(6);
    builditUse(build, true);
    const float expectedFlight[] = {0.0f, 0.1f, 0.206f, 0.318f, 0.436f, 0.56f, 0.69f, 0.826f, 0.968f};
    builditUpdate(build);
    require(build.state == 8 && build.bit == 0, "the first update starts the brick");
    int step = 0;
    for (const float expected : expectedFlight) {
        builditUpdate(build);
        require(std::fabs(build.flight[0] - expected) < 0.0006f, "flight at step " + std::to_string(++step) + " " + std::to_string(build.flight[0]));
    }
    require(std::fabs(build.progress[0] - 1.116f) < 0.0006f, "the ninth step ends with progress 1.116 (oracle row of update 11)");
    builditUpdate(build);
    require(build.state == 9 && build.flight[0] == 1.0f, "the change to state 9 places the brick at its end");
    require(build.flight[1] == 0.0f, "the next brick still lies scattered");
    BuilditState rising(1);
    rising.state = rising.target = 10;
    float seen[3];
    for (int i = 0; i < 3; ++i) {
        builditUpdate(rising);
        seen[i] = rising.propRise;
    }
    require(seen[0] == 0.0f && near(seen[1], builditPropRise(12.0f)) && near(seen[2], builditPropRise(24.0f)), "the rise uses the spin before its step");
    rising.state = rising.target = 10;
    rising.spin = 168.0f;
    builditUpdate(rising);
    builditUpdate(rising);
    require(rising.state == 11 || rising.built, "rise ends");
    require(rising.propRise == 0.0f && rising.flight[0] == 0.0f, "the change to state 11 resets the bricks and the rise");
}

void placementYaw(BuilditObject& object, float degrees) {
    chase::assets::Placement placement;
    placement.orientation[1] = degrees;
    const chase::assets::Matrix node = chase::assets::placementMatrix(placement);
    for (int row = 0; row < 3; ++row) std::copy_n(node.begin() + 4 * row, 3, object.zoneAxes + 3 * row);
}

void turnedZone() {
    BuilditObject object;
    object.hasZone = true;
    object.position[0] = 100.0f;
    object.position[1] = 10.0f;
    object.position[2] = -50.0f;
    object.zoneCentre[0] = 3.0f;
    object.zoneCentre[1] = 0.5f;
    object.zoneHalf[0] = 1.0f;
    object.zoneHalf[1] = 2.0f;
    object.zoneHalf[2] = 4.0f;
    const float unturned[3] = {103.5f, 10.5f, -47.0f}, behind[3] = {96.5f, 10.5f, -47.0f};
    require(BuilditSet::inZone(object, unturned) && !BuilditSet::inZone(object, behind), "an unturned node keeps the zone on its +x side");

    placementYaw(object, 180.0f);
    require(!BuilditSet::inZone(object, unturned) && BuilditSet::inZone(object, behind), "a node turned by 180 degrees has the zone on its -x side");
    float centre[3];
    BuilditSet::zoneWorldCentre(object, centre);
    require(near(centre[0], 97.0f) && near(centre[1], 10.5f) && near(centre[2], -50.0f), "the world centre follows the turn");

    placementYaw(object, 90.0f);
    const float turnedCentre[3] = {100.0f, 10.5f, -53.0f}, nearEdge[3] = {100.0f + 3.9f, 10.5f, -53.0f}, acrossEdge[3] = {100.0f + 4.1f, 10.5f, -53.0f};
    BuilditSet::zoneWorldCentre(object, centre);
    require(near(centre[0], turnedCentre[0]) && near(centre[2], turnedCentre[2]), "a quarter turn moves the centre onto -z");
    require(BuilditSet::inZone(object, nearEdge) && !BuilditSet::inZone(object, acrossEdge), "the local z half extent of 4 lies along the world x axis after a quarter turn");
    const float notBeyondLocalX[3] = {100.0f, 10.5f, -53.0f - 1.1f};
    require(!BuilditSet::inZone(object, notBeyondLocalX), "the local x half extent of 1 lies along the world z axis after a quarter turn");
}
}

int main() {
    turnedZone();
    flightAndRise();
    replay();
    releasedButton();
    propRise();
    if (failures) return EXIT_FAILURE;
    std::cout << "buildit-test: ok\n";
    return EXIT_SUCCESS;
}
