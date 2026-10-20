// Tests of the brick debris (src/runtime/debris.*, src/runtime/prop_destroy.*, assets::whiteMaterialPart). The rules and constants are from
// REA / llvm-objdump of artifacts/game/game.arm.elf: the burst 0x21e2e4 and the spawner 0x19913c (literals 0x199484..0x1994c8), the record update
// 0x180168 (literals 0x180484..0x1804b4), the object update 0x28a4b0, the rotation 0x237c48, 0x20c938, 0x2aa5c8, 0x23aebc and 0x230ae8. The values
// below are worked out by hand from those rules; the random draws are only checked against their ranges.
#include "assets/assets.hpp"
#include "runtime/debris.hpp"
#include "runtime/prop_destroy.hpp"

#include <cmath>
#include <cstdio>

namespace {

using namespace chase::runtime;

int failures = 0;

void expect(bool ok, const char* what) {
    if (!ok) {
        std::printf("FAIL: %s\n", what);
        ++failures;
    }
}

bool near(float a, float b, float tolerance) { return std::fabs(a - b) <= tolerance; }

DebrisLineTest floorAt(float height) {
    return [height](const DebrisVec3& from, const DebrisVec3& to) -> std::optional<DebrisLineHit> {
        if (from[1] < height || to[1] > height) return std::nullopt;
        return DebrisLineHit{{from[0], height, from[2]}, {0, 1, 0}};
    };
}

void colourAndRotation() {
    expect(debrisColourOf({129.9f, 300.0f, -1.0f}) == 0xffff2c81u, "a float3 colour: each component truncated, its low byte kept, alpha 255");
    const std::array<float, 9> identity = debrisRotation(0, 0, 0);
    expect(identity[0] == 1 && identity[4] == 1 && identity[8] == 1 && identity[1] == 0 && identity[6] == 0, "0x237c48 of (0, 0, 0) is the identity");
    const float a = 0.5f;
    const std::array<float, 9> first = debrisRotation(a, 0, 0);
    expect(near(first[4], std::cos(a), 1e-6f) && near(first[5], std::sin(a), 1e-6f) && near(first[7], -std::sin(a), 1e-6f) && near(first[8], std::cos(a), 1e-6f),
           "the first angle turns rows 1 and 2: (0, cos, sin), (0, -sin, cos)");
    const std::array<float, 9> second = debrisRotation(0, a, 0);
    expect(near(second[0], std::cos(a), 1e-6f) && near(second[2], -std::sin(a), 1e-6f) && near(second[6], std::sin(a), 1e-6f),
           "the second angle: row 0 (cos, 0, -sin), row 2 (sin, 0, cos)");
}

void burstOnFloor() {
    GameRandom random(1234);
    DebrisField field(random);
    DebrisBurst burst;
    burst.position = {5.0f, 10.0f, -3.0f};
    burst.colour = 0xff818181u;
    field.burst(burst, {}, floorAt(10.0f));
    int flying = 0;
    for (unsigned i = 0; i < kDebrisRecordCount; ++i) {
        const DebrisRecord& r = field.records()[i];
        if (r.state != DebrisState::Flying) continue;
        ++flying;
        expect(i < 6, "the first burst takes the free records in index order");
        expect(r.position[0] == 5.0f && r.position[1] == 10.5f && r.position[2] == -3.0f, "0x21e2e4 adds 0.5 to y");
        expect(r.velocity[0] >= -0.1f && r.velocity[0] < 0.1f && r.velocity[2] >= -0.1f && r.velocity[2] < 0.1f,
               "x and z: (r - 0.5) x 0.1, doubled for mode 3: within [-0.1, 0.1) per update");
        expect(r.velocity[1] >= 0.3f && r.velocity[1] < 0.7f, "y: 0.3 + 0.4 r per update");
        expect(r.life >= 2.0f && r.life < 3.0f, "life: r + 2.0 seconds");
        expect(near(r.ground, 10.56f, 1e-5f), "the floor 2 units ahead plus 0.56 (0x19957c..0x199598)");
        expect(r.type == kDebrisPickupType && r.colour == 0xff818181u && r.object == i, "type 6, the burst colour, debris object i");
        expect(field.objects()[i].enabled && field.objects()[i].spinSpeed >= 0.8f && field.objects()[i].spinSpeed < 1.2f,
               "the object is enabled and gets a spin speed of 0.8 + 0.4 r");
    }
    expect(flying == int(kDebrisDefaultCount), "six bricks (0x229fb8, the default of 0x20c938)");

    const DebrisRecord before = field.records()[0];
    field.update(1.0f / 30.0f, {100, 100, 100});
    const DebrisRecord& after = field.records()[0];
    expect(after.position[0] == before.position[0] + before.velocity[0] && after.position[1] == before.position[1] + before.velocity[1],
           "0x23d39c adds the velocity once per update");
    expect(near(after.velocity[1], before.velocity[1] - 0.05f, 1e-6f), "above the ground the fall is 0.05 x gravity 1.0 per update");
    expect(near(after.life, before.life - 1.0f / 30.0f, 1e-6f), "the life counts down by the frame time");
    expect(field.objects()[0].matrix[12] == after.position[0] && field.objects()[0].matrix[13] == after.position[1],
           "0x28a4b0 puts the object at the record's position while it flies");
    expect((field.objects()[0].colour >> 24) == 255, "a brick more than 10 units from the camera has alpha 255");
    const DebrisVec3 here = field.records()[0].position;
    field.update(1.0f / 30.0f, {here[0] + field.records()[0].velocity[0] + 7.5f, here[1] + field.records()[0].velocity[1],
                                here[2] + field.records()[0].velocity[2]});
    expect(field.objects()[0].alpha >= 126 && field.objects()[0].alpha <= 127,
           "7.5 units away: (7.5 - 5) / (10 - 5) x 255 truncated (0x28a4b0 with 0x3772a8 = 5, 0x3772ac = 10 as the oracle held them)");
    field.update(1.0f / 30.0f, field.records()[0].position);
    expect(field.objects()[0].alpha == 0, "at the camera the alpha is 0");
}

void bounceAndRest() {
    GameRandom random(99);
    DebrisField field(random);
    DebrisBurst burst;
    burst.position = {0, 0, 0};
    field.burst(burst, {}, floorAt(0.0f));
    bool landed = false;
    for (int update = 0; update < 200 && !landed; ++update) {
        const DebrisRecord before = field.records()[0];
        field.update(1.0f / 30.0f, {50, 50, 50});
        const DebrisRecord& after = field.records()[0];
        if (before.state == DebrisState::Flying && before.position[1] + before.velocity[1] < before.ground) {
            expect(after.position[1] == before.ground, "below the ground the brick is put on it");
            const float bounced = -((before.velocity[1] - 0.0f) * kDebrisTypeBounce);
            expect(near(after.velocity[1], bounced, 1e-6f), "and bounces with -0.3 times its vertical speed (type row 6, +0x84)");
            expect((after.state == DebrisState::Resting) == (bounced < 0.05f), "a bounce below 0.05 per update rests the brick");
            expect(after.velocity[0] == before.velocity[0], "the horizontal speed is kept");
            if (after.state == DebrisState::Resting) landed = true;
        }
    }
    expect(landed, "the first brick comes to rest");
    bool hidden = false, shown = false;
    while (field.records()[0].state == DebrisState::Resting) {
        const DebrisVec3 rest = field.records()[0].position;
        field.update(1.0f / 30.0f, {50, 50, 50});
        if (field.records()[0].state != DebrisState::Resting) break;
        expect(field.records()[0].position == rest, "a resting brick does not move");
        if (field.records()[0].life < 1.0f) {
            const bool off = (int(30.0f * field.records()[0].life) & 4) == 0;
            expect((field.objects()[0].alpha == 0) == off, "its last second blinks with bit 2 of 30 x life (0x28a4b0)");
            hidden |= off;
            shown |= !off;
        }
    }
    expect(hidden && shown, "the blink shows and hides the brick");
    expect(field.records()[0].state == DebrisState::Free && !field.objects()[0].enabled, "at life 0 the record is free and its object disabled");
}

void reuse() {
    GameRandom random(7);
    DebrisField field(random);
    DebrisBurst burst;
    burst.position = {0, 50, 0};
    for (int i = 0; i < 4; ++i) field.burst(burst, {}, floorAt(0.0f));
    unsigned withObject = 0;
    for (const DebrisRecord& r : field.records()) withObject += r.object != 0xff;
    expect(withObject == kDebrisObjectCount, "24 bricks share the 20 debris objects; the last four have none");
    float shortest = 1e9f;
    for (const DebrisRecord& r : field.records()) shortest = std::min(shortest, r.life);
    std::array<float, kDebrisRecordCount> lifeBefore{};
    for (unsigned i = 0; i < kDebrisRecordCount; ++i) lifeBefore[i] = field.records()[i].life;
    field.burst(burst, {}, floorAt(0.0f));
    unsigned renewed = 0;
    bool shortestTaken = false;
    for (unsigned i = 0; i < kDebrisRecordCount; ++i)
        if (field.records()[i].life != lifeBefore[i]) {
            ++renewed;
            shortestTaken |= lifeBefore[i] == shortest;
        }
    expect(renewed == 6 && shortestTaken, "a full pool gives the flying bricks with the shortest life to the next burst (counting sort, then type and life)");
}

void secondColour() {
    GameRandom random(5);
    DebrisField field(random);
    DebrisBurst burst;
    burst.position = {0, 50, 0};
    burst.colour = 0xff0000ffu;
    burst.secondColour = 0xff00ff00u;
    burst.ratio = 0.5f;
    burst.count = 4;
    field.burst(burst, {}, floorAt(0.0f));
    expect(field.records()[0].colour == 0xff0000ffu && field.records()[1].colour == 0xff0000ffu && field.records()[2].colour == 0xff00ff00u &&
               field.records()[3].colour == 0xff00ff00u,
           "DebrisRatio 0.5 of 4: brick i takes the second colour once i / 4 is at least the ratio (0x199738..0x199780)");
    DebrisField none(random);
    burst.secondColour = kDebrisNoSecondColour;
    burst.ratio = 0.0f;
    none.burst(burst, {}, floorAt(0.0f));
    expect(none.records()[3].colour == 0xff0000ffu, "the colour (255, 0, 255) means no second colour");
}

void wallPlane() {
    GameRandom random(11);
    DebrisField field(random);
    DebrisBurst burst;
    burst.position = {0, 5, 0};
    const DebrisLineTest wall = [](const DebrisVec3& from, const DebrisVec3&) -> std::optional<DebrisLineHit> {
        return DebrisLineHit{{from[0] + 0.1f, from[1], from[2]}, {-1, 0, 0}};
    };
    field.burst(burst, wall, floorAt(0.0f));
    expect(field.records()[0].plane == 0 && near(field.planes()[0][3], 0.1f, 1e-6f), "a wall hit stores the plane (normal, -dot(point, normal)) in slot 0");
    field.burst(burst, wall, floorAt(0.0f));
    expect(field.records()[6].plane == 1, "the next burst takes plane slot 1 (0x377294, 0..3)");
}

void props() {
    const std::array<float, 16> node = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 851.61f, 16.0f, 1080.04f, 1};
    const DebrisVec3 centre = propEffectCentre({0, 1.4859f, 0}, node);
    const DebrisVec3 bottom = propDebrisPosition({0, 1.4859f, 0}, {0.762f, 1.4859f, 0.9506f}, node);
    expect(near(centre[1], 17.4859f, 1e-4f) && near(bottom[1], 16.0f, 1e-4f), "0x2aa5c8 uses the box centre, 0x23aebc the box bottom (centre y - half y)");
    PropDestroyEffects effects;
    effects.debrisAmount = 6;
    effects.colour = 0xff818181u;
    const DebrisBurst burst = propDebrisBurst(effects, bottom);
    expect(burst.count == 6 && burst.colour == 0xff818181u && burst.spread == 1.0f, "0x20c938: DebrisAmount bricks, spread 1.0");
}

