#include "scanner_render.hpp"

namespace chase::runtime {

ScannerRender scannerRender(bool audioModeEntered) {
    ScannerRender out;
    if (!audioModeEntered) return out;
    out.active = true;
    out.world.flags = kScannerWorldOverrideFlags;
    out.world.mode = kScannerOverrideMode;
    out.world.texture = 0;
    out.world.extra = 0;
    out.world.constant = kScannerOverrideConstant;
    out.drawList = out.world;
    out.drawList.flags = kScannerDrawListOverrideFlags;
    out.overrideSky = true;
    out.noFog = true;
    out.glowStrength = kScannerGlowStrength;
    out.glowSecond = kScannerGlowSecond;
    return out;
}

}
