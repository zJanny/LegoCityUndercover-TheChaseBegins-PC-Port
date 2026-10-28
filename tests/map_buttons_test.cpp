// Map buttons rules on the layout boxes of sprites/blend/mapbuttons/mapbuttons.bxaml (src/runtime/map_buttons.cpp). Provenance: the create
// 0x1bd424, the update 0x2d9f84, the hit test 0x1ee520 with 0x20a410, the toggles 0x217d0c, 0x2134c8 and 0x217bec, the fast travel open 0x1ccf60
// and close 0x1ee5f8, the touch phase writer 0x108fa0, the level table 0x375afc and the icon table 0x375cd0, read with REA (Ghidra 12.1.2) and
// llvm-objdump of artifacts/game/game.arm.elf by 20261028-uiport-A on 2026-10-28 (docs/re/ui-screens.md). Boxes and storyboard lengths:
// `chase-layoutdump sprites/blend/mapbuttons/mapbuttons.bxaml` (charsOpen 0.7 s, charsClose 0.8 s).
#include "runtime/map_buttons.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase::runtime;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr float kDt = 1.0f / 30.0f;

MapButtonsSetup setup(std::int8_t level) {
    MapButtonsSetup s;
    s.boxes = {{{0, 176, 64, 64}, {0, 176, 64, 64}, {0, 0, 64, 64}, {0, 116, 64, 64}, {256, 27, 64, 64}, {256, 149, 64, 64}, {256, 88, 64, 64},
                {-32, 112, 128, 128}, {19, 141, 128, 128}, {70, 112, 128, 128}, {121, 141, 128, 128}, {172, 112, 128, 128}, {223, 141, 128, 128},
                {95, 1, 128, 128}}};
    s.storyboardLengths = {{"charsOpen", 0.7f}, {"charsClose", 0.8f}};
    s.level = level;
    return s;
}

MapButtonsInput idle() {
    MapButtonsInput input;
    input.dt = kDt;
    input.canChangeCharacter = true;
    input.characterSwitchReady = true;
    return input;
}

MapButtonsInput touching(TouchPhase phase, float x, float y) {
    MapButtonsInput input = idle();
    input.phase = phase;
    input.x = x;
    input.y = y;
    return input;
}

bool has(const std::vector<MapButtonsEvent>& events, MapButtonsEventKind kind, int value = -1000) {
    return std::any_of(events.begin(), events.end(), [&](const MapButtonsEvent& e) { return e.kind == kind && (value == -1000 || e.value == value); });
}

std::string order(const MapButtons& buttons) {
    std::string text;
    for (const MapStoryboard& s : buttons.storyboards()) text += s.name + " ";
    return text;
}

void touchPhases() {
    require(nextTouchPhase(TouchPhase::None, true) == TouchPhase::Pressed, "a new touch is phase 1");
    require(nextTouchPhase(TouchPhase::Pressed, true) == TouchPhase::Held, "a held touch is phase 2");
    require(nextTouchPhase(TouchPhase::Held, false) == TouchPhase::Released, "the release is phase 3");
    require(nextTouchPhase(TouchPhase::Released, false) == TouchPhase::None, "then phase 0");
    require(nextTouchPhase(TouchPhase::Released, true) == TouchPhase::Pressed, "a touch after the release is new");
}

void create() {
    MapButtons buttons(setup(2));
    require(order(buttons) == "fastTravelOff charsClose ScanOff RedBricksOff FreeplayOff AudioOff ExitOff HintOff CharsOnDelay ", "create order: " + order(buttons));
    require(buttons.storyboards().back().time == 8.0f / 30.0f, "CharsOnDelay starts at frame 8");
    require(buttons.button(MapButton::Characters).disabled && buttons.button(MapButton::Audio).disabled, "characters and audio start disabled");
    require(buttons.button(MapButton::Characters).storyboard.name == "disabled", "the characters button plays disabled");
    require(buttons.button(MapButton::Area3).icon == "icon_FortMeadows" && buttons.button(MapButton::AreaExit).icon == "icon_AreaExit", "icons");
    MapButtonsSetup flagged = setup(3);
    flagged.scan = flagged.redBricks = true;
    MapButtons more(flagged);
    require(order(more).find("CharsOnDelay ScanOnDelay RedBricksOnDelay") != std::string::npos, "flagged buttons slide in: " + order(more));
}