void whiteMaterial() {
    chase::assets::ModelPart part;
    part.material.assign(0x60, 0);
    part.material[1] = 0;
    part.material[2] = 2;
    part.material[10] = 0x49;
    part.material[0xb] = 0x08;
    part.material[0x10] = 10;
    const chase::assets::ModelPart white = chase::assets::whiteMaterialPart(part);
    expect(white.material[1] == 3 && white.material[0x10] == 255 && white.material[0x11] == 255 && white.material[0x12] == 255 &&
               white.material[10] == 0x41 && white.material[0xb] == 0x88 && white.material[2] == 2,
           "0x230ae8: mode 3, colour bytes 255, texture count bits cleared (& 0xc7), byte 0xb bit 7 set");
    const chase::assets::MaterialState state = chase::assets::materialState(white);
    expect(state.supported && state.mode == 3 && !state.stages.empty() && state.stages[0].constant == 0xffffff, "mode 3 starts with the constant white stage");
}

// Oracle (round 20, emulator servant for r20-C, request R20-C-1): Azahar New 3DS, OpenGL, RPC, fixed dt 0.033, nothing poked; pickup records
// 20..43 read after every update from A = [Q + 8] (Q = [[[0x376c78]+0x28]+0x10] + [[0x377288]+0x1c]). Run 1: state veh-smash-car, A held, the
// van hits the car ahead; the burst at update 17 (record +0x308 = 1.0). Run 2: state veh-smash-prop, the lamp post 0x08de6690 destroyed at update
// 18 (0x23cd80 in frame 17). The start rows are the records as read after the burst update (they have moved once already: the record update runs
// after the burst in the same update); rest is the first update read in state 1, free the first in state 0.
struct Recorded {
    unsigned record;
    DebrisVec3 position, velocity;
    float ground, life;
    std::uint32_t colour;
    std::uint8_t object, flags22;
    int restUpdate;
    DebrisVec3 rest;
    float restVelocityY;
    int freeUpdate;
    float freeLife;
};

