#pragma once
// The Options page of the pause screen (page vtable 0x364888: enter 0x31aebc, leave request 0x31ae48, A 0x31ada4, selection changed
// 0x31ae14, B 0x31ad20, update 0x31af84; row texts 0x1e4804) and the live volume bytes of the options (docs/re/save-format.md,
// docs/re/ui-screens.md). The page edits the live options block 0x51b490 (the save file's options block) and the live volume bytes
// 0x37751d (sound effects) and 0x37751e (music). No SDL dependency.
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace chase::runtime {

constexpr int kOptionsRowCount = 4;
enum class OptionsRow { SoundVolume = 0, MusicVolume = 1, InvertCamera = 2, Language = 3 };

constexpr const char* kOptionsItemFolder = "Sprites/UI_StatusScreen/Options";
constexpr const char* kOptionsItemImages[kOptionsRowCount] = {"UI_Options_SFX.png", "UI_Options_Music.png", "UI_Options_InvertCamera.png",
                                                             "UI_Options_Language.png"};
// gametext keys: the subtitle (0x31aebc), the row labels and values (0x1e4804) and the Language row (0x31aebc).
constexpr std::uint32_t kOptionsSubtitleKey = 0xba538c9e;
constexpr std::uint32_t kOptionsSoundKey = 0x4a6848ca, kOptionsMusicKey = 0x3e17bd14, kOptionsInvertKey = 0x27bcbd1b;
constexpr std::uint32_t kOptionsYesKey = 0x2c187527, kOptionsNoKey = 0x004e1381, kOptionsLanguageKey = 0xf5479ed8;
constexpr int kOptionsSoundStep = 0x12, kOptionsSoundSelect = 0x14, kOptionsSoundBack = 0x11;
constexpr std::uint8_t kOptionsMaxVolume = 10;
constexpr std::uint8_t kInvertCameraBit = 2;
// The default volume bytes that the boot load 0x2c195c and the front end 0x2d5430 write (sound effects 10, music 8).
constexpr std::uint8_t kDefaultSoundVolume = 10, kDefaultMusicVolume = 8;

// Globals 0x37751d and 0x37751e.
struct OptionVolumes {
    std::uint8_t sound = kDefaultSoundVolume;
    std::uint8_t music = kDefaultMusicVolume;
};

// Options +2 as 0x1e4804 and the writers test it: the slot 0 or 1, empty for any other value (0xff = no slot used yet).
std::optional<std::size_t> optionsSlot(const std::uint8_t* options);

// The enter 0x303a40 of the loading-screen mode (vtable 0x3639d8): with a slot at options +2 the live bytes become options +4 + slot and
// +6 + slot; the gains are then set from the live bytes in either case.
void loadingScreenVolumes(const std::uint8_t* options, OptionVolumes& live);

// The pause update 0x300378 on a D-pad Left or Right press: the sound effects byte is reloaded from options +4 + slot (when a slot is set)
// and the gain set from it again.
void restoreSoundVolume(const std::uint8_t* options, OptionVolumes& live);

inline float gainOf(std::uint8_t volume) { return float(volume) / 10.0f; }

// The text of each row (0x1e4804): "label: value" with ": " (" : " for French, language 2); the volumes of the slot at options +2,
// or of the live bytes when no slot is set; YES or NO for options +3 bit 1. Row 3 is the Language label.
struct OptionsTexts {
    std::string sound, music, invertCamera, yes, no, language;
};
std::string optionsRowText(int row, const OptionsTexts& texts, const std::uint8_t* options, const OptionVolumes& live, int language);

enum class OptionsPageRequest { None, Page0, Languages, Options };

// What a page callback asks of the pause screen.
struct OptionsPageActions {
    std::vector<int> sounds;              // 0x22899c / 0x235684 sound ids
    std::optional<float> soundGain;       // 0x1f4408 -> 0x1dca5c, in call order the last
    std::optional<float> musicGain;       // 0x1f36ec
    OptionsPageRequest page = OptionsPageRequest::None;  // 0x217f88
    bool saveRequest = false;             // 0x2400a0(0x1c34a4, 0x26435c, 0)
    bool closeRoundMenu = false;          // 0x20eec4(1, 0)
    std::optional<bool> backShortcut, selectShortcut;  // 0x228ad0(0, on) and 0x228ad0(1, on)
    bool textsChanged = false;            // 0x1e4804 ran
};

// The page's own state: the saved selection (page +6) and the changed flag of the pause object (+0x350).
class OptionsPage {
public:
    // Enter 0x31aebc: the round menu opens at the saved selection, the texts are written, Back on.
    OptionsPageActions enter();
    // Selection changed 0x31ae14: Select is shown on rows 2 and 3 only.
    OptionsPageActions selectionChanged(int selected);
    // A 0x31ada4: the selection is saved; row 2 toggles options +3 bit 1, row 3 requests the Languages page.
    OptionsPageActions a(int selected, std::uint8_t* options);
    // B 0x31ad20: with a change a save request and the round menu closed (callback 0x26435c later requests page 0 and clears the flag),
    // else page 0 with the back sound; the saved selection becomes 0.
    OptionsPageActions b();
    // Callback 0x26435c of the save request.
    OptionsPageActions saveDone();
    // Callback 0x2b4c44 of the save request of the Languages page: the Options page is requested again and the changed flag cleared.
    OptionsPageActions languagesSaved();
    // Leave request 0x31ae48: the sound effects byte reloaded from the slot's option byte, the round menu closed, both shortcuts off.
    OptionsPageActions leaveRequest(const std::uint8_t* options, OptionVolumes& live);
    // Update 0x31af84: D-pad Down (slot 1) steps the volume of row 0 or 1 down to 0, Up (slot 0) up to 10, or toggles row 2. A volume step
    // writes options +4 / +6 + slot (when a slot is set), sets the gain and plays 0x12; a music step also writes its value into the sound
    // effects byte and sets the sound effects gain to it (the step sound is heard at the music volume).
    OptionsPageActions update(int selected, bool up, bool down, std::uint8_t* options, OptionVolumes& live);

    int savedSelection() const { return savedSelection_; }
    bool changed() const { return changed_; }

private:
    void step(int selected, int direction, std::uint8_t* options, OptionVolumes& live, OptionsPageActions& actions);

    int savedSelection_ = 0;
    bool changed_ = false;
};

}
