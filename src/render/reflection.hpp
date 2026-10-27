#pragma once
#include "assets/assets.hpp"
#include <cstdint>

namespace chase::render {

// Uploads a decoded cube texture (six faces as a vertical strip in stored order) as a cube map; returns its texture name.
std::uint32_t uploadCubeTexture(const assets::Image& image);

// Binds the cube map of texture unit 0 (0 unbinds) to the scene program's `cube0` sampler.
void bindCubeTexture(std::uint32_t program, std::uint32_t texture);

}
