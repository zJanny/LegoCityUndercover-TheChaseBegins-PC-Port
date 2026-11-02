// Spec: docs/design/qol-screen-layout.md §2026-10-30T14:07:44 (QOLBOT-22: with Improved a click on a list row of the front end panel
// selects it and presses A, a click on a button picture A, B or Y presses that button, a click elsewhere in the panel or the window does
// nothing). Pinned values computed by hand by 20261102-qolctl2-D on 2026-11-02: the 1920x1080 panel is at x 240, y 0 with scale 4.5
// (qol-front-panel), so bottom-canvas pixel (x, y) is window pixel (240 + 4.5 x, 4.5 y).
#include "qol/front_panel_clicks.hpp"

#include <iostream>
#include <string>

namespace {

int failures = 0;

void require(bool ok, const std::string& what) {
    if (ok) return;
    std::cerr << "FAIL: " << what << '\n';
    ++failures;
}

const qol::PageRect panel = qol::menuPageRect(1920, 1080);

float windowX(float x) { return 240 + 4.5f * x; }
float windowY(float y) { return 4.5f * y; }

std::vector<qol::ClickTarget> page() {
    return {
        {{276, 209, 32, 32}, false, 0, qol::FrontButton::A},
        {{11, 209, 32, 32}, false, 0, qol::FrontButton::Y},
        {{49, 97, 256, 27}, true, 0, qol::FrontButton::None},
        {{49, 151, 256, 27}, true, 1, qol::FrontButton::None},
    };
}

void rowClicks() {
    const qol::ClickAction first = qol::panelClick(panel, page(), windowX(100), windowY(110));
    require(first.selectRow && *first.selectRow == 0 && first.press == qol::FrontButton::A, "a click on row 1 selects it and presses A");
    const qol::ClickAction second = qol::panelClick(panel, page(), windowX(49), windowY(151));
    require(second.selectRow && *second.selectRow == 1 && second.press == qol::FrontButton::A, "the top-left pixel of row 2 is in row 2");
    require(!qol::panelClick(panel, page(), windowX(305), windowY(160)).any(), "the pixel right of a row is outside it");
}

void buttonClicks() {
    const qol::ClickAction a = qol::panelClick(panel, page(), windowX(290), windowY(225));
    require(!a.selectRow && a.press == qol::FrontButton::A, "a click on the A picture presses A and selects no row");
    const qol::ClickAction y = qol::panelClick(panel, page(), windowX(20), windowY(220));
    require(!y.selectRow && y.press == qol::FrontButton::Y, "a click on the Y picture presses Y");
}

void clicksElsewhere() {
    require(!qol::panelClick(panel, page(), windowX(160), windowY(60)).any(), "a click on empty page does nothing");
    require(!qol::panelClick(panel, page(), windowX(160), windowY(200)).any(), "a click between the pictures does nothing");
    require(!qol::panelClick(panel, page(), 100, windowY(110)).any(), "a click in the bar beside the panel does nothing");
    require(!qol::panelClick(panel, page(), windowX(100), 1081).any(), "a click below the panel does nothing");
    require(!qol::panelClick(panel, {}, windowX(100), windowY(110)).any(), "a page without targets does nothing");
}

void queue() {
    qol::FrontClickQueue queue;
    require(!queue.take().any(), "an empty queue gives nothing");
    queue.add({});
    require(!queue.take().any(), "adding no action queues nothing");
    queue.add({std::size_t(1), qol::FrontButton::A});
    const qol::ClickAction taken = queue.take();
    require(taken.selectRow && *taken.selectRow == 1 && taken.press == qol::FrontButton::A, "a queued action is taken once");
    require(!queue.take().any(), "taking empties the queue");
    queue.add({std::nullopt, qol::FrontButton::B});
    queue.add({std::nullopt, qol::FrontButton::Y});
    require(queue.take().press == qol::FrontButton::Y, "the newest click of one frame wins");
    queue.add({std::nullopt, qol::FrontButton::B});
    queue.add({});
    require(queue.take().press == qol::FrontButton::B, "a click on nothing does not erase a queued one");
}

}

int main() {
    require(panel.x == 240 && panel.y == 0 && panel.scale == 4.5f, "the 1920x1080 panel sits where the pins assume");
    rowClicks();
    buttonClicks();
    clicksElsewhere();
    queue();
    if (failures) return 1;
    std::cout << "qol-front-panel-clicks OK\n";
    return 0;
}