void charactersEnable() {
    MapButtons buttons(setup(2));
    MapButtonsInput input = idle();
    input.canChangeCharacter = false;
    buttons.update(input);
    require(buttons.button(MapButton::Characters).disabled && buttons.button(MapButton::Freeplay).disabled, "0x20ac88 0 keeps them disabled");
    buttons.update(idle());
    require(!buttons.button(MapButton::Characters).disabled && buttons.button(MapButton::Characters).storyboard.name == "default", "enabled");
    require(!buttons.button(MapButton::Freeplay).disabled, "freeplay enabled with 0x27c298");
    input = idle();
    input.freeplayCharacter = true;
    const auto events = buttons.update(input);
    require(has(events, MapButtonsEventKind::FreeplayShown) && buttons.storyboards().back().name == "FreeplayOnDelay", "freeplay appears");
}

void hitAreas() {
    MapButtons buttons(setup(2));
    buttons.update(idle());
    require(buttons.hit(MapButton::Characters, touching(TouchPhase::Pressed, 32, 64)), "radius 32 around (32, 32)");
    require(!buttons.hit(MapButton::Characters, touching(TouchPhase::Pressed, 32, 64.5f)), "outside the radius");
    require(buttons.hit(MapButton::Characters, touching(TouchPhase::Held, 10, 10)), "a held touch hits");
    require(!buttons.hit(MapButton::Characters, touching(TouchPhase::Released, 32, 32)), "a release does not hit");
    require(buttons.update(touching(TouchPhase::Pressed, 159, 65)).empty(), "page 0 does not test area_exit");
    buttons.update(touching(TouchPhase::Released, 159, 65));
    buttons.openFastTravel(0);
    require(buttons.hit(MapButton::AreaExit, touching(TouchPhase::Pressed, 159, 97)), "area_exit: field 64 halved around (159, 65)");
    require(!buttons.hit(MapButton::AreaExit, touching(TouchPhase::Pressed, 159, 98)), "outside area_exit");
}

void charactersTapAndHold() {
    MapButtons buttons(setup(2));
    buttons.update(idle());
    auto events = buttons.update(touching(TouchPhase::Pressed, 32, 32));
    require(events.empty(), "a press does nothing yet");
    events = buttons.update(touching(TouchPhase::Released, 32, 32));
    require(has(events, MapButtonsEventKind::CharactersTapped) && has(events, MapButtonsEventKind::Sound, kMapSoundSelect), "a tap");
    require(buttons.button(MapButton::Characters).storyboard.name == "pressed", "the tap plays pressed");
    buttons.update(idle());

    events = buttons.update(touching(TouchPhase::Pressed, 32, 32));
    bool refused = false;
    for (int i = 0; i < 20; ++i) refused = refused || has(buttons.update(touching(TouchPhase::Held, 32, 32)), MapButtonsEventKind::Sound, kMapSoundRefused);
    require(refused, "a hold without a character to choose is refused");
    events = buttons.update(touching(TouchPhase::Released, 32, 32));
    require(!has(events, MapButtonsEventKind::CharactersTapped), "a long hold is no tap");
    buttons.update(idle());

    MapButtonsInput input = touching(TouchPhase::Pressed, 32, 32);
    input.charactersAvailable = true;
    buttons.update(input);
    input.phase = TouchPhase::Held;
    int opened = -1;
    for (int i = 1; i <= 20 && opened < 0; ++i)
        if (has(buttons.update(input), MapButtonsEventKind::CharactersOpened)) opened = i;
    require(opened == 16, "the portraits open once the hold (a float sum of dt) is greater than 0.5 s: update " + std::to_string(opened));
    require(buttons.charactersOpen() && buttons.button(MapButton::Characters).toggled, "toggled on");
    require(buttons.storyboards().back().name == "charsOpen", "charsOpen plays");
    input.phase = TouchPhase::Released;
    events = buttons.update(input);
    require(!has(events, MapButtonsEventKind::CharactersClosed), "the release of the hold keeps them open");
    for (int i = 0; i < 25; ++i) buttons.update(idle());
    buttons.update(touching(TouchPhase::Pressed, 32, 32));
    events = buttons.update(touching(TouchPhase::Released, 32, 32));
    require(has(events, MapButtonsEventKind::CharactersClosed) && !buttons.charactersOpen(), "a tap closes the portraits");
    require(buttons.storyboards().back().name == "charsClose", "charsClose; 0x217bec runs while +0x472 is still set, so CharsOnDelay is not restarted: " + order(buttons));
}

void firstUpdateOnly() {
    MapButtonsSetup s = setup(3);
    s.redBricks = true;
    MapButtons buttons(s);
    buttons.update(idle());
    auto events = buttons.update(touching(TouchPhase::Pressed, 288, 59));
    require(has(events, MapButtonsEventKind::RedBricks) && has(events, MapButtonsEventKind::Sound, kMapSoundBack), "red bricks on the first update");
    require(buttons.button(MapButton::RedBricks).storyboard.name == "toggleOff", "toggleOff plays");
    events = buttons.update(touching(TouchPhase::Held, 288, 59));
    require(!has(events, MapButtonsEventKind::RedBricks), "not again while held");
    buttons.update(touching(TouchPhase::Released, 288, 59));
    buttons.update(idle());
    buttons.update(touching(TouchPhase::Pressed, 100, 100));
    events = buttons.update(touching(TouchPhase::Held, 288, 59));
    require(!has(events, MapButtonsEventKind::RedBricks), "a touch that slides onto a button does not press it");
}

