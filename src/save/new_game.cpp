#include "save/new_game.hpp"

#include <stdexcept>
#include <string>

namespace chase::save {
namespace {
constexpr unsigned kFirstStartCharacter = 9;
constexpr std::uint8_t kStartGroupLimit = 2;
constexpr std::uint8_t kLevelTwoUnlockBit = 0x10;
constexpr std::uint8_t kSlotCallbackFlag = 0x20;
constexpr std::size_t kLevelOneGroupOffset = 0x43;
constexpr std::size_t kLevelPartyOffset = 0x43;
constexpr std::size_t kCharacterSecondBitOffset = 0x4;
constexpr std::size_t kCharacterStartGroupOffset = 0xc;
constexpr std::size_t kCharacterCustomisationOffset = 0x21;
constexpr std::size_t kCharacterLinkedOffset = 0x22;
constexpr std::size_t kCharacterNextOffset = 0x23;
constexpr std::size_t kAbilityLockOffset = 0x6;
constexpr std::size_t kCustomisationFlagsOffset = 4;
constexpr std::size_t kCustomisationIdOffset = 5;
constexpr std::uint8_t kCustomisationSharedBit = 2;
constexpr std::uint8_t kSoundVolumeDefault = 10, kMusicVolumeDefault = 8;
constexpr std::size_t kSoundVolumeOption = 4, kMusicVolumeOption = 6, kSlotRecordOption = 0x1ec, kSlotRecordStride = 0x18;
constexpr unsigned kSlotOptionLimit = 3;
constexpr unsigned kFirstUnlockLevel = 2, kLastUnlockLevel = 8;
constexpr unsigned kPartySize = 8;

const assets::TableSheet& sheetOf(const assets::Table& table, std::size_t index, unsigned minRows, const char* name) {
    if (table.sheets.size() <= index) throw std::runtime_error(std::string(name) + " has no sheet " + std::to_string(index));
    const assets::TableSheet& sheet = table.sheets[index];
    if (sheet.rows < minRows) throw std::runtime_error(std::string(name) + " sheet " + std::to_string(index) + " is too short");
    return sheet;
}

std::uint32_t fieldAt(const assets::TableSheet& sheet, unsigned row, std::size_t byteOffset, const char* name) {
    for (std::size_t column = 0; column < sheet.columns.size(); ++column) {
        if (sheet.columns[column].offset != byteOffset || !sheet.columns[column].stored) continue;
        const assets::TableValue value = assets::tableValue(sheet, row, column);
        if (value.present) return value.bits;
    }
    throw std::runtime_error(std::string(name) + ": no stored field at byte +" + std::to_string(byteOffset));
}

std::uint8_t byteAt(const assets::TableSheet& sheet, unsigned row, std::size_t byteOffset, const char* name) {
    const std::uint32_t bits = fieldAt(sheet, row, byteOffset, name);
    if (bits > 0xff) throw std::runtime_error(std::string(name) + ": field at byte +" + std::to_string(byteOffset) + " is not a byte");
    return std::uint8_t(bits);
}

void setBit(std::uint8_t* slot, std::size_t offset, unsigned index) { slot[offset + (index >> 3)] |= std::uint8_t(1u << (index & 7)); }
bool bitSet(const std::uint8_t* slot, std::size_t offset, unsigned index) { return slot[offset + (index >> 3)] >> (index & 7) & 1; }

void setCharacterBit(std::uint8_t* slot, unsigned id, unsigned which) {
    const unsigned index = 2 * id + which;
    if (!bitSet(slot, kSlotCharacterBitsOffset, index) && id != kFirstStartCharacter) setBit(slot, kSlotCharacterNewOffset, id);
    setBit(slot, kSlotCharacterBitsOffset, index);
}

const NewGameCharacter& characterRow(const NewGameTables& tables, unsigned id) {
    if (id >= tables.characters.size()) throw std::runtime_error("character id " + std::to_string(id) + " outside TablesCharacters");
    return tables.characters[id];
}

void readCustomisation(std::uint8_t* slot, const NewGameTables& tables) {
    for (unsigned record = 0; record < kCustomisationRecords; ++record) {
        std::uint8_t* bytes = slot + kSlotCustomisationOffset + record * kCustomisationRecordSize;
        for (unsigned part = 0; part < kCustomisationParts; ++part) bytes[part] = kNewGameCustomisationPartsStandIn[record][part];
        const std::uint8_t id = kGroupCharacters[record];
        const bool own = characterRow(tables, id == 0 ? kEmptyGroupCharacter : id).ownCustomisation;
        bytes[kCustomisationFlagsOffset] = std::uint8_t(own ? bytes[kCustomisationFlagsOffset] & ~kCustomisationSharedBit
                                                            : bytes[kCustomisationFlagsOffset] | kCustomisationSharedBit) & 3;
        bytes[kCustomisationIdOffset] = id;
    }
}
}

NewGameTables newGameTables(const assets::Table& levels, const assets::Table& characters) {
    NewGameTables tables;
    const assets::TableSheet& levelSheet = sheetOf(levels, 0, kLastUnlockLevel + 1, "TablesLevels");
    tables.levelOneGroup[0] = byteAt(levelSheet, 1, kLevelOneGroupOffset, "TablesLevels");
    tables.levelOneGroup[1] = byteAt(levelSheet, 1, kLevelOneGroupOffset + 1, "TablesLevels");
    for (unsigned row = 0; row < levelSheet.rows; ++row) {
        std::array<std::uint8_t, 8> party{};
        for (unsigned i = 0; i < kPartySize; ++i) party[i] = byteAt(levelSheet, row, kLevelPartyOffset + i, "TablesLevels");
        tables.levelParty.push_back(party);
    }
    const assets::TableSheet& characterSheet = sheetOf(characters, 0, kCharacterIdLimit, "TablesCharacters");
    for (unsigned row = 0; row < characterSheet.rows; ++row) {
        NewGameCharacter character;
        character.startGroup = byteAt(characterSheet, row, kCharacterStartGroupOffset, "TablesCharacters");
        character.secondBit = fieldAt(characterSheet, row, kCharacterSecondBitOffset, "TablesCharacters") == 0;
        character.ownCustomisation = byteAt(characterSheet, row, kCharacterCustomisationOffset, "TablesCharacters") == 1;
        character.linked = byteAt(characterSheet, row, kCharacterLinkedOffset, "TablesCharacters");
        character.next = byteAt(characterSheet, row, kCharacterNextOffset, "TablesCharacters");
        tables.characters.push_back(character);
    }
    const assets::TableSheet& abilitySheet = sheetOf(characters, 2, kAbilityBits, "TablesCharacters");
    for (unsigned row = 0; row < kAbilityBits; ++row)
        tables.abilityLocked[row] = byteAt(abilitySheet, row, kAbilityLockOffset, "TablesCharacters sheet 2") != 0;
    return tables;
}

NewGameTables newGameTables(assets::FuseArchive& archive) {
    const auto table = [&](const char* path) {
        const assets::FuseEntry* entry = archive.find(path);
        if (!entry) throw std::runtime_error(std::string("not in archive: ") + path);
        return assets::parseTable(archive.read(*entry));
    };
    return newGameTables(table("tables/tableslevels.bxls"), table("tables/tablescharacters.bxls"));
}

void unlockCharacterChain(std::uint8_t* slot, const NewGameTables& tables, unsigned id) {
    do {
        if (id >= kCharacterIdLimit) throw std::runtime_error("character id " + std::to_string(id) + " outside the character bits");
        const NewGameCharacter& row = characterRow(tables, id);
        setCharacterBit(slot, id, 0);
        if (row.secondBit) setCharacterBit(slot, id, 1);
        if (row.linked != 0) unlockCharacterChain(slot, tables, row.linked);
        id = row.next;
    } while (id != 0);
}

Bytes newGameSlot(const NewGameTables& tables) {
    Bytes slot(kSlotSize, 0);
    std::uint8_t* data = slot.data();
    data[kSlotLevelOneGroupOffset] = tables.levelOneGroup[0];
    data[kSlotLevelOneGroupOffset + 1] = tables.levelOneGroup[1];
    data[kSlotCurrentLevelOffset] = std::uint8_t(kNewGameLevel);
    data[kSlotGroupBitsOffset] |= 1;
    for (unsigned id = kFirstStartCharacter; id < kCharacterIdLimit; ++id)
        if (characterRow(tables, id).startGroup < kStartGroupLimit) unlockCharacterChain(data, tables, id);
    for (unsigned i = 0; i < kAbilityBits / 8; ++i) data[kSlotAbilityOffset + i] = 0xff;
    for (unsigned bit = 0; bit < kAbilityBits; ++bit)
        if (tables.abilityLocked[bit]) data[kSlotAbilityOffset + (bit >> 3)] &= std::uint8_t(~(1u << (bit & 7)));
    data[kLevelUnlockBitsOffset] |= kLevelTwoUnlockBit;
    readCustomisation(data, tables);
    data[kSlotStudFlagsOffset] |= kSlotCallbackFlag;
    data[kSlotFlagsOffset] |= kSlotInUseBit;
    return slot;
}

void resetSlotOptions(SaveImage& image, unsigned slotIndex) {
    if (slotIndex >= kSlotOptionLimit) return;
    image.options()[kSoundVolumeOption + slotIndex] = kSoundVolumeDefault;
    image.options()[kMusicVolumeOption + slotIndex] = kMusicVolumeDefault;
    image.options()[kSlotRecordOption + slotIndex * kSlotRecordStride] = 0;
}

LevelEntry enterLevel(std::vector<std::uint8_t>& slot, unsigned level, const NewGameTables& tables) {
    if (slot.size() != kSlotSize) throw std::runtime_error("the live slot is not " + std::to_string(kSlotSize) + " bytes");
    LevelEntry entry;
    entry.level = level == 0 ? kEmptyLevelFallback : level;
    for (unsigned i = 0; i < 8; ++i) entry.studs |= std::uint64_t(slot[kSlotStudsOffset + i]) << (8 * i);
    for (unsigned i = 0; i < 4; ++i) entry.superBricks |= std::uint32_t(slot[kSlotSuperBrickCountOffset + i]) << (8 * i);
    if (entry.level >= kFirstUnlockLevel && entry.level <= kLastUnlockLevel) {
        setBit(slot.data(), kLevelUnlockBitsOffset, 2 * entry.level);
        slot[kSlotCurrentLevelOffset] = std::uint8_t(entry.level);
    }
    if (!(slot[kSlotFlagsOffset] & kSlotOwnPartyBit)) {
        if (entry.level >= tables.levelParty.size()) throw std::runtime_error("level " + std::to_string(entry.level) + " outside TablesLevels");
        for (unsigned i = 0; i < kPartySize; ++i) slot[kSlotPartyOffset + i] = tables.levelParty[entry.level][i];
    }
    return entry;
}
}
