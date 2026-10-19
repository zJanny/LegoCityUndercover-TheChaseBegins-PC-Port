// Waypoint class (src/runtime/waypoints.cpp, docs/re/scripts-missions.md SCRIPT-110): update 0x17acbc, init 0x17aa34.
// The synthetic part pins the state machine from the disassembly. With FIB NAMES the real part replays the oracle run
// d3 (state gameplay-start, teleport to 819.0 25.0 1072.0, stick up, dt 0.033; GDB ctrace 0x17b3a4 / 0x17b03c, recorded
// 2026-10-14): the waypoint tests the player in update 36 and 0x22ce9c starts the thread in update 50, 14 updates later;
// the positions of updates 33 to 36 are the rows of trace d2 (x, y, z of the player node). Exits with 77 without data.
#include "assets/assets.hpp"
#include "runtime/script_runner.hpp"
#include "runtime/waypoints.hpp"
#include <cmath>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase;
using runtime::ScriptRunner;
using runtime::WaypointSet;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

assets::LevelParam floatParam(std::uint32_t hash, float value, std::uint16_t flags = 5) {
    assets::LevelParam param;
    param.hash = hash;
    param.flags = flags;
    param.type = 0;
    std::memcpy(&param.value, &value, 4);
    return param;
}

assets::LevelParam intParam(std::uint32_t hash, std::uint32_t value, std::uint16_t flags = 6) {
    assets::LevelParam param;
    param.hash = hash;
    param.flags = flags;
    param.type = 5;
    param.value = value;
    return param;
}

assets::LevelParam stringParam(std::uint32_t hash, const std::string& text) {
    assets::LevelParam param;
    param.hash = hash;
    param.flags = 6;
    param.type = 1;
    param.blocks.push_back(assets::Bytes(text.begin(), text.end()));
    param.blocks[0].push_back(0);
    return param;
}

assets::LevelParam positionParam(float x, float y, float z) {
    assets::LevelParam param;
    param.hash = 0x86166f20;
    param.flags = 6;
    param.type = 2;
    float v[3] = {x, y, z};
    param.blocks.emplace_back(12);
    std::memcpy(param.blocks[0].data(), v, 12);
    return param;
}

assets::LevelParam vehicleStringClassParam() {
    assets::LevelParam param;
    param.hash = 0x7629ded9;
    param.flags = 2;
    param.type = 4;
    return param;
}

assets::LevelParam objectReference(std::uint32_t hash, const std::string& instance) {
    assets::LevelParam param;
    param.hash = hash;
    param.flags = 6;
    param.type = 4;
    assets::Bytes block(8, 0);
    const std::uint32_t named = assets::nameHash(instance);
    std::memcpy(block.data() + 4, &named, 4);
    param.blocks.push_back(block);
    return param;
}

assets::Level syntheticClass(bool vehicleClass = false, bool getIntoVehicle = false, float radius = 1.0f) {
    assets::Level cls;
    cls.classParams = {floatParam(0xd7954f4e, 0.8f), floatParam(0x190be976, 0.4f), intParam(0x59521dda, 1), intParam(0xc93e4625, 1),
                       stringParam(0xd3575d66, ""), floatParam(assets::nameHash("BadgeFadeInTime"), 0.8f)};
    assets::LevelBound bound;
    bound.name = "TriggerBound";
    bound.half[0] = radius;
    bound.half[1] = vehicleClass ? 3.4f : 0.902f;
    bound.centre[1] = vehicleClass ? 3.4f : 0.7603f;
    bound.shape = 4;
    bound.enabled = bound.active = 1;
    cls.bounds.push_back(bound);
    if (vehicleClass) {
        cls.defaults.push_back(vehicleStringClassParam());
        assets::LevelParam type = intParam(0x5be19715, 0, 2);
        type.type = 7;
        cls.defaults.push_back(type);
    }
    if (getIntoVehicle) cls.defaults.push_back(intParam(0xcae408a0, 1, 1));
    return cls;
}

