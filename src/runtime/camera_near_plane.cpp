#include "runtime/camera_near_plane.hpp"
#include <cmath>

namespace chase::runtime {

namespace {
constexpr float kNearMargin = 0.1f;
constexpr float kNearMinimum = 0.2f;
constexpr float kVerticalViewLimit = 0.98f;  // 0x22859c: above this |forward.y| the right axis is built from the x axis

// 0x257ea0
void normalise(float v[3]) {
    const float length = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    if (length <= 1.1920929e-7f) {
        v[0] = 0.0f;
        v[1] = 0.0f;
        v[2] = 1.0f;
        return;
    }
    const float scale = 1.0f / length;
    for (int i = 0; i < 3; ++i) v[i] *= scale;
}

float dot(const float a[3], const float b[3]) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }

struct View {
    float right[3], up[3], forward[3], eye[3];
    void toView(const float world[3], float out[3]) const {
        const float d[3] = {world[0] - eye[0], world[1] - eye[1], world[2] - eye[2]};
        out[0] = dot(d, right);
        out[1] = dot(d, up);
        out[2] = dot(d, forward);
    }
};

// 0x22859c with roll 0 and the translation replaced by the eye (0x2e0e28).
View lookAt(const float eye[3], const float look[3]) {
    View view;
    for (int i = 0; i < 3; ++i) {
        view.forward[i] = look[i] - eye[i];
        view.eye[i] = eye[i];
    }
    normalise(view.forward);
    const float* f = view.forward;
    if (std::fabs(f[1]) > kVerticalViewLimit) {
        view.right[0] = 0.0f, view.right[1] = -f[2], view.right[2] = f[1];
    } else {
        view.right[0] = f[2], view.right[1] = 0.0f, view.right[2] = -f[0];
    }
    normalise(view.right);
    const float* r = view.right;
    view.up[0] = f[1] * r[2] - f[2] * r[1];
    view.up[1] = f[2] * r[0] - f[0] * r[2];
    view.up[2] = f[0] * r[1] - f[1] * r[0];
    return view;
}
}

float cameraNearDepth(const CameraSweep& sweep, const float eye[3], const float look[3], float xScale, float yScale, float limit) {
    const View view = lookAt(eye, look);
    float planes[5][3] = {{0.0f, 0.0f, -1.0f}, {xScale, 0.0f, -1.0f}, {-xScale, 0.0f, -1.0f}, {0.0f, yScale, -1.0f}, {0.0f, -yScale, -1.0f}};
    for (int p = 1; p < 5; ++p) normalise(planes[p]);
    float nearest = limit + kNearMargin;
    const float* origin = sweep.origin();
    const float* depth = sweep.depthAxis();
    for (const CameraSweep::Shape shape : {CameraSweep::Side, CameraSweep::Up}) {
        const float* lateral = sweep.lateralAxis(shape);
        for (const CameraSweep::Segment& segment : sweep.rawSegments(shape)) {
            float ends[2][3];
            const float local[2][2] = {{segment.x0, segment.y0}, {segment.x1, segment.y1}};
            for (int e = 0; e < 2; ++e) {
                float world[3];
                for (int i = 0; i < 3; ++i) world[i] = local[e][1] * depth[i] + (origin[i] + local[e][0] * lateral[i]);
                view.toView(world, ends[e]);
            }
            bool outside = false;
            for (const float* normal : planes) {
                const float d0 = dot(ends[0], normal), d1 = dot(ends[1], normal);
                if (d0 > 0.0f) {
                    if (d1 > 0.0f) {
                        outside = true;
                        break;
                    }
                    const float t = d1 / (d1 - d0);
                    for (int i = 0; i < 3; ++i) ends[0][i] = (1.0f - t) * ends[1][i] + t * ends[0][i];
                } else if (0.0f < d1) {
                    const float t = d0 / (d0 - d1);
                    for (int i = 0; i < 3; ++i) ends[1][i] = (1.0f - t) * ends[0][i] + t * ends[1][i];
                }
            }
            if (outside) continue;
            if (nearest > ends[0][2]) nearest = ends[0][2];
            if (ends[1][2] < nearest) nearest = ends[1][2];
        }
    }
    const float result = nearest - kNearMargin;
    return result < kNearMinimum ? kNearMinimum : result;
}

}
