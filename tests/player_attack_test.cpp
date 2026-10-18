// Tests of Chase's attack on foot (src/runtime/player_attack.*; docs/re/player-attack.md). Recorded values: Azahar oracle, state m12-table-before
// (the user's, mission 1-2), own emulator without RPC, fixed dt 1/30, scenarios tools/oracle/attack-tap.txt (run atap), attack-variants.txt (runs
// ahold3, ahold12, acombo10, acombo3 and the memory reads), attack-record.txt (run aaway), attack-tap-hit.txt and attack-combo-hit.txt (traps of
// the wobbly handler 0x2a3390). Rows count updates after the load like the trace files (the Y press is held in row 6). Exits with 77 without data.
#include "assets/assets.hpp"
#include "assets/object_collision.hpp"
#include "runtime/carry_animation.hpp"
#include "runtime/player_attack.hpp"

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <string>

namespace {

using namespace chase;
using namespace chase::runtime;

int failures = 0;

void expect(bool ok, const char* what, double got = 0.0) {
    if (!ok) {
        std::printf("FAIL: %s (got %.6f)\n", what, got);
        ++failures;
    }
}

void tap() {
    AttackTap shortPress;
    expect(!shortPress.update(true), "no tap while Y is held");
    expect(shortPress.update(false), "a one-update press taps in the release update (atap: state 0xa3 in row 7, Y held in row 6)");
    expect(!shortPress.update(false), "the tap is an edge");
    AttackTap three;
    for (int i = 0; i < 3; ++i) three.update(true);
    expect(three.update(false), "a three-update press taps on release (ahold3: Y held in rows 6..8, state 0xa3 in row 9)");
    AttackTap nine;
    for (int i = 0; i < 9; ++i) nine.update(true);
    expect(nine.update(false), "nine held updates still tap (0x1bb690: counter below 10)");
    AttackTap twelve;
    for (int i = 0; i < 12; ++i) twelve.update(true);
    expect(!twelve.update(false), "a twelve-update hold does not attack (ahold12: ctrl+0xc bit 0x20 from the 10th held update, state 1 throughout)");
}

void bodies() {
    const AttackBody player = playerAttackBody(0, 0, 0, 0);
    expect(std::fabs(player.radius - 1.16314f) < 1e-5f, "the player's radius equals entity+0x5c of 0x085526f0 (1.16314)", player.radius);
    const AttackBody table = attackBody({1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1}, {0.0016f, 1.5197f, 0}, {1.5243f, 1.5197f, 1.4925f});
    expect(std::fabs(table.radius - 2.6192f) < 1e-3f, "DonutTable_14's radius equals entity+0x5c of 0x08c99320 (2.6192)", table.radius);
}

void geometry() {
    const AttackBody attacker = playerAttackBody(0, 0, 0, 0);
    AttackCandidate ahead;
    ahead.body = attackBody({1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1.5f, 1}, {0, 1.15f, 0}, {0.3f, 0.5f, 0.3f});
    AttackCandidate behind = ahead;
    behind.body.node[14] = -1.5f;
    const std::vector<AttackCandidate> both{behind, ahead};
    const std::optional<std::size_t> found = findAttackTarget(2.2f, 90.0f, attacker, {0, 0, 1}, both);
    expect(found && *found == 1, "the search takes the candidate ahead, not the one behind (angle above 90 degrees)");
    AttackCandidate off = ahead;
    off.collision = false;
    expect(!findAttackTarget(2.2f, 90.0f, attacker, {0, 0, 1}, {off}), "an entity without collision flag 0x200 is no target (0x21a2d4)");
    expect(attackHitTest(2.2f, 90.0f, 0.0f, attacker, ahead.body), "the hit test passes a box 1.5 ahead");
    AttackBody high = ahead.body;
    high.node[13] = 1.5f;
    expect(!attackHitTest(2.2f, 90.0f, 0.0f, attacker, high), "a box whose bottom is less than 0.2 below the attacker's top fails the vertical test");
    expect(!attackHitTest(2.2f, 90.0f, 0.0f, attacker, behind.body), "the hit test fails behind the attacker");
    const float score = attackScore(10.0f, 90.0f, 1.0f, {0, 0, 0}, {0, 0, 1}, {0, 0, 2});
    expect(std::fabs(score - 1.0f) < 1e-6f, "0x21a1d8 straight ahead: 2 * (1.25 - 1) * 2 = 1", score);
}

struct Session {
    AttackClips clips;
    std::vector<AttackTargetObject> objects;
    std::vector<AttackCandidate> candidates;
    std::size_t table = 0;
};

bool load(const char* fib, const char* names, Session& session) {
    assets::FuseArchive archive(fib);
    archive.loadNames(names);
    const assets::FuseEntry* levelEntry = archive.find("levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl");
    const assets::FuseEntry* tableEntry = archive.find("tables/tablesanimations.bxls");
    if (!levelEntry || !tableEntry) {
        std::printf("FAIL: level or animation table not in archive\n");
        ++failures;
        return false;
    }
    const CarryAnimations names2(assets::parseTable(archive.read(*tableEntry)));
    for (const int id : {0x7a, 0x7b, 0x7c, 0x7d, 0x7e}) {
        const assets::FuseEntry* clip = archive.find("models/char_minifig/class_generic/" + names2.name(id) + "_minifig.bfnanm");
        if (!clip) {
            std::printf("FAIL: clip %d (%s) not in archive\n", id, names2.name(id).c_str());
            ++failures;
            return false;
        }
        session.clips.clips.push_back(makeAttackClip(id, assets::parseAnimation(archive.read(*clip))));
    }
    const assets::Level level = assets::parseLevel(archive.read(*levelEntry));
    session.objects = attackTargetObjects(assets::objectCollisionCandidates(archive, level));
    bool found = false;
    for (std::size_t i = 0; i < session.objects.size(); ++i) {
        session.candidates.push_back({session.objects[i].body, session.objects[i].collisionDefault});
        if (session.objects[i].instance == "DonutTable_14") {
            session.table = i;
            found = true;
        }
    }
    expect(found, "DonutTable_14 is a candidate of the attack");
    return found;
}

AttackInput inputAt(const Session& session, const LocomotionState& motion, float y, bool tapped) {
    AttackInput input;
    input.tap = tapped;
    input.allowed = true;
    input.x = motion.x;
    input.y = y;
    input.z = motion.z;
    input.candidates = &session.candidates;
    return input;
}

void clipsMatch(const Session& session) {
    const AttackClip* first = session.clips.find(0x7a);
    expect(first && first->updates == 14, "combo_1 lasts 14 updates (atap: state 0xa3 in rows 9..22)", first ? first->updates : 0);
    const AttackClip* second = session.clips.find(0x7b);
    const AttackClip* third = session.clips.find(0x7c);
    expect(second && second->updates == 12 && third && third->updates == 9, "combo_2 lasts 12 and combo_3 9 updates (acombo10: combo_3 in rows 26..34)");
}

void table(const Session& session) {
    const AttackBody& body = session.objects[session.table].body;
    expect(std::fabs(body.centre[0] - 0.0016f) < 1e-3f && std::fabs(body.half[0] - 1.5243f) < 1e-3f && std::fabs(body.half[2] - 1.4925f) < 1e-3f,
           "DonutTable_14's box in x and z equals entity+0x60 / +0x6c of 0x08c99320 (centre x 0.0016; half 1.5243, 1.4925)", body.half[0]);
    expect(std::fabs(body.half[1] - 1.5197f) < 0.01f, "its height is 1.52876 against the oracle's 1.5197 (PLAYER-13)", body.half[1]);
    LocomotionState motion;
    motion.x = 715.44196f;
    motion.z = 641.61597f;
    motion.facing = motion.lastIntent = -936;
    const float y = 32.71405f;
    PlayerAttack attack;
    const std::int16_t facings[14] = {-375, 18, 293, 486, 621, 715, 781, 827, 860, 883, 899, 910, 918, 923};
    int hitUpdate = -1, noticeUpdate = -1, finishUpdate = -1;
    bool statesOk = true, facingsOk = true, rootOk = true;
    for (int k = 1; k <= 16; ++k) {
        const AttackStep step = attack.update(inputAt(session, motion, y, k == 1), motion, session.clips);
        for (const AttackMessage& message : step.messages) {
            if (message.target != session.table) continue;
            if (message.id == kAttackMessageHit && message.damage == 1 && message.kind == 1 && hitUpdate < 0) hitUpdate = k;
            if (message.id == kAttackMessageNotice && noticeUpdate < 0) noticeUpdate = k;
        }
        if (step.finished && finishUpdate < 0) finishUpdate = k;
        if (k <= 14) {
            statesOk = statesOk && step.active && step.state == kAttackStateAction && step.clip == 0x7a;
            facingsOk = facingsOk && motion.facing == facings[k - 1];
            const float expected = k == 1 ? 0.0f : 0.07548f;
            rootOk = rootOk && std::fabs(std::hypot(step.deltaX, step.deltaZ) - expected) < 5e-5f;
        }
        if (k == 1) expect(attack.target() && *attack.target() == session.table, "the tap from the m12-table-before spot targets DonutTable_14 (ctrl+0x140 = 0x08c99320)");
        if (k == 1) expect(motion.lastIntent == 934, "the target heading ctrl+0xa is 934 (atap row 9)", motion.lastIntent);
        motion.x += step.deltaX;
        motion.z += step.deltaZ;
    }
    expect(statesOk, "state 0xa3 with clip 122 (combo_1) in updates 1..14 (atap rows 9..22, ctrl+0x2bc = 122)");
    expect(facingsOk, "the facing turns toward 934 as recorded (ctrl+8 in atap rows 9..22: -375, 18, 293 ... 923)");
    expect(rootOk, "no root step in the entering update, then 0.07548 per update (the step of aaway; atap rows 15, 16: 0.07550)");
    expect(noticeUpdate == 1, "message 62 reaches the table in the entering update (trap ataphit, lr 0x3082c4)", noticeUpdate);
    expect(hitUpdate == 5, "the hit, message 0 with damage 1 and kind 1, reaches DonutTable_14 in update 5 (trap ataphit, lr 0x31a38c, 4 updates after the entry)", hitUpdate);
    expect(finishUpdate == 15, "the attack ends in update 15 (atap row 23: state 1)", finishUpdate);
}

void noTarget(const Session& session) {
    LocomotionState motion;
    motion.x = 715.53168f;
    motion.z = 638.85754f;
    motion.facing = 32460;
    motion.lastIntent = 32451;
    PlayerAttack attack;
    const std::int16_t facings[4] = {32455, 32453, 32452, 32451};
    bool ok = true;
    for (int k = 1; k <= 14; ++k) {
        if (k == 1) motion.facing = turnFacing(motion.facing, motion.lastIntent);
        const AttackStep step = attack.update(inputAt(session, motion, 32.61741f, k == 1), motion, session.clips);
        ok = ok && step.active && step.messages.empty();
        if (k <= 4) ok = ok && motion.facing == facings[k - 1];
        motion.x += step.deltaX;
        motion.z += step.deltaZ;
    }
    expect(!attack.target(), "facing away from the table there is no target (aaway: ctrl+0x140 = 0)");
    expect(ok, "a tap without a target still punches: state 0xa3, no message; the idle step and then the attack state turn the facing in the entering update (32460 -> 32457 -> 32455), then 32453, 32452, 32451 (aaway rows 27..30)");
    expect(std::fabs(motion.x - 715.56177f) < 2e-4f && std::fabs(motion.z - 637.87683f) < 2e-4f,
           "13 root steps of 0.07548 along heading 32451 end at (715.56177, 637.87683) (aaway row 40)", motion.z);
}

void combo(const Session& session, const char* what, const std::vector<int>& tapRows, const std::vector<std::pair<int, int>>& expectedClips,
           const std::vector<int>& hitRows, int idleRow) {
    LocomotionState motion;
    motion.x = 715.44196f;
    motion.z = 641.61597f;
    motion.facing = motion.lastIntent = -936;
    PlayerAttack attack;
    AttackTap y;
    std::vector<int> hits;
    int firstIdle = -1;
    bool clipsOk = true;
    for (int row = 6; row <= 60; ++row) {
        bool held = false;
        for (const int tapRow : tapRows) held = held || row == tapRow - 1;
        const bool tapped = y.update(held);
        const AttackStep step = attack.update(inputAt(session, motion, 32.71405f, tapped), motion, session.clips);
        for (const AttackMessage& message : step.messages)
            if (message.id == kAttackMessageHit && message.target == session.table) hits.push_back(row);
        for (const auto& [clipRow, clip] : expectedClips)
            if (row == clipRow) clipsOk = clipsOk && step.active && step.clip == clip;
        if (!attack.active() && firstIdle < 0 && row > 7) firstIdle = row;
        motion.x += step.deltaX;
        motion.z += step.deltaZ;
    }
    char text[256];
    std::snprintf(text, sizeof text, "%s: the clips of the combo", what);
    expect(clipsOk, text);
    std::snprintf(text, sizeof text, "%s: the hits", what);
    expect(hits == hitRows, text, hits.empty() ? -1.0 : double(hits.front()));
    std::snprintf(text, sizeof text, "%s: back to state 1", what);
    expect(firstIdle == idleRow, text, firstIdle);
}

void real(const char* fib, const char* names) {
    Session session;
    if (!load(fib, names, session)) return;
    clipsMatch(session);
    table(session);
    noTarget(session);
    combo(session, "acombo10 (taps released in rows 7, 16, 25)", {7, 16, 25}, {{7, 0x7a}, {16, 0x7a}, {17, 0x7b}, {25, 0x7b}, {26, 0x7c}, {34, 0x7c}},
          {11, 21, 29}, 35);
    combo(session, "acombo3 (second tap released in row 9, before the window)", {7, 9}, {{7, 0x7a}, {20, 0x7a}}, {11}, 21);
}

}  // namespace

int main(int argc, char** argv) {
    tap();
    bodies();
    geometry();
    if (argc == 3) {
        if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
            std::printf("player-attack-test: data not available, skipped\n");
            return 77;
        }
        real(argv[1], argv[2]);
    }
    if (failures != 0) return 1;
    std::printf("player attack tests passed\n");
    return 0;
}
