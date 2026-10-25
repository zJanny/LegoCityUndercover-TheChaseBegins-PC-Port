// Detective tool on lvl_01_cherrytreehills (src/runtime/detective.cpp): the objects of mission 1-2, the footprints of 1-2_Dtv_DogSpot_01 against
// the record read in the oracle, and the run from the detective point through the X spot to the trigger of 1-2_SO_IntroCamOff with the update
// numbers of the oracle traces (docs/re/abilities-detective.md). Needs the extracted RomFS; exits with 77 without it.
#include "assets/assets.hpp"
#include "runtime/detective.hpp"
#include "runtime/level_start.hpp"

#include <array>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace chase;
constexpr const char* kLevelFile = "levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl";

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

// Provenance: oracle save state on the detective point of mission 1-2, memtrace of the footprint array (record +0x3c of 1-2_Dtv_DogSpot_01, 0x08e191e0, 33 entries of
// 16 bytes, shorts at +4/+6/+8), scenario b2 of docs/re/abilities-detective.md.
const std::array<std::array<std::int16_t, 3>, 33> kOracleFootprints = {{
    {0, 0, 1},     {-1, 0, -1},   {-4, 0, 1},    {-7, 0, -1},   {-10, 0, 2},   {-13, 0, -1},  {-15, 0, 2},   {-18, 0, 0},   {-21, 0, 2},
    {-22, 0, 0},   {-26, 0, 4},   {-26, 0, 2},   {-30, 0, 6},   {-27, 0, 8},   {-31, 0, 11},  {-28, 1, 13},  {-31, 0, 16},  {-28, 3, 18},
    {-31, 3, 20},  {-28, 7, 22},  {-31, 6, 25},  {-28, 9, 27},  {-31, 9, 29},  {-29, 11, 31}, {-33, 10, 35}, {-32, 11, 34}, {-36, 11, 39},
    {-38, 10, 35}, {-39, 10, 38}, {-43, 11, 33}, {-44, 11, 35}, {-49, 11, 30}, {-49, 11, 32},
}};

const runtime::DetectiveSpot& spotNamed(const runtime::DetectiveSet& set, const char* name) {
    for (const runtime::DetectiveSpot& spot : set.spots())
        if (spot.instance == name) return spot;
    throw std::runtime_error(std::string("no detective spot ") + name);
}
const runtime::RevealSpot& revealSpotNamed(const runtime::DetectiveSet& set, const char* name) {
    for (const runtime::RevealSpot& spot : set.revealSpots())
        if (spot.instance == name) return spot;
    throw std::runtime_error(std::string("no reveal spot ") + name);
}
const runtime::RevealProp& revealPropNamed(const runtime::DetectiveSet& set, const char* name) {
    for (const runtime::RevealProp& prop : set.revealProps())
        if (prop.instance == name) return prop;
    throw std::runtime_error(std::string("no reveal prop ") + name);
}

bool activates(const std::vector<runtime::DetectiveAction>& actions, const std::string& instance) {
    for (const runtime::DetectiveAction& action : actions)
        if (action.kind == runtime::DetectiveActionKind::Activate && action.instance == instance) return true;
    return false;
}

// Provenance: oracle trap E-1 (save state with the detective tool active in mission 1-2, circle pad forward for 220 updates, breakpoint 0x26049c in the footprint placing function
// 0x26024c, trap output E-1 of round 27 agent E; docs/re/abilities-detective.md): the matrix at r1 of every hit. Rotation rows are the identity for all of them;
// the translation is the point the ray hit. The x / z are the matrix's own; the last two are on the dog house stairs.
const float kOracleHits[13][3] = {{718.1799f, 29.7764f, 631.0478f}, {717.9299f, 29.7738f, 630.5478f}, {716.9299f, 29.7790f, 631.5478f},
                                  {716.9299f, 29.7763f, 631.0478f}, {716.6799f, 29.7842f, 632.5478f}, {715.9299f, 29.7817f, 632.0478f},
                                  {716.4299f, 29.9572f, 633.7978f}, {715.6799f, 29.7890f, 633.2978f}, {715.6799f, 30.4681f, 634.5478f},
                                  {716.4299f, 30.7363f, 635.0478f},
                                  // Provenance: oracle trap E-2 (save state on the second detective waypoint of mission 1-2, A press, then forward), the second spot: identity rows too.
                                  {719.1412f, 37.0395f, 656.7838f}, {718.1412f, 37.0395f, 656.0338f}, {719.1412f, 37.0395f, 658.0338f}};

