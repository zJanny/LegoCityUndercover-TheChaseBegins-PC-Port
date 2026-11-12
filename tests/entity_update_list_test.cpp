// Tests of src/runtime/entity_update_list.*: the room test 0x23c1a8, the add 0x173534 and the removal 0x22d480 of the entity update list
// 0x580564. The rules follow from the disassembly (llvm-objdump --triple=armv6k-none-eabi --mattr=+vfp2 of artifacts/game/game.arm.elf,
// read by 20261111-chase-A on 2026-11-11, docs/re/traffic-car-body.md). The junction case is the one the emulator servant recorded from
// a save state (20261111-chase-servant, 2026-11-11, RPC reads, three loads that agree; mission 1-4, the player on the showroom roof): of
// the 16 junction entities of lvl_01 (class byte +0x18 = 2, +0x13 = 0, priority +0x19 = 128) the 15 of the default room (type byte 0)
// are on the update list and the one of a loaded room with type byte 255 is not.
#include "runtime/entity_update_list.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

namespace {

using namespace chase::runtime;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

void roomTest() {
    const RoomLayerTable layers = startupRoomLayers();
    UpdateListEntity junction;
    junction.layerClass = 2;
    junction.priority = 128;
    require(entityBelongsOnUpdateList(junction, std::uint8_t(0), layers), "class 2 in a room of type 0 (servant: 15 junctions listed)");
    require(entityBelongsOnUpdateList(junction, std::uint8_t(15), layers), "class 2 has bit 1 for the types 0 to 15");
    require(!entityBelongsOnUpdateList(junction, std::uint8_t(16), layers), "not for type 16");
    require(!entityBelongsOnUpdateList(junction, std::uint8_t(255), layers), "type 255 reads column 16 (servant: the junction of the far room is not listed)");
    require(!entityBelongsOnUpdateList(junction, std::nullopt, layers), "no room: 0x23c238 goes to the removal");

    UpdateListEntity car;
    require(entityBelongsOnUpdateList(car, std::uint8_t(0), layers), "class 0 in the default room");
    car.disabled = true;
    require(!entityBelongsOnUpdateList(car, std::uint8_t(0), layers), "entity +0x10 bit 0");
    car.disabled = false;
    car.roomList = 1;
    require(!entityBelongsOnUpdateList(car, std::uint8_t(0), layers), "entity byte +0x13 must be 0");
}

void addAndRemove() {
    EntityUpdateList list;
    UpdateListEntity fields;
    fields.priority = 128;
    const std::optional<UpdateListMessage> joined = list.add(7, fields);
    require(joined && joined->entity == 7 && joined->message == 0x80000004u, "a new entry gets message 0x80000004");
    fields.priority = 64;
    require(!list.add(7, fields) && list.entries().size() == 1 && list.entries()[0].priority == 64, "a listed entity only gets its priority refreshed");
    UpdateListEntity still;
    still.typeUpdates = false;
    require(!list.add(8, still) && !list.listed(8), "a type without an update function is not listed");
    list.add(9, fields);
    list.add(10, fields);

    const std::optional<UpdateListMessage> left = list.remove(7, fields);
    require(left && left->message == 0x80000003u, "the removal sends 0x80000003");
    require(list.entries().size() == 2 && list.entries()[0].entity == 10 && list.entries()[1].entity == 9, "the last entry takes the freed place");
    require(!list.remove(7, fields), "an entity that is not listed (hword +0x1a = 0xffff) is left alone");
    UpdateListEntity deaf;
    deaf.typeHearsLeave = false;
    require(!list.remove(9, deaf) && !list.listed(9), "no message when the type record's byte +0x20 is 0");
}

void place() {
    const RoomLayerTable layers = startupRoomLayers();
    EntityUpdateList list;
    UpdateListEntity car;
    const std::optional<UpdateListMessage> on = list.place(3, car, std::uint8_t(0), layers);
    require(on && on->message == kEntityMessageJoinedUpdateList && list.listed(3), "an enabled car of the default room joins");
    require(!list.place(3, car, std::uint8_t(0), layers), "placing it again changes nothing");
    car.disabled = true;
    const std::optional<UpdateListMessage> off = list.place(3, car, std::uint8_t(0), layers);
    require(off && off->message == kEntityMessageLeftUpdateList && !list.listed(3), "disabled, it leaves");
    require(!list.place(3, car, std::nullopt, layers), "and stays out without a room");
}

}

int main() {
    try {
        roomTest();
        addAndRemove();
        place();
    } catch (const std::exception& error) {
        std::cerr << "entity-update-list: " << error.what() << '\n';
        return 1;
    }
    std::cout << "entity-update-list: ok\n";
    return 0;
}
