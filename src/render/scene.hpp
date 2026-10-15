#pragma once
// Development renderer shared by chase-viewer and chase-game: SDL2 + OpenGL 3.3 core through libepoxy, all GL calls
// in src/render. Materials run the texture-environment stages the game builds (assets::materialState) with the original's
// fragment lighting (runtime/lighting.hpp); reflections are stand-ins (docs/plans/features/rendering.md).
#include "assets/assets.hpp"
#include "runtime/lighting.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace chase::render {

// Column-major; the same memory layout as the game's row-major row-vector matrices.
using Mat4 = std::array<float, 16>;

Mat4 identity();
Mat4 multiply(const Mat4& a, const Mat4& b);
Mat4 rigidInverse(const Mat4& m);
// The game world is left-handed (+x right, +y up, +z forward at yaw 0), GL's is right-handed.
// perspective() mirrors x, so a view matrix built with a proper rotation shows the game world
// as the game does. The mirror reverses triangle winding; setFrontFace() accounts for it.
Mat4 perspective(float fovY, float aspect, float nearZ, float farZ);
// Game camera convention: forward = (cos p sin yaw, sin p, cos p cos yaw), screen right = (cos yaw, 0, -sin yaw).
void cameraAxes(float yaw, float pitch, float forward[3], float right[3]);
// View matrix for a camera at `eye` looking along yaw/pitch (radians), +Y up; use with perspective().
Mat4 view(const float eye[3], float yaw, float pitch);
// View matrix of a cutscene camera: the game camera matrix is left-handed (row 0 screen right, row 2 the view
// direction); negating rows 0 and 2 gives GL's camera and perspective() mirrors x back.
Mat4 cutsceneView(const assets::CutsceneCamera& camera);
// Sets which stored winding counts as front: counterClockwise is the winding in the game's own space.
void setFrontFace(bool counterClockwise);
// Sets the scene shader program up for a frame: uniforms and fixed-function state of the scene pass.
void beginFrame(float brightness);

// The original's fog (docs/re/rendering-fog.md): a 128-entry table indexed by the depth the PICA computes from a projection with
// the given near and far, then colour = f * colour + (1 - f) * fogColour for materials that are fogged. Off after beginFrame().
struct FogParameters {
    std::array<std::uint32_t, 128> table;  // value (11 bits) << 13 | signed 13-bit difference, both /2048
    std::array<std::uint8_t, 3> colour;
    float projectionNear, projectionFar;
};
void setFog(const FogParameters& fog);

std::string lower(std::string s);

// Archive convention, not recovered game logic: levels/levels/<name>/x.blvl draws
// models/<name>/*.bfnmdl without the _l1/_lod1 detail levels, or only <room>.bfnmdl.
std::vector<std::string> levelModels(const assets::FuseArchive& archive, const std::string& levelPath,
                                     const std::string& room);

// Saves the current GL framebuffer as a BMP.
void saveScreenshot(const std::string& path, int width, int height);

class SceneImpl;

class Scene {
public:
    Scene(assets::FuseArchive& archive, std::vector<std::string> exclude);
    ~Scene();
    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;

    // Untextured geometry drawn as its own model, e.g. a navigation overlay.
    void addGeometry(const std::string& key, const assets::ModelPart& part);
    // Adds one instance of a model; the model's GPU data is loaded once and shared.
    // textureOverrides maps a lower-case material texture stem to the texture used instead; it
    // only applies when the model is loaded for the first time under `key`. The GPU data is shared by every
    // instance with the same key (the path when `key` is empty), so models that differ in textures or in pose
    // need distinct keys.
    bool addModel(const std::string& path, const Mat4& transform = identity(),
                  const std::map<std::string, std::string>& textureOverrides = {}, const std::string& key = "", int onlyMesh = -1);
    bool setInstanceMesh(std::size_t instance, const std::string& path, int mesh);
    // One instance per node of the model that has a mesh (the mesh's GPU data is shared by key "path#mesh"); the result holds the
    // instance index of every node, -1 for a node without a mesh, and is empty when the model is missing. Move a node with setTransform().
    std::vector<std::ptrdiff_t> addModelNodes(const std::string& path, const Mat4& placement);
    // Re-skins every skinned part of the model loaded under `key` on the CPU.
    void pose(const std::string& key, const std::vector<assets::Matrix>& skin);
    std::size_t skinnedParts(const std::string& path) const;
    void bounds(float centre[3], float& radius) const;
    void draw(const Mat4& viewProjection) const;
    // Lights of the following draws and the camera position the view vector of the specular term uses (world space).
    void setLighting(const runtime::LightSet& lights, const float eye[3]);
    // STAND-IN RENDER-S50: sky instances are drawn by drawSky() only, without depth test or depth write (docs/re/rendering-sky.md).
    void drawSky(const Mat4& viewProjection) const;
    void setInstanceSky(std::size_t instance);
    std::size_t triangles() const;
    void setTransform(std::size_t instance, const Mat4& transform);
    void setInstanceVisible(std::size_t instance, bool visible);
    std::size_t missingTextures() const;
    std::size_t instances() const;

private:
    std::unique_ptr<SceneImpl> impl_;
};

}