assets::Level syntheticLevel(const std::string& className, const std::string& namedVehicle = "", int vehicleType = 0) {
    assets::Level level;
    assets::LevelObject object;
    object.className = className;
    object.instance = "Wp_A";
    object.room = "room8";
    object.params = {positionParam(10.0f, 5.0f, 20.0f), stringParam(0xd3575d66, "Target")};
    if (!namedVehicle.empty()) {
        object.params.push_back(objectReference(0x7629ded9, namedVehicle));
        assets::LevelObject vehicle;
        vehicle.className = "vehicles/vehicle_test";
        vehicle.instance = namedVehicle;
        level.objects.push_back(vehicle);
    }
    if (vehicleType) {
        assets::LevelParam type = intParam(0x5be19715, std::uint32_t(vehicleType), 6);
        type.type = 7;
        object.params.push_back(type);
    }
    level.objects.insert(level.objects.begin(), object);
    assets::LevelScript script;
    script.nameHash = assets::nameHash("Target");
    script.calls = {{"Wait", {"100"}}};
    level.scripts.push_back(script);
    return level;
}

void fades() {
    const assets::Level cls = syntheticClass();
    const assets::ClassLookup classOf = [&](const std::string&) { return &cls; };
    const assets::Level level = syntheticLevel("props/map_waypoint");
    const float dt = 0.4f, inside[3] = {10.5f, 5.5f, 20.0f}, outside[3] = {12.0f, 5.5f, 20.0f};
    WaypointSet set;
    set.load(level, classOf);
    set.setActive("wp_a", true);
    const auto near = [](float a, float b) { return std::fabs(a - b) < 1e-5f; };
    const runtime::Waypoint* wp = set.find("wp_a");
    require(wp->columnFade() == 0.0f && wp->badgeFade() == 0.0f, "enabled: column and badge are transparent until the state 0 handler runs");
    set.update(dt, outside, nullptr, nullptr);
    require(wp->columnFade() == 0.0f && wp->badgeFade() == 0.0f, "state 0 starts the column tween at 0 and sets the badge to 0 (0x17ae0c, 0x17ae28)");
    set.update(dt, outside, nullptr, nullptr);
    require(near(wp->columnFade(), 0.5f) && wp->badgeFade() == 0.0f, "FadeScaleInTime 0.8: half after 0.4 s (linear, 0x159420)");
    set.update(dt, outside, nullptr, nullptr);
    require(near(wp->columnFade(), 1.0f) && wp->badgeFade() == 0.0f, "the column is opaque after FadeScaleInTime; the badge tween starts when state 1 ends (0x17ae64)");
    set.update(dt, outside, nullptr, nullptr);
    require(near(wp->badgeFade(), 0.5f), "BadgeFadeInTime 0.8: half after 0.4 s");
    set.update(dt, outside, nullptr, nullptr);
    require(near(wp->badgeFade(), 1.0f) && near(wp->columnFade(), 1.0f), "both opaque");
    set.update(dt, inside, nullptr, nullptr);
    set.update(dt, inside, nullptr, nullptr);
    require(wp->state == 3 && near(wp->columnFade(), 1.0f) && near(wp->badgeFade(), 1.0f), "state 3: the column tween 1 to 0 starts at 1; the badge is not faded (0x17aebc)");
    set.update(dt, inside, nullptr, nullptr);
    require(!wp->active && wp->columnFade() == 0.0f, "FadeScaleOutTime 0.4: transparent when the waypoint switches off");
    require(!wp->columnShown(), "no disable fade after state 3");

    assets::Level manual = syntheticClass();
    for (assets::LevelParam& param : manual.classParams)
        if (param.hash == 0xc93e4625) param.value = 0;
    const assets::ClassLookup manualOf = [&](const std::string&) { return &manual; };
    WaypointSet scripted;
    scripted.load(level, manualOf);
    scripted.setActive("wp_a", true);
    for (int i = 0; i < 3; ++i) scripted.update(dt, outside, nullptr, nullptr);
    const runtime::Waypoint* off = scripted.find("wp_a");
    require(!scripted.setActive("wp_a", false), "AutoTrigger 0: SetActiveWayPoint 0 switches the waypoint off (bit 4 of +0x8e set)");
    require(off->columnShown() && near(off->columnFade(), 1.0f) && off->badgeFade() == 1.0f,
            "0x229c98 disable: column tween from 1 (0x229e50) to 0 (0x229e4c), applied at once");
    scripted.update(dt, outside, nullptr, nullptr);
    require(near(off->columnFade(), 0.5f) && off->badgeFade() == 0.0f,
            "the column fades over FadeScaleInTime +0x6c (0.8 s, half after 0.4 s); the badge tween lasts 2^-23 s (0x229e48)");
    scripted.update(dt, outside, nullptr, nullptr);
    require(off->columnFade() == 0.0f && !off->columnShown(), "hidden when the tween reaches 0 (0x23f734)");
}

