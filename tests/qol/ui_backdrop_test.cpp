// Spec: docs/design/qol-settings-drawing.md entry 2026-10-30T21:56:04 (menu over the dimmed game picture; the picture is the last
// presented frame) and the stand-in QOLSET-S8 (a hook whose frame has no end-of-frame keep shows a black window). Steps written by
// 20261101-qolctl-C on 2026-11-01: the first look asks for a frame and defers, a frame that never arrives gives black, a kept frame
// gives the picture, release forgets the ask. No GL context is needed: the kept frame itself is tested in render-frame-keep-window.
#include "qol/ui_backdrop.hpp"
#include "render/frame_keep.hpp"

#include <iostream>
#include <string>

namespace {

int failures = 0;

void require(bool ok, const std::string& what) {
    if (ok) return;
    std::cerr << "FAIL: " << what << '\n';
    ++failures;
}

void pureRule() {
    using qol::BackdropStep;
    require(qol::backdropStepFor(true, false) == BackdropStep::Picture, "a kept frame is the picture");
    require(qol::backdropStepFor(true, true) == BackdropStep::Picture, "a kept frame is the picture after the ask");
    require(qol::backdropStepFor(false, false) == BackdropStep::Defer, "no frame and no ask defers");
    require(qol::backdropStepFor(false, true) == BackdropStep::Black, "no frame after the ask is black");
}

void askOnce() {
    qol::Backdrop backdrop;
    require(!chase::render::frameKeepRequested() && !backdrop.asked(), "nothing asked at the start");
    require(backdrop.step() == qol::BackdropStep::Defer, "first look defers");
    require(backdrop.asked() && chase::render::frameKeepRequested(), "the first look asks chase_render for the next frame");
    require(backdrop.step() == qol::BackdropStep::Black, "a frame that never arrives gives black");
    backdrop.release();
    require(!backdrop.asked() && !chase::render::frameKeepRequested() && !chase::render::frameKept(), "release forgets the ask and the frame");
    require(backdrop.step() == qol::BackdropStep::Defer, "a new opening asks again");
    backdrop.release();
}

void releaseWithoutAsk() {
    qol::Backdrop backdrop;
    backdrop.release();
    require(!backdrop.asked() && !chase::render::frameKeepRequested(), "release without an ask changes nothing");
}

}

int main() {
    pureRule();
    askOnce();
    releaseWithoutAsk();
    return failures ? 1 : 0;
}
