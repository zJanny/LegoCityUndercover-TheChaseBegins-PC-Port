// Spec: docs/design/qol-settings-drawing.md entry 2026-10-30T21:56:04 (approved suggestion: master preset "Switch to Custom,
// keep it", one toggle row with help line and default per feature, "Reset to defaults" asks first; answer 8: on the Quality of
// life tab the reset puts the preset back to Original) and docs/rules/qol.md (Custom is skipped while no own mix is set; all
// features default to off). Expectations written from the spec by 20261101-qolui3-C on 2026-11-01.
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

Action change(int row, int direction) {
    Action a;
    a.kind = ActionKind::ValueChanged;
    a.index = row;
    a.direction = direction;
    return a;
}

Action activate(int row) {
    Action a;
    a.kind = ActionKind::Activate;
    a.index = row;
    return a;
}

bool endsWith(const std::string& text, const std::string& tail) {
    return text.size() >= tail.size() && text.compare(text.size() - tail.size(), tail.size(), tail) == 0;
}

void checkRows() {
    Settings settings;
    QolPage page(settings);
    const std::vector<RowSpec> rows = page.rows();
    require(static_cast<int>(rows.size()) == QolPage::rowCount, "row count is preset, features, reset");
    require(QolPage::rowCount == 3, "one feature so far gives three rows");
    require(rows[0].kind == RowKind::Selector && rows[0].value == "Original", "preset selector shows Original");
    require(rows[1].kind == RowKind::Toggle && rows[1].label == "Widened view" && rows[1].value == "Off", "feature toggle off");
    require(rows[2].kind == RowKind::Button && rows[2].label == "Reset to defaults", "reset button last");
    for (const RowSpec& row : rows) require(row.enabled, "rows are enabled");
    require(page.dropdown(0).count == 0 && page.dropdown(1).count == 0 && page.dropdown(2).count == 0, "no dropdown rows");
}

void checkPresetCycling() {
    Settings settings;
    QolPage page(settings);
    require(page.handle(change(QolPage::masterRow, 1)).kind == QolOutcomeKind::Changed, "right changes the preset");
    require(settings.master() == Master::Improved && page.rows()[0].value == "Improved", "Original to Improved");
    require(settings.enabled(Feature::WidenedView) && page.rows()[1].value == "On", "Improved shows the feature on");
    page.handle(change(QolPage::masterRow, 1));
    require(settings.master() == Master::Original, "Custom skipped while no own mix is set (wraps to Original)");
    page.handle(change(QolPage::masterRow, -1));
    require(settings.master() == Master::Improved, "left from Original wraps to Improved without a mix");
    page.handle(change(QolPage::masterRow, -1));
    require(settings.master() == Master::Original, "left from Improved goes to Original");
    require(!settings.mixSet(), "cycling the preset never sets an own mix");
}

void checkCustomSwitchAndMemory() {
    Settings settings;
    QolPage page(settings);
    require(page.handle(change(QolPage::firstFeatureRow, 1)).kind == QolOutcomeKind::Changed, "hand change reports a change");
    require(settings.master() == Master::Custom, "a hand change in Original switches to Custom at once");
    require(settings.customSwitch(Feature::WidenedView) && settings.enabled(Feature::WidenedView), "the toggled feature is on");
    require(page.rows()[0].value == "Custom" && page.rows()[1].value == "On", "rows follow");

    page.handle(change(QolPage::masterRow, 1));
    require(settings.master() == Master::Original, "Custom to Original");
    require(!settings.enabled(Feature::WidenedView) && page.rows()[1].value == "Off", "Original shows all off");
    page.handle(change(QolPage::masterRow, 1));
    require(settings.master() == Master::Improved && page.rows()[1].value == "On", "Improved shows all on");
    page.handle(change(QolPage::masterRow, 1));
    require(settings.master() == Master::Custom, "Custom is offered once an own mix exists");
    require(settings.customSwitch(Feature::WidenedView), "Custom restores the remembered mix");

    page.handle(change(QolPage::firstFeatureRow, 1));
    require(settings.master() == Master::Custom && !settings.customSwitch(Feature::WidenedView), "toggling in Custom flips the switch");
    page.handle(change(QolPage::masterRow, 1));
    page.handle(change(QolPage::masterRow, 1));
    page.handle(change(QolPage::masterRow, 1));
    require(settings.master() == Master::Custom && !settings.customSwitch(Feature::WidenedView), "the changed mix is remembered");
}

