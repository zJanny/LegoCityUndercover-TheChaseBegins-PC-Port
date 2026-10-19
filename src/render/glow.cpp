#include "render/glow.hpp"
#include "render/program.hpp"
#include <epoxy/gl.h>

namespace chase::render {

class GlowPassImpl {
public:
    ~GlowPassImpl() {
        if (!ready_) return;
        glDeleteFramebuffers(3, framebuffers_);
        glDeleteTextures(3, textures_);
        glDeleteVertexArrays(1, &vao_);
    }

    void apply(const PixelRect& screen, const runtime::GlowComposite& composite);

private:
    void create();
    void blur(GLuint from, GLuint to, float offsetU, float offsetV, GLuint program);

    bool ready_ = false;
    GLuint textures_[3] = {0, 0, 0}, framebuffers_[3] = {0, 0, 0}, vao_ = 0;
};

void GlowPassImpl::create() {
    const int widths[3] = {runtime::kGlowFirstTargetWidth, runtime::kGlowBlurTargetWidth, runtime::kGlowBlurTargetWidth};
    const int heights[3] = {runtime::kGlowFirstTargetHeight, runtime::kGlowBlurTargetHeight, runtime::kGlowBlurTargetHeight};
    glGenTextures(3, textures_);
    glGenFramebuffers(3, framebuffers_);
    glGenVertexArrays(1, &vao_);
    for (int i = 0; i < 3; ++i) {
        glBindTexture(GL_TEXTURE_2D, textures_[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, widths[i], heights[i], 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffers_[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures_[i], 0);
    }
    ready_ = true;
}

void GlowPassImpl::blur(GLuint from, GLuint to, float offsetU, float offsetV, GLuint program) {
    glBindFramebuffer(GL_FRAMEBUFFER, to);
    glViewport(0, 0, runtime::kGlowBlurTargetWidth, runtime::kGlowBlurTargetHeight);
    glBindTexture(GL_TEXTURE_2D, from);
    glUniform2f(glGetUniformLocation(program, "tapOffset"), offsetU, offsetV);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

void GlowPassImpl::apply(const PixelRect& screen, const runtime::GlowComposite& composite) {
    if (screen.width <= 0 || screen.height <= 0) return;
    GLint viewport[4], program = 0, texture = 0, readBuffer = 0, drawBuffer = 0, vao = 0, activeUnit = 0, blendSource = 0, blendDestination = 0;
    glGetIntegerv(GL_VIEWPORT, viewport);
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &activeUnit);
    glActiveTexture(GL_TEXTURE0);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &texture);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &readBuffer);
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawBuffer);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao);
    glGetIntegerv(GL_BLEND_SRC_RGB, &blendSource);
    glGetIntegerv(GL_BLEND_DST_RGB, &blendDestination);
    if (!ready_) create();
    const GLboolean scissorWas = glIsEnabled(GL_SCISSOR_TEST), blendWas = glIsEnabled(GL_BLEND), depthWas = glIsEnabled(GL_DEPTH_TEST),
                    cullWas = glIsEnabled(GL_CULL_FACE);
    GLboolean colourMask[4];
    glGetBooleanv(GL_COLOR_WRITEMASK, colourMask);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, GLuint(readBuffer));
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffers_[0]);
    glBlitFramebuffer(screen.x, screen.y, screen.x + screen.width, screen.y + screen.height, 0, 0, runtime::kGlowFirstTargetWidth,
                      runtime::kGlowFirstTargetHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffers_[0]);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffers_[1]);
    glBlitFramebuffer(0, 0, runtime::kGlowFirstTargetWidth, runtime::kGlowFirstTargetHeight, 0, 0, runtime::kGlowBlurTargetWidth,
                      runtime::kGlowBlurTargetHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);

    const GLuint glow = glowProgram();
    glUseProgram(glow);
    glBindVertexArray(vao_);
    glUniform1i(glGetUniformLocation(glow, "image"), 0);
    glUniform1i(glGetUniformLocation(glow, "pass"), 0);
    glUniform1f(glGetUniformLocation(glow, "centreWeight"), runtime::kGlowCentreWeightByte / 255.0f);
    glUniform1f(glGetUniformLocation(glow, "sideWeight"), runtime::kGlowSideWeightByte / 255.0f);
    for (int i = 0; i < runtime::kGlowBlurIterations; ++i) {
        const float offset = runtime::glowBlurOffsetTexels(i);
        blur(textures_[1], framebuffers_[2], 0.0f, offset / float(runtime::kGlowBlurTargetHeight), glow);
        blur(textures_[2], framebuffers_[1], offset / float(runtime::kGlowBlurTargetWidth), 0.0f, glow);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, GLuint(drawBuffer));
    glViewport(screen.x, screen.y, screen.width, screen.height);
    if (scissorWas) glEnable(GL_SCISSOR_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    glBindTexture(GL_TEXTURE_2D, textures_[1]);
    glUniform1i(glGetUniformLocation(glow, "pass"), 1);
    glUniform1f(glGetUniformLocation(glow, "compositeColour"), composite.colour / 255.0f);
    glUniform1f(glGetUniformLocation(glow, "compositeAlpha"), composite.alpha / 255.0f);
    glUniform1f(glGetUniformLocation(glow, "blurredAlpha"), runtime::kGlowBlurredAlpha);
    glUniform1f(glGetUniformLocation(glow, "compositeScale"), composite.doubled ? 2.0f : 1.0f);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glColorMask(colourMask[0], colourMask[1], colourMask[2], colourMask[3]);
    glBlendFunc(GLenum(blendSource), GLenum(blendDestination));
    if (!blendWas) glDisable(GL_BLEND);
    if (depthWas) glEnable(GL_DEPTH_TEST);
    if (cullWas) glEnable(GL_CULL_FACE);
    glBindVertexArray(GLuint(vao));
    glBindTexture(GL_TEXTURE_2D, GLuint(texture));
    glActiveTexture(GLenum(activeUnit));
    glBindFramebuffer(GL_READ_FRAMEBUFFER, GLuint(readBuffer));
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, GLuint(drawBuffer));
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    glUseProgram(GLuint(program));
}

GlowPass::GlowPass() : impl_(std::make_unique<GlowPassImpl>()) {}
GlowPass::~GlowPass() = default;

void GlowPass::apply(const PixelRect& screen, const runtime::GlowComposite& composite) { impl_->apply(screen, composite); }

}
