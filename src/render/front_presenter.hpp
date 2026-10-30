#pragma once
// Presents the two front-end canvases in the window: each one is uploaded as a texture and blitted from a framebuffer
// into its screen's box, the top screen at the top of the window as the 3DS shows it.
#include "render/window.hpp"
#include <memory>

namespace chase::render {

class FrontPresenterImpl;

struct FrontBox {
    float x, y, width, height;
};

class FrontPresenter {
public:
    FrontPresenter(int topWidth, int bottomWidth, int screenHeight);
    ~FrontPresenter();
    FrontPresenter(const FrontPresenter&) = delete;
    FrontPresenter& operator=(const FrontPresenter&) = delete;

    void present(const Pixels& top, const Pixels& bottom, int width, int height);
    void presentBoxes(const Pixels& top, const Pixels& bottom, int width, int height, const FrontBox& topBox, const FrontBox& bottomBox);

private:
    std::unique_ptr<FrontPresenterImpl> impl_;
};

}
