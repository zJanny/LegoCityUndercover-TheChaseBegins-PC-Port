#include "frontend/page_targets.hpp"
#include "frontend/layout.hpp"
#include <stdexcept>
#include <string>

namespace chase::frontend {

namespace {

TargetBox boxOf(Resources& resources, const char* layout, const std::string& path, float offsetX = 0, float offsetY = 0) {
    const auto box = layoutElementBox(resources, layout, path);
    if (!box) throw std::runtime_error(std::string("front end page targets: ") + layout + " has no element " + path);
    return {(*box)[0] + offsetX, (*box)[1] + offsetY, (*box)[2], (*box)[3]};
}

ButtonTarget shortcutButton(Resources& resources, const char* element, std::uint32_t button) {
    return {boxOf(resources, kShortcutsLayout, std::string(element) + "/Button", kShortcutsX, kShortcutsY), button};
}

}

PageTargets alertPageTargets(Resources& resources, const AlertPage& page) {
    PageTargets targets;
    const char* icons[2] = {"ButtonIcon1", "ButtonIcon2"};
    for (std::size_t i = 0; i < page.buttons.size() && i < 2; ++i)
        targets.buttons.push_back({boxOf(resources, kAlertLayout, icons[i]), page.buttons[i].symbol == PadSymbol::A ? kButtonA : kButtonB});
    return targets;
}

PageTargets languageListTargets(Resources& resources, std::size_t rowCount) {
    PageTargets targets;
    targets.buttons.push_back(shortcutButton(resources, "SelectButton_3DS", kButtonA));
    for (std::size_t row = 0; row < rowCount; ++row)
        targets.rows.push_back({0, kLanguageRowCentreY + kLanguageRowPitch * float(row) - kLanguageRowPitch / 2, float(kBottomWidth), kLanguageRowPitch});
    return targets;
}

PageTargets slotPageTargets(Resources& resources, std::size_t rowCount, bool selectedRowUsed, float seconds) {
    PageTargets targets;
    targets.buttons.push_back(shortcutButton(resources, "SelectButton_3DS", kButtonA));
    if (selectedRowUsed) targets.buttons.push_back(shortcutButton(resources, "ExtraButton_3DS", kButtonY));
    const assets::UiLayout& layout = resources.layout(kSlotLayout);
    for (std::size_t row = 0; row < rowCount && row < 2; ++row) {
        const std::string item = "ListItem" + std::to_string(row + 1);
        const auto sliding = layoutChildBox(layout, {{"TransitionOn", seconds}}, item);
        const auto resting = layoutChildBox(layout, {}, item);
        if (!sliding || !resting) throw std::runtime_error(std::string("front end page targets: ") + kSlotLayout + " has no element " + item);
        targets.rows.push_back(boxOf(resources, kSlotLayout, item + "/Button", (*sliding)[0] - (*resting)[0]));
    }
    return targets;
}

}
