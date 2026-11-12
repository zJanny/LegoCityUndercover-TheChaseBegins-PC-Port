#pragma once
// Script natives that reach into the world systems (docs/re/script-natives-world.md): SetPartyCharacter 0x27beb4 (swap of the player entity
// 0x245258) and StreamRoomIn 0x1bfba4 (room request list 0x20f194). The runner records the requests; the host answers them. No SDL dependency.
#include "runtime/party_manager.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace chase::runtime {

enum class WorldNativeKind {
    SetPartyCharacter,
    StreamRoomIn,
};

std::optional<WorldNativeKind> worldNativeKind(const std::string& lowerCaseName);
std::vector<std::string> worldNativeNames();
unsigned worldNativeArguments(WorldNativeKind kind);
bool worldNativeIsStandIn(WorldNativeKind kind);

// SetPartyCharacter(Player1 | Player2, character, flag): 0x27beb4 maps `character & 0xffff` to a party slot (0x2457bc) and calls 0x245258(slot,
// index, 0, 1, flag != 0, 0, 0). `slot` is 0 for Player1 and 1 for Player2; any other object does nothing. The party manager answers it:
// no slot below the walk count, the slot of the player's character (0x245258 returns 0), or a swap.
enum class PartyOutcome { NotInParty, AlreadyCurrent, Swapped };
struct PartyRequest {
    unsigned slot = 0;
    std::uint16_t character = 0;
    bool flag = false;
    PartyOutcome outcome = PartyOutcome::NotInParty;
    std::optional<PartySwap> swap;
};

}
