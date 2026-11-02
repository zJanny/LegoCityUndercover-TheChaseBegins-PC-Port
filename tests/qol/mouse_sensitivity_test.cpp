// The Mouse sensitivity slider of the Controls tab. Spec: docs/design/qol-controls.md, the user's answer of 2026-11-02
// "Accept proposal": x0.25 to x3.00 in 0.25 steps, default x1.00 equal to the earlier fixed 0.0018 button-seconds per pixel, a row above
// the bindings shown while PC controls is on and greyed out in Original, `mouse_sensitivity <factor>` in config/qol.cfg written only
// when not x1.00, help line "How fast the camera turns when you move the mouse." and "Reset to defaults" of the Controls tab sets x1.00 too (the user, 2026-11-02, "Reset it too" and "Camera speed text"). The base 0.0018 is the earlier stand-in value of src/qol/pc_mouse_camera.hpp. Written by 20261102-qolmore-A on 2026-11-02.
#include "qol/active_settings.hpp"
#include "qol/mouse_sensitivity.hpp"
#include "qol/pc_mouse_camera.hpp"
#include "qol/settings.hpp"
#include "qol/ui_page_controls.hpp"

#include <cmath>
#include <iostream>
#include <set>
#include <sstream>
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

bool near(float a, float b) { return std::fabs(a - b) < 1e-6f; }

bool rejects(const std::string& text) {
    std::istringstream stream(text);
    try {
        readSettings(stream);
    } catch (const std::runtime_error&) {
        return true;
    }
    return false;
}

Settings parse(const std::string& text) {
    std::istringstream stream(text);
    return readSettings(stream);
}

std::string written(const Settings& settings) {
    std::ostringstream out;
    writeSettings(out, settings);
    return out.str();
}

void model() {
    require(mouseSensitivitySteps == 11, "twelve values from x0.25 to x3.00");
    require(near(mouseSensitivityFactor(0), 0.25f) && near(mouseSensitivityFactor(3), 1.0f) && near(mouseSensitivityFactor(11), 3.0f),
        "factors at both ends and the default");
    require(mouseSensitivityLabel(0) == "x0.25" && mouseSensitivityLabel(1) == "x0.5" && mouseSensitivityLabel(2) == "x0.75" &&
            mouseSensitivityLabel(3) == "x1.0" && mouseSensitivityLabel(4) == "x1.25" && mouseSensitivityLabel(11) == "x3.0",
        "labels");
    require(defaultMouseSensitivityStep == 3 && Settings{}.mouseSensitivityStep() == 3, "the default is x1.0");
    require(near(mouseSecondsPerPixel(Settings{}), 0.0018f), "the default is the earlier fixed 0.0018 button-seconds per pixel");
    Settings settings;
    require(settings.setMouseSensitivityStep(11) && near(mouseSecondsPerPixel(settings), 0.0054f), "x3.0 triples it");
    require(!settings.setMouseSensitivityStep(12) && !settings.setMouseSensitivityStep(-1) && settings.mouseSensitivityStep() == 11,
        "out-of-range steps are refused");
    require(parseMouseSensitivity("0.25") == 0 && parseMouseSensitivity("1.0") == 3 && parseMouseSensitivity("3.0") == 11, "parse");
    require(!parseMouseSensitivity("1") && !parseMouseSensitivity("3.25") && !parseMouseSensitivity("0.0") && !parseMouseSensitivity("1.1") &&
            !parseMouseSensitivity("x1.0") && !parseMouseSensitivity(""),
        "off-step, out-of-range and malformed text do not parse");
}

void configFile() {
    require(written(Settings{}).find("mouse_sensitivity") == std::string::npos, "no line at the default");
    require(parse("master Original\n").mouseSensitivityStep() == 3, "no line reads the default");
    Settings settings = parse("mouse_sensitivity 2.5\n");
    require(settings.mouseSensitivityStep() == 9, "a line is read");
    require(written(settings).find("mouse_sensitivity 2.5\n") != std::string::npos, "and written back");
    require(parse(written(settings)).mouseSensitivityStep() == 9, "round trip");
    require(parse("mouse_sensitivity 1.0\n").mouseSensitivityStep() == 3, "an explicit default is accepted");
    require(rejects("mouse_sensitivity 3.25\n"), "above the range is rejected");
    require(rejects("mouse_sensitivity 0.0\n"), "below the range is rejected");
    require(rejects("mouse_sensitivity 1.1\n"), "off the step is rejected");
    require(rejects("mouse_sensitivity fast\n"), "text is rejected");
    require(rejects("mouse_sensitivity\n"), "a missing value is rejected");
    require(rejects("mouse_sensitivity 1.0 2.0\n"), "an extra value is rejected");
    require(rejects("mouse_sensitivity 1.0\nmouse_sensitivity 1.5\n"), "a second line is rejected");
}

