#include "render/window.hpp"
#include <epoxy/gl.h>

namespace chase::render {

void setDepthTest(bool enabled) {
    if (enabled) glEnable(GL_DEPTH_TEST);
    else glDisable(GL_DEPTH_TEST);
}

void setCullFace(bool enabled) {
    if (enabled) glEnable(GL_CULL_FACE);
    else glDisable(GL_CULL_FACE);
}

void setScissorTest(bool enabled) {
    if (enabled) glEnable(GL_SCISSOR_TEST);
    else glDisable(GL_SCISSOR_TEST);
}

void setViewport(const PixelRect& rect) { glViewport(rect.x, rect.y, rect.width, rect.height); }

void setScissorBox(const PixelRect& rect) { glScissor(rect.x, rect.y, rect.width, rect.height); }

void clearWindow(const ClearColour& colour, bool depth) {
    glClearColor(colour.r, colour.g, colour.b, colour.a);
    glClear(depth ? GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT : GL_COLOR_BUFFER_BIT);
}

void waitForGpu() { glFinish(); }

}
