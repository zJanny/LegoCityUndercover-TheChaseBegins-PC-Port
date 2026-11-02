// Bug of the user's playtest, 2026-11-02: with the widened view on, switching PC controls off dropped the game to the default
// screen (the bottom screen came back). Spec: docs/design/qol-screen-layout.md, the user's answer of 2026-11-02 (the widened
// view is one setting; when it is on, the bottom screen is hidden and its content moves to the top screen, in any preset;
// Custom is the set of settings that differ from the preset). Cases written by 20261102-qolmenu-B on 2026-11-02. The toggle
// is driven through QolPage, the code of the settings menu.
#include "qol/bottom_hook.hpp"
#include "qol/ui_page_qol.hpp"

#include <iostream>
#include <string>

namespace {

using namespace qol;
using namespace qol::ui;

int failures = 0;

void require(bool ok, const std::string& what) {
    if (ok) return;
    std::cerr << "FAIL: " << what << '\n';
    ++failures;
}

Action flip(int row) {
    Action a;
    a.kind = ActionKind::ValueChanged;
    a.index = row;
    a.direction = 1;
    return a;
}

constexpr int widenedRow = QolPage::firstFeatureRow + static_cast<int>(Feature::WidenedView);
constexpr int controlsRow = QolPage::firstFeatureRow + static_cast<int>(Feature::PcControls);

struct Snapshot {
    bool widened, hidden, minimap;
    bool operator==(const Snapshot& other) const {
        return widened == other.widened && hidden == other.hidden && minimap == other.minimap;
    }
};

Snapshot snapshotOf(const Settings& settings) {
    return {settings.enabled(Feature::WidenedView), bottomScreenHidden(settings), minimapOn(settings)};
}

Settings improved() {
    Settings settings;
    settings.setMaster(Master::Improved);
    return settings;
}

Settings custom(bool widened, bool pcControls) {
    Settings settings;
    settings.setSwitch(Feature::WidenedView, widened);
    settings.setSwitch(Feature::PcControls, pcControls);
    settings.setMaster(Master::Custom);
    return settings;
}

Settings originalThenControlsOn() {
    Settings settings;
    QolPage page(settings);
    page.handle(flip(controlsRow));
    return settings;
}

void controlsOffKeepsTheView(Settings settings, const std::string& name, bool expectWidened) {
    require(settings.enabled(Feature::PcControls), name + ": PC controls is on before");
    require(settings.enabled(Feature::WidenedView) == expectWidened, name + ": widened view as expected before");
    const Snapshot before = snapshotOf(settings);
    QolPage page(settings);
    page.handle(flip(controlsRow));
    require(!settings.enabled(Feature::PcControls), name + ": PC controls is off after");
    require(snapshotOf(settings) == before, name + ": widened view, hidden bottom screen and minimap are unchanged");
    require(bottomScreenHidden(settings) == expectWidened, name + ": the bottom screen is hidden exactly when the widened view is on");
}

void widenedAloneDecidesTheLayout() {
    for (Master master : {Master::Original, Master::Custom, Master::Improved}) {
        for (bool widened : {false, true}) {
            if (master == Master::Original && widened) continue;
            if (master == Master::Improved && !widened) continue;
            Settings settings = master == Master::Original ? Settings{} : master == Master::Improved ? improved() : custom(widened, false);
            require(bottomScreenHidden(settings) == widened && minimapOn(settings) == widened,
                std::string(masterName(master)) + (widened ? " widened" : " not widened") + ": hidden bottom screen follows the widened view");
        }
    }
    Settings settings;
    QolPage page(settings);
    page.handle(flip(widenedRow));
    require(settings.master() == Master::Custom && bottomScreenHidden(settings), "switching the widened view on hides the bottom screen");
    page.handle(flip(widenedRow));
    require(!bottomScreenHidden(settings), "switching it off shows the bottom screen again");
}

}

int main() {
    controlsOffKeepsTheView(originalThenControlsOn(), "Original then on", false);
    controlsOffKeepsTheView(custom(true, true), "Custom widened", true);
    controlsOffKeepsTheView(custom(false, true), "Custom not widened", false);
    controlsOffKeepsTheView(improved(), "Improved", true);
    widenedAloneDecidesTheLayout();
    if (failures != 0) {
        std::cerr << failures << " failure(s)\n";
        return 1;
    }
    return 0;
}
