#pragma once
// The allegiance lists (the "engaged table" 0x58deb0 and its partner 0x58dec8 with the player slot 0x58dee0; docs/re/scripts-missions-1-4-roof.md,
// docs/re/scripts-missions-1-4-spawners-conduct.md): two lists of six characters that own a path record, split by entity +0xc bit 2 (AIBadGuy:
// 0x58deb0), and the player slot the target search 0x1f8d5c writes. 0x206d18 puts a character into the list of its side (ctrl+0xd8 bit 5), 0x21decc
// takes it out. The single owner of the lists: the FallArrestBoundSystem update 0x307980 walks the list of the bad guys, the aggro, the movement under
// the AI and the idle decision (goon_conduct.hpp) engage and release through it. No SDL dependency.
#include "runtime/fall_arrest_bounds.hpp"

#include <array>
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace chase::runtime {

constexpr std::size_t kEngagedListSlots = 6;
constexpr std::uint8_t kAutoArrestMark = 2;      // sub +0x131 bits 6..7 written by 0x307980

struct EngageResult {
    bool alreadyEngaged = false;  // ctrl+0xd8 bit 5 was set: 0x206d18 returned (or took the entity's own entry); the path record rebuild is not modelled
    bool joined = false;          // ctrl+0xd8 bit 5 was clear and a slot was found
    bool listFull = false;        // ctrl+0xd8 bit 5 was clear and all six slots were taken
    bool counterCleared = false;  // ctrl+0x72 = 0 at the end of 0x206d18 (every call that did not return at the first test)
};

class EngagedTable {
public:
    // 0x206d18, the membership part: a character with ctrl+0xd8 bit 5 set returns at once; otherwise it takes the first free slot of the list of its
    // side. The path record in the slot (0x202054, 0x1976b8) is not modelled.
    EngageResult engage(const std::string& name, bool badGuy);
    // 0x21decc: a boss (entity +0xc bit 14) whose AI request ctrl+0x15 is not 1 stays; otherwise its slot is emptied and ctrl+0xd8 bit 5 cleared.
    bool release(const std::string& name, bool badGuy, bool boss, bool aiRequestIsOne);
    bool engaged(const std::string& name) const { return members_.count(name) != 0; }
    std::size_t count(bool badGuy) const;
    const std::array<std::string, kEngagedListSlots>& list(bool badGuy) const { return badGuy ? bad_ : other_; }
    // 0x58dee0: the player as the target search 0x1f8d5c last saw it (empty: none); not counted in the six slots.
    void setPlayerSlot(const std::string& name) { player_ = name; }
    const std::string& playerSlot() const { return player_; }

    // ctrl+0x314 bit 6 (the character stands on the ground). The mover 0x20b498 that writes it is not ported (STAND-IN SCRIPT-S328): the glue leaves it set.
    void setOnGround(const std::string& name, bool onGround);
    bool onGround(const std::string& name) const;
    // Sub +0x131 bits 6..7 of a character (the auto-arrested mark), kept after the character leaves the list.
    std::uint8_t mark(const std::string& name) const;
    bool autoArrested(const std::string& name) const { return mark(name) == kAutoArrestMark; }

    // 0x307980: the characters of the bad guys' list in slot order, each with the enabled bit and position `describe` gives; the ones in the air that
    // fall into an enabled FallArrest bound get the mark.
    using Describe = std::function<std::optional<FallArrestCharacter>(const std::string& name)>;
    std::vector<FallArrestHit> fallArrestUpdate(const assets::Level& level, const std::vector<std::size_t>& bounds,
                                                const std::vector<std::uint8_t>& boundEnabled, const Describe& describe);

private:
    std::array<std::string, kEngagedListSlots> bad_{};
    std::array<std::string, kEngagedListSlots> other_{};
    std::string player_;
    std::map<std::string, bool> members_;
    std::map<std::string, bool> airborne_;
    std::map<std::string, std::uint8_t> marks_;
};

// The sound of a character that falls into a bound: slot 20 (u16 +0x28) of its sound row.
std::uint16_t fallArrestSoundId(const std::array<std::uint16_t, assets::kCharacterSoundSlots>& row);

}
