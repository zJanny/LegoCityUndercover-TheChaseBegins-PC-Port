// The in-game HUD against the real game data and the recorded oracle trace (tools/oracle/hud-bar.txt): the stud bar's
// timeline, the counter sink, the icon bounce and renders of the HUD and the map screen. Arguments: fib, names csv, trace
// directory. Exit code 77 when the game data is missing; the trace comparison is skipped when the trace is missing.
#include "frontend/hud.hpp"
#include "frontend/minimap.hpp"
#include <cmath>
#include <cstdint>
#include <cstring>
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

std::uint32_t fnv1a(const std::vector<std::uint8_t>& bytes) {
    std::uint32_t hash = 2166136261u;
    for (const std::uint8_t byte : bytes) hash = (hash ^ byte) * 16777619u;
    return hash;
}

// The bar object states of update 25..37 of the recorded stud pickup (bar1.mem, byte +0x10 of the object at 0x519760): 1 turning
// on, 2 shown, 3 turning off, 0 hidden. The value 10 was banked in update 25. The game clock advanced 0.2667 s per stepped update
// (the update-time field +0x28 went 228.733, 229.267, 231.4), i.e. the ON storyboard (0.5 s) ended at update 27, the hold of 2.0 s
// at update 35 and the OFF storyboard (0.5 s) at update 37.
constexpr float kRecordedClockStep = 0.26667f;
constexpr int kBarStates[13] = {1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 0};

void barTimeline(Resources& resources, const std::string& traceDirectory) {
    Hud hud(resources, {});
    for (int update = 0; update < 25; ++update) hud.update(kRecordedClockStep);
    require(hud.studBarState() == StudBarState::Hidden, "the bar is hidden before the first pickup");
    hud.update(kRecordedClockStep);
    hud.studsBanked(0, 10);
    for (int row = 0; row < 13; ++row) {
        if (row > 0) hud.update(kRecordedClockStep);
        require(int(hud.studBarState()) == kBarStates[row], "bar state at recorded update " + std::to_string(25 + row));
    }

    std::ifstream in(traceDirectory + "/bar1.mem", std::ios::binary);
    if (!in) {
        std::cout << "hud-real-test: no trace " << traceDirectory << "/bar1.mem, skipping the comparison with it\n";
        return;
    }
    constexpr std::size_t kRecordBytes = 208, kStateOffset = 8 + 8 + 0x10, kHeartsOffset = 8 + 8 + 112 + 76;
    const std::vector<char> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    require(bytes.size() == 160 * kRecordBytes, "trace size");
    if (bytes.size() != 160 * kRecordBytes) return;
    for (int row = 0; row < 13; ++row)
        require(int(std::uint8_t(bytes[std::size_t(25 + row) * kRecordBytes + kStateOffset])) == kBarStates[row],
                "the pinned state matches the trace at update " + std::to_string(25 + row));
    require(std::uint8_t(bytes[kHeartsOffset]) == 4, "the HUD showed four hearts in the recorded state");
}

void sinkAndBounce(Resources& resources) {
    Hud hud(resources, {});
    // 0x58ef4c at the gameplay start (oracle): x = 0x434b0000 = 203.0, y = 0xc1680000 = -14.5 (the centre of Studs_Count)
    require(hud.studSink().x == 203.0f && hud.studSink().y == -14.5f, "initial counter sink");
    require(hud.studIconSize() == 22.0f, "icon size without bounce");
    for (int update = 0; update < 10; ++update) hud.update(1.0f / 30);
    hud.studsBanked(1, 110);
    // 0x166e08: the icon grows by 8 * sin(0.2 pi n) for the five updates n = 0..4 after the change, then returns to 22.
    const float expected[6] = {22.0f, 22.0f + 4.7022f, 22.0f + 7.6085f, 22.0f + 7.6085f, 22.0f + 4.7022f, 22.0f};
    for (int n = 0; n < 6; ++n) {
        if (n > 0) hud.update(1.0f / 30);
        require(std::fabs(hud.studIconSize() - expected[n]) < 1e-3f, "icon size at update " + std::to_string(n) + " after the change");
    }
    Canvas top(kTopWidth, kScreenHeight);
    top.clearTransparent();
    for (int n = 0; n < 40; ++n) hud.update(1.0f / 30);
    hud.draw(top, 0.0);
    require(hud.studSink().y > 0, "after a draw the sink is the icon centre on screen");
}

void hearts(Resources& resources) {
    Hud hud(resources, {});
    require(hud.hearts() == 4, "full health is four hearts: the class attribute Health defaults to 4 (0x236e44)");
    hud.setHearts(3);
    require(hud.hearts() == 3, "three hearts after losing one");
    hud.setHearts(9);
    require(hud.hearts() == 6, "at most six hearts (0x2316b4 limits to 6)");
}

