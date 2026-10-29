#include "render/overlay_quad.hpp"

namespace chase::render {

OverlayQuad overlayQuad(const PixelRect& rect, int windowWidth, int windowHeight) {
    const float left = 2.0f * float(rect.x) / float(windowWidth) - 1, right = 2.0f * float(rect.x + rect.width) / float(windowWidth) - 1;
    const float bottom = 2.0f * float(rect.y) / float(windowHeight) - 1, top = 2.0f * float(rect.y + rect.height) / float(windowHeight) - 1;
    return {{left, top, 0, 0, right, top, 1, 0, left, bottom, 0, 1, right, bottom, 1, 1}};
}

}
