#include "qol/ui_page_qol.hpp"

#include <array>
#include <vector>

namespace qol::ui {

namespace {

struct FeatureHelp {
    Feature feature;
    const char* description;
    bool defaultOn;
};

constexpr std::array<FeatureHelp, featureCount> featureHelp{{
    {Feature::WidenedView, "A wider game picture with the HUD in the corners.", false},
}};

const FeatureHelp& helpOf(Feature feature) {
    return featureHelp.at(static_cast<std::size_t>(feature));
}

std::string defaultText(bool on) { return on ? "Default: On" : "Default: Off"; }

std::vector<Master> presets(const Settings& settings) {
    std::vector<Master> list{Master::Original, Master::Improved};
    if (settings.mixSet()) list.push_back(Master::Custom);
    return list;
}

const char* const resetQuestion = "Reset the quality of life settings to the defaults?";

}

std::vector<RowSpec> QolPage::rows() const {
    std::vector<RowSpec> out;
    out.push_back({"Preset", RowKind::Selector, masterName(settings_.master()), 0, true});
    for (const FeatureInfo& info : featureTable)
        out.push_back({info.label, RowKind::Toggle, settings_.enabled(info.feature) ? "On" : "Off", 0, true});
    out.push_back({"Reset to defaults", RowKind::Button, "Reset", 0, true});
    return out;
}

DropdownInfo QolPage::dropdown(int) const { return {}; }

QolPageOutcome QolPage::stepMaster(int direction) {
    const std::vector<Master> list = presets(settings_);
    std::size_t index = 0;
    while (index + 1 < list.size() && list[index] != settings_.master()) ++index;
    const std::size_t count = list.size();
    const std::size_t target = direction < 0 ? (index + count - 1) % count : (index + 1) % count;
    settings_.setMaster(list[target]);
    return {QolOutcomeKind::Changed, {}};
}

QolPageOutcome QolPage::flipFeature(Feature feature) {
    const bool wanted = !settings_.enabled(feature);
    if (settings_.master() != Master::Custom)
        for (const FeatureInfo& info : featureTable) settings_.setSwitch(info.feature, settings_.enabled(info.feature));
    settings_.setSwitch(feature, wanted);
    settings_.setMaster(Master::Custom);
    return {QolOutcomeKind::Changed, {}};
}

QolPageOutcome QolPage::handle(const Action& action) {
    const int row = action.index;
    switch (action.kind) {
    case ActionKind::ValueChanged:
        if (row == masterRow) return stepMaster(action.direction);
        if (row >= firstFeatureRow && row < resetRow)
            return flipFeature(featureTable.at(static_cast<std::size_t>(row - firstFeatureRow)).feature);
        return {};
    case ActionKind::Activate:
        if (row == resetRow) return {QolOutcomeKind::NeedsConfirm, resetQuestion};
        return {};
    default:
        return {};
    }
}

QolPageOutcome QolPage::reset() {
    settings_.setMaster(Master::Original);
    return {QolOutcomeKind::Changed, {}};
}

PageHelp QolPage::help(int row) const {
    if (row == masterRow)
        return {"Original: as the game was. Improved: all features on. Custom: your mix. Default: Original"};
    if (row >= firstFeatureRow && row < resetRow) {
        const FeatureHelp& entry = helpOf(featureTable.at(static_cast<std::size_t>(row - firstFeatureRow)).feature);
        return {std::string(entry.description) + " " + defaultText(entry.defaultOn)};
    }
    if (row == resetRow) return {"Sets the preset back to Original; your own mix is kept."};
    return {};
}

}
