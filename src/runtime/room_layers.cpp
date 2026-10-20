#include "runtime/room_layers.hpp"

namespace chase::runtime {

void setRoomLayers(RoomLayerTable& table, std::uint8_t layerClass, std::uint8_t firstType, bool bit0, bool bit1) {
    for (std::size_t type = firstType; type < kRoomLayerTypes; ++type) {
        std::uint16_t& word = table[layerClass * kRoomLayerTypes + type];
        word = static_cast<std::uint16_t>((word & ~3u) | (bit0 ? 1u : 0u) | (bit1 ? 2u : 0u));
    }
}

RoomLayerTable startupRoomLayers() {
    RoomLayerTable table{};
    constexpr std::uint8_t last = kRoomLayerTypes - 1;
    for (std::uint8_t layerClass = 0; layerClass < kRoomLayerClasses; ++layerClass) {
        setRoomLayers(table, layerClass, 0, true, true);
        setRoomLayers(table, layerClass, last, true, false);
    }
    setRoomLayers(table, 0, 0, true, true);
    setRoomLayers(table, 0, last, false, false);
    setRoomLayers(table, 1, 0, true, true);
    setRoomLayers(table, 1, last, false, true);
    setRoomLayers(table, 2, 0, false, true);
    setRoomLayers(table, 2, last, false, false);
    setRoomLayers(table, 3, 0, false, false);
    setRoomLayers(table, 4, 0, true, false);
    setRoomLayers(table, 4, last, false, false);
    setRoomLayers(table, 5, 0, true, true);
    setRoomLayers(table, kRoomLayerCutsceneClass, 0, true, true);
    setRoomLayers(table, kRoomLayerCutsceneClass, last, false, false);
    return table;
}

RoomLayerTable cutsceneRoomLayers(const RoomLayerTable& startup) {
    RoomLayerTable table = startup;
    setRoomLayers(table, kRoomLayerCutsceneClass, 0, false, false);
    return table;
}

}
