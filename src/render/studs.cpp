#include "render/studs.hpp"
#include <cstddef>
#include <stdexcept>

namespace chase::render {
using namespace chase::assets;

namespace {
constexpr const char* kSheetPath = "sprites/commonlego/pickups_commonstud.btga";
constexpr float kCell = 0.25f;

struct SpriteVertex {
    float position[3], uv[2], normal[3];
    std::uint8_t colour[4];
    float uv1[2];
};
}

StudSprites::StudSprites(FuseArchive& archive) {
    const FuseEntry* entry = archive.find(kSheetPath);
    if (!entry) throw std::runtime_error(std::string("not in archive: ") + kSheetPath);
    const Image image = decodeTexture(archive.read(*entry));
    glGenTextures(1, &texture_);
    glBindTexture(GL_TEXTURE_2D, texture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, image.width, image.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.rgba.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glGenVertexArrays(1, &vao_);
    glBindVertexArray(vao_);
    glGenBuffers(1, &vbo_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glGenBuffers(1, &ibo_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_);
    const auto attribute = [](GLuint index, GLint size, GLenum type, GLboolean normalised, std::size_t offset) {
        glEnableVertexAttribArray(index);
        glVertexAttribPointer(index, size, type, normalised, sizeof(SpriteVertex), reinterpret_cast<const void*>(offset));
    };
    attribute(0, 3, GL_FLOAT, GL_FALSE, offsetof(SpriteVertex, position));
    attribute(1, 2, GL_FLOAT, GL_FALSE, offsetof(SpriteVertex, uv));
    attribute(2, 3, GL_FLOAT, GL_FALSE, offsetof(SpriteVertex, normal));
    attribute(3, 4, GL_UNSIGNED_BYTE, GL_TRUE, offsetof(SpriteVertex, colour));
    attribute(4, 2, GL_FLOAT, GL_FALSE, offsetof(SpriteVertex, uv1));
}

StudSprites::~StudSprites() {
    glDeleteTextures(1, &texture_);
    glDeleteBuffers(1, &vbo_);
    glDeleteBuffers(1, &ibo_);
    glDeleteVertexArrays(1, &vao_);
}

void StudSprites::draw(GLuint program, const Mat4& view, const Mat4& viewProjection, const std::vector<StudSprite>& sprites,
                       unsigned frame, float halfSize) const {
    if (sprites.empty()) return;
    const float right[3] = {-view[0] * halfSize, -view[4] * halfSize, -view[8] * halfSize};
    const float up[3] = {view[1] * halfSize, view[5] * halfSize, view[9] * halfSize};
    const float u0 = float(frame & 3) * kCell, v0 = float(frame >> 2) * kCell;
    const float corner[4][2] = {{-1, 1}, {1, 1}, {1, -1}, {-1, -1}};
    const float cellU[4] = {u0, u0 + kCell, u0 + kCell, u0}, cellV[4] = {v0, v0, v0 + kCell, v0 + kCell};
    std::vector<SpriteVertex> vertices;
    std::vector<std::uint32_t> indices;
    vertices.reserve(sprites.size() * 4);
    indices.reserve(sprites.size() * 6);
    for (const StudSprite& sprite : sprites) {
        const std::uint32_t base = std::uint32_t(vertices.size());
        for (int k = 0; k < 4; ++k) {
            SpriteVertex vertex{};
            for (int a = 0; a < 3; ++a) vertex.position[a] = sprite.centre[a] + corner[k][0] * right[a] + corner[k][1] * up[a];
            vertex.uv[0] = cellU[k];
            vertex.uv[1] = cellV[k];
            for (int c = 0; c < 4; ++c) vertex.colour[c] = std::uint8_t(sprite.tint >> (8 * c));
            vertex.colour[3] = 255;
            vertices.push_back(vertex);
        }
        for (const std::uint32_t offset : {0u, 1u, 2u, 0u, 2u, 3u}) indices.push_back(base + offset);
    }
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(vertices.size() * sizeof(SpriteVertex)), vertices.data(), GL_STREAM_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, GLsizeiptr(indices.size() * sizeof(std::uint32_t)), indices.data(), GL_STREAM_DRAW);
    glUniformMatrix4fv(glGetUniformLocation(program, "mvp"), 1, GL_FALSE, viewProjection.data());
    const GLint unitModes[3] = {1, 0, 0};
    glUniform1iv(glGetUniformLocation(program, "unitMode"), 3, unitModes);
    glUniform1i(glGetUniformLocation(program, "stageCount"), 0);
    glUniform1i(glGetUniformLocation(program, "hasNormal"), 0);
    glUniform1i(glGetUniformLocation(program, "hasSecondUv"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_);
    GLboolean culling = glIsEnabled(GL_CULL_FACE);
    glDisable(GL_CULL_FACE);
    glDrawElements(GL_TRIANGLES, GLsizei(indices.size()), GL_UNSIGNED_INT, nullptr);
    if (culling) glEnable(GL_CULL_FACE);
}

}
