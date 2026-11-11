#pragma once
// The brake lights of the vehicle record (docs/re/traffic-car-lights.md): the vehicle create 0x1658a8 reads the class values
// BrakeLightModel%02u (2), BrakeLightLocator%02u (4) and BrakeLightParticle (default 242), finds the nodes of the model by name (0x23645c) and
// hides the two brake light meshes (0x2363d4); the post step of the AI driver calls 0x278b78 (runTrafficBrakeLights) every update. This file
// sets the parts up from the class values and keeps the lights of every traffic car with the culling bit of the car's node that 0x278b78
// and the keep counter of the post step read (0x23de70). No SDL, no GL.
#include "runtime/traffic_driver_calls.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace chase::runtime {

constexpr std::int32_t kTrafficBrakeLightDefaultParticle = 242;  // the default of the 0x2451d0 read at 0x165a24

// The class values the create reads; an empty name is a value that is not set (the create skips it).
struct TrafficBrakeLightClass {
    std::array<std::string, kTrafficBrakeLightParts> models;
    std::array<std::string, kTrafficBrakeGlows> locators;
    std::int32_t particle = kTrafficBrakeLightDefaultParticle;
};

// 0x23645c: the index of the first node of the model whose name equals `name` (0x25ad90, without case), -1 for none or an empty name.
std::int32_t trafficLightNodeIndex(const std::vector<std::string>& nodeNames, const std::string& name);

// The create's part lookup: +0x2ac / +0x2b0 from the models, +0x2b4..+0x2c0 from the locators, +0x2d8 = particle.
TrafficBrakeLights makeTrafficBrakeLights(const TrafficBrakeLightClass& values, const std::vector<std::string>& nodeNames);

// One part change the car's node takes: 0x2363d4(node, part, shown) and, when shown, 0x23f124(node, alpha, part, 0).
struct TrafficCarPartChange {
    std::size_t car = 0;
    TrafficBrakeLightPart part;
};

// 0x23de24 / 0x23ddb4 of one glow slot of a car: the glow part node and the effect definition.
struct TrafficCarGlowRequest {
    std::size_t car = 0;
    std::size_t slot = 0;
    std::int32_t part = -1;
    std::uint32_t definition = 0;
    bool start = false;  // false: stop
    std::uint32_t handle = 0;
};

class TrafficCarLights {
public:
    void load(std::size_t carCount);
    // The lights of a car; its mesh parts start hidden.
    void setup(std::size_t car, const TrafficBrakeLights& lights);
    bool configured(std::size_t car) const;
    const TrafficBrakeLights& lights(std::size_t car) const { return cars_.at(car).lights; }

    // The culling bit 18 of the car's node (set by the render pass, TRAFFIC-37). 0x23de70 reads it for the keep counter; 0x278b78 for the glow.
    void setNodeCulled(std::size_t car, bool culled);
    bool nodeCulled(std::size_t car) const { return cars_.at(car).culled; }

    // 0x278b78 of a car's post step; the part changes and glow requests wait for takeChanges().
    void run(std::size_t car, float brake, float dt);
    // The lights of a car whose entity is disabled (0x170e68, and message 251 of the vehicle handler 0x1797c4 at 0x179910): the level is 0, the meshes
    // with a part index above 0 are hidden (0x2363d4) and the running glows are stopped (0x23ddb4) and their handles cleared. STAND-IN TRAFFIC-S36: which of the two runs at a
    // traffic car's removal was not read; the port runs it when the car's entity is disabled.
    void disable(std::size_t car);
    // The handle of the effect started for a glow slot, 0 when the start was refused.
    void glowStarted(std::size_t car, std::size_t slot, std::uint32_t handle);
    // 0x2cb42c: the effect callback; at event 2 the slots holding the handle are cleared.
    void glowEvent(std::size_t car, std::uint32_t handle, int event);

    struct Changes {
        std::vector<TrafficCarPartChange> parts;
        std::vector<TrafficCarGlowRequest> glows;
    };
    Changes takeChanges();

private:
    struct Car {
        TrafficBrakeLights lights;
        bool culled = false;
        bool ready = false;
    };
    std::vector<Car> cars_;
    Changes pending_;
};

}
