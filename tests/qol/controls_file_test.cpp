// Spec: docs/design/qol-settings.md entry 2026-10-28T09:32:07 (key bindings stay in config/controls.cfg) and 2026-10-29T11:06:51 (Walk and DebugOverlay listed with the 3DS actions).
// Line format and action names: docs/controls.md and src/input.cpp (readBindings), recorded 20261029-qolview-C.
// The key table below is a stand-in for the platform resolver (SDL names); only the names used by config/controls.cfg are included.
#include "qol/controls_file.hpp"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>

#include "input.hpp"

namespace {

int failures = 0;

void require(bool ok, const std::string& what) {
    if (ok) return;
    std::cerr << "FAIL: " << what << '\n';
    ++failures;
}

const std::map<std::string, int> keyCodes{{"W", 1}, {"S", 2}, {"A", 3}, {"D", 4}, {"Up", 5}, {"Down", 6}, {"Left", 7}, {"Right", 8},
    {"Space", 9}, {"Q", 10}, {"E", 11}, {"R", 12}, {"Z", 13}, {"C", 14}, {"Return", 15}, {"Tab", 16}, {"Left Alt", 17}, {"F3", 18}, {"#", 19}};

int resolve(const std::string& name) {
    auto found = keyCodes.find(name);
    return found == keyCodes.end() ? 0 : found->second;
}

std::string nameOf(int code) {
    for (const auto& [name, value] : keyCodes)
        if (value == code) return name;
    return {};
}

const char* const sample =
    "# comment\n"
    "Up key W hold\n"
    "Up key Up hold\n"
    "A key Space hold\n"
    "A mouse Left hold\n"
    "B mouse Right toggle\n"
    "Walk key Left_Alt hold\n"
    "DebugOverlay key F3 toggle\n";

using qol::BindingEntry;

std::vector<BindingEntry> sampleEntries() {
    return {{"Up", "key", "W", "hold"}, {"Up", "key", "Up", "hold"}, {"A", "key", "Space", "hold"}, {"A", "mouse", "Left", "hold"},
        {"B", "mouse", "Right", "toggle"}, {"Walk", "key", "Left_Alt", "hold"}, {"DebugOverlay", "key", "F3", "toggle"}};
}

std::vector<BindingEntry> parse(const std::string& text) {
    std::istringstream stream(text);
    return qol::readBindingEntries(stream, resolve, nameOf);
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

void readsEntriesFromTheFile() {
    require(parse(sample) == sampleEntries(), "entries read with underscored key names and mouse names");
    auto actions = qol::controlActions();
    require(actions.size() == static_cast<size_t>(chase::Action::Count) && actions.front() == "Up" && actions[12] == "Walk" &&
            actions[13] == "DebugOverlay" && actions[14] == "Fullscreen",
        "Walk, DebugOverlay and Fullscreen are listed with the 3DS actions");
}

void writerOutputRoundTrips() {
    auto text = qol::formatBindingEntries(sampleEntries(), resolve);
    require(text ==
            "Up key W hold\nUp key Up hold\nA key Space hold\nA mouse Left hold\nB mouse Right toggle\nWalk key Left_Alt hold\nDebugOverlay key F3 toggle\n",
        "canonical text, one line per binding, underscores for spaces");
    require(parse(text) == sampleEntries(), "the reader gives the entries back");
    std::istringstream stream(text);
    auto bindings = chase::readBindings(stream, resolve);
    require(bindings.size() == 7 && bindings[5].code == 17 && bindings[5].action == chase::Action::Walk, "readBindings accepts the output");
    require(bindings[4].source == chase::Source::Mouse && bindings[4].code == 3 && bindings[4].mode == chase::Mode::Toggle, "mouse and mode survive");
}

void tabChangesReachTheFile() {
    qol::ControlsTab tab(qol::AnyInput{}, qol::controlActions(), sampleEntries(), sampleEntries(), qol::fileInputValidator(resolve));
    tab.activate(*tab.rowOf("Up"), 1);
    tab.assignKey("Q");
    tab.toggleMode(*tab.rowOf("A"), 0);
    tab.clearSlot(*tab.rowOf("A"), 1);
    tab.activate(*tab.rowOf("Walk"), 1);
    tab.assignInput("mouse", "X1");
    auto text = qol::formatBindingEntries(tab.bindings(), resolve);
    auto back = parse(text);
    require(back == tab.bindings(), "the tab's list is read back unchanged");
    require(text.find("Up key Q hold") != std::string::npos && text.find("A key Space toggle") != std::string::npos &&
            text.find("A mouse Left") == std::string::npos && text.find("Walk mouse X1 hold") != std::string::npos,
        "rebind, mode, clearing and a mouse slot appear in the file");
}

void validatorFollowsTheFileFormat() {
    auto valid = qol::fileInputValidator(resolve);
    require(valid("key", "Left Alt") && valid("mouse", "X2"), "key and mouse accepted");
    require(!valid("key", "Nonsense") && !valid("key", "#") && !valid("key", "") && !valid("mouse", "Wheel") && !valid("pad", "South"),
        "unknown, unwritable, empty, unknown mouse and pad rejected");
    require(throws([] { qol::fileInputValidator(nullptr); }), "missing resolver rejected");
}

void readerRejections() {
    require(throws([] { parse("Jump key W hold\n"); }), "unknown action");
    require(throws([] { parse("Up key Nonsense hold\n"); }), "unknown key");
    require(throws([] { parse("Up key W sometimes\n"); }), "unknown mode");
    require(throws([] { parse("Up key W hold extra\n"); }), "extra token");
    require(throws([] { parse("Up pad South hold\n"); }), "pad source has no file format");
    require(throws([] { parse("Up mouse Wheel hold\n"); }), "unknown mouse button");
    require(throws([] { parse("# only a comment\n"); }), "no bindings");
    std::istringstream stream("Up key W hold\n");
    require(throws([&] { qol::readBindingEntries(stream, resolve, nullptr); }), "missing namer");
    std::istringstream second("Up key W hold\n");
    require(throws([&] { qol::readBindingEntries(second, resolve, [](int) { return std::string(); }); }), "key without a name");
}

void writerRejections() {
    auto with = [](BindingEntry entry) { return std::vector<BindingEntry>{{"Up", "key", "W", "hold"}, entry}; };
    require(throws([&] { qol::formatBindingEntries({}, resolve); }), "empty list");
    require(throws([&] { qol::formatBindingEntries(with({"Jump", "key", "W", "hold"}), resolve); }), "unknown action");
    require(throws([&] { qol::formatBindingEntries(with({"Up", "key", "W", "sometimes"}), resolve); }), "unknown mode");
    require(throws([&] { qol::formatBindingEntries(with({"Up", "pad", "South", "hold"}), resolve); }), "pad source");
    require(throws([&] { qol::formatBindingEntries(with({"Up", "key", "Nonsense", "hold"}), resolve); }), "unknown key");
    require(throws([&] { qol::formatBindingEntries(with({"Up", "key", "#", "hold"}), resolve); }), "a key the reader cannot see");
    require(throws([&] { qol::formatBindingEntries(with({"Up", "key", "Left  Alt", "hold"}), resolve); }), "double space");
    require(throws([&] { qol::formatBindingEntries(with({"Up", "key", "Left Alt", "hold"}), resolve); }), "entries spell spaces as underscores");
    require(qol::formatBindingEntries(with({"Up", "key", "Left_Alt", "hold"}), resolve).size() > 0, "underscored name accepted");
    require(throws([&] { qol::formatBindingEntries(with({"Up", "mouse", "Wheel", "hold"}), resolve); }), "unknown mouse button");
    require(throws([&] { qol::formatBindingEntries(sampleEntries(), nullptr); }), "missing resolver");
}

void fileIsWrittenAtomically() {
    auto directory = std::filesystem::temp_directory_path() / "chase-qol-controls-file-test";
    std::filesystem::remove_all(directory);
    std::filesystem::create_directories(directory);
    auto path = (directory / "controls.cfg").string();
    {
        std::ofstream old(path);
        old << "Up key W hold\n";
    }
    qol::writeBindingFile(path, sampleEntries(), resolve);
    std::ifstream written(path);
    require(parse(std::string(std::istreambuf_iterator<char>(written), {})) == sampleEntries(), "the file holds the entries");
    require(!std::filesystem::exists(path + ".tmp"), "no temporary file left");
    require(throws([&] { qol::writeBindingFile(path, {{"Jump", "key", "W", "hold"}}, resolve); }), "bad entries rejected");
    std::ifstream kept(path);
    require(parse(std::string(std::istreambuf_iterator<char>(kept), {})) == sampleEntries(), "a rejected write keeps the old file");
    require(throws([&] { qol::writeBindingFile((directory / "missing" / "controls.cfg").string(), sampleEntries(), resolve); }), "unwritable folder");
    std::filesystem::remove_all(directory);
}

}

int main() {
    readsEntriesFromTheFile();
    writerOutputRoundTrips();
    tabChangesReachTheFile();
    validatorFollowsTheFileFormat();
    readerRejections();
    writerRejections();
    fileIsWrittenAtomically();
    if (failures == 0) std::cout << "qol-controls-file: all checks passed\n";
    return failures == 0 ? 0 : 1;
}
