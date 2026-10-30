#include "render/ui_draw.hpp"
#include "render/frame_stats.hpp"
#include "render/shaders.hpp"
#include "render/ui_text.hpp"
#include "render/window.hpp"
#include <algorithm>
#include <cmath>
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
    if (!ok) throw std::runtime_error("ui draw shader failed to compile");
    return shader;
}

}

class UiDrawImpl {
public:
    UiDrawImpl();
    ~UiDrawImpl();
    void draw(const UiBatch& batch, int windowWidth, int windowHeight);

private:
    void drawTexts(const UiBatch& batch, std::size_t& next, std::size_t runsBefore, int windowWidth, int windowHeight);
    UiText text_;
    GLuint program_ = 0, array_ = 0, buffer_ = 0;
    GLint sizeUniform_ = -1;
    std::size_t capacity_ = 0;
};

UiDrawImpl::UiDrawImpl() {
    program_ = glCreateProgram();
    const GLuint vertex = compile(GL_VERTEX_SHADER, shaders::uiDrawVertex), fragment = compile(GL_FRAGMENT_SHADER, shaders::uiDrawFragment);
    glAttachShader(program_, vertex);
    glAttachShader(program_, fragment);
    glLinkProgram(program_);
    GLint linked = 0;
    glGetProgramiv(program_, GL_LINK_STATUS, &linked);
    if (!linked) throw std::runtime_error("ui draw shader failed to link");
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    sizeUniform_ = glGetUniformLocation(program_, "windowSize");
    glGenVertexArrays(1, &array_);
    glGenBuffers(1, &buffer_);
    glBindVertexArray(array_);
    glBindBuffer(GL_ARRAY_BUFFER, buffer_);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(UiVertex), nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(UiVertex), reinterpret_cast<void*>(2 * sizeof(float)));
}

UiDrawImpl::~UiDrawImpl() {
    glDeleteBuffers(1, &buffer_);
    glDeleteVertexArrays(1, &array_);
    glDeleteProgram(program_);
}

namespace {

std::uint8_t colourByte(float value) { return std::uint8_t(std::lround(std::clamp(value, 0.0f, 1.0f) * 255.0f)); }

}

void UiDrawImpl::drawTexts(const UiBatch& batch, std::size_t& next, std::size_t runsBefore, int windowWidth, int windowHeight) {
    bool queued = false;
    for (; next < batch.texts.size() && batch.texts[next].runsBefore <= runsBefore; ++next) {
        const UiTextRun& run = batch.texts[next];
        const UiTextColour colour{colourByte(run.colour.r), colourByte(run.colour.g), colourByte(run.colour.b), colourByte(run.colour.a)};
        text_.draw(run.text, run.pixelSize, run.x, run.y, colour, &run.clip);
        queued = true;
    }
    if (queued) text_.flush(windowWidth, windowHeight);
}

void UiDrawImpl::draw(const UiBatch& batch, int windowWidth, int windowHeight) {
    if (batch.vertices.empty() && batch.texts.empty()) return;
    if (!batch.vertices.empty()) {
        glBindVertexArray(array_);
        glBindBuffer(GL_ARRAY_BUFFER, buffer_);
        const std::size_t bytes = batch.vertices.size() * sizeof(UiVertex);
        if (bytes > capacity_) {
            glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(bytes), batch.vertices.data(), GL_DYNAMIC_DRAW);
            capacity_ = bytes;
        } else {
            glBufferSubData(GL_ARRAY_BUFFER, 0, GLsizeiptr(bytes), batch.vertices.data());
        }
    }
    std::size_t nextText = 0;
    for (std::size_t index = 0; index < batch.runs.size(); ++index) {
        drawTexts(batch, nextText, index, windowWidth, windowHeight);
        const UiRun& run = batch.runs[index];
        setDepthTest(false);
        setCullFace(false);
        setViewport({0, 0, windowWidth, windowHeight});
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glUseProgram(program_);
        glUniform2f(sizeUniform_, float(windowWidth), float(windowHeight));
        glBindVertexArray(array_);
        glBindBuffer(GL_ARRAY_BUFFER, buffer_);
        setScissorTest(run.clipped);
        if (run.clipped) setScissorBox({run.clip.x, windowHeight - run.clip.y - run.clip.height, run.clip.width, run.clip.height});
        countDrawCall();
        glDrawArrays(GL_TRIANGLES, GLint(run.first), GLsizei(run.count));
        setScissorTest(false);
        glDisable(GL_BLEND);
        setDepthTest(true);
        setCullFace(true);
    }
    drawTexts(batch, nextText, batch.runs.size(), windowWidth, windowHeight);
}

UiDraw::UiDraw() : impl_(std::make_unique<UiDrawImpl>()) {}

UiDraw::~UiDraw() = default;

void UiDraw::draw(const UiBatch& batch, int windowWidth, int windowHeight) { impl_->draw(batch, windowWidth, windowHeight); }

}
