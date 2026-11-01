// Escape in chase-game never quits: it is a fixed Start binding, so it does what the original does with Start in each state.
// Spec: docs/design/qol-controls.md entry 2026-11-01T13:50:44 (Escape never closes the game; in play it opens the pause menu through
// the path of Start; in movies, logos and the title it acts like the original skip where one exists and does nothing else; the rules
// apply in Original mode too). Skip rules of the front end: tests/frontend_test.cpp (movieSkip, logoTiming), docs/re/front-end.md.
// Cases written by 20261101-qolctl-B on 2026-11-01.
#include "frontend/flow.hpp"
#include "input.hpp"
#include "runtime/qol_escape.hpp"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

using namespace chase;
using namespace chase::frontend;

constexpr int kEscape = 27;
constexpr int kReturn = 13;
constexpr int kSpace = 32;

int failures = 0;

void require(bool ok, const std::string& what) {
    if (ok) return;
    std::cerr << "FAIL: " << what << '\n';
    ++failures;
}

int resolve(const std::string& name) { return name == "Return" ? kReturn : name == "Space" ? kSpace : 0; }

std::vector<Binding> fileBindings(const std::string& text = "Start key Return hold\nA key Space hold\n") {
    std::istringstream file(text);
    return readBindings(file, resolve);
}

void escapeIsStart() {
    InputState input(fileBindings());
    runtime::bindEscape(input, kEscape);
    require(!input.active(Action::Start), "nothing is pressed before the key");
    input.event(Source::Key, kEscape, true);
    require(input.active(Action::Start), "Escape held is Start held");
    require(!input.active(Action::A) && !input.active(Action::B) && !input.active(Action::Select),
        "Escape presses no other action");
    input.event(Source::Key, kEscape, false);
    require(!input.active(Action::Start), "releasing Escape releases Start");
    input.event(Source::Key, kReturn, true);
    require(input.active(Action::Start), "Return still presses Start");
    input.event(Source::Key, kEscape, true);
    input.event(Source::Key, kReturn, false);
    require(input.active(Action::Start), "one release keeps the other binding's Start");
}

void survivesTheControlsFile() {
    InputState input(fileBindings("A key Space hold\n"));
    runtime::bindEscape(input, kEscape);
    input.rebind(fileBindings("Start key Return hold\n"));
    input.event(Source::Key, kEscape, true);
    require(input.active(Action::Start), "Escape stays Start after the controls file is read again");
    input.rebind(fileBindings("A key Space hold\n"));
    require(input.active(Action::Start), "a held Escape keeps Start through a rebind");
    input.event(Source::Key, kEscape, false);
    input.event(Source::Key, kReturn, true);
    require(!input.active(Action::Start), "a file without Start leaves Return unbound but not Escape");
    input.event(Source::Key, kEscape, true);
    require(input.active(Action::Start), "Escape is Start even when the file binds no Start (Original mode default file)");
}

void focusLossClears() {
    InputState input(fileBindings());
    runtime::bindEscape(input, kEscape);
    input.event(Source::Key, kEscape, true);
    input.reset();
    require(!input.active(Action::Start), "focus loss clears a held Escape");
    input.event(Source::Key, kEscape, true, true);
    require(!input.active(Action::Start), "a key repeat is ignored");
}

class FakeMovies : public MovieHost {
public:
    bool open(const std::string&, const std::string&) override { clock_ = 0; return true; }
    bool advance(double dt) override { clock_ += dt; return clock_ < 6.36; }
    double frameSeconds() const override { return clock_; }

private:
    double clock_ = 0;
};

constexpr float kDt = 1.0f / 30.0f;

void toScreen(Flow& flow, Screen wanted) {
    for (unsigned i = 0; i < 4000 && flow.screen() != wanted; ++i) flow.update(kDt, FrontInput{0});
    for (unsigned i = 0; i < Flow::kBlackUpdates + 1; ++i) flow.update(kDt, FrontInput{0});
}

std::uint32_t pressedBy(InputState& input) {
    return input.active(Action::Start) ? kButtonStart : 0;
}

void frontEnd() {
    {
        FakeMovies movies;
        FlowConfig config;
        config.firstRun = false;
        Flow flow(&movies, config);
        InputState input(fileBindings());
        runtime::bindEscape(input, kEscape);
        toScreen(flow, Screen::FusionMovie);
        require(flow.screen() == Screen::FusionMovie, "the movie plays");
        input.event(Source::Key, kEscape, true);
        flow.update(kDt, FrontInput{pressedBy(input)});
        require(flow.screen() == Screen::Prompt, "returning player: Escape skips the movie like Start does");
    }
    {
        FakeMovies movies;
        Flow flow(&movies, {});
        InputState input(fileBindings());
        runtime::bindEscape(input, kEscape);
        toScreen(flow, Screen::FusionMovie);
        input.event(Source::Key, kEscape, true);
        flow.update(kDt, FrontInput{pressedBy(input)});
        require(flow.screen() == Screen::FusionMovie, "first run: Escape does not skip the movie (the original has no skip there)");
    }
    {
        FakeMovies movies;
        Flow flow(&movies, {});
        InputState input(fileBindings());
        runtime::bindEscape(input, kEscape);
        toScreen(flow, Screen::Logos);
        input.event(Source::Key, kEscape, true);
        for (int i = 0; i < 20; ++i) flow.update(0.07f, FrontInput{pressedBy(input)});
        require(flow.screen() == Screen::Logos, "Escape does not skip the logos (no button does)");
    }
    {
        FakeMovies movies;
        FlowConfig config;
        config.firstRun = false;
        Flow flow(&movies, config);
        InputState input(fileBindings());
        runtime::bindEscape(input, kEscape);
        toScreen(flow, Screen::Prompt);
        require(flow.prompt() == Prompt::StreetPassQuestion, "the first prompt page is the question");
        input.event(Source::Key, kEscape, true);
        flow.update(kDt, FrontInput{pressedBy(input)});
        require(flow.screen() == Screen::Prompt && flow.prompt() == Prompt::StreetPassQuestion,
            "Escape answers no question: the page stays");
    }
}

}

int main() {
    escapeIsStart();
    survivesTheControlsFile();
    focusLossClears();
    frontEnd();
    if (failures == 0) std::cout << "escape-binding: all checks passed\n";
    return failures == 0 ? 0 : 1;
}
