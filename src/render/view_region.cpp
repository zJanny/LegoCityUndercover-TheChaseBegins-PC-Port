#include "render/view_region.hpp"
#include <stdexcept>

namespace chase::render {

ViewRegion originalViewRegion(const PixelRect& rect) { return {rect, kTopScreenAspectWidth, kTopScreenAspectHeight}; }

ViewRegion makeViewRegion(const PixelRect& rect, int aspectWidth, int aspectHeight) {
    if (rect.width <= 0 || rect.height <= 0) throw std::invalid_argument("view region: rectangle size must be positive");
    if (aspectWidth <= 0 || aspectHeight <= 0) throw std::invalid_argument("view region: aspect terms must be positive");
    return {rect, aspectWidth, aspectHeight};
}

float viewAspect(const ViewRegion& region) { return float(region.aspectWidth) / float(region.aspectHeight); }

void beginViewRegion(int windowWidth, int windowHeight, const ViewRegion& region, const ClearColour& inside) {
    setScissorTest(false);
    setViewport({0, 0, windowWidth, windowHeight});
    clearWindow({0, 0, 0, 1}, true);
    setScissorTest(true);
    setScissorBox(region.rect);
    setViewport(region.rect);
    clearWindow(inside, true);
}

}
