#include "render/shadow.hpp"
#include "render/frame_stats.hpp"
#include "render/program.hpp"
#include "runtime/shadow.hpp"
#include <epoxy/gl.h>

namespace chase::render {

class ShadowPassImpl {
public:
    ~ShadowPassImpl() {
        if (!ready_) return;
        glDeleteFramebuffers(1, &framebuffer_);
        glDeleteTextures(1, &texture_);
    }
    void render(const Scene& scene, const Mat4& shadowViewProjection);
    void apply(const Scene& scene, const Mat4& viewProjection, const std::array<std::uint8_t, 3>& constant);

private:
    void create();
    void drawParts(const std::vector<Scene::ShadowPart>& parts, GLuint program, const Mat4& viewProjection) const;

    bool ready_ = false, rendered_ = false;
    GLuint texture_ = 0, framebuffer_ = 0;
    Mat4 shadowViewProjection_ = identity();
};

void ShadowPassImpl::create() {
    GLint bound = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &bound);
    glGenTextures(1, &texture_);
    glBindTexture(GL_TEXTURE_2D, texture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, runtime::kShadowTextureSize, runtime::kShadowTextureSize, 0, GL_DEPTH_COMPONENT,
                 GL_UNSIGNED_INT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    const float white[4] = {1, 1, 1, 1};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, white);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    glBindTexture(GL_TEXTURE_2D, GLuint(bound));
    glGenFramebuffers(1, &framebuffer_);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texture_, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    ready_ = true;
}

void ShadowPassImpl::drawParts(const std::vector<Scene::ShadowPart>& parts, GLuint program, const Mat4& viewProjection) const {
    const GLint mvp = glGetUniformLocation(program, "mvp"), shadowMvp = glGetUniformLocation(program, "shadowMvp");
    for (const Scene::ShadowPart& part : parts) {
        const Mat4 clip = multiply(viewProjection, part.world), shadowClip = multiply(shadowViewProjection_, part.world);
        glUniformMatrix4fv(mvp, 1, GL_FALSE, clip.data());
        glUniformMatrix4fv(shadowMvp, 1, GL_FALSE, shadowClip.data());
        glBindVertexArray(part.vertexArray);
        countDrawCall();
        glDrawElements(GL_TRIANGLES, part.indexCount, GL_UNSIGNED_SHORT, nullptr);
    }
}

void ShadowPassImpl::render(const Scene& scene, const Mat4& shadowViewProjection) {
    GLint viewport[4], program = 0, framebuffer = 0;
    glGetIntegerv(GL_VIEWPORT, viewport);
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &framebuffer);
    const GLboolean scissor = glIsEnabled(GL_SCISSOR_TEST), culling = glIsEnabled(GL_CULL_FACE), blend = glIsEnabled(GL_BLEND);
    if (!ready_) create();
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
    glViewport(0, 0, runtime::kShadowTextureSize, runtime::kShadowTextureSize);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
    glClear(GL_DEPTH_BUFFER_BIT);
    const GLuint shadow = shadowProgram();
    glUseProgram(shadow);
    glUniform1i(glGetUniformLocation(shadow, "receiving"), 0);
    shadowViewProjection_ = shadowViewProjection;
    drawParts(scene.shadowParts(true), shadow, shadowViewProjection);
    rendered_ = true;
    glBindFramebuffer(GL_FRAMEBUFFER, GLuint(framebuffer));
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    if (scissor) glEnable(GL_SCISSOR_TEST);
    if (culling) glEnable(GL_CULL_FACE);
    if (blend) glEnable(GL_BLEND);
    glUseProgram(GLuint(program));
}

void ShadowPassImpl::apply(const Scene& scene, const Mat4& viewProjection, const std::array<std::uint8_t, 3>& constant) {
    if (!rendered_) return;
    GLint program = 0, activeUnit = 0, texture = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &activeUnit);
    const GLboolean culling = glIsEnabled(GL_CULL_FACE), blend = glIsEnabled(GL_BLEND);
    glActiveTexture(GL_TEXTURE0);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &texture);
    glBindTexture(GL_TEXTURE_2D, texture_);
    const GLuint shadow = shadowProgram();
    glUseProgram(shadow);
    glUniform1i(glGetUniformLocation(shadow, "receiving"), 1);
    glUniform1i(glGetUniformLocation(shadow, "shadowTexture"), 0);
    glUniform3f(glGetUniformLocation(shadow, "shadowConstant"), float(constant[0]) / 255.0f, float(constant[1]) / 255.0f,
                float(constant[2]) / 255.0f);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_DST_COLOR, GL_ZERO);
    glDepthMask(GL_FALSE);
    glDepthFunc(GL_EQUAL);
    drawParts(scene.shadowParts(false), shadow, viewProjection);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);
    if (!blend) glDisable(GL_BLEND);
    if (culling) glEnable(GL_CULL_FACE);
    glBindTexture(GL_TEXTURE_2D, GLuint(texture));
    glActiveTexture(GLenum(activeUnit));
    glUseProgram(GLuint(program));
}

ShadowPass::ShadowPass() : impl_(std::make_unique<ShadowPassImpl>()) {}
ShadowPass::~ShadowPass() = default;
void ShadowPass::render(const Scene& scene, const Mat4& shadowViewProjection) { impl_->render(scene, shadowViewProjection); }
void ShadowPass::apply(const Scene& scene, const Mat4& viewProjection, const std::array<std::uint8_t, 3>& constant) {
    impl_->apply(scene, viewProjection, constant);
}

}
