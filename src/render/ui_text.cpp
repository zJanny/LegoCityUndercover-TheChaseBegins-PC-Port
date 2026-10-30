#include "render/ui_text.hpp"
#include "render/frame_stats.hpp"
#include "render/shaders.hpp"
#include "render/window.hpp"
#include <epoxy/gl.h>
#include <stdexcept>
#include <vector>

namespace chase::render {
namespace {

GLuint compile(GLenum type, const char* source) {
    const GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) throw std::runtime_error("UI text shader failed to compile");
    return shader;
}

struct Vertex {
    float x, y, u, v;
    std::uint8_t r, g, b, a;
};

}

class UiTextImpl {
public:
    UiTextImpl();
    ~UiTextImpl();
    void draw(std::string_view utf8, float pixelSize, float x, float y, UiTextColour colour, const UiClip* clip);
    void flush(int windowWidth, int windowHeight);

private:
    UiGlyphAtlas atlas_;
    std::vector<Vertex> vertices_;
    GLuint program_ = 0, array_ = 0, buffer_ = 0, texture_ = 0;
};

UiTextImpl::UiTextImpl() {
    program_ = glCreateProgram();
    const GLuint vertex = compile(GL_VERTEX_SHADER, shaders::uiTextVertex), fragment = compile(GL_FRAGMENT_SHADER, shaders::uiTextFragment);
    glAttachShader(program_, vertex);
    glAttachShader(program_, fragment);
    glLinkProgram(program_);
    GLint linked = 0;
    glGetProgramiv(program_, GL_LINK_STATUS, &linked);
    if (!linked) throw std::runtime_error("UI text shader failed to link");
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    glGenVertexArrays(1, &array_);
    glGenBuffers(1, &buffer_);
    glBindVertexArray(array_);
    glBindBuffer(GL_ARRAY_BUFFER, buffer_);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(2 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), reinterpret_cast<void*>(4 * sizeof(float)));
    glGenTextures(1, &texture_);
    glBindTexture(GL_TEXTURE_2D, texture_);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, atlas_.size(), atlas_.size(), 0, GL_RED, GL_UNSIGNED_BYTE, atlas_.pixels().data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

UiTextImpl::~UiTextImpl() {
    glDeleteTextures(1, &texture_);
    glDeleteBuffers(1, &buffer_);
    glDeleteVertexArrays(1, &array_);
    glDeleteProgram(program_);
}

void UiTextImpl::draw(std::string_view utf8, float pixelSize, float x, float y, UiTextColour colour, const UiClip* clip) {
    for (const UiGlyphQuad& q : layoutUiText(atlas_, utf8, pixelSize, x, y, clip)) {
        const Vertex corners[4] = {{q.x0, q.y0, q.u0, q.v0, colour.r, colour.g, colour.b, colour.a},
                                   {q.x1, q.y0, q.u1, q.v0, colour.r, colour.g, colour.b, colour.a},
                                   {q.x1, q.y1, q.u1, q.v1, colour.r, colour.g, colour.b, colour.a},
                                   {q.x0, q.y1, q.u0, q.v1, colour.r, colour.g, colour.b, colour.a}};
        for (const int index : {0, 1, 2, 0, 2, 3}) vertices_.push_back(corners[index]);
    }
}

void UiTextImpl::flush(int windowWidth, int windowHeight) {
    if (vertices_.empty()) return;
    setDepthTest(false);
    setCullFace(false);
    setScissorTest(false);
    setViewport({0, 0, windowWidth, windowHeight});
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(program_);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_);
    if (atlas_.takeDirty()) {
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, atlas_.size(), atlas_.size(), GL_RED, GL_UNSIGNED_BYTE, atlas_.pixels().data());
    }
    glUniform1i(glGetUniformLocation(program_, "atlas"), 0);
    glUniform2f(glGetUniformLocation(program_, "windowSize"), static_cast<float>(windowWidth), static_cast<float>(windowHeight));
    glBindVertexArray(array_);
    glBindBuffer(GL_ARRAY_BUFFER, buffer_);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices_.size() * sizeof(Vertex)), vertices_.data(), GL_STREAM_DRAW);
    countDrawCall();
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices_.size()));
    glDisable(GL_BLEND);
    setDepthTest(true);
    setCullFace(true);
    vertices_.clear();
}

UiText::UiText() : impl_(std::make_unique<UiTextImpl>()) {}
UiText::~UiText() = default;

void UiText::draw(std::string_view utf8, float pixelSize, float x, float y, UiTextColour colour, const UiClip* clip) {
    impl_->draw(utf8, pixelSize, x, y, colour, clip);
}

void UiText::flush(int windowWidth, int windowHeight) { impl_->flush(windowWidth, windowHeight); }

}
