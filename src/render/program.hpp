#pragma once
// Internal to src/render: the shader program shared by Scene and StudSprites. Not included outside this directory.
#include <epoxy/gl.h>

namespace chase::render {

GLuint sceneProgram();

}
