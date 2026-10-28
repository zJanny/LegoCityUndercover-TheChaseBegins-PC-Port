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

// The prop kind 67 (props/prop_prop, leprops/leprop_prop and 55 other classes; create 0x15a4b4, post-link 0x15a640 -> 0x16aeb4): its post-link
// registers collision with 0x23aff8 (automatic box, no margin) and `HasCollisionMesh` (0x2337e0), whatever the class folder (docs/re/vehicles-prop-smash.md).
constexpr std::uint8_t kPropObjectKind = 67;

// The super build-it plate (props/prop_superbuildit, create 0x1af05c, link 0x1af224): its link registers collision with 0x23aff8 (automatic box, no
// margin) and then takes the model's first collision item or box record unconditionally (0x2337e0), docs/re/collision.md.
constexpr std::uint8_t kSuperBuilditObjectKind = 98;
// The platform class (props/prop_platform, leprops/leprop_platform): its link 0x19a3e4 registers collision like kind 67 without the `Destructable` test.
constexpr std::uint8_t kPlatformObjectKind = 64;

// Class files of the vehicle object type (ObjectType 183, class descriptor 0x155b5c; section +0x14 of levels/vehicles/vehicle_*).
constexpr std::uint32_t kVehicleObjectType = 183;
bool isVehicleClassFile(const Level& classFile);

// Instance with the stud object type (0x3d) and a pickup type of 0..3: collected by the stud system.
bool isStudObject(const LevelObject& object, const Level& classFile);

// Instance with the pickup object type 0x84 and the pickup type 103 (the super build bricks, runtime/super_brick.hpp).
bool isSuperBuildBrickObject(const LevelObject& object, const Level& classFile);

// Kind byte of the placed entity (entity +0x12): the low byte of the effective ObjectType, 0 when the class has none (docs/re/script-natives-flags.md).
std::uint8_t objectKind(const LevelObject& object, const Level& classFile);

// Kinds whose create builds the model with the loader 0x2442b4 and no character setup: 0 (0x1d1450 through 0x234004),
// 64 (0x199adc) and 67 (0x15a4b4 through 0x16a268 and 0x234004). Their objects with an animal rig ("char_dog", "char_cat",
// "char_Crow") are placed props (docs/re/objects.md). STAND-IN LEVOBJ-S84: until a clip plays, such a skinned model is drawn with its
// stored vertices; the original's pose of a skeleton without a clip was not compared.
bool createsModelAsProp(std::uint8_t kind);

// CSCharacter of the object (the cutscene actor folder it stands in for, builder 0x2d1ddc), empty when unset (docs/re/cutscene-actors.md).
std::string objectCutsceneCharacter(const LevelObject& object, const Level& classFile);

enum ObjectBehaviour : unsigned {
    BehaviourCollision = 1,  // objectCollisions registers a shape for it
    BehaviourDoor = 2,       // the door state machine drives it
    BehaviourStud = 4,       // the stud system collects it
    BehaviourVehicle = 8,    // the vehicle records and flag natives act on it (runtime/vehicle.hpp)
    BehaviourSuperBrick = 16,  // the super brick field picks it up (runtime/super_brick.hpp)
};

// Bit set of ObjectBehaviour for one placed object; 0 when the runtime only draws it or ignores it.
unsigned objectBehaviours(const LevelObject& object, const Level& classFile);

// Comma-separated names of the bits ("collision,door"), empty for 0.
std::string behaviourNames(unsigned behaviours);

}
