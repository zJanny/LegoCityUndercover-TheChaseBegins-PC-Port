#include "render/hud_overlay.hpp"
#include "render/frame_stats.hpp"
#include "render/shaders.hpp"
#include <epoxy/gl.h>
#include <stdexcept>

namespace chase::render {
namespace {

GLuint compile(GLenum type, const char* source) {
    const GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) throw std::runtime_error("overlay shader failed to compile");
    return shader;
}

}

class HudOverlayImpl {
public:
    HudOverlayImpl(int topWidth, int bottomWidth, int screenHeight);
    ~HudOverlayImpl();
    void draw(const Pixels& top, const Pixels& bottom, const PixelRect& topRect, const PixelRect& bottomRect, int windowWidth, int windowHeight);

private:
    void quad(int index, const Pixels& image, const PixelRect& rect, int windowWidth, int windowHeight);

    GLuint program_ = 0, array_ = 0, buffer_ = 0;
    GLuint textures_[2] = {0, 0};
};

HudOverlayImpl::HudOverlayImpl(int topWidth, int bottomWidth, int screenHeight) {
    program_ = glCreateProgram();
    const GLuint vertex = compile(GL_VERTEX_SHADER, shaders::hudVertex), fragment = compile(GL_FRAGMENT_SHADER, shaders::hudFragment);
    glAttachShader(program_, vertex);
    glAttachShader(program_, fragment);
    glLinkProgram(program_);
    GLint linked = 0;
    glGetProgramiv(program_, GL_LINK_STATUS, &linked);
    if (!linked) throw std::runtime_error("overlay shader failed to link");
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
    glGenTextures(2, textures_);
    const int widths[2] = {topWidth, bottomWidth};
    for (int i = 0; i < 2; ++i) {
        glBindTexture(GL_TEXTURE_2D, textures_[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, widths[i], screenHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
}

HudOverlayImpl::~HudOverlayImpl() {
    glDeleteTextures(2, textures_);
    glDeleteBuffers(1, &buffer_);
    glDeleteVertexArrays(1, &array_);
    glDeleteProgram(program_);
}

void HudOverlayImpl::draw(const Pixels& top, const Pixels& bottom, const PixelRect& topRect, const PixelRect& bottomRect, int windowWidth,
                          int windowHeight) {
    setDepthTest(false);
    setCullFace(false);
    setScissorTest(false);
    setViewport({0, 0, windowWidth, windowHeight});
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(program_);
    glActiveTexture(GL_TEXTURE0);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glBindVertexArray(array_);
    quad(0, top, topRect, windowWidth, windowHeight);
    quad(1, bottom, bottomRect, windowWidth, windowHeight);
    glDisable(GL_BLEND);
    setDepthTest(true);
    setCullFace(true);
}

void HudOverlayImpl::quad(int index, const Pixels& image, const PixelRect& rect, int windowWidth, int windowHeight) {
    glBindTexture(GL_TEXTURE_2D, textures_[index]);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, image.width, image.height, GL_RGBA, GL_UNSIGNED_BYTE, image.rgba);
    const float left = 2.0f * float(rect.x) / float(windowWidth) - 1, right = 2.0f * float(rect.x + rect.width) / float(windowWidth) - 1;
    const float bottom = 2.0f * float(rect.y) / float(windowHeight) - 1, top = 2.0f * float(rect.y + rect.height) / float(windowHeight) - 1;
    const float vertices[16] = {left, top, 0, 0, right, top, 1, 0, left, bottom, 0, 1, right, bottom, 1, 1};
    glBindBuffer(GL_ARRAY_BUFFER, buffer_);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    countDrawCall();
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

HudOverlay::HudOverlay(int topWidth, int bottomWidth, int screenHeight)
    : impl_(std::make_unique<HudOverlayImpl>(topWidth, bottomWidth, screenHeight)) {}

HudOverlay::~HudOverlay() = default;

void HudOverlay::draw(const Pixels& top, const Pixels& bottom, const PixelRect& topRect, const PixelRect& bottomRect, int windowWidth,
                      int windowHeight) {
    impl_->draw(top, bottom, topRect, bottomRect, windowWidth, windowHeight);
}

}