void checkChangeFromImproved() {
    Settings settings;
    QolPage page(settings);
    page.handle(change(QolPage::masterRow, 1));
    require(settings.master() == Master::Improved, "Improved selected");
    page.handle(change(QolPage::firstFeatureRow, -1));
    require(settings.master() == Master::Custom, "a hand change in Improved switches to Custom");
    require(!settings.enabled(Feature::WidenedView) && page.rows()[1].value == "Off", "the feature is off");
    page.handle(change(QolPage::masterRow, -1));
    require(settings.master() == Master::Improved, "left from Custom goes to Improved");
    require(settings.enabled(Feature::WidenedView), "Improved is all on again");
}

void checkHelp() {
    Settings settings;
    QolPage page(settings);
    require(endsWith(page.help(QolPage::masterRow).text, "Default: Original"), "preset help names its default");
    const std::string feature = page.help(QolPage::firstFeatureRow).text;
    require(!feature.empty() && endsWith(feature, "Default: Off"), "feature help ends with its default");
    require(!page.help(QolPage::resetRow).text.empty(), "reset help");
    require(page.help(-1).text.empty() && page.help(QolPage::rowCount).text.empty(), "no help for rows that do not exist");
    page.handle(change(QolPage::masterRow, 1));
    require(endsWith(page.help(QolPage::firstFeatureRow).text, "Default: Off"), "the default does not follow the current value");
}

void checkReset() {
    Settings settings;
    QolPage page(settings);
    page.handle(change(QolPage::firstFeatureRow, 1));
    require(settings.master() == Master::Custom, "custom set up");
    const QolPageOutcome asked = page.handle(activate(QolPage::resetRow));
    require(asked.kind == QolOutcomeKind::NeedsConfirm && !asked.question.empty(), "reset asks first");
    require(settings.master() == Master::Custom, "nothing changes before the answer");
    const QolPageOutcome done = page.reset();
    require(done.kind == QolOutcomeKind::Changed && settings.master() == Master::Original, "confirmed reset puts the preset to Original");
    require(!settings.enabled(Feature::WidenedView), "all features off after the reset");
    require(settings.mixSet() && settings.customSwitch(Feature::WidenedView), "the own mix stays remembered");
    page.handle(change(QolPage::masterRow, 1));
    page.handle(change(QolPage::masterRow, 1));
    require(settings.master() == Master::Custom && settings.customSwitch(Feature::WidenedView), "Custom still restores it");
}

void checkIgnored() {
    Settings settings;
    QolPage page(settings);
    Action other;
    other.kind = ActionKind::TabChanged;
    other.index = 0;
    require(page.handle(other).kind == QolOutcomeKind::Nothing, "other actions do nothing");
    require(page.handle(activate(QolPage::masterRow)).kind == QolOutcomeKind::Nothing, "activate on the selector does nothing");
    require(page.handle(activate(QolPage::firstFeatureRow)).kind == QolOutcomeKind::Nothing, "activate on a toggle does nothing");
    require(page.handle(change(QolPage::resetRow, 1)).kind == QolOutcomeKind::Nothing, "a value change on the button does nothing");
    require(page.handle(change(99, 1)).kind == QolOutcomeKind::Nothing && page.handle(change(-1, 1)).kind == QolOutcomeKind::Nothing,
            "rows out of range do nothing");
    require(settings.master() == Master::Original && !settings.mixSet(), "settings untouched");
}

float fakeWidth(std::string_view text, float pixelSize) { return 0.5f * pixelSize * static_cast<float>(text.size()); }

void checkThroughNavigation() {
    Settings settings;
    QolPage page(settings);
    UiNav nav(fakeWidth);
    nav.setWindow(1920, 1080);
    nav.setTabs({"Game", "Display", "Controls", "Quality of life"});
    nav.setRows(page.rows());
    nav.setDropdownInfo([&page](int row) { return page.dropdown(row); });
    nav.handle(keyEvent(Key::Down));
    require(nav.screen().focusRow == 1, "focus on the feature row");
    std::vector<Action> actions = nav.handle(keyEvent(Key::Enter));
    require(actions.size() == 1 && page.handle(actions[0]).kind == QolOutcomeKind::Changed, "Enter on a toggle reaches the page");
    require(settings.master() == Master::Custom && settings.customSwitch(Feature::WidenedView), "Enter turned the feature on");
    nav.setRows(page.rows());
    require(nav.screen().rows[1].value == "On", "rows rebuilt from the page");
    nav.handle(keyEvent(Key::Down));
    actions = nav.handle(keyEvent(Key::Enter));
    require(actions.size() == 1 && page.handle(actions[0]).kind == QolOutcomeKind::NeedsConfirm, "Enter on reset asks");
}

}

int main() {
    checkRows();
    checkPresetCycling();
    checkCustomSwitchAndMemory();
    checkChangeFromImproved();
    checkHelp();
    checkReset();
    checkIgnored();
    checkThroughNavigation();
    if (failures != 0) {
        std::cerr << failures << " failure(s)\n";
        return 1;
    }
    return 0;
}
