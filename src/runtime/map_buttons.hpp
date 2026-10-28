#pragma once
// The map buttons of the bottom screen (object 0x519e24, create 0x1bd424, update 0x2d9f84; layout sprites/blend/mapbuttons/mapbuttons.bxaml):
// the buttons scan, exit, characters, audio, red bricks, freeplay, hint, the six level slots and `area_exit` of the fast travel map, their
// click areas (0x1ee520), what a touch on each does, the storyboards that slide them in and out, and the page byte +0x49b (0 normal, 1 the
// fast travel map, 2 the "go there?" question, 3 a switch to another level's map). Rules and evidence: docs/re/ui-screens.md. No SDL dependency.
#include <array>
#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace chase::runtime {

// The touch phase byte +0x1d of the touch record 0x52a4d8, written once per update by 0x108fa0: a touch in an update after no touch is 1,
// a touch after a touch 2, no touch after a touch 3 (the last position is kept), else 0.
enum class TouchPhase : std::uint8_t { None = 0, Pressed = 1, Held = 2, Released = 3 };
TouchPhase nextTouchPhase(TouchPhase previous, bool touching);

// The button records of 0x1bd424 in the order of the object (0x2c bytes each from +0x28; the six slots from +0x15c, `area_exit` at +0x264).
enum class MapButton : std::uint8_t { Scan, Exit, Characters, Audio, RedBricks, Freeplay, Hint, Area0, Area1, Area2, Area3, Area4, Area5, AreaExit };
constexpr std::size_t kMapButtonCount = 14;
constexpr std::size_t kMapSlotCount = 6;
constexpr std::size_t kMapPortraitCount = 8;

struct MapButtonInfo {
    const char* element;   // the reference of mapbuttons.bxaml the button instance (sprites/blend/mapbuttons/button.bxaml) is attached to
    const char* icon;      // the `icon` image of the instance (Sprites/blend/mapButtons/Images/<icon>.png, 0x1ede80)
    float radiusField;     // record +0x24: 0 for the buttons (the element's width is used), 64.0 for the slots and `area_exit` (0x1bd424)
    bool originOffset;     // the instance root is moved by (32, 32) inside the 128 x 128 reference (0x217200 in 0x1bd424)
};
// The slots' icons are those of the level table 0x375cd0 entries 0..5 at the create; 0x1ccf60 replaces them when the map opens.
const std::array<MapButtonInfo, kMapButtonCount>& mapButtonInfo();

constexpr const char* kMapButtonsLayout = "sprites/blend/mapbuttons/mapbuttons.bxaml";
constexpr const char* kMapButtonLayout = "sprites/blend/mapbuttons/button.bxaml";
constexpr const char* kMapPortraitLayout = "sprites/blend/mapbuttons/portrait.bxaml";
constexpr float kMapButtonOriginOffset = 32.0f;
constexpr float kMapStoryboardFrameRate = 30.0f;  // a start frame of 0x245b40: 8 skips the 0.267 s delay of the *OnDelay storyboards
constexpr float kMapCharactersHold = 0.5f;        // 0x3f000000: the characters button opens the portraits after a hold of more than 0.5 s
constexpr std::uint8_t kMapSoundSelect = 0x14;    // 0x235684(*0x375f44, id): pressed and toggled on
constexpr std::uint8_t kMapSoundBack = 0x11;      // toggled off
constexpr std::uint8_t kMapSoundRefused = 0x13;   // the characters hold without a character to choose

// The fast travel level table at 0x375afc + level x 0x34: +0 the map's level, +0x14 the `scenechange` object's name. Entries 2..7 (from
// 0x375b64) are the six levels of the fast travel map, the slot icons come from 0x375cd0 in the same order.
struct FastTravelLevel {
    std::int8_t level;
    const char* sceneChange;
    const char* icon;
};
const std::array<FastTravelLevel, 7>& fastTravelLevels();
constexpr int kFastTravelFirstLevel = 2;
constexpr const char* kFastTravelLockedIcon = "icon_Locked";

// The element box the hit test uses: position (node +0xf0) and size (+0x54, +0x58) of the reference.
struct MapButtonBox {
    float x = 0, y = 0, width = 0, height = 0;
};

struct MapButtonsSetup {
    std::array<MapButtonBox, kMapButtonCount> boxes{};
    std::map<std::string, float> storyboardLengths;  // seconds, by name, of the mapbuttons.bxaml storyboards
    // The flags 0x1bd424 sets: +0x476 scan (the level index is greater than 2, or greater than 1 with a count above 2 at 0x203464(2) +0x1c),
    // +0x477 red bricks (0x21456c answers for one of 0..0x14), +0x478 freeplay (0x21793c answers for one of 10..0x8c); +0x454 hint (not set here).
    bool scan = false, redBricks = false, freeplay = false, hint = false;
    std::int8_t level = 0;  // 0x51640e
};

