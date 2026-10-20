// Tests of the smash of destructible props by a moving vehicle (src/runtime/vehicle_prop_smash.*; the vehicle pass 0x16ce9c, the query 0x275c00 with the
// grid gather 0x237f30, the filter 0x2c02c8, the box helpers 0x22ec3c, 0x2335e4, 0x237eb4 and the wobbly handler 0x2a3390). Recorded values: Azahar
// oracle, state drv-police-car-seated, fixed dt 0.033, the trooper poked to (854.43, 17.07, 1087.36) with body+0x8c = -1 and driven with A (scenario p1,
// agent A of round 16, `memtrace 40 p1 0x8a8889c:0x40 0x8e1707c:12 0x8db1dd0:8 0x8db26d0:0x20`): the trooper's node matrix (0x08a8889c), its rigid
// velocity (0x08e1707c), the phone box's entity word +0x10 (0x08db1dd0: 0x0740, bit 0 set from trace row 19 on) and its record +0x10 (health 4, then
// -96 = 0xffa0 from row 19). The first row that shows the smash is row 19. Exits with 77 without data.
#include "assets/assets.hpp"
#include "assets/object_collision.hpp"
#include "runtime/script_runner.hpp"
#include "runtime/vehicle_prop_smash.hpp"

#include <cmath>
#include <map>
#include <set>
#include <string>
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

// Node matrices of the police van in the user state veh-smash-prop, own Azahar (New 3DS, OpenGL, no RPC, port 24732), fixed dt 0.033, A held, circle pad
// x = -156 for the first 20 updates then 0; `memtrace 20 lampA` and `memtrace 40 lampB` of the node matrix (vehicle entity 0x08a9e580, node 0x08a9e980, matrix at
// node+0x5c). `frame` is the scheduler call counter of the trace. The oracle's 0x23cd80 for Lampost_05 (wobbly entity 0x08d89760, handler 0x2a3390)
// ran in frame 45 (`ctrace 0x23cd80`, same scenario, lampcB); its entity word +0x10 shows bit 0 in the same frame's memtrace row.
const Recorded kLampRows[] = {
    {40, {0.176940501f, 0.0503541902f, -0.982932687f, 0.0f, 0.0342248231f, 0.997771621f, 0.0572752841f, 0.0f, 0.983626366f, -0.0437750183f, 0.174822867f, 0.0f, 870.454041f, 32.714119f, 924.682495f, 1.0f}},
    {41, {0.177574515f, 0.0593529008f, -0.982315958f, 0.0f, 0.0258894451f, 0.997552395f, 0.0649535805f, 0.0f, 0.983766854f, -0.0369657204f, 0.175603271f, 0.0f, 871.121338f, 32.7013664f, 924.820251f, 1.0f}},
    {42, {0.177387893f, 0.0708923489f, -0.981584251f, 0.0f, 0.0101830997f, 0.997216523f, 0.0738615915f, 0.0f, 0.984088242f, -0.0230977219f, 0.176172256f, 0.0f, 871.789856f, 32.6864014f, 924.960938f, 1.0f}},
    {43, {0.176736236f, 0.0825145841f, -0.980793357f, 0.0f, -0.00840741023f, 0.996569932f, 0.0823268816f, 0.0f, 0.984222293f, -0.00630420446f, 0.176823795f, 0.0f, 872.466064f, 32.6678848f, 925.102356f, 1.0f}},
    {44, {0.175935864f, 0.0933942795f, -0.979961276f, 0.0f, -0.026746951f, 0.995575249f, 0.0900803804f, 0.0f, 0.984038234f, 0.0103626065f, 0.177655399f, 0.0f, 873.15509f, 32.6460648f, 925.24408f, 1.0f}},
    {45, {0.175876498f, 0.098397404f, -0.979482174f, 0.0f, -0.0376286693f, 0.994936705f, 0.0931933075f, 0.0f, 0.983692765f, 0.0204661004f, 0.178688526f, 0.0f, 873.86731f, 32.6325722f, 925.380188f, 1.0f}},
    {46, {0.176262319f, 0.101471826f, -0.979099154f, 0.0f, -0.0418856591f, 0.994544744f, 0.0955321118f, 0.0f, 0.983451724f, 0.0241715014f, 0.179550946f, 0.0f, 874.602661f, 32.6274261f, 925.518005f, 1.0f}},
    {47, {0.177038848f, 0.103421852f, -0.978754938f, 0.0f, -0.0402858593f, 0.99439162f, 0.0977871567f, 0.0f, 0.983379066f, 0.0221178606f, 0.180212379f, 0.0f, 875.359924f, 32.6305847f, 925.658691f, 1.0f}},
};