void disableAndSlots() {
    const assets::Level cls = syntheticClass();
    const assets::ClassLookup classOf = [&](const std::string&) { return &cls; };
    assets::Level manual = syntheticClass();
    for (assets::LevelParam& param : manual.classParams)
        if (param.hash == 0xc93e4625) param.value = 0;
    const assets::ClassLookup manualOf = [&](const std::string&) { return &manual; };
    const assets::Level level = syntheticLevel("props/map_waypoint");
    const float dt = 0.4f, outside[3] = {12.0f, 5.5f, 20.0f};
    const auto near = [](float a, float b) { return std::fabs(a - b) < 1e-5f; };
    {
        WaypointSet set;
        set.load(level, manualOf);
        const runtime::Waypoint* wp = set.find("wp_a");
        require(!wp->entityEnabled && !wp->columnShown() && wp->mapSlot == -1, "a waypoint starts hidden (0x170fd8 disables it) without a map slot");
        require(!set.setActive("wp_a", false), "AutoTrigger 0: the disable of a never enabled waypoint is accepted");
        require(wp->disableFade && near(wp->columnFade(), 1.0f), "0x229dc8: any disable outside state 3 starts the tween, whether or not the waypoint was active");
        require(!wp->columnShown(), "the entity is still disabled, so nothing is drawn (the original relies on the entity flag)");
        set.update(dt, outside, nullptr, nullptr);
        set.update(dt, outside, nullptr, nullptr);
        require(wp->columnFade() == 0.0f && !wp->disableFade, "the tween runs out");
        set.setActive("wp_a", true);
        require(wp->entityEnabled && wp->mapSlot == 0 && wp->columnShown() && wp->columnFade() == 0.0f, "enable: 0x242cf0 clears the disabled flag, 0x1a2114 gives slot 0; column alpha is 0 until state 0 ends");
        set.update(dt, outside, nullptr, nullptr);
        set.update(dt, outside, nullptr, nullptr);
        require(wp->columnShown() && near(wp->columnFade(), 0.5f), "shown while fading in");
        set.setActive("wp_a", false);
        require(wp->mapSlot == -1, "0x229b9c frees the slot at once");
        require(wp->entityEnabled && wp->columnShown(), "a disable does not disable the entity (0x230ef4 is not the waypoint): the column fades out instead");
    }
    {
        WaypointSet set;
        set.load(level, manualOf);
        set.setActive("wp_a", true);
        for (int i = 0; i < 8; ++i) set.update(dt, outside, nullptr, nullptr);
        const_cast<runtime::Waypoint*>(set.find("wp_a"))->state = 3;
        set.setActive("wp_a", false);
        require(!set.find("wp_a")->disableFade, "state 3: no tween (0x229dc8 cmp #3)");
    }
    {
        assets::Level many;
        for (int i = 0; i < 17; ++i) {
            assets::LevelObject object = level.objects[0];
            object.instance = "Wp_" + std::to_string(i);
            many.objects.push_back(object);
        }
        WaypointSet set;
        set.load(many, manualOf);
        for (int i = 0; i < 17; ++i) set.setActive("wp_" + std::to_string(i), true);
        for (int i = 0; i < 16; ++i) require(set.find("wp_" + std::to_string(i))->mapSlot == i, "the first free of 16 slots in order (0x1a2114 loop to 16)");
        require(set.find("wp_16")->mapSlot == -1 && set.find("wp_16")->active, "all 16 used: slot -1 (0x1a2148), the waypoint is still on");
        set.setActive("wp_3", false);
        set.setActive("wp_16", true);
        require(set.find("wp_16")->mapSlot == 3, "a freed slot is reused");
        set.setActive("wp_16", true);
        require(set.find("wp_16")->mapSlot == 3, "an enabled waypoint with a slot keeps it (0x229cc8 tests +0x84 < 0)");
    }
    {
        WaypointSet set;
        set.load(level, classOf);
        set.setActive("wp_a", true);
        set.update(dt, outside, nullptr, nullptr);
        const float inside[3] = {10.5f, 5.5f, 20.0f};
        for (int i = 0; i < 8 && set.find("wp_a")->active; ++i) set.update(dt, inside, nullptr, nullptr);
        require(!set.find("wp_a")->active && set.find("wp_a")->mapSlot == -1, "the switch-off of state 3 frees the slot (0x17b0a8)");
    }
    {
        assets::Level vehicleClass = syntheticClass(true, false, 4.0f);
        assets::Level typeEleven = vehicleClass;
        typeEleven.defaults.push_back(intParam(assets::nameHash("WaypointType"), 11, 2));
        assets::Level getInto = syntheticClass(true, true, 4.0f);
        const auto build = [&](const assets::Level& cls) {
            const assets::ClassLookup lookup = [&](const std::string&) { return &cls; };
            WaypointSet set;
            set.load(syntheticLevel("props/map_vehiclewaypoint", "Some_Car"), lookup);
            set.setActive("wp_a", true);
            return set.vehicleWaypoint();
        };
        require(build(vehicleClass) == "wp_a", "0x2b4dac: a kind 1 waypoint with a named vehicle becomes *(0x529c18 + 0x28)");
        require(build(typeEleven).empty(), "WaypointType 11 (0x229d3c) does not");
        require(build(getInto).empty(), "GetIntoVehicle (bit 0 of +0x8f) does not");
        const assets::ClassLookup lookup = [&](const std::string&) { return &vehicleClass; };
        WaypointSet set;
        set.load(syntheticLevel("props/map_vehiclewaypoint", "Some_Car"), lookup);
        set.setActive("wp_a", true);
        set.setActive("wp_a", false);
        require(set.vehicleWaypoint().empty() == false, "AutoTrigger set: the disable is ignored and the global stays");
        const assets::ClassLookup manualVehicle = [&](const std::string&) {
            static assets::Level cls = [] {
                assets::Level c = syntheticClass(true, false, 4.0f);
                for (assets::LevelParam& param : c.classParams)
                    if (param.hash == 0xc93e4625) param.value = 0;
                return c;
            }();
            return &cls;
        };
        WaypointSet manualSet;
        manualSet.load(syntheticLevel("props/map_vehiclewaypoint", "Some_Car"), manualVehicle);
        manualSet.setActive("wp_a", true);
        manualSet.setActive("wp_a", false);
        require(manualSet.vehicleWaypoint().empty(), "0x229d7c: the disable clears the global");
    }
}

