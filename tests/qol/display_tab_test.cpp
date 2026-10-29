// Spec: docs/design/qol-settings.md 2026-10-28T22:38:57 (window_mode and resolution values),
// docs/design/qol-screen-layout.md 2026-10-28T22:39:39 (minimum 400x240).
// Cases written by 20261029-qolset-A on 2026-10-29. The resolution list is a test fixture.
#include "qol/display_tab.hpp"

#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>

namespace {

int failures = 0;

void require(bool ok, const std::string& what) {
    if (ok) return;
    std::cerr << "FAIL: " << what << '\n';
    ++failures;
}

const qol::ResolutionChoices fixture{{800, 600}, {1280, 720}, {1920, 1080}};

void rowsShowTheStore() {
    qol::Settings settings;
    const auto rows = qol::displayRows(settings, fixture);
    require(rows.size() == 2, "two rows");
    require(rows[0].kind == qol::DisplayRowKind::WindowMode && rows[0].label == "Window mode" &&
        rows[0].value == "fullscreen", "window mode row shows the fullscreen default");
    require(rows[1].kind == qol::DisplayRowKind::Resolution && rows[1].label == "Resolution" &&
        rows[1].value == "desktop" && !rows[1].custom, "resolution row shows desktop when unset");
    settings.setWindowMode(qol::WindowMode::Windowed);
    settings.setResolution({1280, 720});
    const auto changed = qol::displayRows(settings, fixture);
    require(changed[0].value == "windowed" && changed[1].value == "1280x720" &&
        !changed[1].custom, "rows follow the store");
    settings.setResolution({1000, 600});
    const auto custom = qol::displayRows(settings, fixture);
    require(custom[1].value == "1000x600" && custom[1].custom,
        "a stored size outside the list is kept and shown as custom (user, 2026-10-29)");
}

void originalModeUnaffected() {
    qol::Settings settings;
    qol::activateDisplayRow(settings, 0, fixture);
    qol::activateDisplayRow(settings, 1, fixture);
    require(settings.resolution() == std::optional<qol::Resolution>{{800, 600}},
        "unset resolution moves to the first listed mode");
    require(settings.master() == qol::Master::Original && !settings.mixSet(),
        "display rows leave master and mix alone");
    for (const qol::FeatureInfo& info : qol::featureTable)
        require(!settings.enabled(info.feature), std::string("still off: ") + info.name);
}

void windowModeToggles() {
    qol::Settings settings;
    require(qol::activateDisplayRow(settings, 0, fixture) == qol::DisplayResult::WindowModeChanged,
        "window mode result");
    require(settings.windowMode() == qol::WindowMode::Windowed, "fullscreen -> windowed");
    qol::activateDisplayRow(settings, 0, fixture);
    require(settings.windowMode() == qol::WindowMode::Fullscreen, "windowed -> fullscreen");
}

void resolutionCycles() {
    qol::Settings settings;
    settings.setResolution({1280, 720});
    require(qol::activateDisplayRow(settings, 1, fixture) == qol::DisplayResult::ResolutionChanged,
        "resolution result");
    require(settings.resolution() == std::optional<qol::Resolution>{{1920, 1080}},
        "1280x720 -> 1920x1080");
    qol::activateDisplayRow(settings, 1, fixture);
    require(settings.resolution() == std::optional<qol::Resolution>{{800, 600}}, "list wraps to the first");
    settings.setResolution({1000, 500});
    qol::activateDisplayRow(settings, 1, fixture);
    require(settings.resolution() == std::optional<qol::Resolution>{{800, 600}},
        "a stored size outside the list moves to the first entry");
    require(qol::activateDisplayRow(settings, 1, {}) == qol::DisplayResult::Ignored &&
        settings.resolution() == std::optional<qol::Resolution>{{800, 600}}, "empty list ignored");
}

void clampToLargestMode() {
    const qol::ResolutionChoices unordered{{1280, 720}, {1920, 1080}, {800, 600}};
    require(qol::largestMode(unordered) == std::optional<qol::Resolution>{{1920, 1080}}, "largest mode");
    require(!qol::largestMode({}), "no largest mode of an empty list");
    require(qol::clampToLargest({3840, 2160}, unordered) == qol::Resolution{1920, 1080},
        "above the largest mode is limited to it (user, 2026-10-29: Largest display mode)");
    require(qol::clampToLargest({1920, 1200}, unordered) == qol::Resolution{1920, 1080}, "one side above");
    require(qol::clampToLargest({1920, 1080}, unordered) == qol::Resolution{1920, 1080}, "equal unchanged");
    require(qol::clampToLargest({1000, 600}, unordered) == qol::Resolution{1000, 600}, "below unchanged");
    require(qol::clampToLargest({3840, 2160}, {}) == qol::Resolution{3840, 2160}, "empty list never clamps");

    qol::Settings settings;
    require(!qol::clampStoredResolution(settings, unordered) && !settings.resolution(),
        "unresolved resolution is not clamped");
    settings.setResolution({3840, 2160});
    require(qol::clampStoredResolution(settings, unordered) &&
        settings.resolution() == std::optional<qol::Resolution>{{1920, 1080}},
        "stored size above the largest mode is replaced at startup (user, 2026-10-29: Clamp to largest mode)");
    require(!qol::clampStoredResolution(settings, unordered), "second clamp changes nothing");
    settings.setResolution({1000, 600});
    require(!qol::clampStoredResolution(settings, unordered) &&
        qol::displayRows(settings, unordered)[1].custom, "a custom size within the limit stays custom");
    settings.setResolution({3840, 2160});
    require(!qol::clampStoredResolution(settings, {{399, 240}}) &&
        settings.resolution() == std::optional<qol::Resolution>{{3840, 2160}}, "invalid list refused");
    settings.setResolution({3840, 2160});
    qol::clampStoredResolution(settings, unordered);
    require(!qol::displayRows(settings, unordered)[1].custom, "the clamped value shows as a normal entry");
}

void desktopReference() {
    qol::Settings settings;
    qol::activateDisplayRow(settings, 1, fixture, qol::Resolution{1280, 720});
    require(settings.resolution() == std::optional<qol::Resolution>{{1920, 1080}},
        "unresolved steps on from the desktop mode");
    qol::Settings other;
    qol::activateDisplayRow(other, 1, fixture, qol::Resolution{1000, 500});
    require(other.resolution() == std::optional<qol::Resolution>{{800, 600}},
        "a desktop size outside the list starts at the first entry");
}

void rejections() {
    qol::Settings settings;
    require(qol::activateDisplayRow(settings, 2, fixture) == qol::DisplayResult::Rejected,
        "row out of range rejected");
    require(qol::activateDisplayRow(settings, 1, {{399, 240}}) == qol::DisplayResult::Rejected,
        "choice below the minimum rejected");
    require(qol::activateDisplayRow(settings, 0, {{400, 239}}) == qol::DisplayResult::Rejected,
        "invalid list rejected for every row");
    require(settings.windowMode() == qol::WindowMode::Fullscreen && !settings.resolution(),
        "rejections change nothing");
    require(qol::choicesValid(fixture) && qol::choicesValid({{400, 240}}), "valid lists");
}

}

int main() {
    rowsShowTheStore();
    originalModeUnaffected();
    windowModeToggles();
    resolutionCycles();
    clampToLargestMode();
    desktopReference();
    rejections();
    if (failures) return EXIT_FAILURE;
    std::cout << "qol-display-tab: ok\n";
    return EXIT_SUCCESS;
}