// Node matrices of the same van without steering (`memtrace 62 smashmem2 0x08a9e9dc:0x40 ...`, rows start at frame 3), A held from the start. The oracle's
// 0x23cd80 ran for Trafficlight31 (entity 0x08de6690, handler 0x1bc38c, health 3 -> -97) in frame 19 and for Generic_Tree_13 (0x08d6cfb0, handler 0x2a3390) in
// frame 50 (`ctrace 0x23cd80,... 160 lamp2`, ctrace frame numbers equal the memtrace frame = row + 3). The vehicle's node bounds (node+0xa0) and entity box (E+0x60)
// are the first row's values.
const Recorded kCarRows[] = {
    {14, {-0.43558836f, 0.00239482173f, -0.900142848f, 0.0f, -0.00550552364f, 0.999970675f, 0.00532459235f, 0.0f, 0.900129139f, 0.00727508729f, -0.435562372f, 0.0f, 859.210388f, 32.5707664f, 922.386292f, 1.0f}},
    {15, {-0.435714722f, 0.00236855447f, -0.900081635f, 0.0f, -0.0056232065f, 0.99996984f, 0.00535351178f, 0.0f, 0.90006721f, 0.0073939492f, -0.435688257f, 0.0f, 859.467896f, 32.5705795f, 922.262512f, 1.0f}},
    {16, {-0.435843229f, 0.00234081526f, -0.900019586f, 0.0f, -0.0055465484f, 0.999970615f, 0.00528674293f, 0.0f, 0.900005519f, 0.00729619246f, -0.43581748f, 0.0f, 859.742737f, 32.5705986f, 922.130188f, 1.0f}},
    {17, {-0.435972691f, 0.00225050515f, -0.89995718f, 0.0f, -0.00548514863f, 0.999971628f, 0.00515781995f, 0.0f, 0.899943233f, 0.00718506705f, -0.435948014f, 0.0f, 860.035156f, 32.5708008f, 921.989319f, 1.0f}},
    {18, {-0.436103582f, 0.00214541983f, -0.89989388f, 0.0f, -0.00545748137f, 0.999972463f, 0.00502880197f, 0.0f, 0.899879932f, 0.00710423291f, -0.43607986f, 0.0f, 860.345215f, 32.5708885f, 921.839783f, 1.0f}},
    {19, {-0.436233997f, 0.0020303661f, -0.899830878f, 0.0f, -0.00547093293f, 0.999972999f, 0.00490860967f, 0.0f, 0.899816573f, 0.00706421724f, -0.436211109f, 0.0f, 860.672668f, 32.5708084f, 921.681824f, 1.0f}},
    {44, {-0.443801522f, 0.235453188f, -0.864639699f, 0.0f, 0.00576686859f, 0.965595484f, 0.259984791f, 0.0f, 0.896106422f, 0.110395394f, -0.429890513f, 0.0f, 873.649048f, 33.1733551f, 915.546082f, 1.0f}},
    {45, {-0.441913247f, 0.258621812f, -0.858968973f, 0.0f, 0.000404328108f, 0.957597435f, 0.288109273f, 0.0f, 0.897057891f, 0.12697196f, -0.423279643f, 0.0f, 874.343323f, 33.2395363f, 915.274719f, 1.0f}},
    {46, {-0.440819144f, 0.28194809f, -0.852164268f, 0.0f, 7.23749399e-05f, 0.949396193f, 0.314080983f, 0.0f, 0.897596002f, 0.138391197f, -0.418532372f, 0.0f, 875.054871f, 33.3227272f, 914.999451f, 1.0f}},
    {47, {-0.438774943f, 0.295358628f, -0.84866941f, 0.0f, 0.00580945611f, 0.945351005f, 0.326002687f, 0.0f, 0.898578167f, 0.138111502f, -0.416512251f, 0.0f, 875.795654f, 33.3784637f, 914.693909f, 1.0f}},
    {48, {-0.434295654f, 0.313762695f, -0.844357967f, 0.0f, 0.020727098f, 0.940605819f, 0.338867337f, 0.0f, 0.900532007f, 0.129667461f, -0.415004373f, 0.0f, 876.561462f, 33.4037094f, 914.388428f, 1.0f}},
    {49, {-0.427795053f, 0.335646421f, -0.839245498f, 0.0f, 0.0464712083f, 0.935435891f, 0.350428522f, 0.0f, 0.902680457f, 0.110910803f, -0.415772676f, 0.0f, 877.338562f, 33.4191551f, 914.078247f, 1.0f}},
    {50, {-0.425827265f, 0.349687248f, -0.834499776f, 0.0f, 0.0812880993f, 0.933354199f, 0.349631429f, 0.0f, 0.901145518f, 0.0810477287f, -0.42587316f, 0.0f, 878.144958f, 33.4717064f, 913.742676f, 1.0f}},
    {51, {-0.426166892f, 0.346981913f, -0.835455179f, 0.0f, 0.105662473f, 0.936288536f, 0.334961534f, 0.0f, 0.89845264f, 0.054473266f, -0.435678124f, 0.0f, 878.949524f, 33.5298119f, 913.381226f, 1.0f}},
    {52, {-0.428689957f, 0.341916978f, -0.836252272f, 0.0f, 0.117876753f, 0.938874841f, 0.323448718f, 0.0f, 0.895728886f, 0.0400844812f, -0.44279027f, 0.0f, 879.740906f, 33.5743828f, 913.016357f, 1.0f}},
};

