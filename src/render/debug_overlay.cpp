#include "render/debug_overlay.hpp"
#include "render/debug_text.hpp"
#include "render/shaders.hpp"
#include <stdexcept>

namespace chase::render {
namespace {

GLuint compile(GLenum type, const char* source) {
    const GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) throw std::runtime_error("debug overlay shader failed to compile");
    return shader;
}

}

DebugOverlay::DebugOverlay() {
    program_ = glCreateProgram();
    const GLuint vertex = compile(GL_VERTEX_SHADER, shaders::hudVertex), fragment = compile(GL_FRAGMENT_SHADER, shaders::hudFragment);
    glAttachShader(program_, vertex);
    glAttachShader(program_, fragment);
    glLinkProgram(program_);
    GLint linked = 0;
    glGetProgramiv(program_, GL_LINK_STATUS, &linked);
    if (!linked) throw std::runtime_error("debug overlay shader failed to link");
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    glGenVertexArrays(1, &array_);
    glGenBuffers(1, &buffer_);
    glBindVertexArray(array_);
    glBindBuffer(GL_ARRAY_BUFFER, buffer_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 16, nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(2 * sizeof(float)));
    glGenTextures(1, &texture_);
    glBindTexture(GL_TEXTURE_2D, texture_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

DebugOverlay::~DebugOverlay() {
    glDeleteTextures(1, &texture_);
    glDeleteBuffers(1, &buffer_);
    glDeleteVertexArrays(1, &array_);
    glDeleteProgram(program_);
}

void DebugOverlay::draw(const std::vector<std::string>& lines, int windowWidth, int windowHeight) {
    const int scale = windowHeight >= 600 ? 2 : 1;
    const DebugTextImage image = rasteriseDebugText(lines, scale);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_SCISSOR_TEST);
    glViewport(0, 0, windowWidth, windowHeight);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glUseProgram(program_);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, image.width, image.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.rgba.data());
    const float right = 2.0f * float(image.width) / float(windowWidth) - 1, bottom = 1.0f - 2.0f * float(image.height) / float(windowHeight);
    const float vertices[16] = {-1, 1, 0, 0, right, 1, 1, 0, -1, bottom, 0, 1, right, bottom, 1, 1};
    glBindVertexArray(array_);
    glBindBuffer(GL_ARRAY_BUFFER, buffer_);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
}

}
