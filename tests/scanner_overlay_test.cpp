// Layout data of the audio scanner overlay (sprites/blend/DRC_Scanner) and its drawing (src/frontend/scanner_overlay.cpp). Needs the extracted
// RomFS; exits with 77 without it. Values read with the layout decoder from lego_city3ds.fib (docs/re/abilities-scanner.md).
#include "frontend/scanner_overlay.hpp"
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace chase;

namespace {
void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

const assets::UiElement* find(const assets::UiElement& root, const std::string& name) {
    if (root.name == name) return &root;
    for (const assets::UiElement& child : root.children)
        if (const assets::UiElement* found = find(child, name)) return found;
    return nullptr;
}

float boardLength(const assets::UiLayout& layout, const std::string& name) {
    float length = 0;
    for (const assets::UiStoryboard& board : layout.storyboards)
        if (board.name == name)
            for (const assets::UiTrack& track : board.tracks) length = std::max(length, track.duration);
    return length;
}

int litPixels(const frontend::Canvas& canvas) {
    int count = 0;
    for (int y = 0; y < canvas.height(); ++y)
        for (int x = 0; x < canvas.width(); ++x)
            if (canvas.pixel(x, y)[3] != 0) ++count;
    return count;
}
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: scanner-overlay-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "scanner-overlay-test: data not available, skipped\n";
        return 77;
    }
    try {
        assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        frontend::Resources resources(archive);
        const assets::UiLayout& main = resources.layout(frontend::kScannerLayout);
        require(main.root.size[0] == 1024.0f && main.root.size[1] == 512.0f, "drc_scanner.bxaml is a 1024 x 512 layout (the screen is 400 x 240)");
        const assets::UiElement* reticle = find(main.root, "Reticule_Audio");
        require(reticle && reticle->position[0] == 262.0f && reticle->position[1] == 128.0f && reticle->size[0] == 500.0f && reticle->size[1] == 256.0f,
                "Reticule_Audio sits at 262, 128 with size 500 x 256: its centre is the screen centre at half scale (262 + 250 = 512 = 1024 / 2)");
        require(frontend::kScannerLayoutScale * (262.0f + 250.0f) + (400.0f - 1024.0f * frontend::kScannerLayoutScale) / 2 == 200.0f, "reticle centre x on the 400 pixel screen");
        require(boardLength(resources.layout("sprites/blend/drc_scanner/zoom_meter.bxaml"), "Zoom") == 3.0f, "storyboard Zoom is 3 s long (0x1d99a8 multiplies the zoom fraction by its length)");
        const assets::UiLayout& reticleLayout = resources.layout(frontend::kScannerReticleLayout);
        require(std::fabs(boardLength(reticleLayout, "Ret_Focus") - 0.26f) < 1e-4f, "Ret_Focus is 0.26 s long");
        require(std::fabs(boardLength(reticleLayout, "Ret_LockingOn") - 2.26f) < 1e-4f, "Ret_LockingOn is 2.26 s long");
        require(boardLength(resources.layout(frontend::kScannerMeterLayout), "Scanning") == 3.0f, "storyboard Scanning of ScanMeter is 3 s long");

        runtime::ScannerReticle state;
        runtime::ScannerReticleInput input;
        frontend::Canvas idle(400, 240);
        idle.clearTransparent();
        frontend::drawScannerOverlay(idle, resources, state.update(input), 0.0f);
        const int idlePixels = litPixels(idle);
        require(idlePixels > 3000, "the frame and the meters are drawn: " + std::to_string(idlePixels));
        input.state = 5;
        input.scanProgress = 0.5f;
        input.hasCurrent = true;
        input.currentScreen[0] = 200.0f;
        input.currentScreen[1] = 120.0f;
        frontend::Canvas scanning(400, 240);
        scanning.clearTransparent();
        frontend::drawScannerOverlay(scanning, resources, state.update(input), 0.0f);
        require(litPixels(scanning) > idlePixels + 200, "scanning adds the reticle rings and the scan meter");

        require(resources.tableText(frontend::kScannerInfoIconTable, 0, 13, 1).find("ui_scanner_dog.png") != std::string::npos,
                "tablespoi.bxls sheet 0 row 13 is the dog icon (chase-tabledump tables/tablespoi.bxls; class value InfoIcon 13 of 1-2_Dog_Prop)");
        require(std::fabs(boardLength(resources.layout(frontend::kScannerInfoLayouts[0]), "InfoOn") - runtime::kScannerInfoOnSeconds) < 1e-4f &&
                    std::fabs(boardLength(resources.layout(frontend::kScannerInfoLayouts[1]), "InfoOff") - runtime::kScannerInfoOffSeconds) < 1e-4f,
                "InfoOn is 0.7 s and InfoOff 0.2 s long (chase-layoutdump --tracks info_top.bxaml)");
        runtime::ScannerInfoPose shownInfo;
        shownInfo.box = runtime::ScannerInfoBox::Top;
        shownInfo.opacity = 1.0f;
        shownInfo.time = runtime::kScannerInfoOnSeconds;
        shownInfo.icon = 13;
        frontend::Canvas withInfo(400, 240);
        withInfo.clearTransparent();
        runtime::ScannerReticle plain;
        const runtime::ScannerReticlePose basePose = plain.update(runtime::ScannerReticleInput());
        frontend::drawScannerOverlay(withInfo, resources, basePose, 0.0f, {}, shownInfo);
        frontend::Canvas without(400, 240);
        without.clearTransparent();
        frontend::drawScannerOverlay(without, resources, basePose, 0.0f);
        require(litPixels(withInfo) > litPixels(without) + 500, "the info box adds its hexagon and icon");
    } catch (const std::exception& error) {
        std::cerr << "scanner-overlay-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "scanner-overlay-test: ok\n";
}
