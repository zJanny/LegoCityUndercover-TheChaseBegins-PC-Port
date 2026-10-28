#include "runtime/map_buttons.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace chase::runtime {

namespace {

constexpr float kStoryboardAtEnd = std::numeric_limits<float>::max();

void add(std::vector<MapButtonsEvent>& events, MapButtonsEventKind kind, int value = 0) { events.push_back({kind, value}); }

}

TouchPhase nextTouchPhase(TouchPhase previous, bool touching) {
    const bool wasDown = previous == TouchPhase::Pressed || previous == TouchPhase::Held;
    if (touching) return wasDown ? TouchPhase::Held : TouchPhase::Pressed;
    return wasDown ? TouchPhase::Released : TouchPhase::None;
}

const std::array<MapButtonInfo, kMapButtonCount>& mapButtonInfo() {
    static const std::array<MapButtonInfo, kMapButtonCount> info = {{
        {"buttonScan", "buttonIcon_scan", 0, false},
        {"buttonExit", "buttonIcon_exit", 0, false},
        {"buttonChars", "buttonIcon_chars", 0, false},
        {"buttonAudio", "buttonIcon_audio", 0, false},
        {"buttonRedBricks", "buttonIcon_redbricks", 0, false},
        {"buttonFreeplay", "buttonIcon_freeplay", 0, false},
        {"buttonHint", "buttonIcon_hint", 0, false},
        {"area_0", "icon_CherryTreeHills", 64, true},
        {"area_1", "icon_AlbatrossIsland", 64, true},
        {"area_2", "icon_Auburn", 64, true},
        {"area_3", "icon_FortMeadows", 64, true},
        {"area_4", "icon_ApolloIsland", 64, true},
        {"area_5", "icon_BluebellNatPark", 64, true},
        {"area_exit", "icon_AreaExit", 64, true},
    }};
    return info;
}

const std::array<FastTravelLevel, 7>& fastTravelLevels() {
    static const std::array<FastTravelLevel, 7> levels = {{
        {2, "FastTravel_01_CherryTreeHills", "icon_CherryTreeHills"},
        {3, "FastTravel_02_AlbatrossIsland", "icon_AlbatrossIsland"},
        {4, "FastTravel_03_Auburn", "icon_Auburn"},
        {5, "FastTravel_04_FortMeadows", "icon_FortMeadows"},
        {6, "FastTravel_06_ApolloIsland", "icon_ApolloIsland"},
        {7, "FastTravel_05_BluebellWoods", "icon_BluebellNatPark"},
        {8, "FastTravel_07_Downtown", "icon_Downtown"},
    }};
    return levels;
}

MapButtons::MapButtons(const MapButtonsSetup& setup) : setup_(setup) {
    for (std::size_t i = 0; i < kMapButtonCount; ++i) buttons_[i].icon = mapButtonInfo()[i].icon;
    setDisabled(MapButton::Audio, true);
    for (const char* name : {"fastTravelOff", "charsClose", "ScanOff", "RedBricksOff", "FreeplayOff", "AudioOff", "ExitOff", "HintOff"}) playEnd(name);
    slideIn(false);
    setDisabled(MapButton::Characters, true);
}

void MapButtons::play(const std::string& name, float startFrame) {
    storyboards_.erase(std::remove_if(storyboards_.begin(), storyboards_.end(), [&](const MapStoryboard& s) { return s.name == name; }),
                       storyboards_.end());
    storyboards_.push_back({name, startFrame / kMapStoryboardFrameRate});
}

void MapButtons::playEnd(const std::string& name) {
    play(name);
    storyboards_.back().time = kStoryboardAtEnd;
}

bool MapButtons::ended(const std::string& name) const {
    for (const MapStoryboard& playing : storyboards_) {
        if (playing.name != name) continue;
        const auto length = setup_.storyboardLengths.find(name);
        return length == setup_.storyboardLengths.end() || playing.time >= length->second;
    }
    return true;
}

void MapButtons::setDisabled(MapButton which, bool disabled) {
    MapButtonState& button = state(which);
    button.storyboard = {disabled ? "disabled" : "default", 0};
    button.disabled = disabled;
}

void MapButtons::toggle(MapButton which, bool on, std::vector<MapButtonsEvent>& events) {
    MapButtonState& button = state(which);
    button.storyboard = {on ? "toggleOn" : "toggleOff", 0};
    add(events, MapButtonsEventKind::Sound, on ? kMapSoundSelect : kMapSoundBack);
    button.toggled = on;
}

void MapButtons::slideIn(bool fromStart) {
    const float frame = fromStart ? 0.0f : 8.0f;
    if (!charactersOpen_) play("CharsOnDelay", frame);
    if (setup_.scan) play("ScanOnDelay", frame);
    if (setup_.redBricks) play("RedBricksOnDelay", frame);
    if (setup_.freeplay) play("FreeplayOnDelay", frame);
}

void MapButtons::closeCharacters(std::vector<MapButtonsEvent>& events) {
    toggle(MapButton::Characters, false, events);
    play("charsClose");
    slideIn(false);
    charactersOpen_ = false;
    add(events, MapButtonsEventKind::CharactersClosed);
}

