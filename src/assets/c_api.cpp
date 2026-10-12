#include "assets/assets.hpp"
#include <cstring>
#include <stdexcept>
#include <string>

namespace {
thread_local std::string lastError;
}

extern "C" {
const char* chase_last_error() { return lastError.c_str(); }

int chase_skin_matrices(const std::uint8_t* skeletonData, std::size_t skeletonSize, const std::uint8_t* animationData,
                        std::size_t animationSize, double frame, float* out, std::size_t capacity) {
    try {
        using namespace chase::assets;
        Skeleton skeleton = parseSkeleton(Bytes(skeletonData, skeletonData + skeletonSize));
        Animation animation = parseAnimation(Bytes(animationData, animationData + animationSize));
        std::vector<Matrix> skin = skinMatrices(skeleton, animation, frame);
        if (skin.size() * 16 > capacity) throw std::runtime_error("output buffer too small");
        for (std::size_t i = 0; i < skin.size(); ++i) std::memcpy(out + 16 * i, skin[i].data(), sizeof(Matrix));
        return int(skin.size());
    } catch (const std::exception& error) {
        lastError = error.what();
        return -1;
    }
}
}