void footprintGround(assets::FuseArchive& archive) {
    const assets::FuseEntry* master = archive.find("models/lvl_01_cherrytreehills/lvl_01_cherrytreehills.bfnmdl");
    require(master != nullptr, "level master model");
    const assets::Model model = assets::parseModel(archive.read(*master));
    runtime::CollisionWorld world;
    for (const assets::CollisionItem& item : model.collision) world.add(item);
    for (const auto& hit : kOracleHits) {
        const float point[3] = {hit[0], hit[1] + 0.1f, hit[2]};
        const std::optional<float> ground = runtime::footprintGroundHeight(world, point);
        require(ground && std::fabs(*ground - hit[1]) < 2e-4f, "footprint ground at " + std::to_string(hit[0]) + " " + std::to_string(hit[2]));
    }
}

void footprintModels(assets::FuseArchive& archive) {
    const assets::FuseEntry* charbits = archive.find("levels/charbits.blvl");
    require(charbits != nullptr, "levels/charbits.blvl");
    const assets::Level bits = assets::parseLevel(archive.read(*charbits));
    int found = 0;
    for (const assets::Placement& placement : assets::placements(bits)) {
        if (placement.instance == "DetectiveFP00") require(placement.mesh == "Ability_Detective_Foot", "DetectiveFP00 mesh " + placement.mesh), ++found;
        if (placement.instance == "DogFP15") require(placement.mesh == "Ability_Detective_Dog", "DogFP15 mesh " + placement.mesh), ++found;
    }
    require(found == 2, "pool objects found");
}

// Provenance: dump of models/ability_detective_dog (parseModel): the 20 card vertices ring (0.8501, 0.026, -2.9071) in model space and node 0 has the
// matrix translation (-0.8547, 0, 2.9071); the foot model has no matrix block and its card is centred on the origin.
void footprintCardCentre(assets::FuseArchive& archive) {
    const assets::FuseEntry* dog = archive.find("models/ability_detective_dog/ability_detective_dog.bfnmdl");
    require(dog != nullptr, "dog footprint model");
    const assets::Model model = assets::parseModel(archive.read(*dog));
    require(model.nodeMatrices.size() == 1 && model.nodeMatrices[0][15] == 1.0f, "dog node matrix");
    require(std::fabs(model.nodeMatrices[0][12] + 0.8547f) < 1e-3f && model.nodeMatrices[0][13] == 0.0f && std::fabs(model.nodeMatrices[0][14] - 2.9071f) < 1e-3f,
            "dog node translation");
    float sum[3] = {0, 0, 0};
    const assets::ModelPart& part = model.meshes.at(0).parts.at(0);
    for (const assets::Vertex& vertex : part.vertices)
        for (int a = 0; a < 3; ++a) sum[a] += vertex.position[a] + model.nodeMatrices[0][12 + a];
    require(std::fabs(sum[0] / float(part.vertices.size())) < 0.01f && std::fabs(sum[2] / float(part.vertices.size())) < 0.01f, "dog card is centred by its node matrix");
}

// Provenance: the quaternion block of the paths (level section +0x20, record +0x10) read by 0x270b70; the DetectivePaths of 1-2_Dtv_DogSpot_01 and _02
// carry only identity quaternions (dump of Level::paths), which the oracle trap E-1 confirms (identity rows). The bytes of 1-3_Dtv_BrickSpot_01 are
// computed with the port's reading of 0x270b70 / 0x21a600 (no oracle measurement; the state is not available).
void footprintRotation(const runtime::DetectiveSet& set) {
    for (const char* name : {"1-2_dtv_dogspot_01", "1-2_dtv_dogspot_02"})
        for (const runtime::DetectiveFootprint& footprint : spotNamed(set, name).footprints)
            require(footprint.rotation == std::array<std::int8_t, 4>({0, 0, 0, 127}), std::string("identity rotation on ") + name);
    const runtime::DetectiveSpot& brick = spotNamed(set, "1-3_dtv_brickspot_01");
    require(brick.footprints.size() == 69, "brick spot footprints");
    require(brick.footprints[0].rotation == std::array<std::int8_t, 4>({0, 126, 0, -4}), "brick spot footprint 0 rotation");
    require(brick.footprints[6].rotation == std::array<std::int8_t, 4>({0, 120, 0, -39}), "brick spot footprint 6 rotation");
}