void renders(Resources& resources) {
    // Recorded with chase-hudshot at the commit that added this test. The HUD pixels agree with the oracle screenshots
    // (artifacts/review/hud, docs/re/hud.md): mean abs difference over the opaque pixels 6.7 for the portrait, 2.5 for the
    // hearts and 5.8 for the stud number.
    Hud hud(resources, {});
    Canvas top(kTopWidth, kScreenHeight);
    std::vector<std::pair<std::string, std::uint32_t>> got;
    auto shot = [&](const std::string& name, double spin) {
        top.clearTransparent();
        hud.draw(top, spin);
        got.emplace_back(name, fnv1a(top.rgba()));
    };
    for (int update = 0; update < 90; ++update) hud.update(1.0f / 30);
    shot("settled", 0.0);
    require(top.pixel(32, 32)[3] == 255, "the portrait is opaque");
    require(top.pixel(300, 100)[3] == 0, "the rest of the screen stays transparent");
    hud.studsBanked(0, 10);
    for (int update = 0; update < 6; ++update) hud.update(1.0f / 30);
    shot("bar-on", 0.5);
    for (int update = 0; update < 20; ++update) hud.update(1.0f / 30);
    shot("bar-shown", 1.0);
    const std::pair<const char*, std::uint32_t> expected[] = {{"settled", 0x678225e3}, {"bar-on", 0x3df78606}, {"bar-shown", 0xed449703}};
    require(got.size() == 3, "three renders");
    for (std::size_t i = 0; i < got.size() && i < 3; ++i) {
        require(got[i].first == expected[i].first, "render order");
        require(got[i].second == expected[i].second, "render hash " + got[i].first);
    }
}

void minimap(Resources& resources) {
    Canvas bottom(kBottomWidth, kScreenHeight);
    MinimapView view;
    view.x = 886.764f;
    view.z = 1111.166f;
    view.cameraYaw = -2.3501f;
    view.playerYaw = -2.35f;
    drawMinimap(bottom, resources, "lvl_01_cherrytreehills", view);
    const std::uint8_t* corner = bottom.pixel(10, 200);
    require(corner[0] + corner[1] + corner[2] > 100, "the map covers the screen");
    const std::uint8_t* arrow = bottom.pixel(160, 120);
    require(arrow[2] > arrow[0] + 30, "the player's arrow is blue in the middle of the screen");

    // Provenance: oracle state gameplay-start, player teleported to (700, 16.6, 900), shotloop picture arrowA_201 (OpenGL, RPC):
    // camera look (-0.6548, -0.7370); the blue pixels (74, 174, 255) below row 180 of the bottom screen number 88 with centroid
    // (172.74, 222.08).
    MinimapView edge;
    edge.x = 700.0f;
    edge.z = 900.0f;
    edge.cameraYaw = -2.4152f;
    edge.playerYaw = -2.4152f;
    edge.markers = {{0, 817.35f, 1071.20f}};
    MapMarkerField field;
    for (int update = 0; update < 45; ++update) field.advance(1.0f / 30, edge.x, edge.z, edge.cameraYaw, edge.markers);
    field.setRingClock(2.5f);
    edge.markerField = &field;
    drawMinimap(bottom, resources, "lvl_01_cherrytreehills", edge);
    int blue = 0;
    double sumX = 0, sumY = 0;
    for (int y = 181; y < 240; ++y)
        for (int x = 0; x < 320; ++x) {
            const std::uint8_t* p = bottom.pixel(x, y);
            if (std::abs(p[0] - 74) < 8 && std::abs(p[1] - 174) < 8 && std::abs(p[2] - 255) < 8) {
                ++blue;
                sumX += x;
                sumY += y;
            }
        }
    require(blue == 88, "pixels of the marker blue at the screen edge (oracle 88)");
    require(blue > 0 && std::fabs(sumX / blue - 172.74) < 0.05 && std::fabs(sumY / blue - 222.08) < 0.05,
            "the edge marker and its arrow sit where the oracle shows them (centroid 172.74, 222.08)");
    drawMinimap(bottom, resources, "lvl_missing", view);
    require(bottom.pixel(10, 200)[0] == 0 && bottom.pixel(10, 200)[1] == 0, "a level without a map shows no map");
}
}

int main(int argc, char** argv) {
    if (argc < 4) {
        std::cerr << "usage: hud-real-test FIB CSV TRACE_DIR\n";
        return 2;
    }
    if (!std::ifstream(argv[1]).good() || !std::ifstream(argv[2]).good()) {
        std::cout << "hud-real-test: no game data, skipping\n";
        return 77;
    }
    try {
        assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        Resources resources(archive);
        barTimeline(resources, argv[3]);
        sinkAndBounce(resources);
        hearts(resources);
        renders(resources);
        minimap(resources);
    } catch (const std::exception& error) {
        std::cerr << "hud-real-test: " << error.what() << '\n';
        return 1;
    }
    if (failures) return 1;
    std::cout << "hud-real-test: ok\n";
    return 0;
}
