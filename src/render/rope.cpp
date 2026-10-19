#include "render/rope.hpp"
#include "render/frame_stats.hpp"
#include "render/program.hpp"
#include <cstddef>
#include <map>
#include <epoxy/gl.h>

namespace chase::render {
using namespace chase::assets;

class RopeRendererImpl {
public:
    explicit RopeRendererImpl(FuseArchive& archive) : archive_(archive) {
        glGenVertexArrays(1, &vao_);
        glBindVertexArray(vao_);
        glGenBuffers(1, &vbo_);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        const auto attribute = [](GLuint index, GLint size, GLenum type, GLboolean normalised, std::size_t offset) {
            glEnableVertexAttribArray(index);
            glVertexAttribPointer(index, size, type, normalised, sizeof(runtime::RopeVertex), reinterpret_cast<const void*>(offset));
        };
        attribute(0, 3, GL_FLOAT, GL_FALSE, offsetof(runtime::RopeVertex, position));
        attribute(1, 2, GL_FLOAT, GL_FALSE, offsetof(runtime::RopeVertex, uv));
        attribute(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, offsetof(runtime::RopeVertex, rgba));
    }

    ~RopeRendererImpl() {
        for (const auto& entry : textures_)
            if (entry.second) glDeleteTextures(1, &entry.second);
        glDeleteBuffers(1, &vbo_);
        glDeleteVertexArrays(1, &vao_);
    }

    GLuint texture(const std::string& name) {
        const auto found = textures_.find(name);
        if (found != textures_.end()) return found->second;
        GLuint& id = textures_[name];
        std::string path = name;
        const std::size_t dot = path.rfind('.');
        if (dot != std::string::npos) path.erase(dot);
        const FuseEntry* entry = archive_.find(path + ".btga");
        if (!entry) {
            ++missing_;
            return id;
        }
        const Image image = decodeTexture(archive_.read(*entry));
        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D, id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, image.width, image.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.rgba.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        return id;
    }

    unsigned missing() const { return missing_; }

    void draw(const Mat4& viewProjection, const std::vector<runtime::RopeDraw>& draws) {
        if (draws.empty()) return;
        GLint previousProgram = 0;
        glGetIntegerv(GL_CURRENT_PROGRAM, &previousProgram);
        const GLboolean blendWas = glIsEnabled(GL_BLEND), cullWas = glIsEnabled(GL_CULL_FACE), depthWas = glIsEnabled(GL_DEPTH_TEST);
        GLboolean depthMaskWas = GL_TRUE;
        glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMaskWas);
        const GLuint program = particleProgram();
        glUseProgram(program);
        glUniformMatrix4fv(glGetUniformLocation(program, "mvp"), 1, GL_FALSE, viewProjection.data());
        glUniform1i(glGetUniformLocation(program, "diffuse"), 0);
        glUniform1i(glGetUniformLocation(program, "alphaTest"), 0);
        glUniform1f(glGetUniformLocation(program, "alphaReference"), 0.0f);
        glActiveTexture(GL_TEXTURE0);
        glDisable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);
        glBindVertexArray(vao_);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        for (const runtime::RopeDraw& draw : draws) {
            const GLuint id = texture(draw.texture);
            if (!id) continue;
            glBindTexture(GL_TEXTURE_2D, id);
            const bool faded = draw.alpha < 255;
            if (faded) {
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            } else {
                glDisable(GL_BLEND);
            }
            glDepthMask(faded ? GL_FALSE : GL_TRUE);
            glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(sizeof(draw.strip)), draw.strip.data(), GL_STREAM_DRAW);
            countDrawCall();
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }
        glBlendFunc(GL_ONE, GL_ZERO);
        glDepthMask(depthMaskWas);
        if (!depthWas) glDisable(GL_DEPTH_TEST);
        if (cullWas) glEnable(GL_CULL_FACE);
        if (blendWas) glEnable(GL_BLEND);
        else glDisable(GL_BLEND);
        glUseProgram(GLuint(previousProgram));
    }

private:
    FuseArchive& archive_;
    std::map<std::string, GLuint> textures_;
    unsigned missing_ = 0;
    GLuint vao_ = 0, vbo_ = 0;
};

RopeRenderer::RopeRenderer(FuseArchive& archive) : impl_(std::make_unique<RopeRendererImpl>(archive)) {}

RopeRenderer::~RopeRenderer() = default;

void RopeRenderer::draw(const Mat4& viewProjection, const std::vector<runtime::RopeDraw>& draws) { impl_->draw(viewProjection, draws); }

unsigned RopeRenderer::missingTextures() const { return impl_->missing(); }

}
