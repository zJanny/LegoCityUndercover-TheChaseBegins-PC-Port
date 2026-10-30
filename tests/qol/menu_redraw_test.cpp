// The page cache key of src/qol/menu_redraw.cpp: two scenes that differ only in an element scale (the two-line Controls bars,
// QOLSET-43, docs/design/qol-settings-drawing.md entry 2026-10-29T12:11:55) must not share a cached page. Written by
// 20261030-qolmenu-B, 2026-10-30.
#include "qol/menu_redraw.hpp"

#include <iostream>
#include <string>

namespace {

int failures = 0;

void require(bool ok, const std::string& what) {
    if (ok) return;
    std::cerr << "FAIL: " << what << '\n';
    ++failures;
}

void scaleChangesFingerprint() {
    chase::frontend::LayoutScene plain;
    chase::frontend::LayoutScene scaled = plain;
    scaled.scales["TEXT_Menu_1_DS"] = {0.7f, 0.7f};
    chase::frontend::LayoutScene other = plain;
    other.scales["TEXT_Menu_1_DS"] = {0.6f, 0.7f};
    chase::frontend::LayoutScene renamed = plain;
    renamed.scales["TEXT_Menu_2_DS"] = {0.7f, 0.7f};
    require(qol::sceneFingerprint(plain) != qol::sceneFingerprint(scaled), "a scale changes the fingerprint");
    require(qol::sceneFingerprint(scaled) != qol::sceneFingerprint(other), "a different scale changes the fingerprint");
    require(qol::sceneFingerprint(scaled) != qol::sceneFingerprint(renamed), "the scaled element changes the fingerprint");
    require(qol::sceneFingerprint(scaled) == qol::sceneFingerprint(chase::frontend::LayoutScene(scaled)), "equal scenes share it");
}

}

int main() {
    scaleChangesFingerprint();
    if (failures) return 1;
    std::cout << "qol-menu-redraw: ok\n";
    return 0;
}