void synthetic() {
    const assets::Level cls = syntheticClass();
    const assets::ClassLookup classOf = [&](const std::string&) { return &cls; };
    const float dt = 0.033f;
    const float inside[3] = {10.5f, 5.5f, 20.0f}, outside[3] = {12.0f, 5.5f, 20.0f}, high[3] = {10.5f, 7.0f, 20.0f};
    runtime::WaypointVehicle anyVehicle;
    anyVehicle.instance = "some_car";
    anyVehicle.matrix = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 10.5f, 5.0f, 20.0f, 1};
    anyVehicle.boxHalf[0] = anyVehicle.boxHalf[1] = anyVehicle.boxHalf[2] = 1.0f;

    {
        const assets::Level level = syntheticLevel("props/map_waypoint");
        WaypointSet set;
        set.load(level, classOf);
        require(set.known("wp_a") && !set.active("wp_a"), "an inactive waypoint is loaded");
        require(set.update(dt, inside, nullptr, nullptr).startScripts.empty() && !set.find("wp_a")->fired, "inactive waypoints are not updated");
        require(set.setActive("wp_a", true), "enable");
        set.update(dt, outside, nullptr, nullptr);
        require(set.find("wp_a")->state == 1 && set.find("wp_a")->timer == 0.8f, "state 0 ends in the first update: state 1, timer FadeScaleInTime");
        set.update(dt, high, nullptr, nullptr);
        require(!set.find("wp_a")->fired, "the bound is a cylinder of half height 0.902 around centre y 0.7603");
        set.update(dt, inside, nullptr, nullptr);
        require(set.find("wp_a")->fired && set.find("wp_a")->state == 2, "a hit sets state 2");
        require(!set.playerControlTaken(), "control is taken by the state 2 handler, not by the hit");
        set.update(dt, inside, nullptr, nullptr);
        require(set.find("wp_a")->state == 3 && set.playerControlTaken(), "update after the hit: state 3, TakePlayerControl");
        unsigned updates = 0;
        runtime::WaypointUpdate result;
        while (result.startScripts.empty() && updates < 100) {
            result = set.update(dt, inside, nullptr, nullptr);
            ++updates;
        }
        require(updates == 13, "the script starts 13 updates after state 3: FadeScaleOutTime 0.4 at dt 0.033");
        require(result.startScripts.size() == 1 && result.startScripts[0] == "Target", "TriggerScript");
        require(!set.playerControlTaken() && !set.active("wp_a") && result.deactivated.size() == 1, "released and switched off");
        require(!set.setActive("wp_a", false), "a disable of an inactive waypoint stays off");
        set.setActive("wp_a", true);
        require(set.find("wp_a")->state == 0 && !set.find("wp_a")->fired, "enabling again re-arms it");
    }
    {
        const assets::Level level = syntheticLevel("props/map_waypoint");
        WaypointSet set;
        set.load(level, classOf);
        set.setActive("wp_a", true);
        require(set.setActive("wp_a", false), "SetActiveWayPoint 0x26e168 ignores a disable while AutoTrigger is set");
    }
    {
        const assets::Level level = syntheticLevel("props/map_waypoint");
        WaypointSet set;
        set.load(level, classOf);
        set.setActive("wp_a", true);
        set.update(dt, inside, &anyVehicle, nullptr);
        require(!set.find("wp_a")->fired, "a foot waypoint does not fire for a player in a vehicle (0x17b31c)");
    }
    const assets::Level vehicleClass = syntheticClass(true, false, 4.0f);
    const assets::Level getIntoClass = syntheticClass(true, true, 4.0f);
    const assets::ClassLookup vehicleClassOf = [&](const std::string&) { return &vehicleClass; };
    const assets::ClassLookup getIntoClassOf = [&](const std::string&) { return &getIntoClass; };
    {
        const assets::Level level = syntheticLevel("props/map_vehiclewaypoint");
        WaypointSet set;
        set.load(level, vehicleClassOf);
        set.setActive("wp_a", true);
        set.update(dt, inside, nullptr, nullptr);
        require(!set.find("wp_a")->fired, "a vehicle waypoint without a specific vehicle needs a vehicle (0x17b104)");
        const runtime::WaypointUpdate hit = set.update(dt, inside, &anyVehicle, nullptr);
        require(set.find("wp_a")->fired, "...and fires with one");
        require(hit.vehicleActions.size() == 1 && hit.vehicleActions[0].vehicle == "some_car" && hit.vehicleActions[0].stop,
                "TakePlayerControl without GetIntoVehicle: forced stop on the hit vehicle (0x17b3cc)");
        runtime::WaypointUpdate result;
        for (unsigned i = 0; i < 100 && result.startScripts.empty(); ++i) result = set.update(dt, inside, &anyVehicle, nullptr);
        require(result.vehicleActions.size() == 1 && !result.vehicleActions[0].stop && result.vehicleActions[0].vehicle == "some_car",
                "the script start releases the forced stop (0x17b06c)");
    }
    {
        const assets::Level level = syntheticLevel("props/map_getintovehiclewaypoint");
        WaypointSet set;
        set.load(level, getIntoClassOf);
        set.setActive("wp_a", true);
        set.update(dt, inside, nullptr, nullptr);
        require(set.find("wp_a")->getIntoVehicle && !set.update(dt, inside, &anyVehicle, nullptr).vehicleActions.size(),
                "GetIntoVehicle 1 (class default): the hit does not stop the vehicle");
    }
    {
        const assets::Level level = syntheticLevel("props/map_vehiclewaypoint", "Some_Van");
        WaypointSet set;
        set.load(level, vehicleClassOf);
        set.setActive("wp_a", true);
        require(set.find("wp_a")->namedVehicle == "some_van", "VehicleString names an object of the level");
        set.update(dt, inside, &anyVehicle, nullptr);
        require(!set.find("wp_a")->fired, "a named vehicle: another vehicle does not count (0x17b140)");
        runtime::WaypointVehicle van = anyVehicle;
        van.instance = "some_van";
        set.update(dt, inside, &van, nullptr);
        require(set.find("wp_a")->fired, "...the named one does");
    }
    {
        const assets::Level level = syntheticLevel("props/map_vehiclewaypoint", "", 12);
        WaypointSet set;
        set.load(level, vehicleClassOf);
        set.setActive("wp_a", true);
        runtime::WaypointVehicle other = anyVehicle;
        other.type = 15;
        set.update(dt, inside, &other, nullptr);
        require(!set.find("wp_a")->fired, "VehicleType 12 does not take a type 15 vehicle (0x17b168)");
        other.type = 12;
        set.update(dt, inside, &other, nullptr);
        require(set.find("wp_a")->fired, "...and takes a type 12 vehicle");
    }
    {
        const assets::Level level = syntheticLevel("props/map_vehiclewaypoint");
        WaypointSet set;
        set.load(level, vehicleClassOf);
        set.setActive("wp_a", true);
        const float away[3] = {100.0f, 5.0f, 100.0f};
        runtime::WaypointVehicle car = anyVehicle;
        car.matrix[12] = 19.0f;
        require(!set.update(dt, away, &car, nullptr).vehicleActions.size() && !set.find("wp_a")->fired,
                "second test 0x17b21c: a vehicle 9 m from the waypoint is outside the radius 4 bound");
        car.matrix[12] = 12.5f;
        car.boxHalf[2] = 2.0f;
        set.update(dt, away, &car, nullptr);
        require(set.find("wp_a")->fired, "second test 0x17b21c: the box of a vehicle 2.5 m away reaches the bound while the player node does not");
    }
    {
        const assets::Level level = syntheticLevel("props/map_waypoint");
        WaypointSet set;
        set.load(level, classOf);
        set.setActive("wp_a", true);
        const runtime::WaypointUpdate foot = set.update(dt, inside, &anyVehicle, nullptr);
        require(!set.find("wp_a")->fired && foot.vehicleHint && foot.vehicleActions.empty(),
                "a foot waypoint hit from a vehicle does not fire and shows the hint 0xa0fc8748 (0x17b31c)");
        runtime::WaypointVehicle remote = anyVehicle;
        remote.instance = "5-8_vehicle_rc";
        require(!set.update(dt, inside, &remote, nullptr).vehicleHint && !set.find("wp_a")->fired, "the remote control car 5-8_Vehicle_RC gets neither");
    }
    {
        const assets::Level level = syntheticLevel("props/map_waypoint");
        assets::PlacedObjectState placed;
        ScriptRunner runner(level, placed);
        runner.loadWaypoints(classOf);
        const float far[3] = {0, 0, 0};
        runner.updateWaypoints(dt, far, nullptr);
        require(!runner.running("Target", false), "nothing started");
    }
}

