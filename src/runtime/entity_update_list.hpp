#pragma once
// The entity update list 0x580564 (docs/re/traffic-car-body.md): which entities are updated, and the messages 0x80000004 / 0x80000003 an
// entity gets when it joins or leaves. 0x23c1a8(entity, room) decides from the room layer table, 0x173534 adds and 0x22d480 removes.
// No SDL dependency.
#include "runtime/room_layers.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace chase::runtime {

constexpr std::uint32_t kEntityMessageLeftUpdateList = 0x80000003;
constexpr std::uint32_t kEntityMessageJoinedUpdateList = 0x80000004;

// What 0x23c1a8 reads of an entity.
struct UpdateListEntity {
    bool disabled = false;           // hword +0x10 bit 0
    std::uint8_t roomList = 0;       // byte +0x13: which of the room's entity lists (+0x58 + 8 x list) holds the entity
    std::uint8_t layerClass = 0;     // byte +0x18
    std::uint8_t priority = 0;       // byte +0x19
    bool typeUpdates = true;         // the type row (0x583124 + 40 x byte +0x12) has an update function at +0xc
    bool typeHearsLeave = true;      // byte +0x20 of the type record (entity +0x20) is not 0
};

// 0x23c1a8(entity, room): an entity belongs on the update list when it has a room, bit 1 of the layer hword
// [class x 17 + min(room type, 16)] is set, the entity is enabled and its byte +0x13 is 0. The room apply 0x189c90 runs the same test for
// every entity of a room whose state or type byte (+0x39, +0x3a) changed.
bool entityBelongsOnUpdateList(const UpdateListEntity& entity, const std::optional<std::uint8_t>& roomType, const RoomLayerTable& layers);

struct UpdateListMessage {
    std::size_t entity = 0;
    std::uint32_t message = 0;
};

// The list 0x580564 (8 bytes per entry: entity and priority byte, count 0x376aa0) with the entity's index hword +0x1a (0xffff: not listed).
class EntityUpdateList {
public:
    // 0x173534: a listed entity only gets its priority refreshed. Otherwise an entity whose type updates is appended and gets message
    // 0x80000004. Returns the message sent.
    std::optional<UpdateListMessage> add(std::size_t entity, const UpdateListEntity& fields);
    // 0x22d480: the last entry takes the place of the removed one; message 0x80000003 when the type record's byte +0x20 is set.
    std::optional<UpdateListMessage> remove(std::size_t entity, const UpdateListEntity& fields);
    // 0x23c1a8: add() or remove() by entityBelongsOnUpdateList().
    std::optional<UpdateListMessage> place(std::size_t entity, const UpdateListEntity& fields, const std::optional<std::uint8_t>& roomType,
                                           const RoomLayerTable& layers);

    bool listed(std::size_t entity) const;
    struct Entry {
        std::size_t entity = 0;
        std::uint8_t priority = 0;
    };
    const std::vector<Entry>& entries() const { return entries_; }

private:
    std::vector<Entry> entries_;
};

}
