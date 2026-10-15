// Tests of the wheels and suspension of the vehicle physics body (src/runtime/vehicle_wheels.*, docs/re/vehicles-physics.md). Pinned values:
// Azahar GDB memory traces of 2026-10-15 at fixed dt 0.033 (agentVPHYS pdrive.mem: state drv-police-car-seated, A held; wheel records
// body+0x2b8 + n * 0x154 and the rigid body). Each force row is w+0xb4 of the trace with the distance w+0xe0 and the hit point's velocity along
// the normal w+0xc8 (rigid velocity and angular velocity of the update before). With FIB NAMES the real part puts 1-1_Police_Car on the level
// collision where the oracle trooper came to rest and compares the rest height and wheel distances. Exits with 77 without data.
#include "assets/assets.hpp"
#include "assets/object_classes.hpp"
#include "runtime/vehicle_wheels.hpp"

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

void near(double got, double want, double tolerance, const char* what) { expect(std::fabs(got - want) <= tolerance, what, got); }

WheelSuspension trooperWheel(bool rear) {
    WheelSuspension wheel;
    wheel.mount = {rear ? 0.85f : -0.85f, rear ? 0.067f : 0.0685f, rear ? -1.52f : 1.52f};
    wheel.sideOffset = rear ? 0.1055f : -0.07825f;
    wheel.radius = 0.48f;
    wheel.travel = 1.0f;
    wheel.stiffness = 18.2f;
    wheel.dampingExtend = rear ? 2.8f : 5.2f;
    wheel.dampingCompress = rear ? 2.1f : 3.9f;
    return wheel;
}

std::vector<VehicleWheel> trooperWheels() {
    std::vector<VehicleWheel> wheels(4);
    for (std::size_t i = 0; i < 4; ++i) {
        wheels[i].suspension = trooperWheel(i >= 2);
        if (i % 2 == 1) {
            wheels[i].suspension.mount[0] = -wheels[i].suspension.mount[0];
            wheels[i].suspension.sideOffset = -wheels[i].suspension.sideOffset;
        }
    }
    return wheels;
}

VehicleRigidBody trooperBody(const Vector3& position, float heading) {
    VehicleRigidBody body;
    body.position = position;
    body.orientation = headingOrientation(heading);
    body.inverseMass = 1.0f / 2600.0f;
    body.inverseInertia = vehicleInverseInertiaStandIn("vehicles/vehicle_police_trooper");
    return body;
}

void settle(VehicleRigidBody& body, const std::vector<VehicleWheel>& wheels, const WheelRay& ray, int updates, std::vector<WheelState>& states) {
    for (int i = 0; i < updates; ++i) {
        states = applyWheelForces(body, wheels, 1.0f, ray);
        const float speed = std::sqrt(body.velocity[0] * body.velocity[0] + body.velocity[1] * body.velocity[1] + body.velocity[2] * body.velocity[2]);
        stepRigidBody(body, vehicleLinearDamping(0.02f, speed), 0.1f, 0.033f);
        body.velocity[0] = body.velocity[2] = 0.0f;
    }
}

