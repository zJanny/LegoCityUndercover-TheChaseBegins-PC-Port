#pragma once
// The pedestrian's vehicle test (docs/re/pedestrians-inputs.md): 0x1ffcd4(pedestrian, position, vector, vehicle, dt) against one vehicle,
// and the loop over the vehicle list 0x5863b4 + 0x305c (count +0x3058) that both the spawn (0x26cd38, zero vector and dt 0.0)
// and the walk (0x2b9340 .. 0x2b93a4, the step velocity and dt) run: listed entities whose body is kinematic are skipped, the
// first non-zero result is returned, else 0. The test can write the pedestrian's requested state, its dodge side and its walk
// byte, and slides the vector along the vehicle's box. No SDL dependency.
#include "runtime/game_random.hpp"
#include "runtime/pedestrian_manager.hpp"
#include "runtime/pedestrian_walk.hpp"

#include <array>
#include <cstdint>
#include <functional>
#include <vector>

namespace chase::runtime {

constexpr float kPedestrianBlockMarginX = 0.9f;        // 0x200048
constexpr float kPedestrianBlockMarginZ = 1.8f;        // 0x20004c
constexpr float kPedestrianBlockReachPerStep = 8.0f;   // 0x200050
constexpr float kPedestrianBlockSlowStep = 0.1f;       // 0x200054, compared as bits
constexpr float kPedestrianDodgeRayHeight = 0.25f;     // 0x200064
constexpr float kPedestrianDodgeOffset = 5.0f;         // *0x37449c, stored by the static initialiser 0x347904
constexpr std::uint16_t kPedestrianDodgeRightState = 3;  // table 0x357e1c
constexpr std::uint16_t kPedestrianDodgeLeftState = 4;
constexpr std::uint16_t kPedestrianWalkState = 2;
constexpr std::uint16_t kPedestrianSplatState = 6;
constexpr int kPedestrianBlockDodged = 3;
constexpr int kPedestrianBlockTurnBack = 4;

// One entity of the list 0x5863b4 + 0x305c, which the physics step 0x2e2bc8 rebuilds from the vehicle bodies of the world.
struct PedestrianBlockVehicle {
    std::array<PedestrianVec3, 4> matrix{};  // 0x250810 of entity +0x3c: rows 0 .. 2 and the translation
    PedestrianVec3 halfExtents{};            // entity +0x6c .. +0x74
    float radius = 0;                        // entity +0x5c
    float speed = 0;                         // 0x229948 of the body +0x84: body +0x24 . rigid body +0x13c
    bool kinematic = false;                  // the body's rigid body (+0x7c) has bit 1 of its flags +0xcc
};

// 0x22b108 from `from` to `to` with the descriptor {0x362298, 1, 16, -1, 0, 0, 0, 512, 3, 0, byte 1}: true when anything is hit.
using PedestrianLineTest = std::function<bool(const PedestrianVec3& from, const PedestrianVec3& to)>;

// 0x1ffcd4. `radius` is the pedestrian's entity +0x5c. Returns 0, 3 (a dodge was requested) or 4 (the target lies past the
// vehicle). `vector` loses its part into a face of the box when it would cross that face.
int pedestrianVehicleBlock(PedestrianBody& body, float radius, const PedestrianVec3& position, PedestrianVec3& vector, float dt,
                           const PedestrianBlockVehicle& vehicle, const PedestrianLineTest& lineBlocked, GameRandom& random);

// 0x26cd38 and the walk's own loop: 0x1ffcd4 for each listed vehicle in list order, kinematic ones skipped.
int pedestrianBlocked(PedestrianBody& body, float radius, const PedestrianVec3& position, PedestrianVec3& vector, float dt,
                      const std::vector<PedestrianBlockVehicle>& vehicles, const PedestrianLineTest& lineBlocked, GameRandom& random);

}
