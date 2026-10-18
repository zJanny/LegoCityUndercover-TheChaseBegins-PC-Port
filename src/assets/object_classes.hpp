#pragma once
// The object classes the port has a recovered rule for. One table for the runtime and for the progress report
// (tools/progress_report.py). A class has behaviour when the runtime acts on its instances beyond drawing them.
// No SDL dependency.
#include "assets/assets.hpp"
#include <cstdint>
#include <string>

namespace chase::assets {

bool isPropClass(const std::string& className);
bool isDoorClass(const std::string& className);
bool isPropFamilyClass(const std::string& className);
bool isDummyCollisionClass(const std::string& className);

// Class files of the vehicle object type (ObjectType 183, class descriptor 0x155b5c; section +0x14 of levels/vehicles/vehicle_*).
constexpr std::uint32_t kVehicleObjectType = 183;
bool isVehicleClassFile(const Level& classFile);

// Instance with the stud object type (0x3d) and a pickup type of 0..3: collected by the stud system.
bool isStudObject(const LevelObject& object, const Level& classFile);

// Kind byte of the placed entity (entity +0x12): the low byte of the effective ObjectType, 0 when the class has none (docs/re/script-natives-flags.md).
std::uint8_t objectKind(const LevelObject& object, const Level& classFile);

enum ObjectBehaviour : unsigned {
    BehaviourCollision = 1,  // objectCollisions registers a shape for it
    BehaviourDoor = 2,       // the door state machine drives it
    BehaviourStud = 4,       // the stud system collects it
    BehaviourVehicle = 8,    // the vehicle records and flag natives act on it (runtime/vehicle.hpp)
};

// Bit set of ObjectBehaviour for one placed object; 0 when the runtime only draws it or ignores it.
unsigned objectBehaviours(const LevelObject& object, const Level& classFile);

// Comma-separated names of the bits ("collision,door"), empty for 0.
std::string behaviourNames(unsigned behaviours);

}
