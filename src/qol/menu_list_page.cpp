#include "qol/menu_list_page.hpp"

#include <algorithm>
#include <string>

namespace qol {

namespace {

using chase::frontend::Align;
using chase::frontend::LayoutScene;
using chase::frontend::TextSlot;

constexpr float menuBarsY = 58;
constexpr float barTops[listBarCount] = {-9, 25, 59, 93, 127};
constexpr float highlightOffsetY = 16;
constexpr float barArtLeft = 70, barArtWidth = 180, barArtHeight = 37, barArtCentreY = 32;
constexpr float actionRight = 160, slotSplit = 205;
constexpr float arrowLeftX = 2, arrowRightX = 298, arrowY = 95, arrowWidth = 16, arrowHeight = 32;
constexpr float menuListX = 32, menuListOffsetY = -6;
constexpr float pageNumberX = 211, pageNumberY = -128;
constexpr float highlightedText[3] = {0.3f, 0.04f, 0.04f};
constexpr float noticeTransitionEnd = 0.7f;
constexpr float noticeLinePitch = 27;
constexpr float yesX = 166, noX = 6, buttonY = 208, buttonWidth = 148, buttonHeight = 26;
constexpr const char* buttonA = "sprites/buttons/dsbuttona.btga";
constexpr const char* buttonB = "sprites/buttons/dsbuttonb.btga";
constexpr const char* pageTitle = "PC Settings";
constexpr const char* yesLabel = "Yes";
constexpr const char* noLabel = "No";
constexpr const char* unboundSlot = "-";

TextSlot slotOf(const std::string& text, Align align, bool highlighted = false) {
    TextSlot slot;
    slot.text = text;
    slot.align = align;
    if (highlighted) {
        slot.tinted = true;
        std::copy(highlightedText, highlightedText + 3, slot.tint);
    }
    return slot;
}

std::string barName(std::size_t index) { return "Menu_Bar_" + std::to_string(index + 1); }
std::string barText(std::size_t index, const char* kind) { return "TEXT_Menu_" + std::to_string(index + 1) + "_" + kind; }

std::size_t firstVisible(std::size_t selected, std::size_t count) {
    if (count <= listBarCount) return 0;
    const std::size_t lead = listBarCount / 2;
    const std::size_t start = selected > lead ? selected - lead : 0;
    return std::min(start, count - listBarCount);
}

std::string slotsText(const ControlsRowText& row, bool selected, std::size_t chosen) {
    std::string out;
    for (std::size_t slot = 0; slot < row.slots.size(); ++slot) {
        std::string text = row.slots[slot].bound ? row.slots[slot].text : unboundSlot;
        if (selected && slot == chosen) text = "[" + text + "]";
        if (slot) out += " | ";
        out += text;
    }
    return out;
}

std::string waitingText(const MenuModel& model) {
    const auto waiting = model.controlsWaiting();
    const auto rows = model.controlsRows();
    const std::string action = waiting && waiting->row < rows.size() ? rows[waiting->row].action : std::string();
    return "Press a key or button for " + action;
}

LayoutScene alertScene(const std::string& body, bool header, bool question, const std::string& yes = yesLabel,
    const std::string& no = noLabel) {
    LayoutScene scene;
    scene.storyboard = question ? "TransitionOn2" : "TransitionOn";
    scene.time = noticeTransitionEnd;
    scene.hidden = {"FontPlaceholder", "FontPlaceholder_Small", "Save_Icon"};
    scene.hidden.insert(header ? "Text_Info_NoHeader" : "Text_Info");
    if (!header) scene.hidden.insert("Text_Header");
    TextSlot text = slotOf(body, Align::Centre);
    text.wrap = true;
    text.pitch = int(noticeLinePitch);
    scene.text[header ? "Text_Info" : "Text_Info_NoHeader"] = text;
    if (question) {
        scene.text["Text_Option1"] = slotOf(yes, Align::Right);
        scene.text["Text_Option2"] = slotOf(no, Align::Left);
        scene.images["ButtonIcon1"] = buttonA;
        scene.images["ButtonIcon2"] = buttonB;
    } else {
        scene.hidden.insert("Option1");
        scene.hidden.insert("Option2");
    }
    return scene;
}

Box barBox(std::size_t slot, float left, float right) {
    const float centre = menuBarsY + barTops[slot] + barArtCentreY;
    return {left, centre - barArtHeight / 2, right - left, barArtHeight};
}

}

bool controlsQuestionShown(const MenuModel& model) {
    return model.isOpen() && model.tab() == Tab::Controls && model.controlsConfirming();
}

bool controlsNoticeShown(const MenuModel& model) {
    if (!model.isOpen() || model.tab() != Tab::Controls || model.controlsConfirming()) return false;
    return model.controlsWaiting().has_value() || !model.controlsMessage().empty();
}

bool controlsListShown(const MenuModel& model) {
    return model.isOpen() && model.tab() == Tab::Controls && !controlsQuestionShown(model) && !controlsNoticeShown(model);
}

LayoutScene listPageScene(const MenuModel& model) {
    LayoutScene scene;
    scene.hidden = {"Particles", "Fade", "Menu", "Item_Name", "Menu_Strip", "Title", "codegrp", "itemSmallGroup",
                    "backButton", "selectButton", "Menu_Bar_HI_Down"};
    scene.text["TEXT_Title"] = slotOf(pageTitle, Align::Left);
    scene.text["TEXT_Subtitle"] = slotOf(tabName(model.tab()), Align::Left);
    scene.text["PageNumber"] =
        slotOf(std::to_string(static_cast<std::size_t>(model.tab()) + 1) + "/" + std::to_string(tabCount), Align::Centre);

    scene.positions["PageNumber"] = {pageNumberX, pageNumberY};
    const std::vector<Row> rows = model.rows();
    const std::vector<ControlsRowText> texts = model.controlsRows();
    const std::size_t selected = model.row();
    const std::size_t first = firstVisible(selected, rows.size());
    bool highlighted = false;
    for (std::size_t slot = 0; slot < listBarCount; ++slot) {
        const std::size_t row = first + slot;
        scene.positions[barName(slot)] = {0, barTops[slot]};
        if (row >= rows.size()) {
            scene.hidden.insert(barName(slot));
            continue;
        }
        if (row == selected) {
            std::string line = rows[row].label;
            if (rows[row].kind == RowKind::Control && row < texts.size())
                line += "  " + slotsText(texts[row], true, model.controlsSlot());
            scene.text["TEXT_Icon"] = slotOf(line, Align::Centre, true);
            scene.positions["Menu_Bar_HI"] = {0, barTops[slot] + highlightOffsetY};
            scene.positions["Menu_List"] = {menuListX, menuBarsY + barTops[slot] + highlightOffsetY + menuListOffsetY};
            highlighted = true;
            continue;
        }
        scene.text[barText(slot, "3DS")] = slotOf(rows[row].label, Align::Left);
        if (rows[row].kind == RowKind::Control && row < texts.size())
            scene.text[barText(slot, "DS")] = slotOf(slotsText(texts[row], false, model.controlsSlot()), Align::Right);
    }
    if (!highlighted) {
        scene.hidden.insert("Menu_Bar_HI");
        scene.hidden.insert("Menu_List");
    }
    return scene;
}

LayoutScene noticeScene(const MenuModel& model) {
    const std::string body = model.controlsWaiting() ? waitingText(model) : model.controlsMessage();
    return alertScene(body, false, false);
}

LayoutScene questionScene(const MenuModel&) {
    return alertScene(resetAllQuestion, false, true);
}

LayoutScene questionScene(const MenuModel&, const std::string& yes, const std::string& no) {
    return alertScene(resetAllQuestion, false, true, yes, no);
}

std::vector<HitArea> listHitAreas(const MenuModel& model) {
    std::vector<HitArea> areas;
    if (controlsQuestionShown(model)) {
        areas.push_back({{yesX, buttonY, buttonWidth, buttonHeight}, false, Event{EventKind::Activate}, std::nullopt});
        areas.push_back({{noX, buttonY, buttonWidth, buttonHeight}, false, Event{EventKind::Back}, std::nullopt});
        return areas;
    }
    if (!controlsListShown(model)) return areas;
    const std::vector<Row> rows = model.rows();
    const std::size_t first = firstVisible(model.row(), rows.size());
    for (std::size_t slot = 0; slot < listBarCount && first + slot < rows.size(); ++slot) {
        const std::size_t row = first + slot;
        areas.push_back({barBox(slot, barArtLeft, rows[row].kind == RowKind::Control ? actionRight : barArtLeft + barArtWidth), false,
                         rowClick(row), rowMove(row)});
        if (rows[row].kind != RowKind::Control) continue;
        areas.push_back({barBox(slot, actionRight, slotSplit), false, slotClick(row, 0), rowMove(row)});
        areas.push_back({barBox(slot, slotSplit, barArtLeft + barArtWidth), false, slotClick(row, 1), rowMove(row)});
    }
    areas.push_back({{arrowLeftX, arrowY, arrowWidth, arrowHeight}, false, Event{EventKind::TabPrevious}, std::nullopt});
    areas.push_back({{arrowRightX, arrowY, arrowWidth, arrowHeight}, false, Event{EventKind::TabNext}, std::nullopt});
    return areas;
}

}
