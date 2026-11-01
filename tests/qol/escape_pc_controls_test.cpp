// Escape stays the fixed Start binding with PC controls on (QOLCTL-1 with the set of QOLCTL-8): the built-in set binds no Escape, and
// the running input still reads Escape as Start after that set is rebound (the path of QolWindow::syncControls and MenuStores::commit).
// Spec: docs/design/qol-controls.md entry 2026-11-01T13:50:44 (the Escape rules apply in both modes) and 2026-11-01T13:50:47 (Esc = pause).
// Cases written by 20261101-qolctl-B on 2026-11-01.
#include "input.hpp"
#include "qol/pc_controls.hpp"
#include "runtime/qol_escape.hpp"

#include <iostream>
#include <map>
#include <sstream>
#include <string>

namespace {

using namespace chase;

int failures = 0;

void require(bool ok, const std::string& what) {
    if (ok) return;
    std::cerr << "FAIL: " << what << '\n';
    ++failures;
}

constexpr int kEscape = 27;
constexpr int kSpace = 32;

void escapeSurvivesTheSet() {
    std::string text;
    std::map<std::string, int> codes;
    bool escapeInSet = false;
    for (const qol::BindingEntry& entry : qol::pcControlsDefaults()) {
        text += entry.action + " " + entry.source + " " + entry.code + " " + entry.mode + "\n";
        if (entry.source == "key" && entry.code == "Escape") escapeInSet = true;
    }
    require(!escapeInSet, "the PC controls set binds no Escape: the fixed binding is its only Escape");
    std::istringstream original("A key Space hold\n");
    InputState input(readBindings(original, [](const std::string&) { return kSpace; }));
    runtime::bindEscape(input, kEscape);
    std::istringstream file(text);
    input.rebind(readBindings(file, [&codes](const std::string& name) {
        return name == "Escape" ? 0 : codes.emplace(name, 1000 + int(codes.size())).first->second;
    }));
    input.event(Source::Key, kEscape, true);
    require(input.active(Action::Start), "Escape is Start after the PC controls set is rebound");
    input.event(Source::Key, kEscape, false);
    require(!input.active(Action::Start), "and it releases again");
}

}

int main() {
    escapeSurvivesTheSet();
    if (failures == 0) std::cout << "qol-escape-pc-controls: all checks passed\n";
    return failures == 0 ? 0 : 1;
}
