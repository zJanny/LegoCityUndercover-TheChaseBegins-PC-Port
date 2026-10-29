// The HUD part subsets of Hud::draw (frontend/hud_parts.hpp) against the real game data: every part drawn alone leaves opaque pixels
// that no other part leaves, and together they are the full draw, byte for byte. Arguments: fib, names csv. Exit code 77 when the
// game data is missing. Scene recorded by 20261029-qolmenu-G on 2026-10-29: a settled HUD with four hearts, the stud bar shown with
// 10 studs, a super brick pickup shown and the camera lock icon at full alpha.
#include "frontend/hud.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {
using namespace chase;
using namespace chase::frontend;

int failures = 0;

void require(bool condition, const std::string& what) {
    if (!condition) {
        std::cerr << "FAIL: " << what << '\n';
        ++failures;
    }
}

constexpr HudPart kParts[] = {HudPart::Portrait, HudPart::Hearts, HudPart::Studs, HudPart::SuperBricks, HudPart::CameraLock};
const char* const kNames[] = {"portrait", "hearts", "studs", "super bricks", "camera lock"};
constexpr std::size_t kPixels = std::size_t(kTopWidth) * kScreenHeight;

Canvas drawn(Hud& hud, HudPartSet parts, double spin) {
    Canvas canvas(kTopWidth, kScreenHeight);
    canvas.clearTransparent();
    hud.draw(canvas, spin, parts);
    return canvas;
}

bool opaqueAt(const Canvas& canvas, std::size_t pixel) { return canvas.rgba()[pixel * 4 + 3] != 0; }

void scene(Resources& resources) {
    Hud hud(resources, {});
    for (int update = 0; update < 90; ++update) hud.update(1.0f / 30);
    hud.studsBanked(0, 10);
    hud.superBrickPickedUp();
    hud.setCameraLockAlpha(1.0f);
    for (int update = 0; update < 20; ++update) hud.update(1.0f / 30);
    const double spin = 1.0;

    const Canvas full = drawn(hud, HudPartSet::all(), spin);
    std::vector<Canvas> alone;
    for (const HudPart part : kParts) alone.push_back(drawn(hud, part, spin));
    for (std::size_t i = 0; i < alone.size(); ++i) {
        std::size_t opaque = 0;
        for (std::size_t p = 0; p < kPixels; ++p) opaque += opaqueAt(alone[i], p);
        require(opaque > 0, std::string(kNames[i]) + " draws something");
    }

    std::size_t shared = 0, differing = 0;
    for (std::size_t p = 0; p < kPixels; ++p) {
        int owners = 0;
        const std::uint8_t zero[4] = {0, 0, 0, 0};
        const std::uint8_t* joined = zero;
        for (const Canvas& canvas : alone) {
            if (!opaqueAt(canvas, p)) continue;
            ++owners;
            joined = &canvas.rgba()[p * 4];
        }
        const std::uint8_t* expected = &full.rgba()[p * 4];
        if (owners > 1) ++shared;
        else if (!std::equal(expected, expected + 4, joined)) ++differing;
    }
    require(shared == 0, "the parts leave disjoint opaque pixels, shared: " + std::to_string(shared));
    require(differing == 0, "the parts together equal the full draw, differing pixels: " + std::to_string(differing));

    const Canvas none = drawn(hud, HudPartSet(), spin);
    std::size_t drawnByNone = 0;
    for (std::size_t p = 0; p < kPixels; ++p) drawnByNone += opaqueAt(none, p);
    require(drawnByNone == 0, "the empty set draws nothing");
}

}

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "usage: hud-parts-real-test FIB CSV\n";
        return 2;
    }
    if (!std::ifstream(argv[1]).good() || !std::ifstream(argv[2]).good()) {
        std::cout << "hud-parts-real-test: no game data, skipping\n";
        return 77;
    }
    try {
        assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        Resources resources(archive);
        scene(resources);
    } catch (const std::exception& error) {
        std::cerr << "hud-parts-real-test: " << error.what() << '\n';
        return 1;
    }
    if (failures) return 1;
    std::cout << "hud-parts-real-test: ok\n";
    return 0;
}
