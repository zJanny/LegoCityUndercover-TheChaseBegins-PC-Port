#pragma once
// What an active director camera does to the characters (docs/re/dcam.md, SCRIPT-47): the per-update character routine
// 0x1835d0 (and its siblings 0x1571dc, 0x158134, 0x190174) picks its control routine by whether the camera slot 0x37689c is the
// director camera: 0x26be04 normally, 0x29bf30 while the director camera is current. No SDL dependency.
#include <string>

namespace chase::runtime {

enum class CharacterControl {
    None,  // held / press / heading of the controller are zeroed and nothing else runs
    Pad,   // the player reads the pad (0x200354)
    Ai,    // the AI controller runs (0x200c30)
};

// 0x26be04 (`directorHolds` false): the player reads the pad unless the AI controls him (bit 4 of +0xd8, AIEnable), every other
// character runs the AI. 0x29bf30 (`directorHolds` true): the player not under AI control gets nothing; anybody else runs the AI
// only when DCamActive set bit 1 of +0xd9.
CharacterControl characterControl(bool directorHolds, bool isPlayer, bool aiControlled, bool dcamActive);

}
