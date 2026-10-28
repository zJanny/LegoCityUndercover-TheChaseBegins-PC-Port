// Spec: docs/design/qol-settings.md entries 2026-10-28T09:32:07, 09:40:40, 09:53:33.
// Cases written by 20261028-qol-A on 2026-10-28.
#include "qol/settings.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

int failures = 0;

void require(bool ok, const std::string& what) {
    if (ok) return;
    std::cerr << "FAIL: " << what << '\n';
    ++failures;
}

qol::Settings parse(const std::string& text) {
    std::istringstream in(text);
    return qol::readSettings(in);
}

std::string rejection(const std::string& text) {
    try {
        parse(text);
    } catch (const std::runtime_error& error) {
        return error.what();
    }
    return "";
}

std::string written(const qol::Settings& settings) {
    std::ostringstream out;
    qol::writeSettings(out, settings);
    return out.str();
}

void originalModeFirst() {
    qol::Settings fresh;
    require(fresh.master() == qol::Master::Original, "default master is Original");
    require(!fresh.mixSet(), "default has no own mix");
    for (const qol::FeatureInfo& info : qol::featureTable)
        require(!fresh.enabled(info.feature), std::string("default off: ") + info.name);

    const auto missing = std::filesystem::temp_directory_path() / "qol-settings-test-absent.cfg";
    std::filesystem::remove(missing);
    qol::Settings loaded = qol::loadSettings(missing.string());
    require(loaded.master() == qol::Master::Original && !loaded.mixSet(),
        "absent file gives defaults");
    for (const qol::FeatureInfo& info : qol::featureTable)
        require(!loaded.enabled(info.feature), std::string("absent file off: ") + info.name);

    qol::Settings empty = parse("# only a comment\n\n");
    require(empty.master() == qol::Master::Original, "empty file is Original");
}

void masterStates() {
    qol::Settings settings;
    settings.setSwitch(qol::Feature::WidenedView, false);
    require(settings.setMaster(qol::Master::Improved), "Improved selectable");
    require(settings.enabled(qol::Feature::WidenedView), "Improved enables every feature");
    settings.setSwitch(qol::Feature::WidenedView, true);
    require(settings.setMaster(qol::Master::Original), "Original selectable");
    require(!settings.enabled(qol::Feature::WidenedView),
        "Original disables every feature despite switches");
    require(settings.setMaster(qol::Master::Custom), "Custom selectable with a mix");
    require(settings.enabled(qol::Feature::WidenedView), "Custom follows the switch on");
    settings.setSwitch(qol::Feature::WidenedView, false);
    require(!settings.enabled(qol::Feature::WidenedView), "Custom follows the switch off");
}

void customUnavailableWithoutMix() {
    qol::Settings settings;
    require(!settings.setMaster(qol::Master::Custom), "Custom refused without a mix");
    require(settings.master() == qol::Master::Original, "refusal keeps the master");
    require(settings.cycleMaster() == qol::Master::Improved, "cycle: Original -> Improved");
    require(settings.cycleMaster() == qol::Master::Original,
        "cycle skips Custom without a mix");
}

void cycleWithMix() {
    qol::Settings settings;
    settings.setSwitch(qol::Feature::WidenedView, true);
    require(settings.cycleMaster() == qol::Master::Improved, "cycle 1");
    require(settings.cycleMaster() == qol::Master::Custom, "cycle 2 reaches Custom");
    require(settings.customSwitch(qol::Feature::WidenedView), "Custom restores the last mix");
    require(settings.cycleMaster() == qol::Master::Original, "cycle 3 back to Original");
    require(settings.customSwitch(qol::Feature::WidenedView), "Original keeps the stored mix");
}

void roundTrip() {
    qol::Settings fresh;
    require(written(fresh) == "master Original\n", "fresh settings written");
    require(parse(written(fresh)).master() == qol::Master::Original, "fresh round trip");

    qol::Settings custom;
    custom.setSwitch(qol::Feature::WidenedView, true);
    custom.setMaster(qol::Master::Custom);
    const std::string text = written(custom);
    require(text == "master Custom\nwidened_view on\n", "custom settings written");
    qol::Settings back = parse(text);
    require(back.master() == qol::Master::Custom && back.mixSet() &&
        back.enabled(qol::Feature::WidenedView), "custom round trip");
    require(written(back) == text, "writer is stable");

    qol::Settings improved;
    improved.setSwitch(qol::Feature::WidenedView, false);
    improved.setMaster(qol::Master::Improved);
    qol::Settings improvedBack = parse(written(improved));
    require(improvedBack.master() == qol::Master::Improved &&
        improvedBack.enabled(qol::Feature::WidenedView) &&
        !improvedBack.customSwitch(qol::Feature::WidenedView),
        "Improved round trip keeps the stored mix");

    const auto path = std::filesystem::temp_directory_path() / "qol-settings-test-roundtrip.cfg";
    qol::saveSettings(path.string(), custom);
    qol::Settings fromFile = qol::loadSettings(path.string());
    require(fromFile.master() == qol::Master::Custom &&
        fromFile.customSwitch(qol::Feature::WidenedView), "file round trip");
    std::filesystem::remove(path);

    require(parse("# note\nmaster Improved # trailing\n").master() == qol::Master::Improved,
        "comments are ignored");
}

void rejections() {
    const struct { const char* text; const char* expect; } cases[] = {
        {"colour red\n", "line 1: colour red"},
        {"master Original\nmaster Improved\n", "line 2: master Improved"},
        {"master\n", "line 1: master"},
        {"master Original extra\n", "line 1: master Original extra"},
        {"master original\n", "line 1: master original"},
        {"master Hybrid\n", "line 1: master Hybrid"},
        {"widened_view yes\n", "line 1: widened_view yes"},
        {"widened_view on\nwidened_view off\n", "line 2: widened_view off"},
        {"widened_view\n", "line 1: widened_view"},
        {"widened_view on on\n", "line 1: widened_view on on"},
        {"master Original\nbogus on\n", "line 2: bogus on"},
    };
    for (const auto& item : cases) {
        const std::string message = rejection(item.text);
        require(message.find(item.expect) != std::string::npos,
            std::string("rejects with line and text: ") + item.text);
    }
    require(!rejection("master Custom\n").empty(), "Custom without a stored mix is rejected");
}

}

int main() {
    originalModeFirst();
    masterStates();
    customUnavailableWithoutMix();
    cycleWithMix();
    roundTrip();
    rejections();
    if (failures) return EXIT_FAILURE;
    std::cout << "qol-settings: ok\n";
    return EXIT_SUCCESS;
}
