// Tests of the smash of destructible props by a moving vehicle (src/runtime/vehicle_prop_smash.*; the vehicle pass 0x16ce9c, the query 0x275c00 with the
// grid gather 0x237f30, the filter 0x2c02c8, the box helpers 0x22ec3c, 0x2335e4, 0x237eb4 and the wobbly handler 0x2a3390). Recorded values: Azahar
// oracle, state drv-police-car-seated, fixed dt 0.033, the trooper poked to (854.43, 17.07, 1087.36) with body+0x8c = -1 and driven with A (scenario p1,
// agent A of round 16, `memtrace 40 p1 0x8a8889c:0x40 0x8e1707c:12 0x8db1dd0:8 0x8db26d0:0x20`): the trooper's node matrix (0x08a8889c), its rigid
// velocity (0x08e1707c), the phone box's entity word +0x10 (0x08db1dd0: 0x0740, bit 0 set from trace row 19 on) and its record +0x10 (health 4, then
// -96 = 0xffa0 from row 19). The first row that shows the smash is row 19. Exits with 77 without data.
#include "assets/assets.hpp"
#include "assets/object_collision.hpp"
#include "runtime/vehicle_prop_smash.hpp"

#include <cmath>
#include <cstdio>
#include <filesystem>

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

assets::Matrix identity() { return {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1}; }

void boxes() {
    assets::Matrix turned = identity();
    turned[0] = 0.0f;
    turned[1] = 1.0f;
    turned[4] = -1.0f;
    turned[5] = 0.0f;
    turned[12] = 10.0f;
    const SmashBox world = worldSmashBox(turned, SmashBox{{1, 2, 3}, {0.5f, 1.0f, 2.0f}});
    expect(std::fabs(world.centre[0] - 8.0f) < 1e-6f && std::fabs(world.centre[1] - 1.0f) < 1e-6f && world.centre[2] == 3.0f, "turned box centre", world.centre[0]);
    expect(std::fabs(world.half[0] - 1.0f) < 1e-6f && std::fabs(world.half[1] - 0.5f) < 1e-6f && world.half[2] == 2.0f, "turned box half extent", world.half[0]);
    const SmashBox flat = worldSmashBox(identity(), SmashBox{{0, 0, 0}, {1, 2, 3}});
    expect(flat.half[1] == 2.0f, "an upright matrix keeps the half extent");
    expect(smashBoxesOverlap(SmashBox{{0, 0, 0}, {1, 1, 1}}, SmashBox{{2, 0, 0}, {1, 1, 1}}), "touching boxes overlap (the compare is inclusive)");
    expect(!smashBoxesOverlap(SmashBox{{0, 0, 0}, {1, 1, 1}}, SmashBox{{2.01f, 0, 0}, {1, 1, 1}}), "separate boxes do not");
}

void damage() {
    expect(applySmashDamage(4).destroyed && applySmashDamage(4).health == -96, "health 4 minus 100 is -96 and destroys (oracle record +0x10 0xffa0)");
    expect(applySmashDamage(100).destroyed, "health 100 is destroyed (0 is not above 0)");
    expect(!applySmashDamage(101).destroyed && applySmashDamage(101).health == 1, "health 101 survives");
    SmashingVehicle still;
    expect(!vehicleSmashPassRuns(still), "no velocity: the pass does not run");
    still.velocity[0] = 0.5f;
    expect(vehicleSmashPassRuns(still), "a velocity runs the pass");
    still.dynamicBody = false;
    expect(!vehicleSmashPassRuns(still), "a kinematic body is not ported");
}

struct Recorded {
    int row;
    assets::Matrix node;
};

const Recorded kRows[] = {
    {14, {-0.709531188f, 0.00157538173f, 0.704672337f, 0, -0.00266553718f, 0.999984324f, -0.00491950475f, 0, -0.704668999f, -0.00536887255f, -0.70951581f, 0, 854.189331f, 18.0370617f, 1082.63489f, 1}},
    {15, {-0.709521055f, 0.00164960383f, 0.70468235f, 0, -0.00461030845f, 0.999965012f, -0.00698280055f, 0, -0.704669237f, -0.00820324663f, -0.70948863f, 0, 854.111084f, 18.0433846f, 1082.55579f, 1}},
    {16, {-0.709509611f, 0.00167536258f, 0.704693735f, 0, -0.00428801402f, 0.99996841f, -0.00669467729f, 0, -0.704682648f, -0.00777167454f, -0.709479928f, 0, 854.028442f, 18.0380917f, 1082.47241f, 1}},
    {17, {-0.709497452f, 0.00166961458f, 0.704705954f, 0, -0.00213354104f, 0.999987543f, -0.00451725442f, 0, -0.704704642f, -0.00470849965f, -0.709485054f, 0, 853.941406f, 18.0267506f, 1082.3844f, 1}},
    {18, {-0.709485173f, 0.00163661502f, 0.704718649f, 0, 0.00119381794f, 0.999998689f, -0.00112047116f, 0, -0.704719484f, 4.63480828e-05f, -0.709486246f, 0, 853.849182f, 18.0128632f, 1082.2915f, 1}},
    {19, {-0.709472179f, 0.00158624689f, 0.704731703f, 0, 0.00491751498f, 0.999984264f, 0.00269977609f, 0, -0.704716325f, 0.00538094481f, -0.709468842f, 0, 853.750916f, 17.9997883f, 1082.19263f, 1}},
    {20, {-0.709459066f, 0.00152928545f, 0.704745173f, 0, 0.00829754211f, 0.999946475f, 0.00618317444f, 0, -0.704697967f, 0.0102343606f, -0.709433675f, 0, 853.645752f, 17.9897709f, 1082.08667f, 1}},
    {21, {-0.709445596f, 0.00147332763f, 0.704758823f, 0, 0.0107798763f, 0.9999035f, 0.00876122341f, 0, -0.70467788f, 0.0138128232f, -0.709393024f, 0, 853.532654f, 17.9838676f, 1081.97302f, 1}},
};