void objects(const runtime::DetectiveSet& set) {
    const runtime::DetectiveSpot& first = spotNamed(set, "1-2_dtv_dogspot_01");
    require(first.revealSwitch == "1-2_dtv_dogpath_xspot", "RevealSwitch " + first.revealSwitch);
    require(first.triggerObject == "1-2_dtv_dogspot_01_script", "TriggerObject " + first.triggerObject);
    require(first.footprintType == 1, "FootprintType 1");
    // Provenance: oracle the save state on the detective point of mission 1-2, record +0x40 = 0x21 footprints and the array above.
    require(first.footprints.size() == kOracleFootprints.size(), "footprint count " + std::to_string(first.footprints.size()));
    for (std::size_t i = 0; i < kOracleFootprints.size(); ++i)
        for (std::size_t a = 0; a < 3; ++a)
            require(first.footprints[i].offset[a] == kOracleFootprints[i][a],
                    "footprint " + std::to_string(i) + " axis " + std::to_string(a) + ": " + std::to_string(first.footprints[i].offset[a]));
    // Provenance: oracle the save state on the detective point of mission 1-2, use zone record of the spot (0x086e87b8): centre (0, 1, 0), half (1.5, 1.5, 1.5), shape 3; class file UseBound.
    require(first.useZone.present && first.useZone.centre[1] == 1.0f && first.useZone.half[0] == 1.5f && first.useZone.half[2] == 1.5f, "UseBound");
    require(first.questionZone.present && first.questionZone.half[0] == 30.0f, "QuestionBound");
    const runtime::DetectiveSpot& second = spotNamed(set, "1-2_dtv_dogspot_02");
    require(second.revealSwitch == "1-2_dtv_dogpath_xspot01" && second.triggerObject.empty(), "1-2_Dtv_DogSpot_02 references");

    const runtime::RevealSpot& x = revealSpotNamed(set, "1-2_dtv_dogpath_xspot");
    require(x.revealTime == 1.0f && x.fadeObject == "1-2_trapdoor_01" && x.revealObject == "1-2_trapdoor_01" && x.triggerObject == "1-2_so_introcamoff",
            "1-2_Dtv_DogPath_XSpot parameters");
    const runtime::RevealSpot& x01 = revealSpotNamed(set, "1-2_dtv_dogpath_xspot01");
    require(x01.revealObject == "1-2_trapdoor_03" && x01.triggerObject.empty(), "1-2_Dtv_DogPath_XSpot01 parameters");

    // Provenance: oracle d1 (the save state on the dog house X of mission 1-2): launch 30 updates after the change to state 1 (RevealDelay 1.2 read as 1, times 30) and the trigger 18
    // updates after the launch (class RevealTime 0.6 times 30); spawn sound 0xa5 (ATTR_SFXObjectSpawn), done sound 0.
    const runtime::RevealProp& mound = revealPropNamed(set, "1-2_trapdoor_01");
    require(mound.revealObject == "1-2_dog_bone" && mound.triggerObject == "1-2_so_introcamoff", "1-2_TrapDoor_01 references");
    require(mound.delayUpdates == 30 && mound.flightUpdates == 18 && mound.soundSpawn == 0xa5 && mound.soundDone == 0, "1-2_TrapDoor_01 timing");
    const runtime::RevealProp& third = revealPropNamed(set, "1-2_trapdoor_03");
    require(third.revealObject == "ctg_tshirt" && third.triggerObject == "1-2_so_dspot_02_x", "1-2_TrapDoor_03 references");
    require(revealPropNamed(set, "1-2_trapdoor_02").triggerObject.empty(), "1-2_TrapDoor_02 has no trigger");
}

