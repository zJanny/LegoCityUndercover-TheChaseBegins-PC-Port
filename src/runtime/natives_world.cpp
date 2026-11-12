#include "runtime/natives_world.hpp"

namespace chase::runtime {
namespace {

struct Entry {
    const char* name;
    WorldNativeKind kind;
    unsigned arguments;
    bool standIn;
};

constexpr Entry kEntries[] = {
    {"setpartycharacter", WorldNativeKind::SetPartyCharacter, 3, true},
    {"streamroomin", WorldNativeKind::StreamRoomIn, 1, false},
};

}

std::optional<WorldNativeKind> worldNativeKind(const std::string& lowerCaseName) {
    for (const Entry& entry : kEntries)
        if (lowerCaseName == entry.name) return entry.kind;
    return std::nullopt;
}

std::vector<std::string> worldNativeNames() {
    std::vector<std::string> names;
    for (const Entry& entry : kEntries) names.push_back(entry.name);
    return names;
}

unsigned worldNativeArguments(WorldNativeKind kind) {
    for (const Entry& entry : kEntries)
        if (entry.kind == kind) return entry.arguments;
    return 0;
}

bool worldNativeIsStandIn(WorldNativeKind kind) {
    for (const Entry& entry : kEntries)
        if (entry.kind == kind) return entry.standIn;
    return false;
}

}
