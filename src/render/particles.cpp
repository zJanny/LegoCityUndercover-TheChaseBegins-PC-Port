#include "render/particles.hpp"
#include "render/frame_stats.hpp"
#include "render/program.hpp"
#include <cstddef>
#include <epoxy/gl.h>

namespace chase::render {
using namespace chase::assets;

class ParticleRendererImpl {
public:
    ParticleRendererImpl(FuseArchive& archive, std::vector<std::string> names, std::string directory)
        : archive_(archive), names_(std::move(names)), directory_(std::move(directory)) {
        textures_.assign(names_.size(), 0);
        failed_.assign(names_.size(), false);
        glGenVertexArrays(1, &vao_);
        glBindVertexArray(vao_);
        glGenBuffers(1, &vbo_);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glGenBuffers(1, &ibo_);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_);
        const auto attribute = [](GLuint index, GLint size, GLenum type, GLboolean normalised, std::size_t offset) {
            glEnableVertexAttribArray(index);
            glVertexAttribPointer(index, size, type, normalised, sizeof(runtime::ParticleVertex), reinterpret_cast<const void*>(offset));
        };
        attribute(0, 3, GL_FLOAT, GL_FALSE, offsetof(runtime::ParticleVertex, position));
        attribute(1, 2, GL_FLOAT, GL_FALSE, offsetof(runtime::ParticleVertex, uv));
        attribute(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, offsetof(runtime::ParticleVertex, rgba));
    }

    ~ParticleRendererImpl() {
        for (const GLuint texture : textures_)
            if (texture) glDeleteTextures(1, &texture);
        glDeleteBuffers(1, &vbo_);
        glDeleteBuffers(1, &ibo_);
        glDeleteVertexArrays(1, &vao_);
    }

    GLuint texture(int index) {
        if (index < 0 || std::size_t(index) >= names_.size() || failed_[std::size_t(index)]) return 0;
        GLuint& id = textures_[std::size_t(index)];
        if (id) return id;
        const FuseEntry* entry = archive_.find(directory_ + names_[std::size_t(index)] + ".btga");
        if (!entry) {
            failed_[std::size_t(index)] = true;
            return 0;
        }
        const Image image = decodeTexture(archive_.read(*entry));
        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D, id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, image.width, image.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.rgba.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        return id;
    }

    unsigned missing() const {
        unsigned count = 0;
        for (const bool failed : failed_) count += failed;
        return count;
    }

    void draw(const Mat4& viewProjection, const std::vector<runtime::ParticleBatch>& batches) {
        if (batches.empty()) return;
        GLint previousProgram = 0;
        glGetIntegerv(GL_CURRENT_PROGRAM, &previousProgram);
        const GLboolean blendWas = glIsEnabled(GL_BLEND), cullWas = glIsEnabled(GL_CULL_FACE), depthWas = glIsEnabled(GL_DEPTH_TEST);
        GLboolean depthMaskWas = GL_TRUE;
        glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMaskWas);
        GLint depthFunctionWas = GL_LESS;
        glGetIntegerv(GL_DEPTH_FUNC, &depthFunctionWas);
        const GLuint program = particleProgram();
        glUseProgram(program);
        glUniformMatrix4fv(glGetUniformLocation(program, "mvp"), 1, GL_FALSE, viewProjection.data());
        glUniform1i(glGetUniformLocation(program, "diffuse"), 0);
        glActiveTexture(GL_TEXTURE0);
        glEnable(GL_BLEND);
        glDisable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        glDepthMask(GL_FALSE);
        glBindVertexArray(vao_);
        for (const runtime::ParticleBatch& batch : batches) {
            const GLuint id = texture(batch.texture);
            if (!id) continue;
            glBindTexture(GL_TEXTURE_2D, id);
            switch (batch.blendMode) {
            case 1:
                glBlendEquation(GL_FUNC_ADD);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                break;
            case 2:
                glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                break;
            case 3:
                glBlendEquation(GL_FUNC_ADD);
                glBlendFunc(GL_ZERO, GL_SRC_COLOR);
                break;
            default:
                glBlendEquation(GL_FUNC_ADD);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                break;
            }
            glUniform1i(glGetUniformLocation(program, "alphaTest"), batch.alphaTest ? 1 : 0);
            glUniform1f(glGetUniformLocation(program, "alphaReference"), batch.alphaReference);
            indices_.clear();
            for (std::uint32_t quad = 0; quad < batch.vertices.size() / 4; ++quad)
                for (const std::uint32_t offset : {0u, 1u, 2u, 0u, 2u, 3u}) indices_.push_back(quad * 4 + offset);
            glBindBuffer(GL_ARRAY_BUFFER, vbo_);
            glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(batch.vertices.size() * sizeof(runtime::ParticleVertex)), batch.vertices.data(), GL_STREAM_DRAW);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, GLsizeiptr(indices_.size() * sizeof(std::uint32_t)), indices_.data(), GL_STREAM_DRAW);
            countDrawCall();
            glDrawElements(GL_TRIANGLES, GLsizei(indices_.size()), GL_UNSIGNED_INT, nullptr);
        }
        glBlendEquation(GL_FUNC_ADD);
        glBlendFunc(GL_ONE, GL_ZERO);
        glDepthMask(depthMaskWas);
        glDepthFunc(GLenum(depthFunctionWas));
        if (!depthWas) glDisable(GL_DEPTH_TEST);
        if (cullWas) glEnable(GL_CULL_FACE);
        if (!blendWas) glDisable(GL_BLEND);
        glUseProgram(GLuint(previousProgram));
    }

private:
    FuseArchive& archive_;
    std::vector<std::string> names_;
    std::string directory_;
    std::vector<GLuint> textures_;
    std::vector<bool> failed_;
    std::vector<std::uint32_t> indices_;
    GLuint vao_ = 0, vbo_ = 0, ibo_ = 0;
};

ParticleRenderer::ParticleRenderer(FuseArchive& archive, std::vector<std::string> textureNames, std::string directory)
    : impl_(std::make_unique<ParticleRendererImpl>(archive, std::move(textureNames), std::move(directory))) {}

ParticleRenderer::~ParticleRenderer() = default;

void ParticleRenderer::draw(const Mat4& viewProjection, const std::vector<runtime::ParticleBatch>& batches) {
    impl_->draw(viewProjection, batches);
}

unsigned ParticleRenderer::missingTextures() const { return impl_->missing(); }

}
