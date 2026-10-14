#pragma once
#include "frontend/canvas.hpp"
#include "render/shaders.hpp"
#include <SDL.h>
#include <epoxy/gl.h>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace game_hud {

namespace fe = chase::frontend;

struct Rect {
    int x = 0, y = 0, width = 0, height = 0;   // pixels, y from the bottom of the window (GL)
};

struct Screens {
    Rect top, bottom;
};

inline Screens screensFor(int width, int height) {
    const float scale = std::min(float(width) / fe::kTopWidth, float(height) / (2 * fe::kScreenHeight));
    const float originX = (float(width) - scale * fe::kTopWidth) / 2, originY = (float(height) - scale * 2 * fe::kScreenHeight) / 2;
    auto rect = [&](float x, float yFromTop, float w, float h) {
        const int left = int(std::lround(x)), right = int(std::lround(x + w));
        const int bottom = height - int(std::lround(yFromTop + h)), top = height - int(std::lround(yFromTop));
        return Rect{left, bottom, right - left, top - bottom};
    };
    Screens screens;
    screens.top = rect(originX, originY, scale * fe::kTopWidth, scale * fe::kScreenHeight);
    screens.bottom = rect(originX + scale * (fe::kTopWidth - fe::kBottomWidth) / 2, originY + scale * fe::kScreenHeight,
                          scale * fe::kBottomWidth, scale * fe::kScreenHeight);
    return screens;
}

class Overlay {
public:
    Overlay() {
        program_ = glCreateProgram();
        const GLuint vertex = compile(GL_VERTEX_SHADER, chase::render::shaders::hudVertex), fragment = compile(GL_FRAGMENT_SHADER, chase::render::shaders::hudFragment);
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
        const int widths[2] = {fe::kTopWidth, fe::kBottomWidth};
        for (int i = 0; i < 2; ++i) {
            glBindTexture(GL_TEXTURE_2D, textures_[i]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, widths[i], fe::kScreenHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        }
    }
    ~Overlay() {
        glDeleteTextures(2, textures_);
        glDeleteBuffers(1, &buffer_);
        glDeleteVertexArrays(1, &array_);
        glDeleteProgram(program_);
    }
    Overlay(const Overlay&) = delete;
    Overlay& operator=(const Overlay&) = delete;

    void draw(const fe::Canvas& hud, const fe::Canvas& bottom, const Screens& screens, int windowWidth, int windowHeight) {
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glDisable(GL_SCISSOR_TEST);
        glViewport(0, 0, windowWidth, windowHeight);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glUseProgram(program_);
        glActiveTexture(GL_TEXTURE0);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glBindVertexArray(array_);
        quad(0, hud, screens.top, windowWidth, windowHeight);
        quad(1, bottom, screens.bottom, windowWidth, windowHeight);
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
    }

private:
    static GLuint compile(GLenum type, const char* source) {
        const GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);
        GLint ok = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
        if (!ok) throw std::runtime_error("overlay shader failed to compile");
        return shader;
    }

    void quad(int index, const fe::Canvas& canvas, const Rect& rect, int windowWidth, int windowHeight) {
        glBindTexture(GL_TEXTURE_2D, textures_[index]);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, canvas.width(), canvas.height(), GL_RGBA, GL_UNSIGNED_BYTE, canvas.rgba().data());
        const float left = 2.0f * float(rect.x) / float(windowWidth) - 1, right = 2.0f * float(rect.x + rect.width) / float(windowWidth) - 1;
        const float bottom = 2.0f * float(rect.y) / float(windowHeight) - 1, top = 2.0f * float(rect.y + rect.height) / float(windowHeight) - 1;
        const float vertices[16] = {left, top, 0, 0, right, top, 1, 0, left, bottom, 0, 1, right, bottom, 1, 1};
        glBindBuffer(GL_ARRAY_BUFFER, buffer_);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

    GLuint program_ = 0, array_ = 0, buffer_ = 0;
    GLuint textures_[2] = {0, 0};
};

}