const Recorded kCarRun[] = {
    {21, {883.4412231445312f, 32.480201721191406f, 934.900146484375f}, {0.07072144001722336f, 0.3996337950229645f, -0.08591613918542862f}, 31.497140884399414f,
     2.7281389236450195f, 0xffcfcfcfu, 1, 0, 49, {885.7048950195312f, 31.497140884399414f, 932.150146484375f}, 0.02835989184677601f, 100, -0.01086048036813736f},
    {22, {883.40185546875f, 32.5915641784668f, 934.9021606445312f}, {0.03138427808880806f, 0.510998547077179f, -0.08390197902917862f}, 31.501827239990234f,
     2.7165117263793945f, 0xff7f7f7fu, 2, 0, 54, {884.5626220703125f, 31.501827239990234f, 931.7969970703125f}, 0.026253044605255127f, 100, -0.02248767763376236f},
    {23, {883.3859252929688f, 32.58661651611328f, 934.9781494140625f}, {0.015451050363481045f, 0.5060486197471619f, -0.007916259579360485f}, 31.49237060546875f,
     2.9436540603637695f, 0xff7f7f7fu, 3, 1, 54, {883.957275390625f, 31.49237060546875f, 934.6845703125f}, 0.026386693120002747f, 107, -0.02634529024362564f},
    {24, {883.4057006835938f, 32.52920150756836f, 935.0836791992188f}, {0.03519287332892418f, 0.4486328363418579f, 0.09758605808019638f}, 31.50177001953125f,
     2.001988410949707f, 0xff7f7f7fu, 4, 1, 50, {884.56787109375f, 31.50177001953125f, 938.3043212890625f}, 0.02838691882789135f, 78, -0.011011160910129547f},
    {25, {883.3815307617188f, 32.4039421081543f, 934.9539184570312f}, {0.011056519113481045f, 0.32337647676467896f, -0.03214416652917862f}, 31.50187110900879f,
     2.708195686340332f, 0xffcfcfcfu, 5, 1, 41, {883.6466674804688f, 31.50187110900879f, 934.1819458007812f}, 0.0471038781106472f, 100, -0.03080371767282486f},
};
const Recorded kPropRun[] = {
    {23, {862.68310546875f, 32.026939392089844f, 920.8951416015625f}, {-0.02113647572696209f, 0.31373900175094604f, -0.01884155347943306f}, 31.606691360473633f,
     2.1288957595825195f, 0xff7f7f7fu, 3, 1, 39, {862.2396240234375f, 31.606691360473633f, 920.4990844726562f}, 0.04023651033639908f, 83, -0.016103781759738922f},
    {24, {862.6604614257812f, 32.1798095703125f, 920.8591918945312f}, {-0.04376831278204918f, 0.4666076898574829f, -0.05482177808880806f}, 31.626516342163086f,
     2.609135627746582f, 0xffc0c0c0u, 4, 0, 51, {861.21630859375f, 31.626516342163086f, 919.0504760742188f}, 0.02790159545838833f, 98, -0.030863799154758453f},
    {25, {862.7959594726562f, 32.34092712402344f, 920.8482666015625f}, {0.09174194186925888f, 0.627728283405304f, -0.06573486328125f}, 31.763912200927734f,
     2.6549272537231445f, 0xffc0c0c0u, 5, 1, 59, {866.55712890625f, 31.763912200927734f, 918.1531372070312f}, 0.022651340812444687f, 99, -0.018072165548801422f},
    {26, {862.683349609375f, 32.31284713745117f, 920.9761962890625f}, {-0.02086791954934597f, 0.5996460318565369f, 0.062164306640625f}, 31.517337799072266f,
     1.975773811340332f, 0xffc0c0c0u, 6, 1, 59, {861.8275146484375f, 31.517337799072266f, 923.523681640625f}, 0.023409560322761536f, 78, -0.0042257606983184814f},
    {27, {862.8002319335938f, 32.18590545654297f, 920.9136352539062f}, {0.09598083794116974f, 0.47270506620407104f, -0.0003662109375f}, 31.723169326782227f,
     2.309575080871582f, 0xff7f7f7fu, 7, 0, 51, {865.968505859375f, 31.723169326782227f, 920.9015502929688f}, 0.02773696556687355f, 88, -0.00042441487312316895f},
    {28, {862.662841796875f, 32.253623962402344f, 920.8348999023438f}, {-0.04139099270105362f, 0.540423572063446f, -0.0791015625f}, 31.642662048339844f,
     2.6969499588012695f, 0xff7f7f7fu, 8, 0, 55, {861.1317138671875f, 31.642662048339844f, 917.9081420898438f}, 0.025458568707108498f, 100, -0.009049452841281891f},
};