void run(const assets::Level& level, const assets::ClassLookup& classOf) {
    runtime::DetectiveSet set;
    set.load(level, classOf);
    assets::PlacedObjectState state = runtime::levelInitialState(level, classOf);
    require(state.isDisabled("1-2_dtv_dogpath_xspot") && state.isDisabled("1-2_trapdoor_01") && state.isDisabled("1-2_dog_bone"),
            "the post-links hide the X spot, the reveal prop and the bone");
    const auto apply = [&](const std::vector<runtime::DetectiveAction>& actions) {
        for (const runtime::DetectiveAction& action : actions) {
            if (action.kind == runtime::DetectiveActionKind::Show || action.kind == runtime::DetectiveActionKind::Activate) state.disabled.erase(action.instance);
            if (action.kind == runtime::DetectiveActionKind::Hide) state.disabled.insert(action.instance);
        }
    };
    // Provenance: oracle save state on the detective point of mission 1-2 (player 723.675 29.768 630.775) and the save state with the detective tool active in mission 1-2 (forward -0.926 0 0.378);
    // teleport 711.99 32.6 637.44 of scenario c1.
    runtime::DetectiveInput input;
    input.dt = 0.033f;
    input.player[0] = 723.675f, input.player[1] = 29.768f, input.player[2] = 630.775f;
    input.forward[0] = -0.926f, input.forward[2] = 0.378f;
    input.actionPressed = true;
    apply(set.update(state, input));
    input.actionPressed = false;
    require(set.playerState() == runtime::DetectivePlayerState::Out && set.modeActive(), "the A press on the point starts detect_out");
    apply(set.update(state, input));
    require(spotNamed(set, "1-2_dtv_dogspot_01").state == 1, "the spot is in state 1 one update after the press (oracle a1: press 4, change 5)");
    for (unsigned i = 1; i < 36; ++i) apply(set.update(state, input));
    require(set.playerState() == runtime::DetectivePlayerState::Out, "detect_out still playing at update 36");
    apply(set.update(state, input));
    require(set.playerState() == runtime::DetectivePlayerState::Normal, "back to state 1 37 updates after the press (oracle a1: 4 -> 41)");
    require(set.modeActive(), "detective mode stays on");

    input.player[0] = 711.99f, input.player[1] = 32.6f, input.player[2] = 637.44f;
    apply(set.update(state, input));
    apply(set.update(state, input));
    require(spotNamed(set, "1-2_dtv_dogspot_01").state == 2 && set.playerState() == runtime::DetectivePlayerState::In && !set.modeActive(),
            "the X in front ends detective mode (oracle c1: state 2 and request 0x118 in one update)");
    require(!state.isDisabled("1-2_dtv_dogpath_xspot"), "state 2 shows the X spot");
    for (unsigned i = 0; i < 19; ++i) apply(set.update(state, input));
    require(set.playerState() == runtime::DetectivePlayerState::Normal, "detect_in ends after 19 updates (oracle c1: 4 -> 23)");

    // Provenance: oracle d1 (the save state on the dog house X of mission 1-2, update numbers relative to the press in frame 3): request 0x1a2 at 0, animation event 43, the
    // X spot's state 2 with both activations at 76, launch of the bone at 107, the trigger of 1-2_SO_IntroCamOff from the reveal prop at 125.
    input.player[0] = 710.082f, input.player[1] = 32.677f, input.player[2] = 638.595f;
    input.actionPressed = true;
    apply(set.update(state, input));
    input.actionPressed = false;
    require(set.playerState() == runtime::DetectivePlayerState::Use, "the A press on the X starts the use state");
    int stateTwo = -1, launch = -1, landed = -1;
    for (int update = 1; update <= 130; ++update) {
        const std::vector<runtime::DetectiveAction> actions = set.update(state, input);
        apply(actions);
        if (stateTwo < 0 && activates(actions, "1-2_trapdoor_01")) {
            stateTwo = update;
            require(activates(actions, "1-2_so_introcamoff"), "the X spot fires its TriggerObject in the same update");
        }
        if (launch < 0 && revealPropNamed(set, "1-2_trapdoor_01").flightMode == 2) launch = update;
        if (landed < 0 && launch >= 0 && activates(actions, "1-2_so_introcamoff")) landed = update;
        if (update == 116) require(set.playerState() == runtime::DetectivePlayerState::Normal, "the use state ends at 116");
    }
    require(stateTwo == 76, "X spot state 2 at " + std::to_string(stateTwo));
    require(launch == 107, "launch at " + std::to_string(launch));
    require(landed == 125, "reveal prop trigger at " + std::to_string(landed));
    require(!state.isDisabled("1-2_dog_bone"), "the bone is shown");
}
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: detective-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "detective-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        const assets::FuseEntry* entry = archive.find(kLevelFile);
        require(entry != nullptr, std::string("not in archive: ") + kLevelFile);
        const assets::Level level = assets::parseLevel(archive.read(*entry));
        const assets::ClassLookup classOf = assets::archiveClassLookup(archive);
        runtime::DetectiveSet set;
        set.load(level, classOf);
        objects(set);
        footprintRotation(set);
        footprintGround(archive);
        footprintModels(archive);
        footprintCardCentre(archive);
        run(level, classOf);
    } catch (const std::exception& error) {
        std::cerr << "detective-real-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "detective-real-test: ok\n";
    return 0;
}
