// Tests of src/runtime/traffic_car_lights.*: the part lookup of the vehicle create 0x1658a8 (the class values BrakeLightModel%02u,
// BrakeLightLocator%02u and BrakeLightParticle, the node search 0x23645c) and the lights of the traffic cars with the culling bit of the node
// (0x278b78 with 0x23de70). Every expected value follows from the disassembly (llvm-objdump --triple=armv6k-none-eabi --mattr=+vfp2 of
// artifacts/game/game.arm.elf, read by 20261111-chase-G on 2026-11-11, docs/re/traffic-car-lights.md). With the two arguments of the shipped
// archive and its names table the class values and node names of the lvl_01 traffic cars are read from the data (exit 77 without it).
#include "assets/assets.hpp"
#include "assets/object_classes.hpp"
#include "runtime/level_start.hpp"
#include "runtime/traffic_car_lights.hpp"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

using namespace chase;
using namespace chase::runtime;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

// The node names of the model of the lvl_01 taxi (Traffic02), read with assets::parseModel from the shipped archive by 20261111-chase-G on
// 2026-11-11 (the model with the atlas of lvl_01_cherrytreehills_main).
const std::vector<std::string> kTaxiNodes = {"Chassis", "LightsRL", "LightsRR", "BrakeLight"};

TrafficBrakeLights taxiLights() {
    TrafficBrakeLightClass values;
    values.models[0] = "BrakeLight";
    values.locators[0] = "LightsRL";
    values.locators[1] = "LightsRR";
    return makeTrafficBrakeLights(values, kTaxiNodes);
}

void testPartLookup() {
    require(trafficLightNodeIndex(kTaxiNodes, "BrakeLight") == 3, "the node BrakeLight is node 3");
    require(trafficLightNodeIndex(kTaxiNodes, "brakelight") == 3, "the compare 0x25ad90 ignores case");
    require(trafficLightNodeIndex(kTaxiNodes, "LightsRR") == 2, "LightsRR is node 2");
    require(trafficLightNodeIndex(kTaxiNodes, "Missing") == -1, "an unknown name gives -1");
    require(trafficLightNodeIndex(kTaxiNodes, "") == -1, "an empty value is not read");
    const TrafficBrakeLights lights = taxiLights();
    require(lights.parts[0] == 3 && lights.parts[1] == -1, "the second model value is not set");
    require(lights.glowParts[0] == 1 && lights.glowParts[1] == 2 && lights.glowParts[2] == -1 && lights.glowParts[3] == -1, "two glow parts");
    require(lights.glowEffect == 242, "BrakeLightParticle defaults to 242");
    require(lights.level == 0.0f && lights.glows[0] == 0, "no level, no effect");
}

void testNoPartsNoLights() {
    TrafficCarLights cars;
    cars.load(2);
    cars.setup(0, makeTrafficBrakeLights(TrafficBrakeLightClass{}, kTaxiNodes));
    cars.run(0, 1.0f, 0.033f);
    const TrafficCarLights::Changes changes = cars.takeChanges();
    require(changes.parts.empty() && changes.glows.empty(), "a car without a first brake light part does nothing");
    cars.run(1, 1.0f, 0.033f);
    require(cars.takeChanges().parts.empty(), "a car that was not set up does nothing");
}

void testSetupHidesTheParts() {
    TrafficCarLights cars;
    cars.load(1);
    cars.setup(0, taxiLights());
    const TrafficCarLights::Changes changes = cars.takeChanges();
    require(changes.parts.size() == 1 && changes.parts[0].part.part == 3 && !changes.parts[0].part.shown, "the create hides the brake light mesh (0x2363d4 with 0)");
}

void testBrakingLightsTheCar() {
    TrafficCarLights cars;
    cars.load(1);
    cars.setup(0, taxiLights());
    cars.takeChanges();
    cars.run(0, 1.0f, 0.033f);
    TrafficCarLights::Changes changes = cars.takeChanges();
    require(changes.parts.size() == 1 && changes.parts[0].part.shown && changes.parts[0].part.alpha == 255, "braking shows the mesh at alpha 255");
    require(changes.glows.size() == 2 && changes.glows[0].start && changes.glows[0].part == 1 && changes.glows[0].definition == 242 &&
                changes.glows[1].slot == 1 && changes.glows[1].part == 2,
            "braking starts the glow of both locators");
    cars.glowStarted(0, 0, 11);
    cars.glowStarted(0, 1, 12);
    cars.run(0, 1.0f, 0.033f);
    changes = cars.takeChanges();
    require(changes.parts.empty() && changes.glows.empty(), "a level that does not change and running glows give nothing");
    cars.run(0, 0.0f, 0.25f);
    changes = cars.takeChanges();
    require(changes.parts.size() == 1 && changes.parts[0].part.shown && changes.parts[0].part.alpha == 127, "releasing fades by 2 x dt: level 0.5, alpha 127");
    require(changes.glows.size() == 2 && !changes.glows[0].start && changes.glows[0].handle == 11 && changes.glows[1].handle == 12, "releasing stops the running glows");
    cars.glowEvent(0, 11, 1);
    require(cars.lights(0).glows[0] == 11, "an event other than 2 keeps the handle");
    cars.glowEvent(0, 11, 2);
    require(cars.lights(0).glows[0] == 0 && cars.lights(0).glows[1] == 12, "the end event clears the slots holding the handle");
    cars.run(0, 0.0f, 0.25f);
    changes = cars.takeChanges();
    require(changes.parts.size() == 1 && !changes.parts[0].part.shown && changes.parts[0].part.alpha == 0, "the level reaches 0: the mesh is hidden");
}

