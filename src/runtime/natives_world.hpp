#pragma once
// Script natives that reach into the world systems (docs/re/script-natives-world.md): SetPartyCharacter 0x27beb4 (swap of the player entity
// 0x245258) and StreamRoomIn 0x1bfba4 (room request list 0x20f194). The runner records the requests; the host answers them. No SDL dependency.
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

// SetPartyCharacter(Player1 | Player2, character, flag): 0x27beb4 maps `character & 0xffff` to a party index (0x2457bc) and calls 0x245258(slot,
// index, 0, 1, flag != 0, 0, 0). `slot` is 0 for Player1 and 1 for Player2; any other object does nothing.
struct PartyRequest {
    unsigned slot = 0;
    std::uint16_t character = 0;
    bool flag = false;
};

// 0x245258 returns at once when the party index already is the one of the player entity's character (the byte at component +0x2cc); a different one
// swaps the player entity to the party member "Player<slot+1>.Player<index+1>". The port knows the character of the player (`current`) and has no
// party members, so a different request is reported to the host as not ported.
enum class PartyOutcome { AlreadyCurrent, NotPorted };
PartyOutcome partyOutcome(std::uint16_t current, const PartyRequest& request);

}
