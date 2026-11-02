#pragma once
// Front end of the original game from power-on to the first level (docs/re/front-end.md): the screen order, the
// triggers and the page state machine of the prompts. The logic has no drawing and no SDL dependency; the movie
// decoder is supplied through MovieHost.
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace chase::frontend {

enum class Screen : std::uint8_t {
    Boot,
    Logos,
    FusionMovie,
    Language,
    Prompt,
    Autosave,
    SlotSelect,
    SavingGame,
    Loading,
    IntroMovie,
    MissionCard,
    Done,
};

// Page states of the prompt mode 0x5143cc (byte 0x372e73, update 0x313368). The page of each state is built by the
// switch at the end of that update.
enum class Prompt : std::uint8_t {
    None = 0,
    StreetPassQuestion = 1,  // gametext 0x4617a2ca, Yes (A) / No (B)
    AgreementMissing = 2,    // gametext 0x62ab3f48, Continue
    AgreementHelp = 3,       // gametext 0x62ab3f49, Continue / Back
    ParentalControls = 4,    // gametext 0x64301deb, Continue
    TooManyTitles = 5,       // gametext 0x71ba2807, Continue
    StreetPassOn = 6,        // gametext 0xe10321ee, Continue
    HomeNotice = 7,          // gametext 0x65213108, Continue
};

// Values the original reads from the 3DS system when the prompt mode starts (0x31315c): 0x329338 (+4 at 0x372e70),
// 0x3293b8 (+5) and the result of the activation call 0x1e59f8 (+6). STAND-IN: the oracle's values (StreetPass is
// available, not restricted, activation succeeds); a PC has no StreetPass.
struct StreetPassEnvironment {
    bool available = true;
    bool restricted = false;
    bool activationFails = false;
    // 0x372e6f stays 1 when the system already reports StreetPass as active for this title: the first page is then
    // StreetPassOn instead of the question.
    bool alreadyActive = false;
};

constexpr float kLogoSeconds = 4.0f + 0.5f;  // hold + fade in of row 1 (0x372eb4)
// Language list rows -> language index (picker 0x3077ac); index order 1 UK, 2 French, 3 Italian, 4 German,
// 5 Spanish, 6 Dutch, 7 Danish, 8 Portuguese, 9 Russian.
constexpr std::array<int, 9> kLanguageOfRow = {1, 2, 3, 4, 5, 7, 6, 9, 8};
constexpr std::array<std::uint32_t, 9> kLanguageRowKeys = {0x4f11523f, 0x4ee198b, 0x952b154f, 0x4723177, 0xf7468743,
                                                          0xf63a151e, 0x363c0017, 0x3f283c62, 0xeecdb206};
// STAND-IN: localisation table suffix per language index (the code that maps them was not read).
std::string localisationSuffix(int language);

constexpr int kSoundMove = 18;
constexpr int kSoundConfirm = 20;

constexpr std::uint32_t kButtonA = 0x1, kButtonB = 0x2, kButtonStart = 0x8, kButtonUp = 0x40, kButtonDown = 0x80,
                        kButtonX = 0x400, kButtonY = 0x800;

struct FrontInput {
    std::uint32_t pressed = 0;
    // A pointer's pick of a list row on the language and slot pages, applied before the pressed buttons; the pad never sets it.
    std::optional<unsigned> selectRow;
    FrontInput() = default;
    FrontInput(std::uint32_t pressedButtons) : pressed(pressedButtons) {}
};

class MovieHost {
public:
    virtual ~MovieHost() = default;
    virtual bool open(const std::string& name, const std::string& languageSuffix) = 0;
    virtual bool advance(double dt) = 0;
    virtual double frameSeconds() const = 0;
};

struct SlotSummary {
    bool used = false;
    std::string time, percent;
    std::uint32_t locationKey = 0;
};

struct FlowConfig {
    // 0x313018 sets the flag 0x372e6c from the sign of the options' last-slot byte: true while no slot was ever used.
    bool firstRun = true;
    int language = 1;
    StreetPassEnvironment streetPass;
    // Without a save file every slot is an empty "New Game" slot (STAND-IN FE-S3); `slots` holds the rows of a used save.
    unsigned slotCount = 2;
    std::vector<SlotSummary> slots;
};

struct Handoff {
    unsigned slot = 0;
    bool newGame = true;
};

class Flow {
public:
    // STAND-INS, measured in Azahar (docs/re/front-end.md): see flow.cpp for each.
    static constexpr unsigned kBlackUpdates = 3;

    Flow(MovieHost* movies, FlowConfig config);
    void start(Screen screen);
    void update(float dt, const FrontInput& input);

    Screen screen() const { return screen_; }
    Prompt prompt() const { return prompt_; }
    unsigned screenUpdates() const { return screenUpdates_; }
    float screenSeconds() const { return screenSeconds_; }
    float pageSeconds() const { return pageSeconds_; }
    unsigned selectedRow() const { return selectedRow_; }
    unsigned selectedSlot() const { return selectedSlot_; }
    float selectionSeconds() const { return selectionSeconds_; }
    float confirmSeconds() const { return confirmSeconds_; }
    bool confirming() const { return confirming_; }
    unsigned tick() const { return tick_; }
    int language() const { return language_; }
    const FlowConfig& config() const { return config_; }
    bool blackout() const { return blackUpdates_ > 0; }
    bool done() const { return screen_ == Screen::Done; }
    bool slotUsed(unsigned slot) const { return slot < config_.slots.size() && config_.slots[slot].used; }
    const Handoff& handoff() const { return handoff_; }
    double movieSeconds() const { return movieSeconds_; }
    bool movieShowing() const { return movieOpen_ && !blackout(); }
    std::vector<int> takeSounds();

private:
    void enter(Screen screen);
    void next(Screen screen, bool blackBefore);
    void showPrompt(Prompt prompt);
    void updatePrompt(const FrontInput& input);

    MovieHost* movies_;
    FlowConfig config_;
    Screen screen_ = Screen::Boot;
    Prompt prompt_ = Prompt::None;
    unsigned screenUpdates_ = 0;
    float screenSeconds_ = 0, pageSeconds_ = 0, selectionSeconds_ = 0, confirmSeconds_ = 0;
    unsigned tick_ = 0, blackUpdates_ = 0;
    unsigned selectedRow_ = 0, selectedSlot_ = 0;
    bool movieOpen_ = false, movieEnded_ = false, confirming_ = false;
    double movieSeconds_ = 0;
    int language_ = 1;
    Screen pending_ = Screen::Done;
    Handoff handoff_;
    std::vector<int> sounds_;
};

}
