// Vehicle bindings of the PC controls and the add-missing-keys rule (QOLCTL-7, QOLCTL-3, QOLCTL-10). Spec: docs/design/qol-controls.md entry
// 2026-11-01T13:50:47 (in a vehicle W or Up accelerate (A), S or Down brake and reverse (B), A and D steer, Space brake (B), H horn (Y),
// F leave (X), hand brake and siren unbound; Q2 the bindings depend on whether the player sits in a vehicle) and the user's answer of
// 2026-11-02 on the same entry ("Add missing keys"). The keys the spec does not list (camera, Select, Start, Walk, debug overlay, full
// screen kept; E, Return, left mouse and R dropped) are stand-in QOLCTL-S4. Cases written by 20261102-qolctl2-A on 2026-11-02.
#include "input.hpp"
#include "qol/active_settings.hpp"
#include "qol/controls_file.hpp"
#include "qol/controls_merge.hpp"
#include "qol/pc_controls.hpp"
#include "qol/pc_vehicle_controls.hpp"
#include "runtime/qol_controls_bindings.hpp"
#include "runtime/qol_pc_controls.hpp"

#include <SDL.h>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

namespace {

int failures = 0;

void require(bool ok, const std::string& what) {
    if (ok) return;
    std::cerr << "FAIL: " << what << '\n';
    ++failures;
}

template <class F>
bool throws(F&& call) {
    try {
        call();
    } catch (const std::exception&) {
        return true;
    }
    return false;
}

const char* const faithfulText = "Up key W hold\nA key Space hold\nA mouse Left hold\nWalk key Left_Alt hold\n";

std::string slurp(const std::string& path) {
    std::ifstream file(path);
    return std::string(std::istreambuf_iterator<char>(file), {});
}

void spit(const std::string& path, const std::string& text) {
    std::ofstream(path, std::ios::binary) << text;
}

std::string freshFolder(const std::string& name) {
    auto folder = std::filesystem::temp_directory_path() / ("chase-qol-pc-vehicle-" + name);
    std::filesystem::remove_all(folder);
    std::filesystem::create_directories(folder);
    return folder.string();
}

void improved() {
    qol::Settings settings;
    settings.setMaster(qol::Master::Improved);
    qol::setActiveSettings(settings);
}

std::vector<std::string> lines(const std::vector<qol::BindingEntry>& entries) {
    std::vector<std::string> out;
    for (const auto& entry : entries) out.push_back(entry.action + " " + entry.source + " " + entry.code + " " + entry.mode);
    return out;
}

std::vector<qol::BindingEntry> readEntries(const std::string& path) {
    std::ifstream file(path);
    return qol::readBindingEntries(file, chase::runtime::sdlKeyResolver, chase::runtime::sdlKeyNamer);
}

bool has(const std::vector<std::string>& all, const std::string& line) {
    return std::find(all.begin(), all.end(), line) != all.end();
}

void originalModeFirst() {
    qol::setActiveSettings(qol::Settings{});
    const std::string directory = freshFolder("original");
    const std::string controls = directory + "/controls.cfg";
    spit(controls, faithfulText);
    qol::SeatedControls seated;
    require(!seated.update(false, true).has_value(), "Original: sitting in a vehicle switches nothing");
    require(!seated.update(false, false).has_value(), "Original: leaving switches nothing");
    require(!seated.update(false, true).has_value(), "Original: sitting again switches nothing");
    require(chase::runtime::readActiveBindings(controls).size() == 4, "Original reads the faithful file on foot");
    require(!std::filesystem::exists(directory + "/controls.pc.cfg") && !std::filesystem::exists(qol::pcVehiclePathFor(controls)),
        "Original writes neither PC controls file");
    require(slurp(controls) == faithfulText, "controls.cfg untouched");
}

void vehicleSetMatchesTheSpec() {
    const std::vector<std::string> expected{
        "Left key A hold", "Left key Left hold", "Right key D hold", "Right key Right hold",
        "A key W hold", "A key Up hold", "B key S hold", "B key Down hold", "B key Space hold",
        "X key F hold", "Y key H hold",
        "L key Z hold", "L mouse Middle hold", "R key C hold", "R mouse Middle hold",
        "Start key Keypad_Enter hold", "Select key Tab hold",
        "Walk key Left_Ctrl hold", "Walk key Left_Alt hold",
        "DebugOverlay key F3 toggle", "Fullscreen key F11 hold"};
    const auto set = qol::pcVehicleDefaults();
    require(lines(set) == expected, "the vehicle set is the approved layout");
    std::vector<std::string> inputs;
    for (const auto& entry : set) {
        require(entry.code != "Escape", "Escape is not bound by the vehicle set");
        const std::string input = entry.source + " " + entry.code;
        if (input != "mouse Middle") require(!has(inputs, input), "no input twice: " + input);
        inputs.push_back(input);
    }
    std::istringstream stream(qol::formatBindingEntries(set, chase::runtime::sdlKeyResolver));
    require(qol::readBindingEntries(stream, chase::runtime::sdlKeyResolver, chase::runtime::sdlKeyNamer) == set,
        "the set is writable and reads back unchanged");
}

void inputStateOfTheVehicleSet() {
    const std::string directory = freshFolder("input");
    const std::string controls = directory + "/controls.cfg";
    spit(controls, faithfulText);
    improved();
    chase::InputState input(chase::runtime::readVehicleBindings(controls));
    using chase::Action;
    using chase::Source;
    const auto press = [&](Source source, int code) {
        input.reset();
        input.event(source, code, true);
    };
    const auto only = [&](Action expected) {
        for (std::size_t i = 0; i < chase::actionCount; ++i) {
            const Action action = static_cast<Action>(i);
            if (input.active(action) != (action == expected)) return false;
        }
        return true;
    };
    press(Source::Key, SDLK_w);
    require(only(Action::A), "W accelerates (A) and is not Up");
    press(Source::Key, SDLK_UP);
    require(only(Action::A), "Up accelerates (A)");
    press(Source::Key, SDLK_s);
    require(only(Action::B), "S brakes and reverses (B)");
    press(Source::Key, SDLK_DOWN);
    require(only(Action::B), "Down brakes and reverses (B)");
    press(Source::Key, SDLK_SPACE);
    require(only(Action::B), "Space brakes (B)");
    press(Source::Key, SDLK_a);
    require(only(Action::Left), "A steers left");
    press(Source::Key, SDLK_d);
    require(only(Action::Right), "D steers right");
    press(Source::Key, SDLK_LEFT);
    require(only(Action::Left), "Left steers left");
    press(Source::Key, SDLK_RIGHT);
    require(only(Action::Right), "Right steers right");
    press(Source::Key, SDLK_h);
    require(only(Action::Y), "H is the horn (Y)");
    press(Source::Key, SDLK_f);
    require(only(Action::X), "F leaves the vehicle (X)");
    press(Source::Key, SDLK_e);
    require(only(Action::Count), "E is unbound in a vehicle");
    press(Source::Key, SDLK_RETURN);
    require(only(Action::Count), "Return is unbound in a vehicle");
    press(Source::Mouse, SDL_BUTTON_LEFT);
    require(only(Action::Count), "left mouse is unbound in a vehicle");
    press(Source::Key, SDLK_r);
    require(only(Action::Count), "R is unbound in a vehicle");
    press(Source::Key, SDLK_q);
    require(only(Action::Count), "Q is unbound (no hand brake, no siren)");
    press(Source::Key, SDLK_F11);
    require(only(Action::Fullscreen), "F11 stays Fullscreen in a vehicle");
    press(Source::Key, SDLK_z);
    require(only(Action::L), "Z turns the camera left in a vehicle");
    press(Source::Key, SDLK_TAB);
    require(only(Action::Select), "Tab is Select in a vehicle");
    press(Source::Key, SDLK_KP_ENTER);
    require(only(Action::Start), "Keypad Enter is Start in a vehicle");
}

void seatedSwitch() {
    qol::SeatedControls seated;
    require(!seated.update(true, false).has_value(), "on foot with PC controls: no change from the start");
    const auto sat = seated.update(true, true);
    require(sat.has_value() && *sat, "sitting down switches to the vehicle set");
    require(!seated.update(true, true).has_value(), "staying seated switches nothing");
    const auto left = seated.update(true, false);
    require(left.has_value() && !*left, "leaving switches back to the on-foot set");
    require(!seated.update(true, false).has_value(), "standing switches nothing");
    seated.update(true, true);
    const auto off = seated.update(false, true);
    require(off.has_value() && !*off, "PC controls switched off while seated returns to the faithful file");
    require(!seated.update(false, true).has_value(), "and stays there");
    const auto again = seated.update(true, true);
    require(again.has_value() && *again, "PC controls switched on while seated takes the vehicle set");
}

void vehicleFileKeptPerMapping() {
    const std::string directory = freshFolder("file");
    const std::string controls = directory + "/controls.cfg";
    const std::string footPath = directory + "/controls.pc.cfg";
    const std::string vehiclePath = directory + "/controls.pc.vehicle.cfg";
    spit(controls, faithfulText);
    improved();
    const auto foot = chase::runtime::readActiveBindings(controls);
    require(std::filesystem::exists(footPath) && !std::filesystem::exists(vehiclePath), "on foot creates only controls.pc.cfg");
    const auto vehicle = chase::runtime::readVehicleBindings(controls);
    require(std::filesystem::exists(vehiclePath), "the vehicle file is created from the built-in vehicle set");
    require(readEntries(vehiclePath) == qol::pcVehicleDefaults(), "and holds the built-in vehicle set");
    require(vehicle.size() == qol::pcVehicleDefaults().size() && foot.size() == qol::pcControlsDefaults().size(), "both sets are read");
    require(slurp(controls) == faithfulText, "controls.cfg untouched");

    std::string text = slurp(vehiclePath);
    const std::string rebound = text.replace(text.find("B key Space hold"), 16, "B key Left_Shift hold");
    spit(vehiclePath, rebound);
    const std::string footBefore = slurp(footPath);
    chase::runtime::readActiveBindings(controls);
    require(slurp(footPath) == footBefore, "reading the on-foot set leaves the vehicle file alone");
    chase::InputState input(chase::runtime::readVehicleBindings(controls));
    input.event(chase::Source::Key, SDLK_LSHIFT, true);
    require(input.active(chase::Action::B), "a rebind of the vehicle file is used");
    require(slurp(vehiclePath) == rebound, "and kept");
    spit(vehiclePath, "A key W hold\nBogus line\n");
    require(throws([&] { chase::runtime::readVehicleBindings(controls); }), "a malformed vehicle file stops with an error");
    spit(vehiclePath, "");
    require(throws([&] { chase::runtime::readVehicleBindings(controls); }), "an empty vehicle file stops with an error");
}

void oldFormatFileGetsMissingKeys() {
    const std::string directory = freshFolder("merge");
    const std::string controls = directory + "/controls.cfg";
    const std::string path = directory + "/controls.pc.cfg";
    spit(controls, faithfulText);
    improved();
    const std::string old =
        "Up key F5 hold\nUp key Up hold\nDown key S hold\nLeft key A hold\nRight key D hold\n"
        "A key E hold\nB key Space hold\nX key F11 hold\nY mouse Left hold\nL key Z hold\nR key C hold\n"
        "Start key Keypad_Enter hold\nSelect key Tab hold\nWalk key Left_Alt hold\n";
    spit(path, old);
    const auto before = readEntries(path);
    const auto bindings = chase::runtime::readActiveBindings(controls);
    const auto after = readEntries(path);
    require(!bindings.empty(), "the merged file is read");
    for (const auto& entry : before) require(std::find(after.begin(), after.end(), entry) != after.end(), "kept: " + entry.action + " " + entry.code);
    const auto written = lines(after);
    require(has(written, "Up key F5 hold") && !has(written, "Up key W hold"), "the user's rebind of Up is kept, W is not taken back");
    require(has(written, "DebugOverlay key F3 toggle"), "the missing DebugOverlay gets F3");
    require(!has(written, "Fullscreen key F11 hold"), "Fullscreen does not take F11, which the user uses for X");
    require(has(written, "X key F11 hold"), "X keeps F11");
    require(has(written, "Walk key Left_Alt hold") && !has(written, "Walk key Left_Ctrl hold"),
        "an action the user has is left as it is (Walk is not given Left Ctrl back)");
    require(after.size() == before.size() + 1, "exactly one binding was added");
    const std::string once = slurp(path);
    chase::runtime::readActiveBindings(controls);
    require(slurp(path) == once, "a second start changes nothing");
    std::istringstream stream(once);
    require(qol::readBindingEntries(stream, chase::runtime::sdlKeyResolver, chase::runtime::sdlKeyNamer) == after, "the rewritten file reads back strictly");
    require(slurp(controls) == faithfulText, "controls.cfg untouched");
}

void keysOfOtherActionsAreNotTaken() {
    const std::string directory = freshFolder("taken");
    const std::string controls = directory + "/controls.cfg";
    const std::string path = directory + "/controls.pc.cfg";
    spit(controls, faithfulText);
    improved();
    spit(path, "Up key W hold\nA key E hold\nB key Space hold\nX key F hold\nY key Left_Ctrl hold\nL key Z hold\nR key C hold\n");
    chase::runtime::readActiveBindings(controls);
    const auto written = lines(readEntries(path));
    require(has(written, "Walk key Left_Alt hold") && !has(written, "Walk key Left_Ctrl hold"),
        "Walk is added with Left Alt only, Left Ctrl is the user's Y");
    require(has(written, "Fullscreen key F11 hold") && has(written, "DebugOverlay key F3 toggle"), "free keys are added");
    require(has(written, "Down key S hold") && has(written, "Down key Down hold") && has(written, "Left key A hold") && has(written, "Right key D hold"),
        "the actions the file lacks come back with their keys");
    require(has(written, "Start key Keypad_Enter hold") && has(written, "Select key Tab hold"), "Start and Select come back");
    require(has(written, "Y key Left_Ctrl hold") && !has(written, "Y mouse Left hold") && !has(written, "Y key R hold"),
        "an action the user has gets nothing back");
    int middle = 0;
    for (const auto& line : written) middle += line.find("mouse Middle") != std::string::npos ? 1 : 0;
    require(middle == 0, "L and R exist in the file, so the middle button is not added");

    spit(path, "Up key W hold\n");
    chase::runtime::readActiveBindings(controls);
    const auto fresh = lines(readEntries(path));
    require(has(fresh, "L key Z hold") && has(fresh, "L mouse Middle hold") && has(fresh, "R key C hold") && has(fresh, "R mouse Middle hold"),
        "two new actions that share a default input both get it");
    require(has(fresh, "Up key W hold") && !has(fresh, "Up key Up hold"), "the user's Up keeps only W");
}

void addMissingActionsRule() {
    const auto resolve = chase::runtime::sdlKeyResolver;
    const std::vector<qol::BindingEntry> defaults{
        {"Up", "key", "W", "hold"}, {"Up", "key", "Up", "hold"}, {"A", "key", "E", "hold"}, {"B", "pad", "a", "hold"}, {"B", "mouse", "Right", "hold"}};
    const std::vector<qol::BindingEntry> user{{"X", "key", "w", "hold"}, {"A", "mouse", "Right", "hold"}};
    const auto merged = qol::addMissingActions(user, defaults, resolve);
    require(lines(merged) == std::vector<std::string>{"X key w hold", "A mouse Right hold", "Up key Up hold", "B pad a hold"},
        "a key (any case) or mouse button the user uses is not taken; the others are added after the user's lines");
    require(qol::addMissingActions(defaults, defaults, resolve) == defaults, "a complete file stays as it is");
    require(qol::addMissingActions({}, defaults, resolve) == defaults, "an empty user list gets all defaults");
}

void rejections() {
    const std::string directory = freshFolder("reject");
    const std::string controls = directory + "/controls.cfg";
    const std::string path = directory + "/controls.pc.cfg";
    spit(controls, faithfulText);
    improved();
    spit(path, "Up key W hold\nUp key Nonsense_Key hold\n");
    require(throws([&] { chase::runtime::readActiveBindings(controls); }), "an unknown key in the file stops with an error, nothing is merged");
    require(slurp(path) == "Up key W hold\nUp key Nonsense_Key hold\n", "and the file is not rewritten");
    spit(path, "Up key W\n");
    require(throws([&] { chase::runtime::readActiveBindings(controls); }), "a short line stops with an error");
    spit(path, "Jump key W hold\n");
    require(throws([&] { chase::runtime::readActiveBindings(controls); }), "an unknown action stops with an error");
}

}

int main() {
    originalModeFirst();
    vehicleSetMatchesTheSpec();
    inputStateOfTheVehicleSet();
    seatedSwitch();
    vehicleFileKeptPerMapping();
    oldFormatFileGetsMissingKeys();
    keysOfOtherActionsAreNotTaken();
    addMissingActionsRule();
    rejections();
    if (failures != 0) {
        std::cerr << failures << " failure(s)\n";
        return 1;
    }
    return 0;
}
