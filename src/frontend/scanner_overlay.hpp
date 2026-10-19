#pragma once
// Top-screen overlay of the audio scanner view (sprites/blend/DRC_Scanner, docs/re/abilities-scanner.md): frame and headphone icon, zoom
// meter, reticle, scan meter and info boxes (runtime/scanner_info.hpp). No SDL dependency.
// STAND-INS (SCAN-S5, SCAN-S6): the layout is drawn at half scale centred on the screen (read from the 3DS screenshot and the
// LayoutRoot scale 0.5 set by 0x20f540); the layers of the layout that depend on unread state (physical reticle, sonar segments,
// satellite meter, button prompt) are not drawn; the Reticule_Spinner loop starts with the view; the animation evaluator is the
// data fit of layout.hpp with rotation.
#include "frontend/layout.hpp"
#include "runtime/scanner_info.hpp"
#include "runtime/scanner_markers.hpp"
#include "runtime/scanner_reticle.hpp"

namespace chase::frontend {

constexpr const char* kScannerLayout = "sprites/blend/drc_scanner/drc_scanner.bxaml";
constexpr const char* kScannerReticleLayout = "sprites/blend/drc_scanner/reticule_audio.bxaml";
constexpr const char* kScannerMeterLayout = "sprites/blend/drc_scanner/scanmeter.bxaml";
constexpr float kScannerLayoutScale = 0.5f;
constexpr float kScannerLayoutWidth = 1024.0f, kScannerLayoutHeight = 512.0f;
constexpr const char* kScannerInfoLayouts[2] = {"sprites/blend/drc_scanner/info_top.bxaml", "sprites/blend/drc_scanner/info_bot.bxaml"};
constexpr const char* kScannerInfoIconTable = "tables/tablespoi.bxls";
constexpr float kScannerReticleSpinnerSeconds = 6.0f;

constexpr const char* kScannerCentreLayout = "sprites/blend/drc_scanner/soundcentre.bxaml";
constexpr const char* kScannerPulseLayout = "sprites/blend/drc_scanner/soundpulse.bxaml";
constexpr float kScannerCentreSize = 32.0f, kScannerPulseSize = 256.0f;

void drawScannerOverlay(Canvas& top, Resources& resources, const runtime::ScannerReticlePose& pose, float seconds,
                        const std::vector<runtime::ScannerMarkerPose>& markers = {}, const runtime::ScannerInfoPose& info = {});

}