constexpr int kLampPortFrame = 44;

// A prop that a car smashes goes through the same entity destroy as an on-foot hit (0x23cd80, message 1 of the base prop handler 0x1f8948), so its
// TriggerObject fires. 1-3_Crate_Swing is a leprops/leprop_wobblydestructible (kind 119) of lvl_01 whose TriggerObject names the object
// 1-3_Crate_Swing_Script (level data, census of the C++ loader 2026-10-19).
void realTriggerObject(const assets::Level& level, const VehiclePropSmasher& smasher, const assets::ClassLookup& classOf) {
    const std::optional<SmashedProp> data = smasher.destroyData("1-3_crate_swing");
    expect(data.has_value() && data->effects.explosionParticle != 0, "the crate has the data the car path hands to the destroy");
    assets::PlacedObjectState state;
    state.disabled.insert("1-3_crate_swing_script");
    ScriptRunner runner(level, state);
    runner.loadObjectKinds(classOf);
    runner.destroyEntity("1-3_crate_swing");
    expect(state.destroyed.count("1-3_crate_swing") == 1 && state.isHidden("1-3_crate_swing"), "the smashed crate is destroyed and hidden");
    expect(!state.isHidden("1-3_crate_swing_script"), "TriggerObject enables its object (0x244f40)");
    bool sent = false;
    for (const ScriptObjectMessage& message : runner.takeObjectMessages()) sent |= message.instance == "1-3_crate_swing_script" && message.message == kMessageTrigger;
    expect(sent, "and sends it the trigger message 0xff");
    runner.destroyEntity("1-3_crate_swing");
    expect(runner.takeObjectClassMessages().size() == 1, "a second destroy of the same entity does nothing");
}

