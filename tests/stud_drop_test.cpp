// Tests of the dropped studs (src/runtime/stud_drop.*), docs/re/studs.md entry 2026-10-19 "Dropped studs ported". The rules and constants were read in
// llvm-objdump of artifacts/game/game.arm.elf: 0x21e2e4 (literals 0x21e698..0x21e6a8, 0x21e6fc), 0x19913c (literals 0x199484..0x1994d0, 0x199ad4),
// 0x180168 (literals 0x180484..0x1804b4, 0x180998), 0x17fc38 (0x180160), 0x2149ac (0x214ba4..0x214bd8) and the type table 0x2ed0b4. Expected
// values are worked by hand from those rules (float32 arithmetic in a scratch Python session, 2026-10-19). The random generator is built from
// tables whose draws are known: all words 0 (every draw 0, so r = 0 and mode 1 becomes 2), or word 1 at the first cursor with the second cursor one
// below it (first draw 8, the next nine draws 0).
#include "runtime/stud_drop.hpp"

#include <cmath>
#include <cstdio>
#include <string>

namespace {

using namespace chase::runtime;

int failures = 0;

void expect(bool ok, const std::string& what) {
    if (!ok) {
        std::printf("FAIL: %s\n", what.c_str());
        ++failures;
    }
}

bool near(float a, float b, float tolerance = 1e-5f) { return std::fabs(a - b) <= tolerance; }

GameRandom zeroDraws() { return GameRandom(std::array<std::uint32_t, kGameRandomTableSize>{}, 0, 7); }

GameRandom firstDrawEight() {
    std::array<std::uint32_t, kGameRandomTableSize> table{};
    table[0] = 1;
    return GameRandom(table, 0, 10);
}

DebrisLineTest floorAt(float height) {
    return [height](const DebrisVec3& from, const DebrisVec3& to) -> std::optional<DebrisLineHit> {
        if (from[1] < height || to[1] > height) return std::nullopt;
        return DebrisLineHit{{from[0], height, from[2]}, {0, 1, 0}};
    };
}

unsigned flying(const StudDropField& field) {
    unsigned n = 0;
    for (const StudDropRecord& r : field.records()) n += r.state == StudDropState::Flying;
    return n;
}

void direction() {
    expect(!studDropDirection({0, 5, 0}, 1.0f), "x = z = 0: no direction (byte 255)");
    const auto east = studDropDirection({1, 0, 0}, 1.0f);
    expect(east && near((*east)[0], 1.0f) && near((*east)[2], 7.54979e-08f, 1e-7f), "(1, 0, 0): byte 96, back to (sin, 0, cos) of pi/2");
    const auto slanted = studDropDirection({0.3f, 0, 1}, 1.0f);
    expect(slanted && near((*slanted)[0], 0.24298017f) && near((*slanted)[2], 0.97003126f), "(0.3, 0, 1): byte 69, angle 5 pi / 64");
    const auto back = studDropDirection({0, 0, -1}, 2.0f);
    expect(back && near((*back)[2], -2.0f) && near((*back)[0], -1.7484556e-07f, 1e-6f), "(0, 0, -1): byte 128 (angle pi), times the scale 2");
}

void characterDrop() {
    {
        GameRandom random = zeroDraws();
        DebrisPlaneRing planes;
        StudDropField field(random, planes);
        StudDropRequest request;
        request.position = {1, 2, 3};
        expect(field.drop(request, {}, floorAt(0)) == StudDropMode::Heart, "mode 1 with a draw of 0 (a multiple of three) becomes mode 2");
        expect(flying(field) == 0, "count 0: 0x19913c takes no record, not even the heart of mode 2");
        expect(random.firstCursor() == 9 && random.secondCursor() == 5, "two draws: the mode of 0x21e6b0 and the raw word of 0x21e5fc");
    }
    {
        GameRandom random = firstDrawEight();
        DebrisPlaneRing planes;
        StudDropField field(random, planes);
        StudDropRequest request;
        expect(field.drop(request, {}, {}) == StudDropMode::Count, "mode 1 with a draw of 8 becomes mode 0");
        expect(flying(field) == 0 && random.firstCursor() == 9, "count 0: nothing dropped, two draws");
    }
}

void countOut() {
    GameRandom random = zeroDraws();
    DebrisPlaneRing planes;
    StudDropField field(random, planes);
    StudDropRequest request;
    request.mode = StudDropMode::Count;
    request.count = 30;
    request.position = {5, 10, -3};
    field.drop(request, {}, floorAt(10.0f));
    expect(flying(field) == 3, "mode 0 count 30: three records");
    for (unsigned i = 0; i < 3; ++i) {
        const StudDropRecord& r = field.records()[i];
        expect(r.type == std::uint8_t(StudKind::Gold), "count 30 is three golds (10 each)");
        expect(r.position[0] == 5.0f && r.position[1] == 10.5f && r.position[2] == -3.0f, "0x21e2e4 adds 0.5 to y");
        expect(near(r.velocity[0], -0.05f) && r.velocity[1] == 0.5f && near(r.velocity[2], -0.05f), "r = 0: ((r - 0.5) spread) x 0.1, y 0.5");
        expect(r.skip == 30 && r.life == 8.5f && r.plane == -1, "skip counter 30, life 8.5, no wall plane");
        expect(near(r.ground, 10.56f), "the floor 2 units ahead plus 0.56");
    }
    field.drop({StudDropMode::Count, 123, {0, 0, 0}}, {}, {});
    const unsigned expected[] = {2, 1, 1, 0, 0, 0};
    for (unsigned i = 0; i < 6; ++i) expect(field.records()[3 + i].type == expected[i], "count 123: blue, gold, gold, three silvers in the next free records");
    expect(near(field.records()[3].ground, -18.99f), "no ground hit: the start y (0.5) - 20 + 0.56 - 0.05; the bricks take the end y (-9.5)");
}

void limits() {
    GameRandom random = zeroDraws();
    DebrisPlaneRing planes;
    StudDropField field(random, planes);
    field.drop({StudDropMode::Count, 29, {0, 0, 0}}, {}, {});
    expect(flying(field) == 10, "count 29 needs eleven records; 0x19913c stops after ten");
    field.drop({StudDropMode::Heart, 2, {0, 0, 0}}, {}, {});
    expect(field.records()[10].type == kStudDropHeartType && field.records()[11].type == 0 && field.records()[12].type == 0 && flying(field) == 13,
           "mode 2: a heart first without using the count, then mode 0");
    StudDropRecord old;
    old.state = StudDropState::Resting;
    old.life = 0.5f;
    for (unsigned i = 13; i < kStudDropRecordCount; ++i) field.setRecord(i, old);
    old.life = 0.2f;
    field.setRecord(4, old);
    field.drop({StudDropMode::Count, 1, {0, 0, 0}}, {}, {});
    expect(field.records()[4].state == StudDropState::Flying && field.records()[4].type == 0,
           "no free record: the resting one of the lowest type and the shortest life is taken");
}

void slowAndCollectFlag() {
    GameRandom random = zeroDraws();
    DebrisPlaneRing planes;
    StudDropField field(random, planes);
    StudDropRequest request;
    request.mode = StudDropMode::Count;
    request.count = 1;
    request.slow = true;
    request.collectAtOnce = true;
    field.drop(request, {}, {});
    const StudDropRecord& r = field.records()[0];
    expect(near(r.velocity[0], -0.03f) && near(r.velocity[1], 0.05f) && near(r.velocity[2], -0.03f), "slow: x and z times 6, then all times 0.1");
    expect((r.flags49 & 6) == 6, "+0x49 bit 1 slow, bit 2 collect at once");
    StudField counter({});
    StudDropRecord ready = r;
    ready.skip = 1;
    field.setRecord(0, ready);
    const StudDropUpdate u = field.update(0.1f, {500, 500, 500}, std::nullopt, counter);
    expect(u.collected == 1 && field.records()[0].state == StudDropState::Collecting, "bit 2: collected at the test whatever the distance");
}

void motion() {
    GameRandom random = zeroDraws();
    DebrisPlaneRing planes;
    StudDropField field(random, planes);
    StudField counter({});
    const std::array<float, 3> far = {900, 900, 900};
    StudDropRecord r;
    r.state = StudDropState::Flying;
    r.skip = 30;
    r.life = 8.5f;
    r.ground = 10.56f;
    r.position = {0, 12.0f, 0};
    r.velocity = {0.01f, 0.2f, 0};
    field.setRecord(0, r);
    field.update(0.1f, far, std::nullopt, counter);
    expect(near(field.records()[0].position[1], 12.2f) && near(field.records()[0].velocity[1], 0.15f) && near(field.records()[0].life, 8.4f),
           "above the ground: position += velocity, velocity y -= 0.05, life -= frame time");

    r.position = {0, 10.6f, 0};
    r.velocity = {0.01f, -0.1f, 0};
    field.setRecord(0, r);
    StudDropUpdate u = field.update(0.1f, far, std::nullopt, counter);
    expect(u.sounds.size() == 1 && u.sounds[0].id == kStudLandingSound && near(u.sounds[0].frequencyScale, 0.95f),
           "landing: sound 65 at frequency 0.95 + 0.1 r");
    expect(field.landingSoundWait() == 60, "the next landing sound waits 60 updates");
    expect(field.records()[0].position[1] == 10.56f && near(field.records()[0].velocity[1], 0.03f) && field.records()[0].state == StudDropState::Resting,
           "y = the rest height, velocity y = -(y x 0.3); below 0.05 it rests");

    r.velocity = {0.01f, -0.2f, 0};
    field.setRecord(1, r);
    u = field.update(0.1f, far, std::nullopt, counter);
    expect(u.sounds.empty() && field.landingSoundWait() == 59, "a landing within the wait is silent; the wait counts down once per update");
    expect(near(field.records()[1].velocity[1], 0.06f) && field.records()[1].state == StudDropState::Flying, "a bounce of 0.06 keeps flying");

    r.velocity = {0, -0.1f, 0};
    field.setRecord(2, r);
    field.update(0.1f, far, std::nullopt, counter);
    expect(field.records()[2].state == StudDropState::Resting && near(field.records()[2].life, 8.4f), "no horizontal velocity: rests with life 8.5");
    r.position = {0, 9.0f, 0};
    r.velocity = {0, 0.5f, 0};
    field.setRecord(3, r);
    field.update(0.1f, far, std::nullopt, counter);
    expect(field.records()[3].state == StudDropState::Resting && near(field.records()[3].life, 0.9f), "velocity y exactly 0.5: life 1.0");

    StudDropRecord resting = r;
    resting.state = StudDropState::Resting;
    resting.life = 0.05f;
    field.setRecord(4, resting);
    field.update(0.1f, far, std::nullopt, counter);
    expect(field.records()[4].state == StudDropState::Free, "the life runs out: free");
}

void pickup() {
    GameRandom random = zeroDraws();
    DebrisPlaneRing planes;
    StudDropField field(random, planes);
    StudField counter({});
    StudDropRecord r;
    r.state = StudDropState::Resting;
    r.life = 5.0f;
    r.skip = 1;
    r.position = {3.0f, 0, 0};
    field.setRecord(0, r);
    field.update(0.25f, {0, 0, 0}, std::nullopt, counter);
    expect(field.records()[0].skip == 6, "missed at 3.0: skip = int((3.0 - 1.5) / 0.23)");
    r.position = {1.0f, 0, 0};
    field.setRecord(0, r);
    StudDropUpdate u = field.update(0.25f, {0, 0, 0}, std::nullopt, counter);
    expect(u.collected == 1 && u.sounds.size() == 1 && u.sounds[0].id == 64 && near(u.sounds[0].volume, 0.7f), "collected inside 1.5: sound 64 at volume 0.7");
    expect(field.records()[0].state == StudDropState::Collecting && near(field.records()[0].life, 0.75f),
           "a dropped record (+0x45 = 0) loses the frame time from its collect timer in the same update");
    const std::array<float, 3> target = {0, 10, 0};
    field.update(0.25f, {0, 0, 0}, target, counter);
    expect(near(field.records()[0].position[1], 10.0f / (30.0f * 0.5f)) && counter.counter() == 0, "flight: (target - position) / (30 x timer)");
    field.update(0.25f, {0, 0, 0}, target, counter);
    field.update(0.25f, {0, 0, 0}, target, counter);
    expect(counter.counter() == 10 && field.records()[0].state == StudDropState::Free, "the timer runs out in the third update: 10 banked, free");
    expect(counter.takeBanked().size() == 1, "the new total goes to the HUD");
}

void drawRule() {
    GameRandom random = zeroDraws();
    DebrisPlaneRing planes;
    StudDropField field(random, planes);
    StudDropRecord r;
    r.state = StudDropState::Resting;
    const float lives[] = {3.0f, 2.0f, 1.9f};
    const bool shown[] = {true, true, false};
    for (unsigned i = 0; i < 3; ++i) {
        r.life = lives[i];
        field.setRecord(i, r);
        expect(field.drawn(i) == shown[i], "resting: drawn from 2.125 s, below on bit 2 of int(30 x life) (60 set, 57 clear)");
    }
    r.state = StudDropState::Flying;
    r.life = 0.1f;
    field.setRecord(3, r);
    expect(field.drawn(3) && !field.drawn(5), "flying always drawn, free never");
}

void spawnRequest() {
    StudSpawnParameters p;
    expect(!studSpawnRequest(p), "0x2149ac returns at once for count 0");
    p.count = 2;
    p.forward = true;
    p.modeCount = true;
    p.node = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 3, 0, 4, 1};
    auto request = studSpawnRequest(p);
    expect(request && request->mode == StudDropMode::Count && request->count == 2 && request->direction[2] == 1.0f && request->position[0] == 3.0f,
           "no SpawnDir, forward: the node's row 2; mode flag: mode 0");
    p.modeCount = false;
    p.spawnDir = 90;
    request = studSpawnRequest(p);
    expect(request && request->mode == StudDropMode::Random && near(request->direction[0], 1.0f) && near(request->direction[2], -4.371139e-08f, 1e-7f),
           "SpawnDir 90 degrees: (sin, 0, cos)");
    p.spawnDir = 0;
    p.spawnLocal = 1;
    request = studSpawnRequest(p);
    expect(request && near(request->direction[0], 0.51449576f) && near(request->direction[2], 0.85749293f),
           "SpawnLocal: 0x254250 transforms with the translation (3, 0, 4): (0, 0, 1) becomes (3, 0, 5), normalised");
}

}  // namespace

int main() {
    direction();
    characterDrop();
    countOut();
    limits();
    slowAndCollectFlag();
    motion();
    pickup();
    drawRule();
    spawnRequest();
    if (failures) return 1;
    std::printf("stud-drop: all checks passed\n");
    return 0;
}