bool MapButtons::hit(MapButton which, const MapButtonsInput& input) const {
    const MapButtonState& button = buttons_[std::size_t(which)];
    if (button.disabled) return false;
    if (input.phase != TouchPhase::Pressed && input.phase != TouchPhase::Held) return false;
    const MapButtonBox& box = setup_.boxes[std::size_t(which)];
    const float centreX = box.x + box.width * 0.5f, centreY = box.y + box.height * 0.5f;
    const float field = mapButtonInfo()[std::size_t(which)].radiusField;
    const float radius = (field == 0.0f ? box.width : field) * 0.5f;
    return std::hypot(input.x - centreX, input.y - centreY) <= radius;
}

void MapButtons::openFastTravel(std::uint32_t unlocked) {
    for (MapButton which : {MapButton::Scan, MapButton::Characters, MapButton::Exit, MapButton::Audio, MapButton::RedBricks, MapButton::Freeplay,
                            MapButton::Hint})
        setDisabled(which, true);
    for (std::size_t slot = 0; slot < kMapSlotCount; ++slot) setDisabled(MapButton(std::size_t(MapButton::Area0) + slot), false);
    setDisabled(MapButton::AreaExit, false);
    play("fastTravelOn");
    play("CharsOff");
    if (setup_.scan) play("ScanOff");
    if (setup_.redBricks) play("RedBricksOff");
    if (setup_.freeplay) play("FreeplayOff");
    const int index = setup_.level - kFastTravelFirstLevel;
    if (index >= 0 && index < 7) {
        targetLevel_ = setup_.level;
        page_ = 3;
    } else {
        shownLevel_ = -1;
        page_ = 1;
    }
    slotLocked_.fill(true);
    std::size_t filled = 0;
    for (std::size_t entry = 0; entry < fastTravelLevels().size() && filled < kMapSlotCount; ++entry) {
        const int level = fastTravelLevels()[entry].level;
        const int bit = level - kFastTravelFirstLevel;
        if (level == setup_.level || !(unlocked >> bit & 1u)) continue;
        slotLevel_[filled] = int(entry);
        slotLocked_[filled] = false;
        ++filled;
    }
    for (std::size_t slot = 0; slot < kMapSlotCount; ++slot)
        buttons_[std::size_t(MapButton::Area0) + slot].icon = slotLocked_[slot] ? kFastTravelLockedIcon : fastTravelLevels()[std::size_t(slotLevel_[slot])].icon;
}

bool MapButtons::closeFastTravel(bool forced, bool mapReady, std::vector<MapButtonsEvent>& events) {
    if (!forced && !mapReady) return false;
    for (MapButton which : {MapButton::Scan, MapButton::Characters, MapButton::RedBricks, MapButton::Freeplay, MapButton::Hint, MapButton::Exit,
                            MapButton::Audio})
        setDisabled(which, false);
    for (std::size_t slot = 0; slot < kMapSlotCount; ++slot) setDisabled(MapButton(std::size_t(MapButton::Area0) + slot), true);
    setDisabled(MapButton::AreaExit, true);
    play("fastTravelOff");
    slideIn(true);
    page_ = 0;
    add(events, MapButtonsEventKind::FastTravelClosed);
    return true;
}

