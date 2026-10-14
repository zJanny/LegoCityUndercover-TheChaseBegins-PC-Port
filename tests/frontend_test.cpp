// Flow state machine of the front end with a fake movie host. Reference values come from the oracle (Azahar,
// docs/re/front-end.md) and from the decompiled prompt mode 0x313368.
#include "frontend/flow.hpp"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {
using namespace chase::frontend;

int failures = 0;

void require(bool condition, const std::string& what) {
    if (!condition) {
        std::cerr << "FAIL: " << what << '\n';
        ++failures;
    }
}

template <typename T, typename U> void requireEqual(const T& actual, const U& expected, const std::string& what) {
    if (!(actual == expected)) {
        std::cerr << "FAIL: " << what << ": got " << actual << ", expected " << expected << '\n';
        ++failures;
    }
}

class FakeMovies : public MovieHost {
public:
    FakeMovies(double length) : length_(length) {}
    bool open(const std::string& name, const std::string& suffix) override {
        opened.push_back(name + ":" + suffix);
        clock_ = 0;
        return true;
    }
    bool advance(double dt) override {
        clock_ += dt;
        return clock_ < length_;
    }
    double frameSeconds() const override { return clock_; }
    std::vector<std::string> opened;

private:
    double length_, clock_ = 0;
};

constexpr float kDt = 1.0f / 30.0f;

void step(Flow& flow, std::uint32_t pressed = 0, float dt = kDt) {
    flow.update(dt, FrontInput{pressed});
}

unsigned stepsUntilScreen(Flow& flow, Screen wanted, unsigned limit, float dt = kDt) {
    unsigned steps = 0;
    while (flow.screen() != wanted && steps < limit) {
        step(flow, 0, dt);
        ++steps;
    }
    return steps;
}

void run(Flow& flow, unsigned updates) {
    for (unsigned i = 0; i < updates; ++i) step(flow);
}

// Gets to the language screen and past its black updates.
Flow& toLanguage(Flow& flow) {
    stepsUntilScreen(flow, Screen::Language, 4000);
    run(flow, Flow::kBlackUpdates + 1);
    return flow;
}

Flow& toPrompt(Flow& flow) {
    toLanguage(flow);
    step(flow, kButtonA);
    return flow;
}

void logoTiming() {
    FakeMovies movies(1);
    Flow flow(&movies, {});
    stepsUntilScreen(flow, Screen::Logos, 4000);
    requireEqual(int(flow.screen()), int(Screen::Logos), "logos reached");
    // Oracle: the logos last 66 updates at dt 0.07 (hold 4.0 + fade in 0.5, test before the clock advances; the test
    // is at 0x1e7bac), followed by 3 black updates before the movie.
    run(flow, Flow::kBlackUpdates);
    unsigned logoUpdates = 0;
    while (flow.screen() == Screen::Logos && logoUpdates < 1000) {
        step(flow, kButtonA | kButtonStart, 0.07f);
        ++logoUpdates;
    }
    requireEqual(logoUpdates, 66u, "logo updates at dt 0.07 (no button skips the logos)");
    requireEqual(int(flow.screen()), int(Screen::FusionMovie), "movie after logos");
    requireEqual(flow.blackout() ? 1 : 0, 1, "black updates after logos");
    requireEqual(movies.opened.size(), std::size_t(1), "movie opened at entering");
    requireEqual(movies.opened[0], std::string("splash5:uk"), "movie name and suffix");
}

void movieSkip() {
    {
        FakeMovies movies(6.36);
        Flow flow(&movies, {});
        stepsUntilScreen(flow, Screen::FusionMovie, 4000);
        run(flow, Flow::kBlackUpdates);
        step(flow, kButtonA);
        requireEqual(int(flow.screen()), int(Screen::FusionMovie), "first run: A does not skip the movie");
        stepsUntilScreen(flow, Screen::Language, 400);
        requireEqual(int(flow.screen()), int(Screen::Language), "first run: language screen after the movie");
    }
    {
        FakeMovies movies(6.36);
        FlowConfig config;
        config.firstRun = false;
        Flow flow(&movies, config);
        stepsUntilScreen(flow, Screen::FusionMovie, 4000);
        run(flow, Flow::kBlackUpdates);
        step(flow, kButtonStart);
        requireEqual(int(flow.screen()), int(Screen::Prompt), "returning player: any of A/B/X/Y/Start skips the movie and the language screen");
    }
}