constexpr int kFirstRowWithSmash = 19;

void real(const char* fib, const char* names) {
    assets::FuseArchive archive(fib);
    archive.loadNames(names);
    const assets::FuseEntry* levelEntry = archive.find("levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl");
    if (!levelEntry) {
        std::printf("FAIL: level not in archive\n");
        ++failures;
        return;
    }
    const assets::Level level = assets::parseLevel(archive.read(*levelEntry));
    const std::vector<assets::ObjectCollision> candidates = assets::objectCollisionCandidates(archive, level);
    const assets::ObjectCollision* phoneBox = nullptr;
    for (const assets::ObjectCollision& candidate : candidates)
        if (candidate.instance == "Phone_Box_01") phoneBox = &candidate;
    if (!phoneBox) {
        std::printf("FAIL: Phone_Box_01 has no collision entry\n");
        ++failures;
        return;
    }
    expect(!phoneBox->mesh, "the phone box registers a box");
    expect(std::fabs(phoneBox->centre[1] - 1.4859f) < 1e-3f && std::fabs(phoneBox->half[0] - 0.7620f) < 1e-3f && std::fabs(phoneBox->half[2] - 0.9525f) < 1e-3f,
           "the entity box equals the oracle's +0x60 / +0x6c of 0x08db1dc0 (centre 0, 1.4859, 0; half 0.7620, 1.4859, 0.9506 to 2e-3)");
    SmashingVehicle trooper;
    trooper.nodeBounds = SmashBox{{0.0f, 0.0777002573f, -0.080051899f}, {1.0476476f, 0.992205501f, 2.82325363f}};  // 0x08a888e0..0x08a888f4 of the oracle
    trooper.entityBox = SmashBox{{0.0f, 0.0800969899f, -0.0953124762f}, {0.969588041f, 0.82603395f, 2.76406288f}};  // 0x08a884a0..0x08a884b8
    trooper.velocity[0] = -3.0f;
    SmashableProp prop;
    prop.node = phoneBox->matrix;
    prop.entityBox = SmashBox{{phoneBox->centre[0], phoneBox->centre[1], phoneBox->centre[2]}, {phoneBox->half[0], phoneBox->half[1], phoneBox->half[2]}};
    prop.health = 4;
    VehiclePropSmasher smasher;
    smasher.load(level, assets::archiveClassLookup(archive), candidates);
    expect(smasher.propCount() == 281, "281 wobbly props of lvl_01 register a box (the rest registers a mesh)", double(smasher.propCount()));
    const assets::PlacedObjectState untouched;
    int firstSmash = -1, smasherRow = -1;
    for (const Recorded& recorded : kRows) {
        trooper.node = recorded.node;
        if (vehicleSmashPassRuns(trooper) && vehicleSmashesProp(trooper, prop) && firstSmash < 0) firstSmash = recorded.row;
        for (const std::string& name : smasher.smash(trooper, untouched))
            if (name == "Phone_Box_01" && smasherRow < 0) smasherRow = recorded.row;
    }
    expect(firstSmash + 1 == kFirstRowWithSmash, "the first node matrix that touches the phone box is row 18; the oracle shows the smash one update later (the pass runs before the node moves)", double(firstSmash));
    expect(smasherRow == firstSmash, "the smasher destroys the phone box (WobbleHealth 4 from the level data) at the same row", double(smasherRow));
}

}  // namespace

int main(int argc, char** argv) {
    boxes();
    damage();
    if (argc == 3) {
        if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
            std::printf("vehicle-prop-smash-test: data not available, skipped\n");
            return 77;
        }
        real(argv[1], argv[2]);
    }
    if (failures != 0) return 1;
    std::printf("vehicle prop smash tests passed\n");
    return 0;
}