template <std::size_t N>
void replay(const Recorded (&rows)[N], int startUpdate, const char* run) {
    GameRandom random(1);
    DebrisField field(random);
    for (const Recorded& row : rows) {
        DebrisRecord record;
        record.position = row.position;
        record.velocity = row.velocity;
        record.ground = row.ground;
        record.life = row.life;
        record.colour = row.colour;
        record.object = row.object;
        record.flags22 = row.flags22;
        record.state = DebrisState::Flying;
        record.type = kDebrisPickupType;
        field.setRecord(row.record - kDebrisRecordFirst, record);
    }
    std::array<int, N> rested{}, freed{};
    for (int update = startUpdate + 1; update <= startUpdate + 120; ++update) {
        field.update(0.033f, {0, 0, 0});
        for (std::size_t i = 0; i < N; ++i) {
            const Recorded& row = rows[i];
            const DebrisRecord& record = field.records()[row.record - kDebrisRecordFirst];
            if (!rested[i] && record.state == DebrisState::Resting) {
                rested[i] = update;
                const bool ok = update == row.restUpdate && near(record.position[0], row.rest[0], 1e-4f) && record.position[1] == row.rest[1] &&
                                near(record.position[2], row.rest[2], 1e-4f) && near(record.velocity[1], row.restVelocityY, 1e-6f);
                if (!ok) std::printf("  %s record %u: rest at %d (%.4f %.4f %.4f), oracle %d\n", run, row.record, update, double(record.position[0]),
                                     double(record.position[1]), double(record.position[2]), row.restUpdate);
                expect(ok, "a brick comes to rest in the update, at the place and with the last bounce of the oracle trace");
            }
            if (!freed[i] && record.state == DebrisState::Free) {
                freed[i] = update;
                expect(update == row.freeUpdate && near(record.life, row.freeLife, 1e-4f), "and is freed in the update of the oracle trace with its life");
            }
        }
    }
    for (std::size_t i = 0; i < N; ++i) expect(rested[i] && freed[i], "every recorded brick rests and is freed within 120 updates");
}