void testCulledNodeStartsNoGlow() {
    TrafficCarLights cars;
    cars.load(1);
    cars.setup(0, taxiLights());
    cars.takeChanges();
    cars.setNodeCulled(0, true);
    require(cars.nodeCulled(0), "the culling bit");
    cars.run(0, 1.0f, 0.033f);
    const TrafficCarLights::Changes changes = cars.takeChanges();
    require(changes.parts.size() == 1 && changes.parts[0].part.shown && changes.glows.empty(), "a culled node shows the mesh but starts no glow (node word bit 0x40000)");
}

void testDisableStopsTheGlows() {
    TrafficCarLights cars;
    cars.load(1);
    cars.setup(0, taxiLights());
    cars.run(0, 1.0f, 0.033f);
    cars.takeChanges();
    cars.glowStarted(0, 0, 21);
    cars.disable(0);
    const TrafficCarLights::Changes changes = cars.takeChanges();
    require(cars.lights(0).level == 0.0f, "disable: the level is 0");
    require(changes.parts.size() == 1 && changes.parts[0].part.part == 3 && !changes.parts[0].part.shown, "disable hides the mesh with a part index above 0");
    require(changes.glows.size() == 1 && !changes.glows[0].start && changes.glows[0].handle == 21, "disable stops the running glow");
    require(cars.lights(0).glows[0] == 0, "disable clears the handle of the stopped glow (0x179980 stores 0)");
    TrafficCarLights zero;
    zero.load(1);
    TrafficBrakeLights first = taxiLights();
    first.parts[0] = 0;
    zero.setup(0, first);
    zero.takeChanges();
    zero.disable(0);
    require(zero.takeChanges().parts.empty(), "disable skips a part with index 0 (the test is signed greater than 0 at 0x179948)");
}

int shipped(const char* fib, const char* csv) {
    if (!std::filesystem::exists(fib) || !std::filesystem::exists(csv)) {
        std::cout << "traffic-car-lights-test: data not available, skipped\n";
        return 77;
    }
    assets::FuseArchive archive(fib);
    archive.loadNames(csv);
    const assets::FuseEntry* levelEntry = archive.find("levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl");
    require(levelEntry != nullptr, "lvl_01");
    const assets::Level level = assets::parseLevel(archive.read(*levelEntry));
    const assets::ClassLookup classOf = assets::archiveClassLookup(archive);
    const auto stringValue = [](const assets::LevelParam* param) {
        if (!param || param->type != 1 || param->blocks.empty()) return std::string();
        const assets::Bytes& block = param->blocks.front();
        return std::string(block.begin(), std::find(block.begin(), block.end(), std::uint8_t(0)));
    };
    // Reference: the class values of the ten Traffic cars read from lvl_01 by 20261111-chase-G on 2026-11-11 (BrakeLightModel01 "BrakeLight" and
    // the two locators in all cars but Traffic07, which has none; Traffic05 and Traffic09 list the right locator first).
    struct Wanted {
        const char* instance;
        const char* model;
        const char* locator1;
        const char* locator2;
    };
    const Wanted wanted[] = {{"Traffic01", "BrakeLight", "LightsRL", "LightsRR"}, {"Traffic02", "BrakeLight", "LightsRL", "LightsRR"},
                             {"Traffic05", "BrakeLight", "LightsRR", "LightsRL"}, {"Traffic07", "", "", ""},
                             {"Traffic09", "BrakeLight", "LightsRR", "LightsRL"}};
    for (const Wanted& want : wanted) {
        const assets::LevelObject* object = nullptr;
        for (const assets::LevelObject& candidate : level.objects)
            if (candidate.instance == want.instance) object = &candidate;
        require(object != nullptr, std::string(want.instance) + ": in the level");
        const assets::Level* cls = classOf(object->className);
        require(cls != nullptr, std::string(want.instance) + ": class");
        const auto value = [&](const char* name) { return stringValue(effectiveParam(*object, *cls, assets::nameHash(name))); };
        require(value("BrakeLightModel01") == want.model, std::string(want.instance) + ": BrakeLightModel01");
        require(value("BrakeLightLocator01") == want.locator1 && value("BrakeLightLocator02") == want.locator2, std::string(want.instance) + ": locators");
    }
    const assets::FuseEntry* taxi = archive.find(assets::meshModelPath("Veh_Civic_Taxicab/Veh_Civic_Taxicab_atlas_lvl_01_cherrytreehills_main"));
    require(taxi != nullptr, "taxi model");
    const assets::Model model = assets::parseModel(archive.read(*taxi));
    require(model.nodeNames == kTaxiNodes, "the node names of the taxi model");
    const TrafficBrakeLights lights = taxiLights();
    require(lights.parts[0] == 3 && model.nodeMesh[3] == 1 && model.nodeMesh[1] == -1 && model.nodeMesh[2] == -1,
            "BrakeLight is the node with the mesh, the locators are nodes without one");
    require(model.nodeMatrices.size() == 4, "the model has the node matrix block (+0x20) of the glow positions");
    return 0;
}

}

int main(int argc, char** argv) {
    try {
        testPartLookup();
        testNoPartsNoLights();
        testSetupHidesTheParts();
        testBrakingLightsTheCar();
        testCulledNodeStartsNoGlow();
        testDisableStopsTheGlows();
        if (argc == 3) return shipped(argv[1], argv[2]);
    } catch (const std::exception& error) {
        std::cerr << "traffic-car-lights-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "traffic-car-lights-test: ok\n";
    return 0;
}
