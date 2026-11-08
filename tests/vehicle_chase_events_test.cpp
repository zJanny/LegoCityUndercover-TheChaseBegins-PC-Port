// The end of a car chase (src/runtime/vehicle_chase_events.hpp). The values are transcribed from the instructions of the vehicle handler 0x19293c
// and the driver state change 0x1cb148 (llvm-objdump of game.arm.elf, 20261108-traffic-C, 2026-11-08, docs/re/traffic-ai-driver.md). With the
// game data the VehicleChaseEnded event of 1-4_Stolen_Vehicle starts 1-4_Car_Crash.
#include "assets/assets.hpp"
#include "runtime/level_start.hpp"
#include "runtime/script_runner.hpp"
#include "runtime/vehicle_chase_events.hpp"

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

runtime::AiDriverRecord chasedDriver() {
    runtime::AiDriverRecord driver;
    driver.state = runtime::kAiStateSteer;
    driver.requestedState = runtime::kAiStateSteer;
    return driver;
}

void synthetic() {
    runtime::ChasedCarContact hit;
    hit.otherIsPlayerVehicle = true;
    hit.speed = 3.5f;
    require(runtime::chasedCarContactDamage(chasedDriver(), hit).has_value(), "a contact of the player's vehicle above 3 m/s in state 5 sends a damage message");
    const runtime::DamageMessage message = *runtime::chasedCarContactDamage(chasedDriver(), hit);
    require(message.hearts == 1 && message.kind == 11 && message.type == 0 && message.blocked == 0, "the message is one heart of kind 11, type 0 (0x1929c0..0x192a60)");

    runtime::ChasedCarContact slow = hit;
    slow.speed = 3.0f;
    require(!runtime::chasedCarContactDamage(chasedDriver(), slow), "3 m/s is not above the limit (vcmpe 3.0 against the speed, bhs)");
    runtime::ChasedCarContact other = hit;
    other.otherIsPlayerVehicle = false;
    require(!runtime::chasedCarContactDamage(chasedDriver(), other), "another entity does not count");
    other = hit;
    other.otherDestroyed = true;
    require(!runtime::chasedCarContactDamage(chasedDriver(), other), "a destroyed vehicle (entity word +0xc bit 0x10) does not count");
    runtime::AiDriverRecord driver = chasedDriver();
    driver.hold = 1;
    require(!runtime::chasedCarContactDamage(driver, hit), "the hold +0xac refuses the contact");
    driver = chasedDriver();
    driver.state = runtime::kAiStateRoute;
    require(!runtime::chasedCarContactDamage(driver, hit), "only the state 5 counts");

    runtime::VehicleRecord record;
    record.hearts = 2;
    driver = chasedDriver();
    runtime::ChasedCarDamageReply reply = runtime::chasedCarDamage(driver, record, message);
    require(!reply.consumed && !reply.chaseEnded && record.hearts == 2 && driver.requestedState == runtime::kAiStateSteer,
            "a hit with hearts left is passed on to the base handler (0x192b20)");
    require(driver.hold == runtime::kChasedCarHitHold, "kind 11 sets the hold +0xac to 128");

    driver = chasedDriver();
    record.hearts = 1;
    const std::uint8_t flagsBefore = record.flags311;
    reply = runtime::chasedCarDamage(driver, record, message);
    require(reply.consumed && reply.chaseEnded, "the hit on the last heart ends the chase");
    require(driver.requestedState == runtime::kAiStateStopping && record.hearts == 0 && record.flags311 == flagsBefore,
            "the driver requests state 6 and the hearts are 0 (0x192b34); the Invulnerable bit is not set on the chased car (it is in the 7-9_RexCar branch, on the player's vehicle)");
    reply = runtime::chasedCarDamage(driver, record, message);
    require(reply.consumed && !reply.chaseEnded, "once state 6 is requested the handler returns 1 for any message 0");

    driver = chasedDriver();
    record = runtime::VehicleRecord{};
    record.hearts = 1;
    runtime::DamageMessage scripted = message;
    scripted.type = runtime::kVehicleDamageTypeScripted;
    require(!runtime::chasedCarDamage(driver, record, scripted).chaseEnded && driver.requestedState == runtime::kAiStateSteer, "type 6 is not turned into the chase end");
    driver = chasedDriver();
    driver.state = runtime::kAiStateRoute;
    require(!runtime::chasedCarDamage(driver, record, message).chaseEnded, "the chase ends only from state 5");
    record.hearts = 0;
    driver = chasedDriver();
    require(!runtime::chasedCarDamage(driver, record, message).consumed, "a car without hearts is not handled");

    record = runtime::VehicleRecord{};
    const runtime::ChaseEndEntry entry = runtime::chaseEndEntry(record);
    require(entry.event == "vehiclechaseended" && entry.smokeEffect == 0x11f && !entry.kinematic && record.forcedStop == 1,
            "entering state 6: kinematic off, forced stop 1, smoke 0x11f, VehicleChaseEnded");
}

void real(const char* fib, const char* names) {
    assets::FuseArchive archive(fib);
    archive.loadNames(names);
    const assets::FuseEntry* entry = archive.find("levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl");
    require(entry != nullptr, "level not in archive");
    const assets::Level level = assets::parseLevel(archive.read(*entry));
    const assets::ClassLookup classOf = assets::archiveClassLookup(archive);
    assets::PlacedObjectState state = runtime::levelInitialState(level, classOf);
    runtime::ScriptRunner runner(level, state);
    runner.update(0.033f);
    require(!runner.running("1-4_Car_Crash", false), "1-4_Car_Crash does not run on its own");
    runner.fireObjectEvent(runtime::kVehicleChaseEndedEvent, "1-4_stolen_vehicle");
    const float away[3] = {0.0f, 0.0f, 0.0f};
    runner.checkTriggers(away);
    runner.update(0.033f);
    require(runner.running("1-4_Car_Crash", false), "VehicleChaseEnded of 1-4_Stolen_Vehicle starts 1-4_Car_Crash (docs/re/scripts-missions-1-4.md)");
}
}

int main(int argc, char** argv) {
    try {
        synthetic();
        if (argc >= 3) {
            if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) return 77;
            real(argv[1], argv[2]);
        }
    } catch (const std::exception& error) {
        std::cerr << "vehicle-chase-events-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "vehicle-chase-events-test: ok\n";
    return 0;
}