// 0x2b3c48 / 0x2cdca4 / 0x2c8278 / 0x19913c (0x19983c, 0x1998b8) / 0x2c4f9c / 0x180874, read from llvm-objdump; charbits.blvl holds SBDebris01..20
// (pickup_sbplate1x1 / sbbrick1x1, class SuperBuildValue 1.0), so every object is in the table of the value *0x3743f0 = 1.
std::array<std::optional<std::uint16_t>, kSuperDebrisObjectCount> unitSuperObjects() {
    std::array<std::optional<std::uint16_t>, kSuperDebrisObjectCount> values{};
    for (auto& v : values) v = 1;
    return values;
}

void superBricks() {
    expect(!superDebrisBlock(0, 0xcc), "0x2cdca4: a kind outside 0x55, 0x77, 0x8a, 0xd6 without SuperBuildValue has no block");
    const auto tree = superDebrisBlock(0, 0x77);
    expect(tree && tree->value == 6 && tree->count == 9 && tree->kind == 103, "a wobbly prop (kind 0x77): value 6, count 9, kind 'g'");
    const auto light = superDebrisBlock(3, 0xcc);
    expect(light && light->value == 3 && light->count == 9, "prop_trafficlight (class SuperBuildValue 3.0): value 3, count 9");
    expect(superDebrisBlock(500, 0)->value == 244, "the value is SuperBuildValue's low byte (strb at 0x2cdd04)");

    PropDestroyEffects six;
    expect(propDebrisBurst(six, {0, 0, 0}, light).count == 9 && propDebrisBurst(six, {0, 0, 0}, light).super,
           "0x20cb50: the block's count 9 replaces DebrisAmount and the block goes to the burst");
    expect(propDebrisBurst(six, {0, 0, 0}).count == 6 && !propDebrisBurst(six, {0, 0, 0}).super, "without a block the burst keeps DebrisAmount");

    GameRandom random(77);
    DebrisField field(random);
    field.loadSuperObjects(unitSuperObjects());
    field.listFreeSuperObjects();
    DebrisBurst burst;
    burst.position = {0, 10, 0};
    burst.count = light->count;
    burst.super = light;
    field.burst(burst, {}, floorAt(10.0f));
    unsigned supers = 0;
    for (unsigned i = 0; i < 9; ++i) {
        const DebrisRecord& r = field.records()[i];
        const bool super = (r.flags22 & 2) != 0;
        supers += super;
        expect(super == (i < 3), "a value of 3 makes the first three bricks super bricks (one per brick, *0x3743f0 = 1)");
        if (!super) {
            expect(r.phase == 1 && r.superObject == kNoSuperDebrisObject && field.objects()[i].enabled, "a plain brick keeps Debris0N");
            continue;
        }
        expect(r.kind == 103 && r.superValue == 1 && r.life == 8.0f && !(r.flags22 & 4), "kind 'g', value 1, life 255 / 30 = 8, bit 2 clear");
        expect(r.superObject == i && field.superObjects()[i].enabled && !field.objects()[i].enabled, "the slot holds SBDebris0N, Debris0N stays off");
        expect(field.objectOf(r) == &field.superObjects()[i], "objectOf gives the SBDebris object");
    }
    expect(supers == 3, "three super bricks of nine (oracle veh-smash-prop, debris.md §2026-10-18T14:46:04)");

    for (int u = 0; u < 120 && field.records()[0].state != DebrisState::Resting; ++u) field.update(1.0f / 30.0f, {100, 100, 100});
    DebrisRecord& rest = field.records()[0];
    expect(rest.state == DebrisState::Resting, "the super brick comes to rest");
    rest.colour = 0xff000000u;
    rest.life = 4.0f;
    field.animateSuper(0.5f, 0xff00ff00u);
    const DebrisObject& object = field.superObjects()[0];
    expect((rest.flags22 & 4) != 0, "0x2c504c: a resting super brick becomes pickable");
    expect(((object.colour >> 8) & 0xff) == 255 && (object.colour & 0xff) == 0, "life 4: the mix (1 - 120 / 255) x 10 / 3 is clamped to 1");
    const float spin = float(rest.phase) * 0.00392156886f * 6.2831855f + 0.5f * -3.14159274f;
    expect(near(object.matrix[0], std::cos(spin), 1e-5f) && near(object.matrix[2], -std::sin(spin), 1e-5f), "the spin about y");
    rest.life = 8.0f;
    field.animateSuper(0.5f, 0xff00ff00u);
    expect(((object.colour >> 8) & 0xff) == std::uint32_t(std::int32_t((1.0f - 240.0f / 255.0f) * 3.33333325f * 255.0f)),
           "life 8: green = int(255 x (1 - 240 / 255) x 10 / 3)");

    field.freeRecord(0);
    expect(!field.superObjects()[0].enabled && field.objectOf(field.records()[0]) == nullptr, "the free path disables the object, clears the slot");
    field.listFreeSuperObjects();
    DebrisBurst again = burst;
    again.super->value = 1;
    field.burst(again, {}, floorAt(10.0f));
    unsigned taken = 0;
    for (const DebrisRecord& r : field.records())
        if ((r.flags22 & 2) && r.state == DebrisState::Flying) {
            ++taken;
            expect(r.superObject == 0, "the next burst takes the first disabled SBDebris object");
        }
    expect(taken == 1, "a value of 1 gives one super brick");

    GameRandom aimed(5);
    DebrisField directed(aimed);
    directed.loadSuperObjects(unitSuperObjects());
    directed.listFreeSuperObjects();
    DebrisBurst toward = burst;
    toward.super->spawnDir = 90;
    toward.super->spawnCone = 0.0f;
    directed.burst(toward, {}, floorAt(10.0f));
    const DebrisRecord& d = directed.records()[0];
    expect(near(d.velocity[0], 0.1f, 1e-5f) && near(d.velocity[2], 0.0f, 1e-5f) && d.velocity[1] == 0.5f,
           "SpawnDir 90 with cone 0: (sin, 0.5, cos) x 0.1 of the 64-step angle; y 0.5");
}

