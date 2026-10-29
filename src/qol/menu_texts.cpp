#include "qol/menu_texts.hpp"

#include "input.hpp"
#include "qol/display_tab.hpp"
#include "qol/game_tab.hpp"
#include "qol/menu_model.hpp"

namespace qol {

const std::vector<std::string>& mouseButtonNames() {
    static const std::vector<std::string> names{"Left", "Middle", "Right", "X1", "X2"};
    return names;
}

const std::vector<std::string>& padInputNames() {
    static const std::vector<std::string> names{
        "a", "b", "x", "y", "back", "guide", "start", "leftstick", "rightstick",
        "leftshoulder", "rightshoulder", "dpup", "dpdown", "dpleft", "dpright",
        "misc1", "paddle1", "paddle2", "paddle3", "paddle4", "touchpad",
        "leftx", "lefty", "rightx", "righty", "lefttrigger", "righttrigger"};
    return names;
}

const std::vector<std::string>& actionNames() {
    static const std::vector<std::string> names = [] {
        std::vector<std::string> out;
        for (std::size_t i = 0; i < chase::actionCount; ++i)
            out.push_back(chase::actionName(static_cast<chase::Action>(i)));
        return out;
    }();
    return names;
}

namespace {

void addRows(std::vector<MenuText>& out, const std::string& source, const std::string& label,
    const std::vector<std::string>& values) {
    for (const std::string& value : values)
        out.push_back({source, label + ": " + value, namePlateWidth});
}

void addBars(std::vector<MenuText>& out, const std::string& source, const std::vector<std::string>& names) {
    for (const std::string& name : names) out.push_back({source, name, listBarWidth});
}

}

std::vector<MenuText> menuTexts(const MenuTextInputs& inputs) {
    std::vector<MenuText> out;
    const std::vector<std::string> onOff{"On", "Off"};
    const std::vector<std::string> yesNo{"Yes", "No"};
    std::vector<std::string> volumes;
    for (int volume = 0; volume <= maxVolume; ++volume) volumes.push_back(std::to_string(volume));

    out.push_back({"title", "PC Settings", 0});
    for (std::size_t tab = 0; tab < tabCount; ++tab)
        out.push_back({"tab name", tabName(static_cast<Tab>(tab)), 0});
    for (std::size_t tab = 1; tab <= tabCount; ++tab)
        out.push_back({"tab place", std::to_string(tab) + "/" + std::to_string(tabCount), 0});

    addRows(out, "Game tab", gameRowLabel(GameRow::SfxVolume), volumes);
    addRows(out, "Game tab", gameRowLabel(GameRow::MusicVolume), volumes);
    addRows(out, "Game tab", gameRowLabel(GameRow::InvertCamera), onOff);
    addRows(out, "Game tab", gameRowLabel(GameRow::InvertCamera), yesNo);
    addRows(out, "Game tab", gameRowLabel(GameRow::Language), inputs.languageNames);

    std::vector<std::string> windowModes;
    for (WindowMode mode : {WindowMode::Windowed, WindowMode::Fullscreen}) windowModes.push_back(windowModeName(mode));
    std::vector<std::string> resolutions = inputs.resolutions;
    resolutions.push_back("desktop");
    const std::vector<DisplayRow> display = displayRows(Settings{}, {});
    addRows(out, "Display tab", display.at(0).label, windowModes);
    addRows(out, "Display tab", display.at(1).label, resolutions);

    std::vector<std::string> masters;
    for (Master master : {Master::Original, Master::Improved, Master::Custom}) masters.push_back(masterName(master));
    for (const Row& row : rowsOf(Tab::QualityOfLife)) {
        if (row.kind == RowKind::Master) addRows(out, "Quality of life tab", row.label, masters);
        else addRows(out, "Quality of life tab", row.label, onOff);
    }

    addBars(out, "Controls action", actionNames());
    addBars(out, "Controls key", inputs.keyNames);
    addBars(out, "Controls mouse button", mouseButtonNames());
    addBars(out, "Controls pad input", padInputNames());
    addBars(out, "Controls binding mode", {"hold", "toggle"});
    return out;
}

}