const std::vector<std::string> actions{"Up", "A"};

std::vector<BindingEntry> defaults() { return {{"Up", "key", "W", "hold"}, {"A", "key", "Space", "hold"}}; }

bool validInput(const std::string&, const std::string&) { return true; }

struct Fixture {
    Settings settings;
    ControlsTab tab{AnyInput{}, actions, defaults(), defaults(), validInput};
    ControlsTabPort port{tab};
    ControlsPage page{port, &settings};
};

Action slide(double fraction) {
    Action a;
    a.kind = ActionKind::ValueChanged;
    a.index = 0;
    a.fraction = fraction;
    return a;
}

void controlsRow() {
    Fixture f;
    require(f.page.hasSensitivityRow(), "Original shows the row");
    std::vector<RowSpec> rows = f.page.rows();
    require(rows[0].label == "Mouse sensitivity" && rows[0].kind == RowKind::Slider && rows[0].value == "x1.0" && !rows[0].enabled,
        "in Original it shows x1.0 and is greyed out");
    require(f.page.firstActionRow() == 1 && rows.size() == 4, "the bindings follow the row");
    require(f.page.handle(slide(1.0)).event == ControlsPageEvent::Nothing && f.settings.mouseSensitivityStep() == 3,
        "in Original the row ignores changes");

    f.settings.setMaster(Master::Improved);
    rows = f.page.rows();
    require(rows[0].enabled && rows[0].steps == mouseSensitivitySteps, "in Improved it is live with twelve values");
    require(f.page.handle(slide(1.0)).event == ControlsPageEvent::SettingsChanged && f.settings.mouseSensitivityStep() == 11,
        "moving to the end sets x3.0");
    require(f.page.rows()[0].value == "x3.0", "and shows it");
    require(f.page.handle(slide(1.0)).event == ControlsPageEvent::Nothing, "the same step reports no change");
    require(f.page.handle(slide(0.0)).event == ControlsPageEvent::SettingsChanged && f.settings.mouseSensitivityStep() == 0, "the start sets x0.25");

    require(f.page.help(f.page.sensitivityRow()) == "How fast the camera turns when you move the mouse.", "the row's help line");

    f.settings.setMouseSensitivityStep(8);
    const ControlsPageOutcome reset = f.page.reset();
    require(reset.settingsChanged && f.settings.mouseSensitivityStep() == defaultMouseSensitivityStep,
        "Reset to defaults of the Controls tab sets x1.0 and reports the settings change");
    require(!f.page.reset().settingsChanged, "a reset at x1.0 reports no settings change");
    f.settings.setMaster(Master::Original);
    f.settings.setMouseSensitivityStep(8);
    require(f.page.reset().settingsChanged && f.settings.mouseSensitivityStep() == defaultMouseSensitivityStep, "the reset works in every preset");
    f.settings.setMaster(Master::Improved);

    f.settings.setSwitch(Feature::PcControls, false);
    f.settings.setSwitch(Feature::WidenedView, true);
    f.settings.setMaster(Master::Custom);
    require(!f.page.hasSensitivityRow() && f.page.rows().size() == 3 && f.page.firstActionRow() == 0,
        "PC controls off in Custom hides the row");
    f.settings.setSwitch(Feature::PcControls, true);
    require(f.page.hasSensitivityRow(), "PC controls on in Custom shows it");

    ControlsTab other{AnyInput{}, actions, defaults(), defaults(), validInput};
    ControlsTabPort otherPort{other};
    ControlsPage plain{otherPort};
    require(!plain.hasSensitivityRow() && plain.rows().size() == 3, "a page without settings has no row");
}

void cameraUse() {
    constexpr float dt = 1.0f / 60.0f;
    const MouseScreen screen{800, 480, {}};
    for (const int step : {3, 11}) {
        Settings settings;
        settings.setMaster(Master::Improved);
        settings.setMouseSensitivityStep(step);
        setActiveSettings(settings);
        MouseCamera camera;
        camera.update(true, false, {400, 100}, screen);
        camera.motion(18, 0, screen);
        require(near(camera.take(dt), 18 * 0.0018f * mouseSensitivityFactor(step) / dt), "pixels turn by the chosen sensitivity");
    }
    setActiveSettings(Settings{});
}

}

int main() {
    model();
    configFile();
    controlsRow();
    cameraUse();
    return failures == 0 ? 0 : 1;
}
