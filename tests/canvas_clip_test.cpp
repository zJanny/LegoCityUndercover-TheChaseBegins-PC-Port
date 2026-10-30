#include "frontend/canvas.hpp"
#include <cstdio>

using namespace chase;
using namespace chase::frontend;

namespace {
int failures = 0;

void expect(bool ok, const char* what) {
    if (!ok) {
        std::printf("FAIL: %s\n", what);
        ++failures;
    }
}

assets::Image makeImage() {
    assets::Image image;
    image.width = 4;
    image.height = 4;
    image.rgba.resize(4 * 4 * 4);
    for (std::size_t i = 0; i < image.rgba.size(); ++i) image.rgba[i] = std::uint8_t((i * 37 + 11) % 256);
    return image;
}

bool equalToUnclippedDraw(float x, float y, float width, float height) {
    const assets::Image image = makeImage();
    const int margin = 64, size = 16;
    Canvas small(size, size), large(size + 2 * margin, size + 2 * margin);
    small.draw(image, {0, 0, 4, 4}, 2, 2);
    large.draw(image, {0, 0, 4, 4}, margin + 2, margin + 2);
    const Colour tint{1, 0.9f, 0.8f, 0.7f};
    small.drawScaled(image, {0, 0, 4, 4}, x, y, width, height, tint);
    large.drawScaled(image, {0, 0, 4, 4}, x + margin, y + margin, width, height, tint);
    for (int row = 0; row < size; ++row)
        for (int column = 0; column < size; ++column)
            for (int i = 0; i < 4; ++i)
                if (small.pixel(column, row)[i] != large.pixel(column + margin, row + margin)[i]) return false;
    return true;
}
}

int main() {
    expect(equalToUnclippedDraw(2, 3, 9, 7), "inside");
    expect(equalToUnclippedDraw(-5.5f, -3.25f, 12, 9), "partly outside top left");
    expect(equalToUnclippedDraw(10.5f, 12, 20, 20), "partly outside bottom right");
    expect(equalToUnclippedDraw(-20, -20, 60, 60), "covering the canvas");
    expect(equalToUnclippedDraw(-30, 4, 10, 6), "wholly left");
    expect(equalToUnclippedDraw(40, 40, 10, 10), "wholly outside");
    expect(equalToUnclippedDraw(12, 14, -9, -7), "flipped partly outside");

    const assets::Image image = makeImage();
    Canvas huge(16, 16);
    huge.drawScaled(image, {0, 0, 4, 4}, -1000, -1000, 3000, 3000);
    expect(huge.scaledPixelsVisited() == 16 * 16, "a huge destination visits only canvas pixels");
    Canvas outside(16, 16);
    outside.drawScaled(image, {0, 0, 4, 4}, 100, 100, 5000, 5000);
    expect(outside.scaledPixelsVisited() == 0, "a destination wholly outside visits nothing");
    Canvas part(16, 16);
    part.drawScaled(image, {0, 0, 4, 4}, 14, 12, 10, 10);
    expect(part.scaledPixelsVisited() == 2 * 4, "a partly outside destination visits the overlap only");
    return failures ? 1 : 0;
}
