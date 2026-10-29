// Spec: docs/design/qol-settings-drawing.md entry 2026-10-29T12:11:55 (Q2 tab place "3/4", Q3 list bars for Controls,
// Q6 notice form, Reset all question form of page_alerttext with Yes (A) and No (B)).
// Layout element names and positions: docs/re/ui-layouts.md entry 2026-10-29T11:50:58 and chase-layoutdump of
// sprites/ui_roundaboutmenu/main.bxaml and page_alerttext.bxaml, read by 20261029-qolmenu2-B on 2026-10-29.
#include "qol/menu_list_page.hpp"

#include <iostream>
#include <string>

namespace {

int failures = 0;

void require(bool ok, const std::string& what) {
    if (ok) return;
    std::cerr << "FAIL: " << what << '\n';
    ++failures;
}

using qol::EventKind;
using qol::Tab;

struct Fixture {
    std::vector<std::string> actions{"Up", "Down", "Left", "Right", "A", "B", "Walk"};
    std::vector<qol::BindingEntry> defaults{{"Up", "key", "W", "hold"}, {"Up", "key", "Up", "hold"}, {"A", "key", "Space", "hold"},
        {"A", "mouse", "Left", "hold"}, {"Walk", "key", "Left_Alt", "hold"}};
    qol::ControlsTab tab;
    qol::ControlsTabPort port;
    qol::Settings settings;
    qol::MenuModel menu;

