#include "qol/ui_backdrop.hpp"

#include "render/frame_keep.hpp"

namespace qol {

BackdropStep backdropStepFor(bool frameKept, bool asked) {
    if (frameKept) return BackdropStep::Picture;
    return asked ? BackdropStep::Black : BackdropStep::Defer;
}

BackdropStep Backdrop::step() {
    const BackdropStep next = backdropStepFor(chase::render::frameKept(), asked_);
    if (next == BackdropStep::Defer) {
        asked_ = true;
        chase::render::requestFrameKeep();
    }
    return next;
}

void Backdrop::draw(int width, int height) const { chase::render::drawKeptFrame(width, height); }

void Backdrop::release() {
    if (!asked_) return;
    asked_ = false;
    chase::render::releaseKeptFrame();
}

}
