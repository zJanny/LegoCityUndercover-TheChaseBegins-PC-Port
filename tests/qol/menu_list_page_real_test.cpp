// The list, notice and question scenes of src/qol/menu_list_page.hpp drawn with the real layouts. Arguments: fib, names csv.
// Exit code 77 when the game data is missing. Written by 20261029-qolmenu2-B on 2026-10-29; the bar and button boxes were
// measured from the drawn layouts with chase-layoutdump (sprites/ui_roundaboutmenu/main.bxaml, page_alerttext.bxaml).
#include "frontend/resources.hpp"
#include "qol/menu_list_page.hpp"

#include <fstream>
#include <iostream>
#include <string>

namespace {

using namespace chase;
using namespace chase::frontend;

int failures = 0;

void require(bool ok, const std::string& what) {
    if (ok) return;
    std::cerr << "FAIL: " << what << '\n';
    ++failures;
}

Canvas draw(Resources& resources, const char* layout, const LayoutScene& scene) {
    Canvas canvas(kBottomWidth, kScreenHeight);
    canvas.clear(0, 0, 0);
    drawLayout(canvas, resources, layout, scene, 0, 0);
    return canvas;
}

bool same(const Canvas& a, const Canvas& b) { return a.rgba() == b.rgba(); }

bool pixelDiffers(const Canvas& a, const Canvas& b, int x, int y) {
    const std::uint8_t* p = a.pixel(x, y);
    const std::uint8_t* q = b.pixel(x, y);
    return p[0] != q[0] || p[1] != q[1] || p[2] != q[2];
}

void run(Resources& resources) {
    std::vector<std::string> actions{"Up", "Down", "Left", "Right", "A", "B", "Walk"};
    std::vector<qol::BindingEntry> defaults{{"Up", "key", "W", "hold"}, {"A", "key", "Space", "hold"}};
    qol::ControlsTab tab(actions, defaults, defaults, [](const std::string&) { return true; });
    qol::ControlsTabPort port(tab);
    qol::Settings settings;
    qol::MenuModel menu(settings);
    menu.setControls(&port);
    menu.open();
    menu.handle(qol::tabClick(static_cast<std::size_t>(qol::Tab::Controls)));

    const LayoutScene listScene = qol::listPageScene(menu);
    const Canvas list = draw(resources, qol::listPageLayout, listScene);
    menu.handle({qol::EventKind::Down});
    const Canvas moved = draw(resources, qol::listPageLayout, qol::listPageScene(menu));
    require(!same(list, moved), "moving the selection changes the picture");

    LayoutScene barless = listScene;
    for (std::size_t i = 1; i <= qol::listBarCount; ++i) barless.hidden.insert("Menu_Bar_" + std::to_string(i));
    barless.hidden.insert("Menu_Bar_HI");
    const Canvas background = draw(resources, qol::listPageLayout, barless);
    for (const qol::HitArea& area : qol::listHitAreas(menu)) {
        if (area.click->kind != qol::EventKind::RowClick) continue;
        const int x = int(area.box.x + area.box.width / 2), y = int(area.box.y + area.box.height / 2);
        require(pixelDiffers(moved, background, x, y), "bar hit area " + std::to_string(area.click->index) + " lies on its bar");
    }

    const Canvas fresh = draw(resources, qol::listPageLayout, qol::listPageScene(menu));
    require(same(moved, fresh), "the scene draws the same twice");

    qol::Settings settings2;
    qol::MenuModel menu2(settings2);
    menu2.setControls(&port);
    menu2.open();
    menu2.handle(qol::tabClick(static_cast<std::size_t>(qol::Tab::Controls)));
    menu2.handle({qol::EventKind::Activate});
    require(qol::controlsNoticeShown(menu2), "waiting shows the notice");
    const Canvas notice = draw(resources, qol::noticePageLayout, qol::noticeScene(menu2));
    const Canvas question = draw(resources, qol::noticePageLayout, qol::questionScene(menu2));
    require(!same(notice, question), "question draws its buttons, the notice does not");
    require(!same(notice, list), "notice page differs from the list page");

    LayoutScene noButtons = qol::questionScene(menu2);
    noButtons.hidden.insert("Option1");
    noButtons.hidden.insert("Option2");
    const Canvas withoutButtons = draw(resources, qol::noticePageLayout, noButtons);
    menu2.assignInput("key", "W");
    menu2.handle({qol::EventKind::Activate});
    menu2.assignInput("key", "A");
    for (int i = 0; i < 7; ++i) menu2.handle({qol::EventKind::Down});
    menu2.handle({qol::EventKind::Activate});
    require(qol::controlsQuestionShown(menu2), "Reset all asks after a change");
    for (const qol::HitArea& area : qol::listHitAreas(menu2)) {
        const int x = int(area.box.x + area.box.width / 2), y = int(area.box.y + area.box.height / 2);
        require(pixelDiffers(question, withoutButtons, x, y) || pixelDiffers(question, withoutButtons, x - 40, y) ||
                    pixelDiffers(question, withoutButtons, x + 40, y),
            "a question button hit area holds its label or icon");
    }
}

}

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "usage: qol-menu-list-page-real-test FIB CSV\n";
        return 2;
    }
    if (!std::ifstream(argv[1]).good() || !std::ifstream(argv[2]).good()) {
        std::cout << "qol-menu-list-page-real-test: no game data, skipping\n";
        return 77;
    }
    try {
        assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        Resources resources(archive);
        run(resources);
    } catch (const std::exception& error) {
        std::cerr << "qol-menu-list-page-real-test: " << error.what() << '\n';
        return 1;
    }
    if (failures) return 1;
    std::cout << "qol-menu-list-page-real-test: ok\n";
    return 0;
}
