#include "block_reader.hpp"
#include <algorithm>
#include <stdexcept>

namespace chase::assets {
namespace {
using Reader = detail::BlockReader;

void need(const Bytes& block, std::size_t size, const char* what) {
    if (block.size() < size) throw std::runtime_error(std::string("table block too small: ") + what);
}

std::size_t alignUp(std::size_t value, std::size_t alignment) { return (value + alignment - 1) & ~(alignment - 1); }

void layoutColumns(TableSheet& sheet, const Bytes& types, const Bytes& stored) {
    std::size_t at = 0, rowAlignment = 1, nextStored = 0;
    for (std::size_t c = 0; c < types.size(); ++c) {
        TableColumn column;
        column.type = types[c];
        bool optional = column.type >= 8 && column.type <= 10;
        bool listed = optional && nextStored < stored.size() && stored[nextStored] == c;
        if (listed) ++nextStored;
        std::size_t size = 0, alignment = 1;
        switch (column.type) {
        case 0: break;
        case 1: case 4: case 5: case 6: case 7: size = alignment = 4; break;
        case 2: size = alignment = 2; break;
        case 3: size = 1; break;
        case 8: size = listed ? 1 : 0; break;
        case 9: size = listed ? 2 : 0; alignment = listed ? 2 : 1; break;
        case 10: size = listed ? 4 : 0; alignment = listed ? 4 : 1; break;
        default: throw std::runtime_error("unknown table column type " + std::to_string(column.type));
        }
        if (column.type == 1 || (column.type >= 4 && column.type <= 7)) rowAlignment = 4;
        else if (column.type == 2) rowAlignment = std::max<std::size_t>(rowAlignment, 2);
        at = alignUp(at, alignment);
        column.stored = size != 0;
        column.offset = at;
        column.size = size;
        at += size;
        sheet.columns.push_back(column);
    }
    if (nextStored != stored.size()) throw std::runtime_error("table stored-column list does not match the types");
    sheet.stride = alignUp(at, rowAlignment);
}
}

Table parseTable(const Bytes& data) {
    Reader r(data);
    r.cursor.push(Pool::M14);
    Bytes head = r.next();
    need(head, 4, "table header");
    Table table;
    if (unsigned count = readU16(head, 0)) {
        Bytes records = r.next();
        need(records, std::size_t(count) * 0x1c, "table sheet records");
        for (unsigned s = 0; s < count; ++s) {
            std::size_t at = std::size_t(s) * 0x1c;
            TableSheet sheet;
            unsigned columns = readU16(records, at), stride = readU16(records, at + 4);
            sheet.rows = readU16(records, at + 2);
            Bytes types = r.next();
            Bytes rows = r.next();
            Bytes stored;
            if (readU32(records, at + 0x14)) {
                stored = r.next();
                need(stored, records[at + 0x18], "table stored-column list");
                stored.resize(records[at + 0x18]);
            }
            if (readU32(records, at + 0x10)) sheet.strings = r.next();
            need(types, columns, "table column types");
            types.resize(columns);
            layoutColumns(sheet, types, stored);
            if (sheet.stride != stride) throw std::runtime_error("table row stride differs from the column layout");
            if (rows.size() != std::size_t(sheet.rows) * stride && !(columns == 0 && rows.empty()))
                throw std::runtime_error("table row block size mismatch");
            sheet.rowData = std::move(rows);
            table.sheets.push_back(std::move(sheet));
        }
    }
    r.cursor.pop();
    if (!r.cursor.exhausted()) throw std::runtime_error("unconsumed table blocks");
    return table;
}

TableValue tableValue(const TableSheet& sheet, std::size_t row, std::size_t column) {
    if (row >= sheet.rows || column >= sheet.columns.size()) throw std::runtime_error("table cell out of range");
    const TableColumn& c = sheet.columns[column];
    TableValue value;
    value.type = c.type;
    if (!c.stored) return value;
    value.present = true;
    std::size_t at = row * sheet.stride + c.offset;
    if (c.size == 1) value.bits = sheet.rowData[at];
    else if (c.size == 2) value.bits = readU16(sheet.rowData, at);
    else value.bits = readU32(sheet.rowData, at);
    if (c.type == 5 || c.type == 6) {
        if (value.bits >= sheet.strings.size() && !(value.bits == 0 && sheet.strings.empty()))
            throw std::runtime_error("table string offset outside the string pool");
        auto begin = sheet.strings.begin() + std::ptrdiff_t(std::min<std::size_t>(value.bits, sheet.strings.size()));
        value.text.assign(begin, std::find(begin, sheet.strings.end(), std::uint8_t(0)));
    }
    return value;
}

float TableValue::asFloat() const {
    float f;
    std::memcpy(&f, &bits, sizeof f);
    return f;
}

std::vector<SoundEffect> parseSoundEffects(const Table& table) {
    if (table.sheets.empty()) throw std::runtime_error("sound effect table has no sheets");
    const TableSheet& sheet = table.sheets[0];
    if (sheet.columns.size() < 17) throw std::runtime_error("sound effect sheet has too few columns");
    std::vector<SoundEffect> effects(sheet.rows);
    for (std::size_t row = 0; row < sheet.rows; ++row) {
        SoundEffect& effect = effects[row];
        effect.id = std::uint32_t(row);
        effect.name = tableValue(sheet, row, 1).text;
        effect.column2 = tableValue(sheet, row, 2).bits;
        effect.column3 = tableValue(sheet, row, 3).bits;
        effect.column11 = tableValue(sheet, row, 11).bits;
        effect.column12 = tableValue(sheet, row, 12).bits;
        effect.volumePercent = tableValue(sheet, row, 13).bits;
        effect.paramA = tableValue(sheet, row, 14).bits;
        effect.paramB = tableValue(sheet, row, 15).bits;
        effect.column16 = tableValue(sheet, row, 16).asFloat();
    }
    return effects;
}

const SoundEffect* findSoundEffect(const std::vector<SoundEffect>& effects, std::uint32_t id) {
    return id < effects.size() ? &effects[id] : nullptr;
}

std::string soundEffectPath(const SoundEffect& effect) {
    return effect.name.empty() ? std::string() : "sounds/" + effect.name + ".bwav";
}

SoundTables parseSoundTables(const Table& soundfx) {
    if (soundfx.sheets.size() < 4) throw std::runtime_error("sound effect table has fewer than four sheets");
    SoundTables tables;
    const TableSheet& names = soundfx.sheets[1];
    if (names.columns.size() < 2) throw std::runtime_error("music name sheet has too few columns");
    for (std::size_t row = 0; row < names.rows; ++row) tables.musicNames.push_back(tableValue(names, row, 1).text);
    const TableSheet& sets = soundfx.sheets[2];
    if (sets.columns.size() < 3) throw std::runtime_error("music set sheet has too few columns");
    for (std::size_t row = 0; row < sets.rows; ++row)
        tables.musicSets.push_back({std::uint8_t(tableValue(sets, row, 1).bits), std::uint8_t(tableValue(sets, row, 2).bits)});
    const TableSheet& sounds = soundfx.sheets[3];
    if (sounds.columns.size() < kCharacterSoundSlots + 1) throw std::runtime_error("character sound sheet has too few columns");
    for (std::size_t row = 0; row < sounds.rows; ++row) {
        std::array<std::uint16_t, kCharacterSoundSlots> slots{};
        for (std::size_t slot = 0; slot < kCharacterSoundSlots; ++slot) slots[slot] = std::uint16_t(tableValue(sounds, row, slot + 1).bits);
        tables.characterSounds.push_back(slots);
    }
    return tables;
}

std::uint8_t levelMusicSet(const Table& levels, std::size_t row) {
    if (levels.sheets.empty()) throw std::runtime_error("level table has no sheets");
    return std::uint8_t(tableValue(levels.sheets[0], row, 24).bits);
}

std::uint8_t characterSoundCategory(const Table& characters, std::size_t row) {
    if (characters.sheets.empty()) throw std::runtime_error("character table has no sheets");
    return std::uint8_t(tableValue(characters.sheets[0], row, 95).bits);
}
} // namespace chase::assets
