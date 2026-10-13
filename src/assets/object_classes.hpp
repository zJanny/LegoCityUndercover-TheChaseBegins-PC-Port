#pragma once
// The object classes the port has a recovered rule for. One table for the runtime and for the progress report
// (tools/progress_report.py). A class has behaviour when the runtime acts on its instances beyond drawing them.
// No SDL dependency.
#include "assets/assets.hpp"
#include <string>

namespace chase::assets {

bool isPropClass(const std::string& className);
bool isDoorClass(const std::string& className);
bool isPropFamilyClass(const std::string& className);
bool isDummyCollisionClass(const std::string& className);

// Instance with the stud object type (0x3d) and a pickup type of 0..3: collected by the stud system.
bool isStudObject(const LevelObject& object, const Level& classFile);

enum ObjectBehaviour : unsigned {
    BehaviourCollision = 1,  // objectCollisions registers a shape for it
    BehaviourDoor = 2,       // the door state machine drives it
    BehaviourStud = 4,       // the stud system collects it
};

// Bit set of ObjectBehaviour for one placed object; 0 when the runtime only draws it or ignores it.
unsigned objectBehaviours(const LevelObject& object, const Level& classFile);

// Comma-separated names of the bits ("collision,door"), empty for 0.
std::string behaviourNames(unsigned behaviours);

}
