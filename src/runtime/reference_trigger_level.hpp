#pragma once
// The reference records of a level (docs/re/script-triggers.md): the objects whose create function calls the TriggerObject loader 0x233de8 or
// 0x23e84c register a record in the list at 0x591d88 when the object loader creates them. No SDL dependency.

#include "assets/assets.hpp"
#include "runtime/reference_trigger.hpp"

#include <cstdint>
#include <vector>

namespace chase::runtime {

// A kind (entity +0x12) whose create calls a loader, with the address of the `bl`. Both loaders read TriggerObject, NextSwitch, AiAssist and NavNode
// into the record and register it through 0x269d2c; 0x23e84c also reads the switch flags of the record before it.
struct ReferenceLoaderKind {
    std::uint8_t kind = 0;
    std::uint32_t loader = 0;
    std::uint32_t call = 0;
};
const std::vector<ReferenceLoaderKind>& referenceLoaderKinds();
bool kindRegistersReference(std::uint8_t kind);

// Clears `references` and adds one entry per created object of a loader kind, in object order. The target is the object TriggerObject names
// (empty when the parameter is unset or names no object of the level). STAND-IN SCRIPT-S290: the objects nested in class files are not registered, and an object created in the default scene only (create flag bit 1) is registered in every scene.
void registerLevelReferences(ReferenceTriggers& references, const assets::Level& level, const assets::ClassLookup& classOf);

}
