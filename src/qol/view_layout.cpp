#include "qol/view_layout.hpp"

#include <stdexcept>
#include <string>

namespace qol {

ViewLayout computeViewLayout(int windowWidth, int windowHeight) {
    if (windowWidth <= 0 || windowHeight <= 0) {
        throw std::invalid_argument("view layout: window size must be positive, got " + std::to_string(windowWidth) + "x" +
                                    std::to_string(windowHeight));
    }
    if (windowWidth < originalViewWidth || windowHeight < originalViewHeight) {
        throw std::invalid_argument("view layout: window " + std::to_string(windowWidth) + "x" + std::to_string(windowHeight) +
                                    " is smaller than the minimum " + std::to_string(originalViewWidth) + "x" +
                                    std::to_string(originalViewHeight));
    }
    return layoutInArea(windowWidth, windowHeight);
}

ViewLayout layoutInArea(int areaWidth, int areaHeight) {
    if (areaWidth <= 0 || areaHeight <= 0) {
        throw std::invalid_argument("view layout: area size must be positive, got " + std::to_string(areaWidth) + "x" +
                                    std::to_string(areaHeight));
    }
    const long long w = areaWidth;
    const long long h = areaHeight;
    ViewLayout layout;
    int viewWidth = areaWidth;
    int viewHeight = areaHeight;
    if (w * narrowestAspectHeight < h * narrowestAspectWidth) {
        viewHeight = static_cast<int>(w * narrowestAspectHeight / narrowestAspectWidth);
    } else if (w * widestAspectHeight > h * widestAspectWidth) {
        viewWidth = static_cast<int>(h * widestAspectWidth / widestAspectHeight);
    }
    if (viewWidth <= 0 || viewHeight <= 0) {
        throw std::invalid_argument("view layout: area " + std::to_string(areaWidth) + "x" + std::to_string(areaHeight) +
                                    " leaves no view");
    }
    const int barX = (areaWidth - viewWidth) / 2;
    const int barY = (areaHeight - viewHeight) / 2;
    layout.view = {barX, barY, viewWidth, viewHeight};
    layout.aspectWidth = viewWidth;
    layout.aspectHeight = viewHeight;
    if (areaWidth > viewWidth) {
        layout.bars[0] = {0, 0, barX, areaHeight};
        layout.bars[1] = {barX + viewWidth, 0, areaWidth - barX - viewWidth, areaHeight};
        layout.barCount = 2;
    } else if (areaHeight > viewHeight) {
        layout.bars[0] = {0, 0, areaWidth, barY};
        layout.bars[1] = {0, barY + viewHeight, areaWidth, areaHeight - barY - viewHeight};
        layout.barCount = 2;
    }
    return layout;
}

HudAnchor hudAnchor(const ViewLayout& layout, Corner corner) {
    HudAnchor anchor;
    anchor.corner = corner;
    anchor.scale = static_cast<double>(layout.view.height) / originalViewHeight;
    const bool right = corner == Corner::TopRight;
    const bool bottom = corner == Corner::BottomLeft;
    anchor.x = right ? layout.view.x + layout.view.width : layout.view.x;
    anchor.y = bottom ? layout.view.y + layout.view.height : layout.view.y;
    return anchor;
}

HudPoint hudPosition(const HudAnchor& anchor, double originalX, double originalY) {
    const double dx = anchor.corner == Corner::TopRight ? originalX - originalViewWidth : originalX;
    const double dy = anchor.corner == Corner::BottomLeft ? originalY - originalViewHeight : originalY;
    return {anchor.x + dx * anchor.scale, anchor.y + dy * anchor.scale};
}

}
