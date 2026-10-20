#pragma once
// The room layer table *0x376ad4 (docs/re/pedestrians-inputs.md): 7 entity classes (entity byte +0x18) x 17 room types (room byte +0x3a,
// 16 and up share the last), one hword each at +4. Bit 0 is read by 0x1dd590, bit 1 by 0x1ffbd8 (the pedestrian and traffic
// room test). No SDL dependency.
#include "runtime/pedestrian_manager.hpp"

#include <cstdint>

namespace chase::runtime {

constexpr std::size_t kRoomLayerClasses = kPedestrianLayerClasses;
constexpr std::size_t kRoomLayerTypes = kPedestrianRoomTypes;
constexpr std::uint8_t kRoomLayerCutsceneClass = 6;

using RoomLayerTable = PedestrianLayerTable;

// 0x2405a0(class, first type, bit 0, bit 1): both bits of the types from `firstType` to 16 of a class; nothing for a first type
// of 17 and up.
void setRoomLayers(RoomLayerTable& table, std::uint8_t layerClass, std::uint8_t firstType, bool bit0, bool bit1);

// The table after the game set-up: 0x1590b0 (from 0x168de0 at 0x2f5cf4) fills the table 0x5859b0, then the registration of the
// pickup system (slot +8 0x2ed0b4, from 0x157a00 through 0x163c34) sets class 6 again.
RoomLayerTable startupRoomLayers();

// 0x16c060 at the start of a cutscene (record 0x376d18 +0x14, called by 0x196744 after 0x1b3ebc pushed the table 0x5898e0):
// a copy of the set-up table 0x5859b0 (not of the current one) with class 6 cleared for every type. The end of the cutscene
// (0x2fae68 -> 0x1a4b9c) goes back to the table below it.
RoomLayerTable cutsceneRoomLayers(const RoomLayerTable& startup);

}