// Oracle A-1 of round 47 (servant, veh-smash-prop, two stepped loads, dt 0.033, A held; traces x1 and x2): the traffic
// light bursts at update 18 (pool records 20..28), Generic_Tree_13 at update 49 (29..37), the van's contact at update 57 (38..43). Super bricks: records
// 20..22 in slots 0..2 with SBDebris 0..2; 29..34 in slots 9..14 with SBDebris 3..8; 38 in slot 18 with SBDebris 9; record 39 takes slot 19 and 40..43 get
// none. The tree's super bricks fly along SpawnDir 337: y velocity 0.45 after the burst update, x in -0.0916..-0.0073, z in 0.0477..0.1358, life 7.967.
void superBricksOracle() {
    GameRandom random(2024);
    DebrisField field(random);
    field.loadSuperObjects(unitSuperObjects());
    const DebrisLineTest low = floorAt(-100.0f);
    const auto burstWith = [&](std::optional<DebrisSuperBlock> block, unsigned count) {
        field.listFreeSuperObjects();
        DebrisBurst burst;
        burst.position = {0, 30, 0};
        burst.count = count;
        burst.super = block;
        const unsigned supers = field.burst(burst, {}, low);
        field.update(0.033f, {100, 100, 100});
        return supers;
    };
    DebrisSuperBlock light;
    light.value = 3;
    light.count = 9;
    expect(burstWith(light, 9) == 3, "A-1 update 18: the traffic light (value 3) gives three super bricks");
    for (int u = 19; u < 49; ++u) field.update(0.033f, {100, 100, 100});
    DebrisSuperBlock tree;
    tree.value = 6;
    tree.count = 9;
    tree.spawnDir = 337;
    expect(burstWith(tree, 9) == 6, "A-1 update 49: the tree (kind 0x77, value 6) gives six");
    for (int u = 50; u < 57; ++u) field.update(0.033f, {100, 100, 100});
    DebrisSuperBlock van;
    van.value = 1;
    van.count = 6;
    expect(burstWith(van, 6) == 1, "A-1 update 57: the van's contact (value 1) gives one");
    const std::array<std::uint8_t, 24> superOf = {0, 1, 2, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 3, 4, 5, 6, 7, 8, 0xff, 0xff, 0xff, 9, 0xff, 0xff, 0xff, 0xff, 0xff};
    for (unsigned i = 0; i < 24; ++i) {
        const DebrisRecord& r = field.records()[i];
        expect(r.superObject == superOf[i], "A-1: the SBDebris object of each record (slot table 0x58f630)");
        expect(r.object == (i < 20 ? i : 0xff), "A-1: slots in record order, records 40..43 without one");
    }
    GameRandom again(2024);
    DebrisField fresh(again);
    fresh.loadSuperObjects(unitSuperObjects());
    fresh.listFreeSuperObjects();
    DebrisBurst burst;
    burst.position = {0, 30, 0};
    burst.count = 9;
    burst.super = tree;
    fresh.burst(burst, {}, low);
    fresh.update(0.033f, {100, 100, 100});
    for (unsigned i = 0; i < 6; ++i) {
        const DebrisRecord& r = fresh.records()[i];
        expect(r.velocity[1] == 0.45f && near(r.life, 7.967f, 1e-4f), "A-1: a SpawnDir super brick starts at y 0.5 per update, life 8");
        expect(r.velocity[0] >= -0.0928f && r.velocity[0] <= 0.0072f && r.velocity[2] >= 0.0404f && r.velocity[2] <= 0.1404f,
               "A-1: SpawnDir 337 through the 64-step angle (byte 55): x in (sin - 0.5) x 0.1 .. (sin + 0.5) x 0.1, z likewise with cos");
    }
}

void oracleTraces() {
    replay(kCarRun, 17, "veh-smash-car");
    replay(kPropRun, 18, "veh-smash-prop");
}

}  // namespace

int main() {
    oracleTraces();
    colourAndRotation();
    burstOnFloor();
    bounceAndRest();
    reuse();
    secondColour();
    wallPlane();
    props();
    whiteMaterial();
    superBricks();
    superBricksOracle();
    if (failures == 0) std::printf("debris: ok\n");
    return failures == 0 ? 0 : 1;
}
