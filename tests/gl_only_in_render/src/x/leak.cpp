// Fixture for the gl-only-in-render-rejects CTest: a non-render file with an OpenGL call must fail the guard.
#include <epoxy/gl.h>

void leak() { glFinish(); }
