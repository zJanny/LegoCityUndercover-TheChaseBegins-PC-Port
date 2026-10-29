#include "runtime/pad_events.hpp"

#include <iostream>
#include <sstream>

using namespace chase;

namespace {
int failures = 0;
void require(bool ok, const char* what) {
    if (ok) return;
    ++failures;
    std::cerr << "FAIL: " << what << "\n";
}

InputState inputFrom(const char* text) {
    std::istringstream config(text);
    return InputState(readBindings(config, [](const std::string&) { return 1; }));
}
}

int main() {
    InputState input = inputFrom("A pad leftshoulder hold\nB pad dpup toggle\nUp key a hold\n");

    const int shoulder = padButtonCode("leftshoulder");
    const int dpadUp = padButtonCode("dpup");
    require(shoulder == 9 && dpadUp == 11, "SDL game-controller button numbers (LEFTSHOULDER 9, DPAD_UP 11)");

    require(runtime::feedPadButton(input, shoulder, true), "a known button is accepted");
    require(input.active(Action::A), "a bound pad press holds its action");
    require(runtime::feedPadButton(input, shoulder, false), "release accepted");
    require(!input.active(Action::A), "release clears the action");

    runtime::feedPadButton(input, dpadUp, true);
    runtime::feedPadButton(input, dpadUp, false);
    require(input.active(Action::B), "toggle binding latches on a pad press");

    require(!runtime::feedPadButton(input, -1, true), "negative button id rejected");
    require(!runtime::feedPadButton(input, 21, true), "id past touchpad rejected");
    require(runtime::feedPadButton(input, padButtonCode("touchpad"), true), "last known button accepted");

    InputState keyOnly = inputFrom("Up key a hold\n");
    runtime::feedPadButton(keyOnly, shoulder, true);
    require(!keyOnly.active(Action::A) && !keyOnly.active(Action::Up), "a pad press moves nothing without a pad binding");
    keyOnly.event(Source::Key, 1, true);
    require(keyOnly.active(Action::Up), "keyboard still works beside pad events");
    return failures == 0 ? 0 : 1;
}
