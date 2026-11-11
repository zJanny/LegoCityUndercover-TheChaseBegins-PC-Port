#include "runtime/node_view_cull.hpp"

#include <cmath>
#include <cstddef>

namespace chase::runtime {

void viewScales(float fovY, float aspect, float& scaleX, float& scaleY) {
    scaleY = 1.0f / std::tan(fovY * 0.5f);
    scaleX = scaleY / aspect;
}

ViewPlanes viewPlanes(const ViewCamera& camera) {
    ViewPlanes planes{};
    const auto side = [](float scale, float shift, float sign) {
        const float offset = sign - shift;
        float length = offset * offset;
        length += scale * scale;
        const float inverse = 1.0f / std::sqrt(length);
        return std::array<float, 2>{scale * inverse, offset * inverse};
    };
    const std::array<float, 2> left = side(camera.scaleX, camera.shiftX, -1.0f);
    planes[0] = {-left[0], 0.0f, left[1], 0.0f};
    const std::array<float, 2> right = side(camera.scaleX, camera.shiftX, 1.0f);
    planes[1] = {right[0], 0.0f, -right[1], 0.0f};
    const std::array<float, 2> bottom = side(camera.scaleY, camera.shiftY, -1.0f);
    planes[2] = {0.0f, -bottom[0], bottom[1], 0.0f};
    const std::array<float, 2> top = side(camera.scaleY, camera.shiftY, 1.0f);
    planes[3] = {0.0f, top[0], -top[1], 0.0f};
    planes[4] = {0.0f, 0.0f, -1.0f, -camera.nearZ};
    planes[5] = {0.0f, 0.0f, 1.0f, -camera.farZ};
    return planes;
}

namespace {

ViewVec3 transformed(const ViewRows& matrix, const ViewVec3& point) {
    ViewVec3 out{};
    for (std::size_t axis = 0; axis < 3; ++axis) {
        float value = matrix[0][axis] * point[0];
        value += matrix[1][axis] * point[1];
        value += matrix[2][axis] * point[2];
        out[axis] = value + matrix[3][axis];
    }
    return out;
}

}

bool viewSphereVisible(const ViewCamera& camera, const ViewPlanes& planes, const ViewRows& node, const ViewVec3& centre, float radius) {
    const ViewVec3 world = transformed(node, centre);
    const ViewVec3 delta{world[0] - camera.frame[3][0], world[1] - camera.frame[3][1], world[2] - camera.frame[3][2]};
    ViewVec3 local{};
    for (std::size_t axis = 0; axis < 3; ++axis) {
        float value = camera.frame[axis][0] * delta[0];
        value += camera.frame[axis][1] * delta[1];
        value += camera.frame[axis][2] * delta[2];
        local[axis] = value;
    }
    for (const ViewPlane& plane : planes) {
        const float xy = plane.a * local[0] + plane.b * local[1];
        const float zd = plane.c * local[2] + plane.d;
        if (xy + zd > radius) return false;
    }
    return true;
}

bool viewNodeCulled(const ViewCamera& camera, const ViewPlanes& planes, const ViewNode& node) {
    if (node.fade == 0 || node.alpha == 0) return true;
    if (node.sphereTest && !viewSphereVisible(camera, planes, node.matrix, node.centre, node.radius)) return true;
    return false;
}

}
