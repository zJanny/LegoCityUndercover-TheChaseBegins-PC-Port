#include "frontend/scanner_overlay.hpp"
#include <algorithm>
#include <cctype>
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

void drawMarkers(Canvas& top, Resources& resources, const std::vector<runtime::ScannerMarkerPose>& markers) {
    for (const runtime::ScannerMarkerPose& marker : markers) {
        if (!marker.shown || marker.opacity <= 0) continue;
        const float half = kScannerLayoutScale;
        if (marker.pulsesShown)
            for (std::size_t k = 0; k < 3; ++k) {
                if (!marker.pulsePlaying[k]) continue;
                LayoutScene pulse;
                pulse.rotations = true;
                pulse.opacity = marker.opacity;
                pulse.storyboard = "Pulse";
                pulse.time = marker.pulseTime[k];
                drawLayout(top, resources, kScannerPulseLayout, pulse, marker.screen[0] - kScannerPulseSize * half / 2,
                           marker.screen[1] - kScannerPulseSize * half / 2, kScannerLayoutScale);
            }
        LayoutScene centre;
        centre.rotations = true;
        centre.opacity = marker.opacity;
        centre.storyboard = "Pulsing";
        centre.time = marker.centreTime;
        drawLayout(top, resources, kScannerCentreLayout, centre, marker.screen[0] - kScannerCentreSize * half / 2,
                   marker.screen[1] - kScannerCentreSize * half / 2, kScannerLayoutScale);
    }
}

const assets::UiElement* findElement(const assets::UiElement& root, const std::string& name) {
    if (root.name == name) return &root;
    for (const assets::UiElement& child : root.children)
        if (const assets::UiElement* found = findElement(child, name)) return found;
    return nullptr;
}

std::string iconTexture(Resources& resources, int icon) {
    std::string path = resources.tableText(kScannerInfoIconTable, 0, std::size_t(icon), 1);
    for (char& c : path) c = char(std::tolower(static_cast<unsigned char>(c)));
    if (path.size() > 4 && path.compare(path.size() - 4, 4, ".png") == 0) path.replace(path.size() - 4, 4, ".bpng");
    return path;
}

void drawInfo(Canvas& top, Resources& resources, const runtime::ScannerInfoPose& info, float originX, float originY) {
    if (info.box == runtime::ScannerInfoBox::None) return;
    const bool upper = info.box == runtime::ScannerInfoBox::Top;
    const std::string name = upper ? "info_top" : "info_bot";
    const assets::UiElement* placed = findElement(resources.layout(kScannerLayout).root, name);
    if (!placed) return;
    const std::string folder = "sprites/blend/drc_scanner/images/";
    LayoutScene scene;
    scene.rotations = true;
    scene.opacity = info.opacity;
    scene.storyboard = info.leaving ? "InfoOff" : "InfoOn";
    scene.time = info.time;
    scene.images[name + "_box"] = folder + "info_box_green.bpng";
    scene.images[name + "_i"] = folder + "info_i_green.bpng";
    scene.images[name + "_object_icon"] = iconTexture(resources, info.icon);
    drawLayout(top, resources, kScannerInfoLayouts[upper ? 0 : 1], scene, originX + placed->position[0] * kScannerLayoutScale + info.offset[0],
               originY + placed->position[1] * kScannerLayoutScale + info.offset[1], kScannerLayoutScale);
}

}

void drawScannerOverlay(Canvas& top, Resources& resources, const runtime::ScannerReticlePose& pose, float seconds,
                        const std::vector<runtime::ScannerMarkerPose>& markers, const runtime::ScannerInfoPose& info) {
    const float originX = (float(top.width()) - kScannerLayoutWidth * kScannerLayoutScale) / 2;
    const float originY = (float(top.height()) - kScannerLayoutHeight * kScannerLayoutScale) / 2;

    LayoutScene frame;
    frame.rotations = true;
    frame.hidden = {"Physical", "Sonar", "info_top", "info_bot", "Signal_Meter", "Reticule_Audio"};
    frame.referenceStoryboards["Zoom_Meter"] = "Zoom";
    frame.referenceTimes["Zoom_Meter"] = pose.zoomMeterFraction * boardLength(resources, "sprites/blend/drc_scanner/zoom_meter.bxaml", "Zoom");
    drawLayout(top, resources, kScannerLayout, frame, originX, originY, kScannerLayoutScale);
    drawMarkers(top, resources, markers);

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
    drawInfo(top, resources, info, originX, originY);
}

}
