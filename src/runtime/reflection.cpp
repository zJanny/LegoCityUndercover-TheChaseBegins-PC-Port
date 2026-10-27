#include "runtime/reflection.hpp"
#include <stdexcept>

namespace chase::runtime {

UnitSampling unitSampling(int unit, bool reflection, bool hasTexture, bool cubeTexture) {
    if (!hasTexture) return UnitSampling::None;
    if (!cubeTexture) return UnitSampling::Texture;
    return reflection && unit == 0 ? UnitSampling::Cube : UnitSampling::Disabled;
}

int unitUvSet(int unit, bool reflection, bool hasSecondUv) {
    if (unit == 0) return 0;
    if (reflection && unit == 1) return 0;
    return hasSecondUv ? 1 : 0;
}

CubeFace cubeFace(int storedIndex) {
    static const CubeFace order[6] = {CubeFace::PositiveX, CubeFace::NegativeX, CubeFace::PositiveY,
                                      CubeFace::NegativeY, CubeFace::PositiveZ, CubeFace::NegativeZ};
    if (storedIndex < 0 || storedIndex > 5) throw std::out_of_range("cube face index");
    return order[storedIndex];
}

}
