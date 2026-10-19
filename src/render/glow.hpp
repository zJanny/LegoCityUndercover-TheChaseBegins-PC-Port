#pragma once
// The glow post-process 0x2bf830 on the current window framebuffer (runtime/glow.hpp, docs/re/rendering-glow.md): the rectangle is
// copied to a 256 x 128 target and to 128 x 64, blurred three times and added back to the rectangle. Plain types only.
#include "render/window.hpp"
#include "runtime/glow.hpp"
#include <memory>

namespace chase::render {

class GlowPassImpl;

class GlowPass {
public:
    GlowPass();
    ~GlowPass();
    GlowPass(const GlowPass&) = delete;
    GlowPass& operator=(const GlowPass&) = delete;

    // `screen` is the window rectangle of the 3D screen; the scissor box of the caller keeps the final draw inside it.
    void apply(const PixelRect& screen, const runtime::GlowComposite& composite);

private:
    std::unique_ptr<GlowPassImpl> impl_;
};

}
