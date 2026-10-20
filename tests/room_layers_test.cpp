// The room layer table *0x376ad4 (docs/re/pedestrians-inputs.md). Provenance: llvm-objdump of artifacts/game/game.arm.elf. The
// setter 0x2405a0 writes bits 0 and 1 of the hwords from the first type to 16. The set-up 0x1590b0 (from 0x168de0 at 0x2f5cf4)
// calls it as: every class (0, 1, 1) and (16, 1, 0); then class 0 (0, 1, 1) (16, 0, 0); class 1 (0, 1, 1) (16, 0, 1); class 2
// (0, 0, 1) (16, 0, 0); class 3 (0, 0, 0); class 4 (0, 1, 0) (16, 0, 0); class 5 (0, 1, 1). The pickup system's slot +8 0x2ed0b4
// (registered later by 0x157a00 at 0x2f5d08) sets class 6 (0, 1, 1) (16, 0, 0). The cutscene start 0x16c060 copies the table
// and sets class 6 (0, 0, 0).
#include "runtime/pedestrian_manager.hpp"
#include "runtime/room_layers.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase::runtime;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

std::uint16_t word(const RoomLayerTable& table, std::size_t layerClass, std::size_t type) { return table[layerClass * kRoomLayerTypes + type]; }

void requireClass(const RoomLayerTable& table, std::size_t layerClass, std::uint16_t types0To15, std::uint16_t type16) {
    for (std::size_t type = 0; type < 16; ++type)
        require(word(table, layerClass, type) == types0To15, "class " + std::to_string(layerClass) + " type " + std::to_string(type));
    require(word(table, layerClass, 16) == type16, "class " + std::to_string(layerClass) + " type 16");
}
}

int main() {
    try {
        const RoomLayerTable startup = startupRoomLayers();
        requireClass(startup, 0, 3, 0);
        requireClass(startup, 1, 3, 2);
        requireClass(startup, 2, 2, 0);
        requireClass(startup, 3, 0, 0);
        requireClass(startup, 4, 1, 0);
        requireClass(startup, 5, 3, 3);
        requireClass(startup, 6, 3, 0);

        const RoomLayerTable cutscene = cutsceneRoomLayers(startup);
        requireClass(cutscene, 6, 0, 0);
        for (std::size_t layerClass = 0; layerClass < 6; ++layerClass)
            for (std::size_t type = 0; type < kRoomLayerTypes; ++type)
                require(word(cutscene, layerClass, type) == word(startup, layerClass, type), "the cutscene changes class 6 only");

        RoomLayerTable table = startup;
        setRoomLayers(table, 2, 17, true, true);
        require(table == startup, "a first type of 17 writes nothing");
        table[4] = 0xfff0;
        setRoomLayers(table, 0, 4, false, true);
        require(table[4] == 0xfff2, "only bits 0 and 1 change");

        PedestrianLayerTable layers = startup;
        const PedestrianRoom loaded{kPedestrianRoomLoadedState, true, 3, 0};
        PedestrianRoom last = loaded;
        last.type = 255;
        require(pedestrianRoomActive(loaded, 0, layers), "class 0 in a room of type 3 is active");
        require(!pedestrianRoomActive(last, 0, layers), "class 0 in a room of type 16 and up is not");
        require(!pedestrianRoomActive(loaded, 4, layers), "class 4 has bit 1 clear");
    } catch (const std::exception& error) {
        std::cerr << "room-layers-test: " << error.what() << "\n";
        return 1;
    }
    std::cout << "room-layers-test: ok\n";
    return 0;
}