void synthetic() {
    near(suspensionStiffnessCurve(0.0f), 0.0, 0.0, "table 0x376eac starts at 0");
    near(suspensionStiffnessCurve(0.25f), (0x1.2515fep-6 + 0x1.93d32ep-5) / 2, 1e-7, "table 0x376eac halfway between entries 2 and 3");
    near(suspensionStiffnessCurve(1.0f), 1.0, 0.0, "table 0x376eac ends at 1");
    near(suspensionDampingCurve(1.0f), 0.25, 0.0, "table 0x376ed8 ends at 0.25");
    near(wheelRayLength(trooperWheel(false)), 1.0, 1e-6, "trooper ray reach w+0x118 = 1.0 (oracle)");
    WheelSuspension short_ = trooperWheel(false);
    short_.travel = 0.3f;
    near(wheelRayLength(short_), 0.481, 1e-6, "0x3396cc: a travel shorter than the radius still reaches 0.001 past the wheel");

    near(suspensionForce(trooperWheel(false), 0.5170349f, 0.0f, 1.0f), 26846.994, 0.05, "pdrive frame 2 wheel 0 at rest");
    near(suspensionForce(trooperWheel(true), 0.5091404f, 0.0f, 1.0f), 27824.387, 0.05, "pdrive frame 2 wheel 2 at rest");
    near(suspensionForce(trooperWheel(false), 0.5316598f, 0.4380529f, 1.0f), 20173.477, 0.1, "pdrive frame 3 wheel 0 extending (ShockDampingPos)");
    near(suspensionForce(trooperWheel(true), 0.5010136f, -0.2439603f, 1.0f), 29894.326, 0.1, "pdrive frame 3 wheel 2 compressing (ShockDampingNeg)");
    near(suspensionForce(trooperWheel(false), 1.0f, 0.0f, 1.0f), 0.0, 0.0, "no force at full extension");
    near(suspensionForce(trooperWheel(false), 0.9f, 5.0f, 1.0f), 0.0, 0.0, "the force is never negative");

    const std::vector<VehicleWheel> wheels = trooperWheels();
    const WheelRay flat = [](const Vector3& from, const Vector3& to) -> std::optional<WheelHit> {
        if (from[1] < 0.0f || to[1] > 0.0f) return std::nullopt;
        const float t = from[1] / (from[1] - to[1]);
        return WheelHit{{from[0] + (to[0] - from[0]) * t, 0.0f, from[2] + (to[2] - from[2]) * t}, {0.0f, 1.0f, 0.0f}};
    };
    VehicleRigidBody body = trooperBody({0.0f, 0.7f, 0.0f}, 0.3f);
    std::vector<WheelState> states;
    settle(body, wheels, flat, 150, states);
    float total = 0;
    for (const WheelState& state : states) {
        expect(state.contact, "every wheel touches the flat ground");
        total += state.force;
    }
    near(total, 2600.0 * 39.0, 50.0, "at rest the four springs carry the weight (mass 2600, gravity 39)");
    // The equilibrium distance of the formula is 0.5291; the oracle body asleep after the coast (pcoast.mem frames 111..191, a street that
    // tilts by about 0.2 degrees) rests at 0.5288 front and 0.5263 rear.
    near(states[0].distance, 0.5288, 0.003, "front wheel distance at rest (oracle 0.5288)");
    near(states[2].distance, 0.5263, 0.004, "rear wheel distance at rest (oracle 0.5263)");
    near(bodyAxes(body)[1][1], 1.0, 1e-4, "on flat ground the body rests level");

    const float slope = 0.1f;
    const WheelRay ramp = [slope](const Vector3& from, const Vector3& to) -> std::optional<WheelHit> {
        const auto height = [slope](const Vector3& p) { return p[1] - p[2] * slope; };
        if (height(from) < 0.0f || height(to) > 0.0f) return std::nullopt;
        const float t = height(from) / (height(from) - height(to));
        const float length = std::sqrt(1.0f + slope * slope);
        return WheelHit{{from[0] + (to[0] - from[0]) * t, from[1] + (to[1] - from[1]) * t, from[2] + (to[2] - from[2]) * t},
                        {0.0f, 1.0f / length, -slope / length}};
    };
    body = trooperBody({0.0f, 0.7f, 0.0f}, 0.0f);
    settle(body, wheels, ramp, 200, states);
    const Vector3 forward = bodyAxes(body)[2];
    near(std::atan2(forward[1], forward[2]), std::atan(slope), 0.01, "on a 0.1 ramp the body pitches with the ground");
}

int real(const char* fib, const char* names) {
    assets::FuseArchive archive(fib);
    archive.loadNames(names);
    const assets::FuseEntry* entry = archive.find("models/lvl_01_cherrytreehills/lvl_01_cherrytreehills.bfnmdl");
    if (!entry) {
        std::printf("FAIL: level model not in archive\n");
        return 1;
    }
    const assets::Model model = assets::parseModel(archive.read(*entry));
    CollisionWorld world;
    for (const auto& item : model.collision) world.add(item);
    const WheelRay ray = [&world](const Vector3& from, const Vector3& to) { return castWheelRay(world, from, to); };
    // Oracle pcoast.mem frame 120 (drv-police-car-seated, A held 45 updates, then released): the trooper has come to rest and sleeps (mode 2)
    // with the rigid body at (867.893, 17.0445, 1097.718), forward (-0.70526, 0.00166, -0.70895), wheel distances 0.5288 front and 0.5263 rear.
    const float heading = std::atan2(-0.705261f, -0.708946f);
    VehicleRigidBody body = trooperBody({867.893f, 17.3f, 1097.718f}, heading);
    std::vector<WheelState> states;
    settle(body, trooperWheels(), ray, 150, states);
    for (const WheelState& state : states) expect(state.contact, "every wheel finds the street");
    near(body.position[1], 17.0445, 0.005, "rest height of the centre of mass (oracle 17.0445)");
    const double oracle[4] = {0.5288, 0.5288, 0.5263, 0.5263};
    for (std::size_t i = 0; i < 4; ++i) near(states[i].distance, oracle[i], 0.004, "rest wheel distance on the street");
    std::printf("trooper at rest on the street: centre y %.4f, wheels %.4f %.4f %.4f %.4f\n", body.position[1], states[0].distance, states[1].distance,
                states[2].distance, states[3].distance);
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    synthetic();
    if (argc == 3) {
        if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
            std::printf("vehicle-wheels-test: data not available, skipped\n");
            return 77;
        }
        if (real(argv[1], argv[2]) != 0) return 1;
    }
    if (failures != 0) return 1;
    std::printf("vehicle wheels tests passed\n");
    return 0;
}
