#include "frontend/minimap.hpp"
#include "frontend/layout.hpp"
#include <algorithm>
#include <cmath>
#include <utility>

namespace chase::frontend {

namespace {

constexpr const char* kButtonsLayout = "sprites/blend/mapbuttons/mapbuttons.bxaml";

void sampleBilinear(const assets::Image& image, float u, float v, float out[4]) {
    const float fu = u - 0.5f, fv = v - 0.5f;
    const int x0 = int(std::floor(fu)), y0 = int(std::floor(fv));
    const float ax = fu - float(x0), ay = fv - float(y0);
    float texel[4][4];
    for (int i = 0; i < 4; ++i) {
        const int x = std::clamp(x0 + (i & 1), 0, image.width - 1), y = std::clamp(y0 + (i >> 1), 0, image.height - 1);
        const std::uint8_t* t = &image.rgba[(std::size_t(y) * image.width + x) * 4];
        for (int c = 0; c < 4; ++c) texel[i][c] = t[c];
    }
    for (int c = 0; c < 4; ++c)
        out[c] = (texel[0][c] * (1 - ax) + texel[1][c] * ax) * (1 - ay) + (texel[2][c] * (1 - ax) + texel[3][c] * ax) * ay;
}


void drawRing(Canvas& bottom, float centreX, float centreY, const MapRing& ring) {
    constexpr float kTwoPi = 6.2831855f;
    auto edge = [](float distance) { return std::clamp(distance * 0.05f, 0.0f, 1.0f); };
    for (int i = 0; i < kRingSegments; ++i) {
        const float a0 = float(i) * kTwoPi * 0.015625f, a1 = float(i + 1) * kTwoPi * 0.015625f;
        const float x0 = centreX + std::cos(a0) * ring.radius, y0 = centreY + std::sin(a0) * ring.radius;
        const float x1 = centreX + std::cos(a1) * ring.radius, y1 = centreY + std::sin(a1) * ring.radius;
        const float visible = edge(x1) * edge(kMapScreenWidth - x0) * edge(y1) * edge(kMapScreenHeight - y0);
        bottom.drawLine(x0, y0, x1, y1, kRingLineWidth, {1, 1, 1, ring.alpha * visible});
    }
}

void drawGroupTargets(Canvas& bottom, Resources& resources, const MinimapView& view) {
    for (const MinimapGroupTargets& group : view.groupTargets) {
        const char* texture = mapIconTexture(group.iconType);
        if (!texture || !(0.0f < group.alpha)) continue;
        const assets::Image& icon = resources.image(texture);
        const float alpha = float(int(group.alpha * 255.0f)) / 255.0f;
        for (const std::array<float, 2>& target : group.targets) {
            const MapMarkerPlacement placement = placeMapMarker(view.x, view.z, view.cameraYaw, target[0], target[1]);
            bottom.drawScaled(icon, {0, 0, icon.width, icon.height}, placement.x - kGroupTargetHalfSize, placement.y - kGroupTargetHalfSize,
                              kGroupTargetHalfSize * 2, kGroupTargetHalfSize * 2, {1, 1, 1, alpha});
        }
    }
}

void drawGroupSlots(Canvas& bottom, Resources& resources, const MinimapView& view) {
    for (const MinimapGroupSlot& slot : view.groupSlots) {
        const char* texture = mapIconTexture(slot.iconType);
        if (!texture) continue;
        const assets::Image& icon = resources.image(texture);
        const float alpha = float(int(slot.alpha * 255.0f) & 0xff) / 255.0f;
        const MapMarkerPlacement placement = placeMapMarker(view.x, view.z, view.cameraYaw, slot.x, slot.z);
        float shownX = float(roundedCoordinate(placement.x)), shownY = float(roundedCoordinate(placement.y));
        if (placement.outsideEllipse) {
            const assets::Image& arrow = resources.image(mapOffscreenArrowTexture(slot.iconType));
            const float arrowX = placement.edgeX + placement.directionX * kArrowOffset, arrowY = placement.edgeY + placement.directionY * kArrowOffset;
            bottom.drawRotated(arrow, {0, 0, arrow.width, arrow.height}, arrowX, arrowY, float(arrow.width), float(arrow.height),
                               arrowRotation(placement), {1, 1, 1, alpha});
            shownX = float(roundedCoordinate(placement.edgeX));
            shownY = float(roundedCoordinate(placement.edgeY));
        }
        const float half = float(icon.width) * 0.5f;
        bottom.drawScaled(icon, {0, 0, icon.width, icon.height}, shownX - half, shownY - half, half * 2, half * 2, {1, 1, 1, alpha});
    }
}

void drawMarkers(Canvas& bottom, Resources& resources, const MinimapView& view) {
    if (view.markers.empty() || !view.markerField) return;
    const assets::Image& icon = resources.image("sprites/ui_bottomscreen/maps/waypoint.btga");
    const assets::Image& arrow = resources.image("sprites/ui_bottomscreen/maps/waypointoffscreen.btga");
    const MapMarkerField& field = *view.markerField;
    const float half = float(icon.width) * 0.5f;
    for (const MinimapMarker& marker : view.markers) {
        const MapMarkerPlacement placement = placeMapMarker(view.x, view.z, view.cameraYaw, marker.x, marker.z);
        const MapMarkerAnimation& animation = field.animation(marker.id);
        float shownX = float(roundedCoordinate(placement.x)), shownY = float(roundedCoordinate(placement.y));
        const float haloHalf = half * (1.0f - (animation.scale - 0.5f));
        bottom.drawScaled(icon, {0, 0, icon.width, icon.height}, shownX - haloHalf, shownY - haloHalf, haloHalf * 2, haloHalf * 2);
        if (field.ringsActive()) {
            drawRing(bottom, placement.x, placement.y, field.largeRing());
            drawRing(bottom, placement.x, placement.y, field.smallRing());
        }
        if (placement.outsideEllipse) {
            const float arrowX = placement.edgeX + placement.directionX * kArrowOffset, arrowY = placement.edgeY + placement.directionY * kArrowOffset;
            bottom.drawRotated(arrow, {0, 0, arrow.width, arrow.height}, arrowX, arrowY, float(arrow.width), float(arrow.height),
                               arrowRotation(placement), {1, 1, 1, animation.grow});
            shownX = float(roundedCoordinate(placement.edgeX));
            shownY = float(roundedCoordinate(placement.edgeY));
        }
        const float shownHalf = half * animation.scale;
        bottom.drawScaled(icon, {0, 0, icon.width, icon.height}, shownX - shownHalf, shownY - shownHalf, shownHalf * 2, shownHalf * 2,
                          {1, 1, 1, animation.grow});
    }
}

}

void drawMinimap(Canvas& bottom, Resources& resources, const std::string& levelName, const MinimapView& view) {
    bottom.clear(0, 0, 0);
    const std::string mapPath = "sprites/ui_bottomscreen/maps/" + levelName + ".btga";
    if (resources.has(mapPath)) {
        const assets::Image& map = resources.image(mapPath);
        const float c = std::cos(view.cameraYaw), s = std::sin(view.cameraYaw);
        const float playerU = kMapOriginU + kMapTexelsPerUnit * view.x, playerV = kMapOriginV - kMapTexelsPerUnit * view.z;
        for (int y = 0; y < bottom.height(); ++y) {
            for (int x = 0; x < bottom.width(); ++x) {
                const float sx = float(x) + 0.5f - kMapPlayerScreenX, sy = float(y) + 0.5f - kMapPlayerScreenY;
                const float dx = c * sx - s * sy, dz = -s * sx - c * sy;
                const float u = playerU + kMapTexelsPerUnit * dx / kMapPixelsPerUnit, v = playerV - kMapTexelsPerUnit * dz / kMapPixelsPerUnit;
                if (u < 0 || v < 0 || u >= float(map.width) || v >= float(map.height)) continue;
                float texel[4];
                sampleBilinear(map, u * float(map.width) / 1024.0f, v * float(map.height) / 1024.0f, texel);
                std::uint8_t* out = bottom.pixel(x, y);
                out[0] = std::uint8_t(std::lround(texel[0]));
                out[1] = std::uint8_t(std::lround(texel[1]));
                out[2] = std::uint8_t(std::lround(texel[2]));
            }
        }
    }
    const assets::Image& arrow = resources.image("sprites/ui_bottomscreen/maps/playericon.btga");
    bottom.drawRotated(arrow, {0, 0, arrow.width, arrow.height}, kMapPlayerScreenX, kMapPlayerScreenY, float(arrow.width),
                       float(arrow.height), view.playerYaw - view.cameraYaw);
    drawGroupTargets(bottom, resources, view);
    drawMarkers(bottom, resources, view);
    drawGroupSlots(bottom, resources, view);
    if (view.buttons) {
        drawLayout(bottom, resources, kButtonsLayout, *view.buttons, 0, 0);
        return;
    }
    LayoutScene scene;
    scene.hidden = {"blackBG", "buttonScan", "buttonExit", "buttonAudio", "buttonRedBricks", "buttonHint", "buttonFreeplay",
                    "portrait_0", "portrait_1", "portrait_2", "portrait_3", "portrait_4", "portrait_5", "portrait_6", "portrait_7",
                    "area_0", "area_1", "area_2", "area_3", "area_4", "area_5", "area_exit", "textWindow"};
    scene.referenceStoryboards["buttonChars"] = "default";
    if (view.scannerButtons.stage != runtime::ScannerBottomStage::Map) {
        scene.hidden.erase("buttonExit");
        scene.hidden.erase("buttonAudio");
        scene.images["buttonAudio/icon"] = "sprites/blend/mapbuttons/images/buttonicon_audio.bpng";
        scene.images["buttonExit/icon"] = "sprites/blend/mapbuttons/images/buttonicon_exit.bpng";
        const std::pair<const char*, const runtime::ScannerBottomButton*> buttons[] = {
            {"buttonChars", &view.scannerButtons.chars}, {"buttonExit", &view.scannerButtons.exit}, {"buttonAudio", &view.scannerButtons.audio}};
        for (const auto& [name, button] : buttons) {
            scene.referenceStoryboards[name] = button->look;
            scene.referenceTimes[name] = button->lookTime;
            if (button->slide) scene.laterStoryboards.push_back({button->slide, button->slideTime});
        }
    }
    drawLayout(bottom, resources, kButtonsLayout, scene, 0, 0);
}

}