void real(const std::string& fib, const std::string& names) {
    assets::FuseArchive archive(fib);
    archive.loadNames(names);
    const char* file = "levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl";
    assets::Level level = assets::parseLevel(archive.read(*archive.find(file)));
    assets::LevelScript activate;
    activate.nameHash = assets::nameHash("ActivateWaypoint");
    activate.calls = {{"SetActiveWayPoint", {"1-1_Waypoint_01", "1"}}};
    level.scripts.push_back(activate);
    const assets::ClassLookup classOf = assets::archiveClassLookup(archive);
    assets::PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.loadWaypoints(classOf);
    runner.setAllRoomsLoaded();
    require(runner.waypoints().waypoints().size() == 23, "lvl_01 has 23 waypoint objects of the three classes");
    const runtime::Waypoint* donut = runner.waypoints().find("1-1_waypoint_01");
    require(donut && donut->triggerScript == "1-1_VanBreakdown" && donut->hasBound, "1-1_Waypoint_01 names 1-1_VanBreakdown");
    require(donut->bound.half[0] == 1.0f && std::fabs(donut->bound.half[1] - 0.902f) < 1e-6f,
            "class bound of props/map_waypoint: radius 1, half height 0.902");
    require(donut->fadeScaleOut == 0.4f && donut->fadeScaleIn == 0.8f && donut->takePlayerControl && donut->autoTrigger,
            "FadeScaleInTime 0x3f4ccccd, FadeScaleOutTime 0x3ecccccd, TakePlayerControl 1, AutoTrigger 1");
    {
        // Provenance: convert-style dump of lvl_01 (props/map_getintovehiclewaypoint class default GetIntoVehicle 1, map_vehiclewaypoint none;
        // VehicleString of both mission 1-1 vehicle waypoints names 1-1_DonutVan, hash 0xdbd5bf3d), setup 0x170fd8 and bit 5 of +0x8e at 0x1710a0.
        const runtime::Waypoint* enter = runner.waypoints().find("1-1_van_waypoint_enter");
        const runtime::Waypoint* escort = runner.waypoints().find("1-1_waypoint_05");
        require(enter && escort, "the two vehicle waypoints of mission 1-1");
        require(enter->kind == runtime::WaypointKind::Vehicle && escort->kind == runtime::WaypointKind::Vehicle && donut->kind == runtime::WaypointKind::OnFoot,
                "only the vehicle classes are kind 1");
        require(enter->namedVehicle == "1-1_donutvan" && escort->namedVehicle == "1-1_donutvan", "both name 1-1_DonutVan");
        require(enter->getIntoVehicle && !escort->getIntoVehicle, "GetIntoVehicle: the enter waypoint only");
        require(enter->bound.half[0] == 4.0f && std::fabs(enter->bound.half[1] - 3.4f) < 1e-6f && std::fabs(enter->bound.centre[1] - 3.4f) < 1e-6f,
                "class bound of the large waypoints: radius 4, half height 3.4, centre y 3.4");
    }
    require(donut->waypointType == 0 && runner.waypoints().find("1-1_van_waypoint_enter")->waypointType == 0 && runner.waypoints().find("1-1_waypoint_05")->waypointType == 0,
            "WaypointType (hash 0xc471915a, a float 0 in the three class files, flags 5: no instance override) is 0 for every waypoint of lvl_01");
    require(runner.start("ActivateWaypoint") && runner.objectives().waypointActive("1-1_waypoint_01"), "SetActiveWayPoint");

    const float dt = 0.033f;
    const float rows[4][3] = {{818.83801f, 24.95050f, 1071.83997f},
                              {818.66364f, 24.85150f, 1071.69348f},
                              {818.47601f, 24.70300f, 1071.56445f},
                              {818.27850f, 24.50500f, 1071.45117f}};
    for (unsigned i = 0; i < 32; ++i) runner.updateWaypoints(dt, rows[0], nullptr);
    for (unsigned row = 0; row < 4; ++row) {
        runner.updateWaypoints(dt, rows[row], nullptr);
        require(runner.waypoints().find("1-1_waypoint_01")->fired == (row == 3), "oracle: the hit is in update 36 (row 4), not before");
    }
    require(!runner.playerControlTaken(), "oracle d2: the player still moved in update 37 before the control flag changed");
    require(!runner.running("1-1_VanBreakdown", false), "not started yet");
    for (unsigned update = 1; update <= 14; ++update) {
        runner.updateWaypoints(dt, rows[3], nullptr);
        require(runner.playerControlTaken() == (update >= 1 && update < 14), "oracle d2: control taken from update 37, released in update 50");
        require(runner.running("1-1_VanBreakdown", false) == (update == 14), "oracle d3: 0x17b03c starts the thread 14 updates after 0x17b3a4");
    }
    require(!runner.objectives().waypointActive("1-1_waypoint_01"), "the waypoint switches itself off");
}
}

int main(int argc, char** argv) {
    try {
        synthetic();
        fades();
        disableAndSlots();
        if (argc == 1) {
            std::cout << "waypoints-test: synthetic part passed\n";
            return 0;
        }
        if (argc != 3) {
            std::cerr << "usage: waypoints-test [FIB NAMES]\n";
            return 2;
        }
        if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
            std::cout << "waypoints-test: data not available, skipped\n";
            return 77;
        }
        real(argv[1], argv[2]);
        std::cout << "waypoints-test: passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "waypoints-test: " << error.what() << '\n';
        return 1;
    }
}