void fastTravelExit() {
    MapButtons buttons(setup(2));
    buttons.update(idle());
    buttons.openFastTravel(0);
    require(buttons.page() == 3, "the map opens on page 3 for a level of the table");
    require(order(buttons).find("fastTravelOn CharsOff") != std::string::npos, "fastTravelOn and CharsOff: " + order(buttons));
    for (std::size_t slot = 0; slot < kMapSlotCount; ++slot)
        require(buttons.slotLocked(slot) && buttons.button(MapButton(std::size_t(MapButton::Area0) + slot)).icon == kFastTravelLockedIcon, "locked");
    buttons.update(idle());
    require(buttons.page() == 1 && buttons.shownLevel() == 2, "page 1 shows the level's own map");
    require(buttons.button(MapButton::Characters).disabled, "the characters button is disabled on the map");
    MapButtonsInput input = touching(TouchPhase::Pressed, 159, 65);
    input.mapReady = false;
    auto events = buttons.update(input);
    require(!has(events, MapButtonsEventKind::FastTravelClosed) && buttons.page() == 1, "0x1ee5f8 waits for 0x28b930");
    buttons.update(touching(TouchPhase::Released, 159, 65));
    buttons.update(idle());
    events = buttons.update(touching(TouchPhase::Pressed, 159, 65));
    require(has(events, MapButtonsEventKind::FastTravelClosed) && buttons.page() == 0, "area_exit closes the map");
    require(order(buttons).find("fastTravelOff CharsOnDelay") != std::string::npos, "fastTravelOff, then the buttons slide in: " + order(buttons));
    require(buttons.storyboards().back().time == 0.0f, "from frame 0");
    require(!buttons.button(MapButton::Audio).disabled && buttons.button(MapButton::AreaExit).disabled, "0x1ee5f8 enables audio, disables area_exit");
}

void fastTravelSlots() {
    MapButtons buttons(setup(2));
    buttons.update(idle());
    buttons.openFastTravel((1u << 1) | (1u << 3) | (1u << 0));
    require(!buttons.slotLocked(0) && buttons.slotLevel(0) == 1 && !buttons.slotLocked(1) && buttons.slotLevel(1) == 3 && buttons.slotLocked(2),
            "the slots hold the other unlocked levels");
    require(buttons.button(MapButton::Area0).icon == "icon_AlbatrossIsland" && buttons.button(MapButton::Area1).icon == "icon_FortMeadows", "slot icons");
    buttons.update(idle());
    auto events = buttons.update(touching(TouchPhase::Pressed, 32, 176));
    require(has(events, MapButtonsEventKind::MapSwitch, 3), "a slot of another level switches the map");
    buttons.update(touching(TouchPhase::Released, 32, 176));
    require(buttons.page() == 1 && buttons.shownLevel() == 3, "the map of level 3");
    buttons.update(idle());
    events = buttons.update(touching(TouchPhase::Pressed, 32, 176));
    require(has(events, MapButtonsEventKind::FastTravelQuestion, 0) && buttons.page() == 2, "the shown level's slot asks the question");
    buttons.update(touching(TouchPhase::Released, 32, 176));
    MapButtonsInput input = idle();
    input.dialogResult = 2;
    input.mapReady = false;
    events = buttons.update(input);
    require(has(events, MapButtonsEventKind::FastTravelGo, 0) && has(events, MapButtonsEventKind::FastTravelClosed) && buttons.page() == 0, "Yes");

    MapButtons other(setup(2));
    other.update(idle());
    other.openFastTravel(1u << 1);
    other.update(idle());
    other.update(touching(TouchPhase::Pressed, 32, 176));
    other.update(touching(TouchPhase::Released, 32, 176));
    other.update(idle());
    events = other.update(touching(TouchPhase::Pressed, 159, 65));
    require(has(events, MapButtonsEventKind::MapSwitch, 2) && !has(events, MapButtonsEventKind::FastTravelClosed), "area_exit on another map goes back");
}

}

int main() {
    try {
        touchPhases();
        create();
        charactersEnable();
        hitAreas();
        charactersTapAndHold();
        firstUpdateOnly();
        fastTravelExit();
        fastTravelSlots();
    } catch (const std::exception& error) {
        std::cerr << "map-buttons: " << error.what() << '\n';
        return 1;
    }
    std::cout << "map-buttons: ok\n";
    return 0;
}
