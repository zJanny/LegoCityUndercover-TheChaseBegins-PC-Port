#pragma once
// Internal to src/render: the shader programs shared by Scene, StudSprites and ParticleRenderer. Not included outside this directory.
#include <epoxy/gl.h>

namespace chase::render {

GLuint sceneProgram();
GLuint particleProgram();
GLuint glowProgram();

}