void language() {
    FakeMovies movies(0.1);
    Flow flow(&movies, {});
    toLanguage(flow);
    step(flow, kButtonUp);
    requireEqual(flow.selectedRow(), 0u, "Up at the first row stays");
    step(flow, kButtonDown);
    step(flow, kButtonDown);
    requireEqual(flow.selectedRow(), 2u, "Down moves the row");
    for (int i = 0; i < 20; ++i) step(flow, kButtonDown);
    requireEqual(flow.selectedRow(), 8u, "Down stops at the last row (9 languages)");
    const std::vector<int> sounds = flow.takeSounds();
    requireEqual(sounds.size(), std::size_t(2 + 6), "move sounds: 2 + 6 effective moves");
    require(!sounds.empty() && sounds[0] == kSoundMove, "move sound id 18");
    step(flow, kButtonA);
    requireEqual(flow.language(), 8, "row 8 is language 8 (Portuguese) by the picker table");
    requireEqual(int(flow.screen()), int(Screen::Prompt), "prompt after the language");
    const std::vector<int> confirm = flow.takeSounds();
    require(confirm.size() == 1 && confirm[0] == kSoundConfirm, "confirm sound id 20");
    // Rows -> language index, from the picker 0x3077ac.
    const int expected[9] = {1, 2, 3, 4, 5, 7, 6, 9, 8};
    for (std::size_t i = 0; i < 9; ++i) requireEqual(kLanguageOfRow[i], expected[i], "language of row " + std::to_string(i));
}

void promptStateMachine() {
    // Default environment: the oracle's (StreetPass available).
    {
        FakeMovies movies(0.1);
        Flow flow(&movies, {});
        toPrompt(flow);
        requireEqual(int(flow.prompt()), int(Prompt::StreetPassQuestion), "first page is the StreetPass question");
        step(flow, kButtonB);
        requireEqual(int(flow.prompt()), int(Prompt::HomeNotice), "No leads to the HOME notice");
        step(flow, kButtonB);
        requireEqual(int(flow.prompt()), int(Prompt::HomeNotice), "B does nothing on the single-button HOME notice (oracle)");
        step(flow, kButtonA);
        requireEqual(int(flow.screen()), int(Screen::Autosave), "Continue leads to the autosave notice");
    }
    {
        FakeMovies movies(0.1);
        Flow flow(&movies, {});
        toPrompt(flow);
        step(flow, kButtonA);
        requireEqual(int(flow.prompt()), int(Prompt::StreetPassOn), "Yes with StreetPass available shows page 6 (oracle)");
        step(flow, kButtonA);
        requireEqual(int(flow.prompt()), int(Prompt::HomeNotice), "page 6 leads to the HOME notice");
    }
    const auto yesWith = [](StreetPassEnvironment environment) {
        FakeMovies movies(0.1);
        FlowConfig config;
        config.streetPass = environment;
        Flow flow(&movies, config);
        toPrompt(flow);
        step(flow, kButtonA);
        return flow.prompt();
    };
    StreetPassEnvironment unavailable;
    unavailable.available = false;
    requireEqual(int(yesWith(unavailable)), int(Prompt::AgreementMissing), "Yes without StreetPass: agreement page (state 2)");
    StreetPassEnvironment restricted;
    restricted.restricted = true;
    requireEqual(int(yesWith(restricted)), int(Prompt::ParentalControls), "Yes when restricted: parental controls (state 4)");
    StreetPassEnvironment tooMany;
    tooMany.activationFails = true;
    requireEqual(int(yesWith(tooMany)), int(Prompt::TooManyTitles), "Yes when activation fails: state 5");

    FakeMovies movies(0.1);
    FlowConfig config;
    config.streetPass = unavailable;
    Flow flow(&movies, config);
    toPrompt(flow);
    step(flow, kButtonA);
    step(flow, kButtonA);
    requireEqual(int(flow.prompt()), int(Prompt::AgreementHelp), "state 2 leads to state 3");
    step(flow, kButtonB);
    requireEqual(int(flow.prompt()), int(Prompt::AgreementMissing), "Back on state 3 returns to state 2");
    step(flow, kButtonA);
    step(flow, kButtonA);
    requireEqual(int(flow.prompt()), int(Prompt::HomeNotice), "Continue on state 3 leads to the HOME notice");
}