void realOtherClasses(const assets::Level& level, const std::vector<assets::ObjectCollision>& candidates, const assets::ClassLookup& classOf) {
    SmashingVehicle car;
    car.nodeBounds = SmashBox{{-0.00140833855f, 0.0381139517f, -0.0761752129f}, {1.18461537f, 1.25735652f, 2.51457548f}};
    car.entityBox = SmashBox{{-0.00197023153f, -0.0202310085f, -0.0392076969f}, {0.933585763f, 1.19856739f, 2.4914875f}};
    car.velocity[0] = 8.0f;
    VehiclePropSmasher smasher;
    smasher.load(level, classOf, candidates);
    const assets::PlacedObjectState untouched;
    std::map<std::string, int> firstFrame;
    std::map<std::string, std::optional<DebrisSuperBlock>> superBlock;
    for (const Recorded& recorded : kCarRows) {
        car.node = recorded.node;
        for (const SmashedProp& smashed : smasher.smash(car, untouched))
            if (!firstFrame.count(smashed.instance)) {
                firstFrame[smashed.instance] = recorded.row + 3;
                superBlock[smashed.instance] = smashed.effects.superBlock;
            }
    }
    // 0x2cdca4 (llvm-objdump, docs/re/debris.md entry of round 47): the class SuperBuildValue 3.0 of props/prop_trafficlight; the wobbly kind 0x77 has
    // none, so 6, with the instance's SpawnDir 337 (docs/re/studs-drops.md) that 0x2c8278 reads.
    const auto& light = superBlock["Trafficlight31"];
    expect(light && light->value == 3 && light->count == 9 && light->spawnDir == kSuperDebrisNoSpawnDir, "Trafficlight31: super brick value 3, nine bricks");
    const auto& tree = superBlock["Generic_Tree_13"];
    expect(tree && tree->value == 6 && tree->count == 9 && tree->spawnDir == 337, "Generic_Tree_13: kind 0x77 value 6, SpawnDir 337");
    expect(firstFrame.count("Trafficlight31") && firstFrame["Trafficlight31"] == 19,
           "Trafficlight31 (props/prop_trafficlight, box entry) is smashed in the update the oracle's 0x23cd80 runs in: frame 19", double(firstFrame["Trafficlight31"]));
    expect(firstFrame.count("Generic_Tree_13") && firstFrame["Generic_Tree_13"] == 50,
           "Generic_Tree_13 (leprop_wobblydestructible, box entry) is smashed in frame 50 like the oracle's 0x23cd80", double(firstFrame["Generic_Tree_13"]));
}

// Kind 67 (src/runtime/prop_hit_sequence.*): the van's boxes put on each object's node. The five objects of mode 1 (`Destructable`, Health 1) are
// destroyed; mode 0 objects and the four roadblocks with a physics body (`NoPhysics` 0, no message from 0x275c00) are not. Level data of lvl_01 and the
// oracle grid census r41-A-2 / A-3 (artifacts/oracle/r41-servant-out: records +0xcc = 1 for exactly these five entities).
void realKind67(const assets::Level& level, const std::vector<assets::ObjectCollision>& candidates, const assets::ClassLookup& classOf) {
    SmashingVehicle car;
    car.nodeBounds = SmashBox{{-0.00140833855f, 0.0381139517f, -0.0761752129f}, {1.18461537f, 1.25735652f, 2.51457548f}};
    car.entityBox = SmashBox{{-0.00197023153f, -0.0202310085f, -0.0392076969f}, {0.933585763f, 1.19856739f, 2.4914875f}};
    car.velocity[0] = 8.0f;
    VehiclePropSmasher smasher;
    smasher.load(level, classOf, candidates);
    const assets::PlacedObjectState untouched;
    std::set<std::string> destroyed;
    for (const char* instance : {"1-1_Donut_Van_Body", "1-2_Dog", "1-2_RoofProp_03", "1-2_Temp_Trampoline", "1-3_Lawnmower_Final", "1-1_BrokenVan",
                                 "1-3_smash_crate", "1-4_Window", "1-4_Roadblock_12"}) {
        for (const assets::ObjectCollision& candidate : candidates) {
            if (candidate.instance != instance) continue;
            car.node = assets::Matrix{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, candidate.matrix[12], candidate.matrix[13], candidate.matrix[14], 1};
            for (const SmashedProp& smashed : smasher.smash(car, untouched)) destroyed.insert(smashed.instance);
        }
    }
    for (const char* instance : {"1-1_Donut_Van_Body", "1-2_Dog", "1-2_RoofProp_03", "1-2_Temp_Trampoline", "1-3_Lawnmower_Final"})
        expect(destroyed.count(instance) == 1, (std::string("the car destroys the mode 1 object ") + instance).c_str());
    for (const char* instance : {"1-1_BrokenVan", "1-3_smash_crate", "1-4_Window", "1-4_Roadblock_12"})
        expect(destroyed.count(instance) == 0, (std::string("the car leaves ") + instance + " (mode 0, or a physics body)").c_str());
}

