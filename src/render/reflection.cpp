#include "render/reflection.hpp"
#include "runtime/reflection.hpp"
#include <epoxy/gl.h>
#include <cstddef>

namespace chase::render {
namespace {
constexpr GLint kCubeUnit = 3;

GLenum faceTarget(runtime::CubeFace face) {
    switch (face) {
    case runtime::CubeFace::PositiveX: return GL_TEXTURE_CUBE_MAP_POSITIVE_X;
    case runtime::CubeFace::NegativeX: return GL_TEXTURE_CUBE_MAP_NEGATIVE_X;
    case runtime::CubeFace::PositiveY: return GL_TEXTURE_CUBE_MAP_POSITIVE_Y;
    case runtime::CubeFace::NegativeY: return GL_TEXTURE_CUBE_MAP_NEGATIVE_Y;
    case runtime::CubeFace::PositiveZ: return GL_TEXTURE_CUBE_MAP_POSITIVE_Z;
    default: return GL_TEXTURE_CUBE_MAP_NEGATIVE_Z;
    }
}
}

std::uint32_t uploadCubeTexture(const assets::Image& image) {
    GLuint id = 0;
    glGenTextures(1, &id);
    glActiveTexture(GLenum(GL_TEXTURE0 + kCubeUnit));
    glBindTexture(GL_TEXTURE_CUBE_MAP, id);
    const int faceHeight = image.height / 6;
    const std::size_t faceBytes = std::size_t(image.width) * std::size_t(faceHeight) * 4;
    for (int face = 0; face < 6; ++face)
        glTexImage2D(faceTarget(runtime::cubeFace(face)), 0, GL_RGBA8, image.width, faceHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                     image.rgba.data() + faceBytes * std::size_t(face));
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glActiveTexture(GL_TEXTURE0);
    return id;
}

void bindCubeTexture(std::uint32_t program, std::uint32_t texture) {
    glUniform1i(glGetUniformLocation(program, "cube0"), kCubeUnit);
    glActiveTexture(GLenum(GL_TEXTURE0 + kCubeUnit));
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture);
    glActiveTexture(GL_TEXTURE0);
}

}
