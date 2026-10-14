#include "assets/object_classes.hpp"
#include <algorithm>
#include <cctype>

namespace chase::assets {
namespace {
std::string lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return text;
}

bool startsWith(const std::string& text, const char* prefix) { return text.rfind(prefix, 0) == 0; }

bool hasCollisionBound(const Level& classFile) {
    return std::any_of(classFile.bounds.begin(), classFile.bounds.end(), [](const LevelBound& bound) { return lower(bound.name) == "collisionbound"; });
}
}

bool isPropClass(const std::string& className) {
    const std::string name = lower(className);
    return startsWith(name, "props/") || startsWith(name, "leprops/");
}

bool isDoorClass(const std::string& className) {
    const std::string name = lower(className);
    return name == "props/prop_door" || name == "leprops/leprop_door";
}

bool isPropFamilyClass(const std::string& className) {
    const std::string name = lower(className);
    return startsWith(name, "leprops/leprop_") || name == "props/prop_respawnabledestructible" || isDoorClass(name);
}

bool isDummyCollisionClass(const std::string& className) { return lower(className) == "dummy/dummycollision"; }

bool isVehicleClassFile(const Level& classFile) {
    const std::uint32_t hash = nameHash("ObjectType");
    for (const auto* params : {&classFile.defaults, &classFile.classParams})
        for (const LevelParam& param : *params)
            if (param.hash == hash && param.type == 7) return param.value == kVehicleObjectType;
    return false;
}

unsigned objectBehaviours(const LevelObject& object, const Level& classFile) {
    unsigned behaviours = 0;
    const bool vehicle = isVehicleClassFile(classFile);
    const bool collision = isDummyCollisionClass(object.className) || vehicle ||
                           (isPropClass(object.className) && (isPropFamilyClass(object.className) || hasCollisionBound(classFile)));
    if (collision) behaviours |= BehaviourCollision;
    if (vehicle) behaviours |= BehaviourVehicle;
    if (isDoorClass(object.className)) behaviours |= BehaviourDoor;
    if (isStudObject(object, classFile)) behaviours |= BehaviourStud;
    return behaviours;
}

std::string behaviourNames(unsigned behaviours) {
    std::string names;
    const auto add = [&](unsigned bit, const char* name) {
        if (!(behaviours & bit)) return;
        if (!names.empty()) names += ',';
        names += name;
    };
    add(BehaviourCollision, "collision");
    add(BehaviourDoor, "door");
    add(BehaviourStud, "stud");
    add(BehaviourVehicle, "vehicle");
    return names;
}

}
