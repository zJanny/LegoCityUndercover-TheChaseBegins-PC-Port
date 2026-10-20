#include "runtime/ability_table.hpp"

namespace chase::runtime {

void AbilityTable::load(const assets::Table& characters) {
    masks_.clear();
    startsLocked_.clear();
    if (!characters.sheets.empty()) {
        const assets::TableSheet& sheet = characters.sheets[0];
        if (sheet.stride >= kCharacterAbilityMaskOffset + kCharacterAbilityMaskBytes)
            for (unsigned row = 0; row < sheet.rows; ++row) {
                std::array<std::uint8_t, kCharacterAbilityMaskBytes> mask{};
                for (std::size_t i = 0; i < mask.size(); ++i) mask[i] = sheet.rowData[row * sheet.stride + kCharacterAbilityMaskOffset + i];
                masks_.push_back(mask);
            }
    }
    if (characters.sheets.size() > 2) {
        const assets::TableSheet& sheet = characters.sheets[2];
        if (sheet.stride > kAbilityUnlockRowOffset)
            for (unsigned row = 0; row < sheet.rows; ++row) startsLocked_.push_back(sheet.rowData[row * sheet.stride + kAbilityUnlockRowOffset] != 0);
    }
}

bool AbilityTable::characterHas(unsigned character, int ability) const {
    if (character >= masks_.size() || ability < 0 || std::size_t(ability) >= kCharacterAbilityMaskBytes * 8) return false;
    return (masks_[character][std::size_t(ability) >> 3] >> (ability & 7)) & 1;
}

bool AbilityTable::startsLocked(int ability) const {
    return ability >= 0 && std::size_t(ability) < startsLocked_.size() && startsLocked_[std::size_t(ability)];
}

}