std::vector<MapButtonsEvent> MapButtons::update(const MapButtonsInput& input) {
    std::vector<MapButtonsEvent> events;
    for (MapStoryboard& playing : storyboards_)
        if (playing.time != kStoryboardAtEnd) playing.time += input.dt;
    for (MapButtonState& button : buttons_) button.storyboard.time += input.dt;
    if (input.blocked) return events;
    if (closeTimer_ > 0) {
        closeTimer_ -= input.dt;
        if (closeTimer_ > 0) return events;
        closeCharacters(events);
        return events;
    }
    if (charactersOpen_ && input.character != openedCharacter_) closeCharacters(events);
    if (page_ == 2) {
        if (input.dialogResult == 1) {
            closeFastTravel(false, input.mapReady, events);
        } else if (input.dialogResult == 2) {
            add(events, MapButtonsEventKind::FastTravelGo, questionSlot_);
            closeFastTravel(true, input.mapReady, events);
            page_ = 0;
        }
    }
    if (page_ == 3) {
        // STAND-IN LEVOBJ-S102: the switch to another level's map (pages 3 and 4, the fade +0x4a0, +0x4a4) ends at once.
        shownLevel_ = targetLevel_;
        page_ = 1;
    }

    const bool charactersOff = !input.canChangeCharacter || page_ != 0;
    MapButtonState& characters = state(MapButton::Characters);
    if (characters.disabled != charactersOff && !charactersOff) setDisabled(MapButton::Characters, false);
    else characters.disabled = charactersOff;
    if (charactersOff || !input.characterSwitchReady) {
        state(MapButton::Freeplay).disabled = true;
    } else {
        setDisabled(MapButton::Freeplay, false);
        if (!setup_.freeplay && input.freeplayCharacter) {
            setup_.freeplay = true;
            play("FreeplayOnDelay");
            add(events, MapButtonsEventKind::FreeplayShown);
        }
    }

    int touched = -1;
    const auto test = [&](MapButton which, bool enabled) {
        if (touched < 0 && enabled && hit(which, input)) touched = int(which);
    };
    test(MapButton::Scan, setup_.scan);
    test(MapButton::Exit, true);
    test(MapButton::Characters, true);
    test(MapButton::Audio, true);
    test(MapButton::RedBricks, setup_.redBricks);
    test(MapButton::Freeplay, setup_.freeplay);
    test(MapButton::Hint, setup_.hint);
    if (touched < 0 && page_ != 0) {
        if (hit(MapButton::AreaExit, input)) {
            touched = int(MapButton::AreaExit);
        } else {
            for (std::size_t slot = 0; slot < kMapSlotCount; ++slot) {
                const MapButton area = MapButton(std::size_t(MapButton::Area0) + slot);
                if (hit(area, input)) touched = int(area);
            }
        }
    }

    const int charactersIndex = int(MapButton::Characters);
    bool act = false;
    if (ended("charsOpen") && touched >= 0) {
        if (hold_ == 0.0f) pressed_ = touched;
        else if (pressed_ != touched) pressed_ = -1;
        act = true;
    } else {
        touched = -1;
    }
    if (!act) {
        if (input.phase == TouchPhase::Released && hold_ != 0.0f && pressed_ == charactersIndex && hold_ < kMapCharactersHold && !characters.disabled) {
            characters.storyboard = {"pressed", 0};
            add(events, MapButtonsEventKind::Sound, kMapSoundSelect);
            if (!characters.toggled) add(events, MapButtonsEventKind::CharactersTapped);
            else closeCharacters(events);
        }
        // STAND-IN UI-S8: the close of page 1 when the table entry at *(0x3769e4 + 0x14) + [0x3769fc] x 0x14 + 0x10 is set (0x2da784) is not modelled.
        pressed_ = -1;
        hold_ = 0;
    }

    const auto first = [&](MapButton which) { return touched == int(which) && hold_ == 0.0f; };
    if (!characters.toggled) {
        if (first(MapButton::Scan)) {
            if (input.scannerReady) {
                toggle(MapButton::Scan, true, events);
                add(events, MapButtonsEventKind::Scan);
            }
        } else if (input.scannerActive && first(MapButton::Exit)) {
            if (input.scannerReady) {
                toggle(MapButton::Exit, true, events);
                add(events, MapButtonsEventKind::ScannerExit);
            }
        } else if (touched == charactersIndex && pressed_ == charactersIndex && hold_ > kMapCharactersHold && ended("charsClose") && ended("charsOpen")) {
            if (!input.charactersAvailable) {
                add(events, MapButtonsEventKind::Sound, kMapSoundRefused);
            } else {
                closeTimer_ = 0;
                toggle(MapButton::Characters, true, events);
                play("charsOpen");
                if (setup_.scan) play("ScanOff");
                if (setup_.redBricks) play("RedBricksOff");
                if (setup_.freeplay) play("FreeplayOff");
                openedCharacter_ = input.character;
                charactersOpen_ = true;
                add(events, MapButtonsEventKind::CharactersOpened);
            }
        }
        if (first(MapButton::RedBricks)) {
            toggle(MapButton::RedBricks, false, events);
            add(events, MapButtonsEventKind::RedBricks);
        }
        if (first(MapButton::Freeplay)) {
            toggle(MapButton::Freeplay, false, events);
            add(events, MapButtonsEventKind::Freeplay);
        }
        if (first(MapButton::Hint)) {
            toggle(MapButton::Hint, false, events);
            add(events, input.hintAvailable ? MapButtonsEventKind::Hint : MapButtonsEventKind::HintUnavailable);
        }
        if (first(MapButton::Audio)) {
            if (input.scannerReady) {
                const bool on = !state(MapButton::Audio).toggled;
                toggle(MapButton::Audio, on, events);
                add(events, MapButtonsEventKind::Audio, on ? 1 : 0);
            }
        } else if (page_ == 1) {
            if (first(MapButton::AreaExit)) {
                if (shownLevel_ == setup_.level) {
                    closeFastTravel(false, input.mapReady, events);
                } else {
                    targetLevel_ = setup_.level;
                    page_ = 3;
                    add(events, MapButtonsEventKind::MapSwitch, targetLevel_);
                }
            }
            for (std::size_t slot = 0; slot < kMapSlotCount; ++slot) {
                if (slotLocked_[slot] || !first(MapButton(std::size_t(MapButton::Area0) + slot))) continue;
                const FastTravelLevel& entry = fastTravelLevels()[std::size_t(slotLevel_[slot])];
                if (shownLevel_ == entry.level) {
                    questionSlot_ = int(slot);
                    page_ = 2;
                    add(events, MapButtonsEventKind::FastTravelQuestion, int(slot));
                } else {
                    targetLevel_ = entry.level;
                    page_ = 3;
                    add(events, MapButtonsEventKind::MapSwitch, targetLevel_);
                }
            }
        }
    }
    if (input.phase != TouchPhase::None) hold_ += input.dt;
    return events;
}

}
