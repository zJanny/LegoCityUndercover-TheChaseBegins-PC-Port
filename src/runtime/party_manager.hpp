#pragma once
// The party manager 0x51b248 of the original (docs/re/disguise.md): up to ten members with their character ids, removal and skip flags,
// stand-for ids and member entities (`Player1.Player01` and following of levels/players.blvl). Ported: the level set-up 0x171bb0 with the
// list of 0x1b1c90, the reset 0x1677b4, the add 0x22dbcc, the removal 0x22db24, the slot lookup 0x2457bc, the cycle 0x235110, the party part
// of UnlockCharacter 0x263918 and LockCharacter 0x1cda60, and the bookkeeping of the swap 0x245258. What the swap does to the entities (the
// member becoming the player entity, its scripts, state and carried thing) is reported to the host in PartySwap. No SDL dependency.
#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace chase::runtime {

constexpr unsigned kPartySlots = 10;
constexpr unsigned kPartyAddSlots = 8;      // the add 0x22dbcc gives a new id the first empty slot of the first eight
constexpr unsigned kPartyLevelIds = 8;      // TablesLevels sheet 0 row +0x43..+0x4a, slot +0x5201..+0x5208
constexpr unsigned kPartyLevelExtras = 2;   // TablesLevels sheet 0 row +0x4b, +0x4c (0x37590c, 0x37590d)
constexpr std::int8_t kCustomisationLevel = 9;
constexpr std::uint32_t kSwapEffectGateUpdates = 15;  // 0x245258: gate 0x375d44 + 0xc = update count + 15
constexpr unsigned kCycleCheatSlot = 7;               // 0x235110 also takes slot 7 when bit 7 of the group bits is set
constexpr unsigned kPlayerMembers = 16;               // levels/players.blvl: Player01 to Player16 of class chars/char_player
constexpr const char* kFirstPlayerMember = "Player1.Player01";  // 0x19f41c makes it the player entity at the level set-up

// One slot of the manager: id +4, stand-for id +0xe, removal flag +0x18, second skip flag +0x22, entity +0x2c (the member's name, empty for
// none).
struct PartySlot {
    std::uint8_t id = 0;
    std::uint8_t standFor = 0;
    bool removed = false;
    bool skipped = false;
    std::string entity;
};

// The list 0x519c38 that the set-up 0x171bb0 walks, and the walk count (manager +2) that 0x1b1c90 sets.
struct PartyList {
    std::array<std::uint8_t, kPartySlots> ids{};
    std::uint16_t walkCount = 0;
};

// 0x20a4d4 / 0x1b1c90 / 0x171bb0: the eight ids of the level row (the slot's ids +0x5201 when slot +7 bit 0 is set), replaced by
// 9, 19, 29, 39, 49, 59, 69, 80 when the cheat byte 0x375961 has bit 0; the row's extras +0x4b, +0x4c follow the listed ids (up to the
// first 0). The walk count is the number of listed ids plus the extras taken. 0x171bb0 then copies the slot's ids over the first eight
// again when slot +7 bit 0 is set.
PartyList partyList(const std::array<std::uint8_t, kPartyLevelIds>& rowIds, const std::array<std::uint8_t, kPartyLevelExtras>& rowExtras,
                    const std::array<std::uint8_t, kPartyLevelIds>& slotIds, bool ownParty, bool cheatAllCharacters);

// The global bytes the manager reads: the group bits of the live slot (+0x51bb, 0x52086b) and the byte after them (+0x51bc, which the set-up
// reads as the bits of list positions 8 and 9), the cheat byte 0x375961 bit 0 and the byte 0x37751c (1 lets the cycle take the start slot;
// it also turns the AddCharacterToParty natives off; writer not read).
struct PartyFlags {
    std::array<std::uint8_t, 2> groupBytes{};
    bool cheatAllCharacters = false;
    bool partyNativesOff = false;
};

// The arguments of 0x245258 beyond the slot and the index: 5th (no effects, no member scripts), 6th (update the stand-for ids), 7th (no
// OnSceneEnter script).
struct SwapOptions {
    bool quiet = false;
    bool updateStandFor = false;
    bool noScripts = false;
};

// A swap that 0x245258 carried out (it returned 1). `member` is the entity it looked up ("Player<1|2>.Player<index+1>"); `swapped` is false
// when that member is missing or the session byte +0x25 is 9 (0x2254c4), and then the player entity stays. `effects`: effects 7 and 241 at
// the player's node. `scripts`: the member's `startup` script was asked for, then OnSceneEnter<level> when it has none and `sceneScript`.
struct PartySwap {
    unsigned player = 0;
    std::uint8_t index = 0;
    std::string member;
    std::uint8_t character = 0;
    bool swapped = false;
    bool effects = false;
    bool scripts = false;
    bool sceneScript = false;
};

