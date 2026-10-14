#include "render/program.hpp"
#include "render/shaders.hpp"
#include <stdexcept>
#include <string>

namespace chase::render {
namespace {

GLuint compile(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(shader, sizeof log, nullptr, log);
        throw std::runtime_error(std::string("shader: ") + log);
    }
    return shader;
}

GLuint createSceneProgram() {
    GLuint program = glCreateProgram();
    glAttachShader(program, compile(GL_VERTEX_SHADER, shaders::sceneVertex));
    glAttachShader(program, compile(GL_FRAGMENT_SHADER, shaders::sceneFragment));
    glLinkProgram(program);
    GLint linked = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked) throw std::runtime_error("shader link failed");
    return program;
}

}

GLuint sceneProgram() {
    static const GLuint program = createSceneProgram();
    return program;
}

}