    Fixture()
        : tab(actions, defaults, defaults, [](const std::string& name) { return name != "Nonsense"; }), port(tab), menu(settings) {
        menu.setControls(&port);
        menu.open();
        menu.handle(qol::tabClick(static_cast<std::size_t>(Tab::Controls)));
    }
};

std::string textOf(const chase::frontend::LayoutScene& scene, const std::string& name) {
    const auto found = scene.text.find(name);
    return found == scene.text.end() ? std::string("<none>") : found->second.text;
}

bool hidden(const chase::frontend::LayoutScene& scene, const std::string& name) { return scene.hidden.count(name) != 0; }

void listForm() {
    Fixture f;
    require(f.menu.rows().size() == 8, "seven actions and Reset all");
    require(qol::controlsListShown(f.menu), "list form shown by default");
    auto scene = qol::listPageScene(f.menu);
    require(textOf(scene, "TEXT_Subtitle") == "Controls", "subtitle bar shows the tab name");
    require(textOf(scene, "PageNumber") == "3/4", "page number is the tab place");
    require(textOf(scene, "TEXT_Title") == "PC Settings", "title");
    require(textOf(scene, "TEXT_Menu_2_3DS") == "Down" && textOf(scene, "TEXT_Menu_3_3DS") == "Left", "first five rows in the five bars");
    require(textOf(scene, "TEXT_Icon") == "Up  [key W] | key Up", "selected row marks slot 1 on the highlight");
    require(textOf(scene, "TEXT_Menu_1_3DS") == "<none>" && !hidden(scene, "Menu_List"), "selected text sits on the highlight only");
    require(textOf(scene, "TEXT_Menu_3_DS") == "- | -" && textOf(scene, "TEXT_Menu_4_DS") == "- | -", "unbound slots show a dash");
    require(textOf(scene, "TEXT_Menu_5_DS") == "key Space | mouse Left", "unselected row marks no slot");
    require(scene.positions.at("Menu_Bar_HI")[1] == scene.positions.at("Menu_Bar_1")[1] + 16, "highlight sits on the first bar");
    require(!hidden(scene, "Menu_Bar_HI") && hidden(scene, "Menu") && hidden(scene, "Menu_Strip") && hidden(scene, "Item_Name"),
        "highlight shown, round and strip hidden");

    f.menu.handle({EventKind::Right});
    scene = qol::listPageScene(f.menu);
    require(textOf(scene, "TEXT_Icon") == "Up  key W | [key Up]", "Right marks slot 2");

    for (int i = 0; i < 4; ++i) f.menu.handle({EventKind::Down});
    require(f.menu.row() == 4, "selected fifth row");
    scene = qol::listPageScene(f.menu);
    require(textOf(scene, "TEXT_Menu_1_3DS") == "Left" && textOf(scene, "TEXT_Menu_5_3DS") == "Walk", "selection kept in view");
    require(textOf(scene, "TEXT_Icon").rfind("A  ", 0) == 0, "selected row in the middle bar");
    require(scene.positions.at("Menu_Bar_HI")[1] == scene.positions.at("Menu_Bar_3")[1] + 16, "highlight follows the selected bar");
    require(textOf(scene, "TEXT_Icon") == "A  [key Space] | mouse Left", "selected row texts");

    for (int i = 0; i < 3; ++i) f.menu.handle({EventKind::Down});
    require(f.menu.row() == 7, "selected Reset all");
    scene = qol::listPageScene(f.menu);
    require(textOf(scene, "TEXT_Icon") == "Reset all", "Reset all row has no slot text");
    require(textOf(scene, "TEXT_Menu_1_3DS") == "Right" && scene.positions.at("Menu_Bar_HI")[1] == scene.positions.at("Menu_Bar_5")[1] + 16,
        "window stops at the end and the highlight reaches the last bar");
}

void noticeForm() {
    Fixture f;
    require(!qol::controlsNoticeShown(f.menu), "no notice at first");
    f.menu.handle(qol::eventForKey(qol::Key::Enter));
    require(qol::controlsNoticeShown(f.menu) && !qol::controlsListShown(f.menu), "waiting shows the notice");
    auto scene = qol::noticeScene(f.menu);
    require(textOf(scene, "Text_Info_NoHeader") == "Press a key or button for Up", "waiting text names the action");
    require(hidden(scene, "Text_Info") && hidden(scene, "Option1") && hidden(scene, "Option2"), "notice has no buttons");
    require(qol::listHitAreas(f.menu).empty(), "no hit areas on the notice");
    f.menu.assignInput("key", "Space");
    require(textOf(qol::noticeScene(f.menu), "Text_Info_NoHeader") == "Press a key or button for Up" &&
                f.menu.controlsMessage() == "key Space is already used by A",
        "refuse keeps waiting");
    f.menu.handle(qol::eventForKey(qol::Key::Escape));
    require(!qol::controlsNoticeShown(f.menu), "Escape ends the notice");
}

void questionForm() {
    Fixture f;
    f.menu.handle(qol::eventForKey(qol::Key::Enter));
    f.menu.assignInput("key", "E");
    for (int i = 0; i < 7; ++i) f.menu.handle({EventKind::Down});
    f.menu.handle(qol::eventForKey(qol::Key::Enter));
    require(f.menu.controlsConfirming() && qol::controlsQuestionShown(f.menu) && !qol::controlsListShown(f.menu),
        "Reset all asks the question");
    const auto scene = qol::questionScene(f.menu);
    require(textOf(scene, "Text_Info_NoHeader") == qol::resetAllQuestion, "question text");
    require(textOf(scene, "Text_Option1") == "Yes" && textOf(scene, "Text_Option2") == "No", "Yes and No labels");
    require(scene.images.at("ButtonIcon1") == "sprites/buttons/dsbuttona.btga" &&
                scene.images.at("ButtonIcon2") == "sprites/buttons/dsbuttonb.btga",
        "Yes on A, No on B");
    require(!hidden(scene, "Option1") && !hidden(scene, "Option2"), "both buttons shown");
    const auto words = qol::questionScene(f.menu, "Ja", "Nein");
    require(textOf(words, "Text_Option1") == "Ja" && textOf(words, "Text_Option2") == "Nein" &&
                textOf(words, "Text_Info_NoHeader") == qol::resetAllQuestion,
        "the question takes the Yes and No words given (gametext, QOLSET-44)");
    const auto areas = qol::listHitAreas(f.menu);
    require(areas.size() == 2 && areas[0].click->kind == EventKind::Activate && areas[1].click->kind == EventKind::Back,
        "Yes activates, No goes back");
    require(areas[0].box.x == 166 && areas[0].box.y == 208 && areas[1].box.x == 6 && areas[1].box.width == 148, "button boxes");
}

void hitAreas() {
    Fixture f;
    const auto areas = qol::listHitAreas(f.menu);
    require(areas.size() == 5 * 3 + 2, "action and two slots per bar, plus two arrows");
    require(areas[0].click->kind == EventKind::RowClick && areas[0].click->index == 0 && areas[0].hover &&
                areas[0].hover->kind == EventKind::RowMove,
        "action part clicks the row");
    require(areas[1].click->kind == EventKind::SlotClick && areas[1].click->index == 0 && areas[1].click->slot == 0 &&
                areas[2].click->slot == 1,
        "slot parts click the slots");
    require(areas[3].click->index == 1, "second bar is the second row");
    require(areas[0].box.y < areas[3].box.y && areas[0].box.x == 70 && areas[2].box.x + areas[2].box.width == 250, "bars stack top down");
    require(areas[areas.size() - 2].click->kind == EventKind::TabPrevious && areas.back().click->kind == EventKind::TabNext, "arrows");
    for (int i = 0; i < 7; ++i) f.menu.handle({EventKind::Down});
    const auto last = qol::listHitAreas(f.menu);
    require(last.size() == 4 * 3 + 1 + 2 && last[0].click->index == 3 && last[last.size() - 3].click->kind == EventKind::RowClick &&
                last[last.size() - 3].click->index == 7 && last[last.size() - 3].box.width == 180,
        "Reset all bar spans the whole bar and has no slot areas");
}

void otherTabs() {
    qol::Settings settings;
    qol::MenuModel menu(settings);
    menu.open();
    require(!qol::controlsListShown(menu) && qol::listHitAreas(menu).empty(), "no list form on the Game tab");
    menu.handle(qol::tabClick(static_cast<std::size_t>(Tab::Controls)));
    require(qol::controlsListShown(menu) && qol::listPageScene(menu).text.count("TEXT_Menu_1_DS") == 0,
        "Controls tab without a port lists its rows and no slot texts");
}

}

int main() {
    listForm();
    noticeForm();
    questionForm();
    hitAreas();
    otherTabs();
    if (failures) return 1;
    std::cout << "ok\n";
}
