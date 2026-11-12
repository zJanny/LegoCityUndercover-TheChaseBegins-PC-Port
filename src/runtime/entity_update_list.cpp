#include "runtime/entity_update_list.hpp"

#include <algorithm>

namespace chase::runtime {
namespace {

constexpr std::uint16_t kLayerOnUpdateList = 2;

}

bool entityBelongsOnUpdateList(const UpdateListEntity& entity, const std::optional<std::uint8_t>& roomType, const RoomLayerTable& layers) {
    if (!roomType) return false;
    const std::size_t type = std::min<std::size_t>(*roomType, kRoomLayerTypes - 1);
    const std::uint16_t word = layers.at(entity.layerClass * kRoomLayerTypes + type);
    return (word & kLayerOnUpdateList) != 0 && !entity.disabled && entity.roomList == 0;
}

bool EntityUpdateList::listed(std::size_t entity) const {
    return std::any_of(entries_.begin(), entries_.end(), [entity](const Entry& entry) { return entry.entity == entity; });
}

std::optional<UpdateListMessage> EntityUpdateList::add(std::size_t entity, const UpdateListEntity& fields) {
    for (Entry& entry : entries_) {
        if (entry.entity != entity) continue;
        entry.priority = fields.priority;
        return std::nullopt;
    }
    if (!fields.typeUpdates) return std::nullopt;
    entries_.push_back({entity, fields.priority});
    return UpdateListMessage{entity, kEntityMessageJoinedUpdateList};
}

std::optional<UpdateListMessage> EntityUpdateList::remove(std::size_t entity, const UpdateListEntity& fields) {
    const auto found = std::find_if(entries_.begin(), entries_.end(), [entity](const Entry& entry) { return entry.entity == entity; });
    if (found == entries_.end()) return std::nullopt;
    *found = entries_.back();
    entries_.pop_back();
    if (!fields.typeHearsLeave) return std::nullopt;
    return UpdateListMessage{entity, kEntityMessageLeftUpdateList};
}

std::optional<UpdateListMessage> EntityUpdateList::place(std::size_t entity, const UpdateListEntity& fields, const std::optional<std::uint8_t>& roomType,
                                                         const RoomLayerTable& layers) {
    if (entityBelongsOnUpdateList(fields, roomType, layers)) return add(entity, fields);
    return remove(entity, fields);
}

}
