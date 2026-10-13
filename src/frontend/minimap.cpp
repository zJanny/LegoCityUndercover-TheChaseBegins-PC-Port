#include "frontend/minimap.hpp"
#include "frontend/layout.hpp"
#include <algorithm>
#include <cmath>

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
                const float u = playerU + kMapTexelsPerUnit * dx, v = playerV - kMapTexelsPerUnit * dz;
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
    LayoutScene scene;
    scene.hidden = {"blackBG", "buttonScan", "buttonExit", "buttonAudio", "buttonRedBricks", "buttonHint", "buttonFreeplay",
                    "portrait_0", "portrait_1", "portrait_2", "portrait_3", "portrait_4", "portrait_5", "portrait_6", "portrait_7",
                    "area_0", "area_1", "area_2", "area_3", "area_4", "area_5", "area_exit", "textWindow"};
    scene.referenceStoryboards["buttonChars"] = "default";
    drawLayout(bottom, resources, kButtonsLayout, scene, 0, 0);
}

}
