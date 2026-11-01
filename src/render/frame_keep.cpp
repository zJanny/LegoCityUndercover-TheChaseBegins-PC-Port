#include "render/frame_keep.hpp"
#include <epoxy/gl.h>

namespace chase::render {

namespace {

struct Kept {
    bool requested = false;
    bool present = false;
    int width = 0, height = 0;
    GLuint texture = 0, framebuffer = 0;
};

Kept& kept() {
    static Kept state;
    return state;
}

void allocate(Kept& state, int width, int height) {
    if (!state.texture) {
        glGenTextures(1, &state.texture);
        glGenFramebuffers(1, &state.framebuffer);
    }
    if (state.width == width && state.height == height) return;
    glBindTexture(GL_TEXTURE_2D, state.texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, state.framebuffer);
    glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, state.texture, 0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    state.width = width;
    state.height = height;
}

}

void requestFrameKeep() { kept().requested = true; }

bool frameKeepRequested() { return kept().requested; }

bool frameKept() { return kept().present; }

void keepFrameIfRequested(int width, int height) {
    Kept& state = kept();
    if (!state.requested || width <= 0 || height <= 0) return;
    state.requested = false;
    allocate(state, width, height);
    setScissorTest(false);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, state.framebuffer);
    glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    state.present = true;
}

void drawKeptFrame(int width, int height) {
    Kept& state = kept();
    if (!state.present) return;
    setScissorTest(false);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, state.framebuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBlitFramebuffer(0, 0, state.width, state.height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_LINEAR);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
}

void releaseKeptFrame() {
    Kept& state = kept();
    if (state.texture) {
        glDeleteFramebuffers(1, &state.framebuffer);
        glDeleteTextures(1, &state.texture);
    }
    state = Kept{};
}

}
