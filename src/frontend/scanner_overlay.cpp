#include "frontend/scanner_overlay.hpp"
#include <algorithm>
#include <cmath>

namespace chase::frontend {

namespace {

float boardLength(Resources& resources, const std::string& layoutPath, const std::string& storyboard) {
    float length = 0;
    for (const assets::UiStoryboard& board : resources.layout(layoutPath).storyboards) {
        if (board.name != storyboard) continue;
        for (const assets::UiTrack& track : board.tracks) length = std::max(length, track.duration * float(std::max<int>(1, track.repeat)));
    }
    return length;
}

}

void drawScannerOverlay(Canvas& top, Resources& resources, const runtime::ScannerReticlePose& pose, float seconds) {
    const float originX = (float(top.width()) - kScannerLayoutWidth * kScannerLayoutScale) / 2;
    const float originY = (float(top.height()) - kScannerLayoutHeight * kScannerLayoutScale) / 2;

    LayoutScene frame;
    frame.rotations = true;
    frame.hidden = {"Physical", "Sonar", "info_top", "info_bot", "Signal_Meter", "Reticule_Audio"};
    frame.referenceStoryboards["Zoom_Meter"] = "Zoom";
    frame.referenceTimes["Zoom_Meter"] = pose.zoomMeterFraction * boardLength(resources, "sprites/blend/drc_scanner/zoom_meter.bxaml", "Zoom");
    drawLayout(top, resources, kScannerLayout, frame, originX, originY, kScannerLayoutScale);

    const float offsetX = (pose.position[0] - runtime::kScannerScreenCentre[0]);
    const float offsetY = (pose.position[1] - runtime::kScannerScreenCentre[1]);
    const float reticleX = originX + offsetX + 262.0f * kScannerLayoutScale;
    const float reticleY = originY + offsetY + 128.0f * kScannerLayoutScale;

    LayoutScene reticle;
    reticle.rotations = true;
    reticle.storyboard = pose.lockingOn.name;
    reticle.time = pose.lockingOn.fraction * boardLength(resources, kScannerReticleLayout, pose.lockingOn.name);
    if (!pose.focus.name.empty())
        reticle.laterStoryboards.push_back({pose.focus.name, pose.focus.fraction * boardLength(resources, kScannerReticleLayout, pose.focus.name)});
    reticle.laterStoryboards.push_back({"Reticule_Spinner", std::fmod(seconds, kScannerReticleSpinnerSeconds)});
    drawLayout(top, resources, kScannerReticleLayout, reticle, reticleX, reticleY, kScannerLayoutScale);

    if (pose.scanMeterShown) {
        LayoutScene meter;
        meter.rotations = true;
        meter.storyboard = "Scanning";
        meter.time = pose.scanMeterFraction * boardLength(resources, kScannerMeterLayout, "Scanning");
        drawLayout(top, resources, kScannerMeterLayout, meter, reticleX + (500.0f - 350.0f) * 0.5f * kScannerLayoutScale,
                   reticleY + (256.0f - 350.0f) * 0.5f * kScannerLayoutScale, kScannerLayoutScale);
    }
}

}