void slotsAndHandoff() {
    FakeMovies movies(0.2);
    Flow flow(&movies, {});
    toPrompt(flow);
    step(flow, kButtonB);
    step(flow, kButtonA);
    requireEqual(int(flow.screen()), int(Screen::Autosave), "autosave notice");
    step(flow, kButtonB);
    requireEqual(int(flow.screen()), int(Screen::Autosave), "B does nothing on the autosave notice");
    step(flow, kButtonA);
    requireEqual(flow.confirming() ? 1 : 0, 1, "OK starts turning the save indicator off");
    stepsUntilScreen(flow, Screen::SlotSelect, 100);
    requireEqual(int(flow.screen()), int(Screen::SlotSelect), "slot list after the indicator is off");
    step(flow, kButtonDown);
    requireEqual(flow.selectedSlot(), 1u, "Down selects slot 2 (oracle)");
    step(flow, kButtonDown);
    requireEqual(flow.selectedSlot(), 1u, "two slots");
    step(flow, kButtonUp);
    requireEqual(flow.selectedSlot(), 0u, "Up selects slot 1");
    step(flow, kButtonB);
    requireEqual(int(flow.screen()), int(Screen::SlotSelect), "B does nothing on the slot list (oracle)");
    step(flow, kButtonA);
    requireEqual(flow.confirming() ? 1 : 0, 1, "A confirms the slot");
    stepsUntilScreen(flow, Screen::SavingGame, 100);
    requireEqual(flow.handoff().slot, 0u, "chosen slot");
    requireEqual(flow.handoff().newGame ? 1 : 0, 1, "an empty slot starts a new game");
    stepsUntilScreen(flow, Screen::Loading, 1000);
    stepsUntilScreen(flow, Screen::IntroMovie, 1000);
    requireEqual(movies.opened.back(), std::string("01_gameintro:uk"), "intro movie");
    stepsUntilScreen(flow, Screen::MissionCard, 1000);
    requireEqual(int(flow.screen()), int(Screen::MissionCard), "mission card after the movie");
    run(flow, Flow::kBlackUpdates);
    step(flow, kButtonA);
    requireEqual(flow.done() ? 1 : 0, 1, "A on the mission card hands over to the level");
}

void continueUsedSlot() {
    FakeMovies movies(0.2);
    FlowConfig config;
    config.slots = {{true, "05:48", "0.2%", 1}, {false, "", "", 0}};
    Flow flow(&movies, config);
    toPrompt(flow);
    step(flow, kButtonB);
    step(flow, kButtonA);
    step(flow, kButtonA);
    stepsUntilScreen(flow, Screen::SlotSelect, 100);
    step(flow, kButtonA);
    stepsUntilScreen(flow, Screen::Loading, 100);
    requireEqual(flow.handoff().slot, 0u, "chosen used slot");
    requireEqual(flow.handoff().newGame ? 1 : 0, 0, "a used slot continues (oracle state savecont-slotlist-user: no saving page, loading screen at once)");
    stepsUntilScreen(flow, Screen::Done, 1000);
    bool introOpened = false;
    for (const std::string& name : movies.opened) introOpened = introOpened || name == "01_gameintro:uk";
    requireEqual(introOpened ? 1 : 0, 0, "Continue plays no intro movie");

    FakeMovies movies2(0.2);
    Flow second(&movies2, config);
    toPrompt(second);
    step(second, kButtonB);
    step(second, kButtonA);
    step(second, kButtonA);
    stepsUntilScreen(second, Screen::SlotSelect, 100);
    step(second, kButtonDown);
    step(second, kButtonA);
    stepsUntilScreen(second, Screen::SavingGame, 100);
    requireEqual(second.handoff().newGame ? 1 : 0, 1, "an empty slot next to a used one starts a new game");
}

}

int main() {
    logoTiming();
    movieSkip();
    language();
    promptStateMachine();
    slotsAndHandoff();
    continueUsedSlot();
    if (failures) {
        std::cerr << failures << " failure(s)\n";
        return EXIT_FAILURE;
    }
    std::cout << "frontend flow ok\n";
    return EXIT_SUCCESS;
}