void realLamp(const assets::Level& level, const std::vector<assets::ObjectCollision>& candidates, const assets::ClassLookup& classOf) {
    bool lampIsMesh = false;
    for (const assets::ObjectCollision& candidate : candidates)
        if (candidate.instance == "Lampost_05") lampIsMesh = bool(candidate.mesh);
    expect(lampIsMesh, "Lampost_05 registers a mesh entry (all 62 Lampost_* of lvl_01 do)");
    SmashingVehicle car;
    car.nodeBounds = SmashBox{{-0.00140833855f, 0.0381139517f, -0.0761752129f}, {1.18461537f, 1.25735652f, 2.51457548f}};
    car.entityBox = SmashBox{{-0.00197023153f, -0.0202310085f, -0.0392076969f}, {0.933585763f, 1.19856739f, 2.4914875f}};
    car.velocity[0] = 20.0f;
    VehiclePropSmasher smasher;
    smasher.load(level, classOf, candidates);
    const assets::PlacedObjectState untouched;
    int first = -1;
    for (const Recorded& recorded : kLampRows) {
        car.node = recorded.node;
        for (const SmashedProp& smashed : smasher.smash(car, untouched))
            if (smashed.instance == "Lampost_05" && first < 0) first = recorded.row;
    }
    expect(first == kLampPortFrame, "the car smashes the mesh-kind Lampost_05 (the oracle destroys it in frame 45, the port one update earlier: VEH-130)", double(first));
}

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
    realOtherClasses(level, candidates, assets::archiveClassLookup(archive));
    realLamp(level, candidates, assets::archiveClassLookup(archive));
    realKind67(level, candidates, assets::archiveClassLookup(archive));
    realTriggerObject(level, smasher, assets::archiveClassLookup(archive));
    expect(smasher.propCount() == 743, "743 smashable props in lvl_01: 674 leprop_wobblydestructible, 38 prop_trafficlight, 15 prop_respawnabledestructible, "
           "16 of kind 67 without a physics body (level data census, box and mesh entries; the 22 registered kind 67 objects match the oracle grid, r41-A-3)", double(smasher.propCount()));
    const assets::PlacedObjectState untouched;
    int firstSmash = -1, smasherRow = -1;
    PropDestroyEffects phoneBoxEffects;
    for (const Recorded& recorded : kRows) {
        trooper.node = recorded.node;
        if (vehicleSmashPassRuns(trooper) && vehicleSmashesProp(trooper, prop) && firstSmash < 0) firstSmash = recorded.row;
        for (const SmashedProp& smashed : smasher.smash(trooper, untouched))
            if (smashed.instance == "Phone_Box_01" && smasherRow < 0) {
                smasherRow = recorded.row;
                phoneBoxEffects = smashed.effects;
            }
    }
    expect(firstSmash + 1 == kFirstRowWithSmash, "the first node matrix that touches the phone box is row 18; the oracle shows the smash one update later (the pass runs before the node moves)", double(firstSmash));
    expect(smasherRow == firstSmash, "the smasher destroys the phone box (WobbleHealth 4 from the level data) at the same row", double(smasherRow));
    expect(phoneBoxEffects.explosionParticle == 10 && phoneBoxEffects.spawnDebris && phoneBoxEffects.debrisAmount == 6 && phoneBoxEffects.respawn,
           "Phone_Box_01 (lvl_01 and leprop_wobblydestructible values): ExplosionParticleDef 10, SpawnDebris 1, DebrisAmount 6, Respawn 1",
           double(phoneBoxEffects.explosionParticle));
    expect(phoneBoxEffects.colour == 0xff818181u && phoneBoxEffects.secondColour == 0xff00bedfu && phoneBoxEffects.ratio == 0.0f,
           "its DebrisColour (129, 129, 129) and DebrisColour2 (223, 190, 0) with DebrisRatio 0 (level data, 0x20c938 byte order r, g, b, a)",
           double(phoneBoxEffects.colour));
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