class PartyManager {
public:
    // Session bytes +0x26 (level row) and +0x25 that 0x20a4d4 sets: +0x25 is the level for rows 9 and above, else 0.
    void setLevel(std::int8_t level);
    void setFlags(const PartyFlags& flags) { flags_ = flags; }
    const PartyFlags& flags() const { return flags_; }
    // The update count 0x241098 for the swap's effect gate.
    void setClock(std::uint32_t updates) { clock_ = updates; }

    // The level set-up 0x2e41e4 (unless the byte 0x377530 is set, not modelled): 0x19f41c makes Player1.Player01 the player entity, the reset
    // 0x1677b4, then 0x171bb0: the character byte 0x375d45 is the first id, every non-zero id of the list is added with the next member
    // entity (Player1.Player01, 02, ...) and becomes that member's character; then each id after the first leaves the party (0x22db24) when
    // the group bit of its list position is clear, unless the cheat byte is set.
    void setUp(const PartyList& list);
    // 0x1677b4: the usable count, the ids, stand-for ids and both skip flags are cleared, and the swap effect gate. The walk count and the
    // entity words stay.
    void reset();
    // 0x22dbcc(entity, id, 0): does nothing in level 9; id 0 is the entity's own character. A slot that holds the id takes the entity and loses
    // both skip flags (the usable count rises when it was removed); otherwise the first empty slot of the first eight takes id and entity and
    // the usable count rises. The entity side (portrait 0x262320, CharacterClass, the add with a flag) is not modelled.
    void add(const std::string& entity, std::uint8_t id);
    // 0x22db24: false when no slot holds the id; when it is the player's slot the cycle 0x235110 runs forward first; then the slot is flagged
    // removed (the usable count falls once) and its entity cleared.
    bool remove(std::uint8_t id);
    // 0x2457bc: the slot of an id, -1 for none and for id 0.
    int slotOf(std::uint8_t id) const;
    // 0x235110(backward, player, flag): the next usable slot in that direction and a swap to it; nullopt when no slot is taken or the swap
    // returned 0.
    std::optional<PartySwap> cycle(bool backward, bool flag);
    // 0x245258(player, index, ...): nullopt (it returns 0) when index is 255 or the slot of the player's character.
    std::optional<PartySwap> swap(unsigned player, std::uint8_t index, const SwapOptions& options);
    // SetPartyCharacter 0x27beb4: the slot of `character & 0xffff`; nothing when it is not below the walk count; else 0x245258(player, slot,
    // 0, 1, flag, 0, 0). Returns whether the character has such a slot.
    bool setPartyCharacter(unsigned player, std::uint16_t character, bool flag, std::optional<PartySwap>& swap);
    // UnlockCharacter 0x263918 for 9, 19, 29, 39, 49, 59, 69: the add with the member entity of the slot that holds the id (none when no
    // slot does).
    void unlockCharacter(std::int32_t id);
    // LockCharacter 0x1cda60 for the same ids: the removal 0x22db24.
    void lockCharacter(std::int32_t id);

    const std::array<PartySlot, kPartySlots>& slots() const { return slots_; }
    std::uint16_t usableCount() const { return usable_; }
    std::uint16_t walkCount() const { return walk_; }
    // The character id at component +0x2cc of the player entity, and that entity.
    std::uint8_t playerCharacter() const { return playerCharacter_; }
    void setPlayerCharacter(std::uint8_t id) { playerCharacter_ = id; }
    const std::string& playerEntity() const { return playerEntity_; }
    // The byte 0x375d45 (and 0x375d46 from the set-up).
    std::uint8_t characterByte() const { return characterByte_; }
    std::uint8_t secondCharacterByte() const { return secondCharacterByte_; }
    // Swaps carried out since the last call, in order (including those of a removal's cycle).
    std::vector<PartySwap> takeSwaps();

private:
    std::uint8_t memberCharacter(const std::string& member) const;
    void updateStandFor(std::uint8_t index, int current);

    std::array<PartySlot, kPartySlots> slots_{};
    std::uint16_t usable_ = 0;
    std::uint16_t walk_ = 0;
    std::int8_t level_ = 0;
    std::uint8_t entryLevel_ = 0;
    PartyFlags flags_;
    std::uint32_t clock_ = 0;
    std::uint32_t effectGate_ = 0;
    std::uint8_t playerCharacter_ = 9;
    std::string playerEntity_ = kFirstPlayerMember;
    std::uint8_t characterByte_ = 0;
    std::uint8_t secondCharacterByte_ = 0;
    std::map<std::string, std::uint8_t> memberCharacters_;
    std::vector<PartySwap> swaps_;
};

}