struct MapButtonsInput {
    float dt = 0;
    TouchPhase phase = TouchPhase::None;
    float x = 0, y = 0;
    // STAND-IN UI-S8: one flag for the early returns of 0x2d9f84 (0x1ee790, 0x21d744, 0x236008, +0x498, 0x236104, *0x375fc4 +0x24).
    bool blocked = false;
    bool canChangeCharacter = false;   // 0x20ac88
    bool characterSwitchReady = false; // 0x27c298(player, 1)
    bool freeplayCharacter = false;    // a character outside the story list 0x372e08 is unlocked (0x21793c over 9..0x8c)
    bool scannerActive = false;        // 0x246128
    bool scannerReady = false;         // 0x1ee4f0: byte 0x375eb6 set and 0x236104 answers 0
    bool charactersAvailable = false;  // byte 0x51b24d
    bool hintAvailable = false;        // 0x37470c set, +0x42c 0 and +0x430 not 0
    bool mapReady = true;              // 0x28b930
    int dialogResult = 0;              // the question's result (*0x375f00 +0x28): 1 no, 2 yes
    int character = 0;                 // 0x21d340(player): a change closes the portraits
};

enum class MapButtonsEventKind {
    Sound,                // value: the sound id
    Scan,                 // the scan button entered the scanner (0x2249e0(1, 0), sound event 0x5190bc)
    ScannerExit,          // the exit button in the scanner (0x2249e0(1, 0))
    Audio,                // value 1 on, 0 off (0x1bf26c)
    RedBricks,            // 0x2131b4(4, 1): the red brick page
    Freeplay,             // 0x2131b4(1): the freeplay character page
    Hint,                 // 0x21d0f0(0, map): the hint text
    HintUnavailable,      // +0x45c = 1
    CharactersTapped,     // +0x470 = 1: a short tap on the characters button
    CharactersOpened,     // the portraits open (charsOpen)
    CharactersClosed,     // the portraits close (charsClose)
    FreeplayShown,        // +0x478 became set in the update: FreeplayOnDelay and the notice text 0x4dbc477b
    FastTravelClosed,     // 0x1ee5f8 ran: the normal buttons are back, 0x26aa60 (the pad's exit) is due
    FastTravelQuestion,   // value: slot; the "go there?" question opens (mode 2) for the slot's `scenechange` object
    FastTravelGo,         // value: slot; Yes: 0x244f40(object, player), then 0x1ee5f8(map, 1)
    MapSwitch,            // value: the level whose map the page switches to (mode 3)
};

struct MapButtonsEvent {
    MapButtonsEventKind kind = MapButtonsEventKind::Sound;
    int value = 0;
};

// A storyboard started on the layout or on one button instance; `time` is the time since its start plus the start frame.
struct MapStoryboard {
    std::string name;
    float time = 0;
};

struct MapButtonState {
    bool disabled = false;  // record +0x28 bit 0
    bool toggled = false;   // record +0x28 bit 1
    MapStoryboard storyboard;  // the last storyboard started on the instance (pressed and toggleOn / toggleOff animate the same tracks)
    std::string icon;
};

class MapButtons {
public:
    explicit MapButtons(const MapButtonsSetup& setup);

    std::vector<MapButtonsEvent> update(const MapButtonsInput& input);
    // The element boxes as the storyboards leave them (the hit test reads the node's position, which the slide storyboards move).
    void setBoxes(const std::array<MapButtonBox, kMapButtonCount>& boxes) { setup_.boxes = boxes; }

    // 0x1ccf60 (from the pad's use query 0x16c188): the normal buttons disabled and slid out, the six slots and `area_exit` enabled,
    // fastTravelOn; the slots hold the other levels whose bit is set in `unlocked` (0x5208ae, bit level - 2), the rest are locked.
    void openFastTravel(std::uint32_t unlocked);
    // 0x1ee5f8(map, forced): nothing while `forced` is false and 0x28b930 answers 0; otherwise the normal buttons back, the slots and
    // `area_exit` disabled, fastTravelOff, the buttons slid in again and page 0. True when it ran.
    bool closeFastTravel(bool forced, bool mapReady, std::vector<MapButtonsEvent>& events);

    std::uint8_t page() const { return page_; }
    std::int8_t shownLevel() const { return shownLevel_; }
    const MapButtonState& button(MapButton which) const { return buttons_[std::size_t(which)]; }
    const std::vector<MapStoryboard>& storyboards() const { return storyboards_; }
    bool slotLocked(std::size_t slot) const { return slotLocked_[slot]; }
    int slotLevel(std::size_t slot) const { return slotLevel_[slot]; }
    bool charactersOpen() const { return charactersOpen_; }
    bool hit(MapButton which, const MapButtonsInput& input) const;

private:
    void play(const std::string& name, float startFrame = 0);
    void playEnd(const std::string& name);
    bool ended(const std::string& name) const;
    void setDisabled(MapButton which, bool disabled);
    void toggle(MapButton which, bool on, std::vector<MapButtonsEvent>& events);
    void slideIn(bool fromStart);
    void closeCharacters(std::vector<MapButtonsEvent>& events);
    MapButtonState& state(MapButton which) { return buttons_[std::size_t(which)]; }

    MapButtonsSetup setup_;
    std::array<MapButtonState, kMapButtonCount> buttons_{};
    std::vector<MapStoryboard> storyboards_;
    std::array<bool, kMapSlotCount> slotLocked_{};
    std::array<int, kMapSlotCount> slotLevel_{};
    std::uint8_t page_ = 0;          // +0x49b
    std::int8_t shownLevel_ = -1;    // +0x49c
    std::int8_t targetLevel_ = -1;   // +0x49d
    float hold_ = 0;                 // +0x460
    float closeTimer_ = 0;           // +0x464
    int pressed_ = -1;               // +0x290
    int questionSlot_ = -1;          // the object +0x4e0 of the question
    int openedCharacter_ = 0;        // +0x46c
    bool charactersOpen_ = false;    // +0x472
};

}
