// The contact of the player's vehicle on the chased car (src/runtime/chase_end_flow.hpp). Values transcribed from the vehicle handler 0x19293c
// (llvm-objdump of game.arm.elf, 20261112-trafficfeed-A, 2026-11-12): the damage message 0x1929c0..0x192a60, 0x244fc8 (a direct call of the handler of
// the entity kind, table 0x583124, entry +0x14), the factor 0x192a64..0x192a98 (the literals 12.0 at 0x192cfc, 0.9 at 0x192d00, 0.8 at 0x192d04,
// 0.4 at 0x192d08, 0.7 at 0x192d0c) and the velocity change 0x24092c / 0x248850 / 0x229920 (body +0x7c, velocity +0x13c..+0x144 times the factor).
#include "runtime/chase_end_flow.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase::runtime;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

bool near(float a, float b) { return std::fabs(a - b) < 1e-6f; }

AiDriverRecord chasedDriver() {
    AiDriverRecord driver;
    driver.state = kAiStateSteer;
    driver.requestedState = kAiStateSteer;
    return driver;
}

ChasedCarContact playerHit(float speed) { return {true, false, speed}; }

void scaleBands() {
    require(near(chasedCarVelocityScale(3.5f), 0.9f), "below 12 m/s the factor is 0.9");
    require(near(chasedCarVelocityScale(11.99f), 0.9f), "the band of 0 ends below 12");
    require(near(chasedCarVelocityScale(12.0f), 0.8f), "12 m/s starts the band 1: 0.8");
    require(near(chasedCarVelocityScale(23.9f), 0.8f), "the band 1 ends below 24");
    require(near(chasedCarVelocityScale(24.0f), 0.7f), "24 m/s starts the band 2: 0.7");
    require(near(chasedCarVelocityScale(35.9f), 0.7f), "the band 2 ends below 36");
    require(near(chasedCarVelocityScale(36.0f), 0.4f), "36 m/s and more give 0.4");
    require(near(chasedCarVelocityScale(1.0e30f), 0.4f), "a huge speed gives 0.4");
}

void hitsAndHold() {
    AiDriverRecord driver = chasedDriver();
    VehicleRecord record;
    record.hearts = 5;
    for (std::uint32_t expected = 4; expected >= 1; --expected) {
        const std::optional<ChasedCarContactResult> result = chasedCarContact(driver, record, playerHit(20.0f));
        require(result.has_value(), "a hard contact in state 5 sends the damage message");
        require(!result->reply.consumed && !result->reply.chaseEnded, "above the last heart the base handler takes the damage");
        require(near(result->velocityScale, 0.8f), "20 m/s scales the velocity by 0.8");
        require(driver.hold == kChasedCarHitHold, "the contact holds the driver for 128 updates");
        require(!chasedCarContact(driver, record, playerHit(20.0f)), "the hold refuses the next contact");
        record.hearts = expected;
        driver.hold = 0;
    }
    const std::optional<ChasedCarContactResult> last = chasedCarContact(driver, record, playerHit(30.0f));
    require(last && last->reply.chaseEnded && last->reply.consumed, "the hit on the last heart ends the chase");
    require(driver.requestedState == kAiStateStopping && record.hearts == 0, "state 6 is requested and the hearts are 0");
    require(near(last->velocityScale, 0.7f), "the velocity change follows the chase end too");
    const std::optional<ChasedCarContactResult> again = chasedCarContact(driver, record, playerHit(30.0f));
    require(again && again->reply.consumed && !again->reply.chaseEnded, "until the driver step enters state 6 (+0x78 is still 5) a contact is consumed by the pending request");
    driver.state = kAiStateStopping;
    require(!chasedCarContact(driver, record, playerHit(30.0f)), "the car is no longer in state 5");
}

void rejections() {
    AiDriverRecord driver = chasedDriver();
    VehicleRecord record;
    record.hearts = 5;
    require(!chasedCarContact(driver, record, {false, false, 20.0f}), "another entity sends nothing and changes no velocity");
    require(!chasedCarContact(driver, record, {true, true, 20.0f}), "a destroyed player vehicle sends nothing");
    require(!chasedCarContact(driver, record, playerHit(3.0f)), "3 m/s is not above the limit");
    require(record.hearts == 5 && driver.hold == 0, "a refused contact changes nothing");
}

}

int main() {
    try {
        scaleBands();
        hitsAndHold();
        rejections();
    } catch (const std::exception& error) {
        std::cerr << "chased-car-contact: " << error.what() << '\n';
        return 1;
    }
    std::cout << "chased-car-contact: ok\n";
    return 0;
}
