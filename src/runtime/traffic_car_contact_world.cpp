#include "runtime/traffic_car_contact_world.hpp"

#include <cmath>

namespace chase::runtime {

std::array<float, 4> orientationFromAxes(const std::array<Vector3, 3>& axes) {
    const float m00 = axes[0][0], m01 = axes[1][0], m02 = axes[2][0];
    const float m10 = axes[0][1], m11 = axes[1][1], m12 = axes[2][1];
    const float m20 = axes[0][2], m21 = axes[1][2], m22 = axes[2][2];
    const float trace = m00 + m11 + m22;
    std::array<float, 4> q{};
    if (trace > 0.0f) {
        const float s = std::sqrt(trace + 1.0f) * 2.0f;
        q = {(m21 - m12) / s, (m02 - m20) / s, (m10 - m01) / s, 0.25f * s};
    } else if (m00 > m11 && m00 > m22) {
        const float s = std::sqrt(1.0f + m00 - m11 - m22) * 2.0f;
        q = {0.25f * s, (m01 + m10) / s, (m02 + m20) / s, (m21 - m12) / s};
    } else if (m11 > m22) {
        const float s = std::sqrt(1.0f + m11 - m00 - m22) * 2.0f;
        q = {(m01 + m10) / s, 0.25f * s, (m12 + m21) / s, (m02 - m20) / s};
    } else {
        const float s = std::sqrt(1.0f + m22 - m00 - m11) * 2.0f;
        q = {(m02 + m20) / s, (m12 + m21) / s, 0.25f * s, (m10 - m01) / s};
    }
    return q;
}

VehicleRigidBody trafficKinematicRigidBody(const TrafficBodyState& body) {
    VehicleRigidBody rigid;
    rigid.position = {body.rigid[3][0], body.rigid[3][1], body.rigid[3][2]};
    rigid.orientation = orientationFromAxes({Vector3{body.rigid[0][0], body.rigid[0][1], body.rigid[0][2]}, Vector3{body.rigid[1][0], body.rigid[1][1], body.rigid[1][2]},
                                             Vector3{body.rigid[2][0], body.rigid[2][1], body.rigid[2][2]}});
    rigid.velocity = {body.velocity[0], body.velocity[1], body.velocity[2]};
    rigid.inverseMass = 0.0f;
    rigid.inverseInertia = {0.0f, 0.0f, 0.0f};
    return rigid;
}

void trafficCarContact(AiDriverRecord& driver, bool rigidKinematic, const TrafficCarContactMessage& message, const TrafficCarContactHooks& hooks) {
    if (driver.state == kAiStateRoute || driver.state == kAiStateJunction) {
        driver.requestedState = 4;
        if (message.otherEntity && rigidKinematic) {
            if (message.otherBodyFlag20000) {
                if (hooks.dynamicNow) hooks.dynamicNow();
            } else if (hooks.dynamicQueued) {
                hooks.dynamicQueued();
            }
        }
        if ((driver.flags & kAiDriverFlagSignal) != 0 &&hooks.leaveSignalQueue) hooks.leaveSignalQueue(driver.signal);
    }
    if (message.otherEntity) driver.contactTime = message.now;
}

void driverFieldsFromRecord(AiDriverVehicle& driver, const VehicleRecord& record) {
    driver.health = record.healthPoints;
    driver.hearts = record.hearts;
    driver.maxHearts = record.health;
    driver.forcedStop = record.forcedStop;
}

void recordFieldsFromDriver(VehicleRecord& record, const AiDriverVehicle& driver) {
    record.healthPoints = driver.health;
    record.hearts = driver.hearts;
    record.forcedStop = driver.forcedStop;
}

void makeTrafficBodyDynamicNow(TrafficBodyState& body, float globalGravity) {
    std::vector<TrafficBodyCommand> waiting = std::move(body.queue);
    body.queue = {TrafficBodyCommand::Dynamic};
    runTrafficBodyCommands(body, globalGravity);
    body.queue = std::move(waiting);
}

}
