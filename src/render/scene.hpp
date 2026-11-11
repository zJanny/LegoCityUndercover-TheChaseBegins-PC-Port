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
#include <optional>
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
const FogParameters* currentFog();

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
    // The transform is the scene node's world: every node of the model with a mesh is drawn with assets::nodeDrawMatrix first (0x205160 /
    // 0x10e028), skinned parts after their skinning. With onlyMesh set the instance draws that mesh alone and the caller's transform already
    // holds the node's matrix (doors' leaves, animated prop nodes and cutscene object tracks write it, as 0x22cbbc writes a palette entry).
    bool addModel(const std::string& path, const Mat4& transform = identity(),
                  const std::map<std::string, std::string>& textureOverrides = {}, const std::string& key = "", int onlyMesh = -1);
    bool setInstanceMesh(std::size_t instance, const std::string& path, int mesh);
    // One instance per node of the model that has a mesh (the mesh's GPU data is shared by key "path#mesh"); the result holds the
    // instance index of every node, -1 for a node without a mesh, and is empty when the model is missing. Move a node with setTransform(),
    // whose matrix holds the node's matrix as well.
    std::vector<std::ptrdiff_t> addModelNodes(const std::string& path, const Mat4& placement);
    // Replaces the matrix of one model node of a whole-model instance (the palette entry 0x22cbbc writes at node+0x110), row vectors
    // applied before the instance transform.
    void setNodeMatrix(std::size_t instance, int node, const assets::Matrix& matrix);
    // Re-skins every skinned part of the model loaded under `key` on the CPU.
    void pose(const std::string& key, const std::vector<assets::Matrix>& skin);
    std::size_t skinnedParts(const std::string& path) const;
    void bounds(float centre[3], float& radius) const;
    // The draw list of 0x10bbe8 (render/draw_order.hpp): opaque parts sorted by material and front to back, then blended parts (an alpha
    // below 255, a material colour alpha below 255 or a blend destination other than zero) back to front by the view depth of each part's box
    // centre. STAND-IN RENDER-S71: the texture bits of the material key and the order the parts enter the list are the port's.
    void draw(const Mat4& viewProjection) const;
    // The camera's near and far plane (camera +0xc4 / +0xc8) the depth key 0x25688c scales by; 1 and 500 until set, as 0x25688c without camera.
    void setDepthRange(float nearZ, float farZ);
    // Lights of the following draws and the camera position the view vector of the specular term uses (world space).
    void setLighting(const runtime::LightSet& lights, const float eye[3]);
    // STAND-IN RENDER-S50: sky instances are drawn by drawSky() only, without depth test or depth write (docs/re/rendering-sky.md).
    void drawSky(const Mat4& viewProjection) const;
    void setInstanceSky(std::size_t instance);
    std::size_t triangles() const;
    void setTransform(std::size_t instance, const Mat4& transform);
    void setInstanceVisible(std::size_t instance, bool visible);
    Mat4 instanceTransform(std::size_t instance) const;
    bool instanceVisible(std::size_t instance) const;
    // HUD-S70: the node alpha and colour of a node tween (0x21b7cc with flags 3, value 0..1) multiply the instance's colour and alpha. An alpha
    // below 1 puts every part into the blended list (0x10bbe8). STAND-IN PED-S12: such a part blends with (1, 1 - source alpha), or (1, 1) for a
    // destination of one, where 0x188778 uses source alpha.
    void setInstanceFade(std::size_t instance, float fade);
    // 0x2363d4 and 0x23f124: a node of the instance's model is hidden (the node's hidden bit +0x114), or shown with a part alpha 0..255 that scales the
    // instance's fade for that node's parts. A model without the node is not changed.
    void setInstanceNodeShown(std::size_t instance, int node, bool shown, std::uint8_t alpha = 255);
    float instanceFade(std::size_t instance) const;
    // The archive path of the model the instance draws.
    const std::string& instanceModel(std::size_t instance) const;
    // STAND-IN VEH-S100: the node colour of 0x23e110 (mesh node key 1, node +0x118) multiplies the fragment colour; where the original's draw
    // applies it is not read.
    void setInstanceTint(std::size_t instance, const std::array<float, 3>& tint);
    // 0x230ae8: the instance is drawn with the materials of assets::whiteMaterialPart while set.
    void setInstanceWhiteMaterials(std::size_t instance, bool white);
    // The override block 0x530834 for the following draws (assets::MaterialOverride); with flag 6 set, draws whose material is additive
    // (blend destination index 1) are skipped like the draw list 0x188778 does.
    void setMaterialOverride(const std::optional<assets::MaterialOverride>& override);
    std::size_t missingTextures() const;
    std::size_t instances() const;
    // Shadow_Caster / Shadow_Receiver of the instance's mesh nodes (+0xde bits 0 and 1, docs/re/rendering-shadows.md).
    void setInstanceShadow(std::size_t instance, bool caster, bool receiver);
    struct ShadowPart {
        unsigned vertexArray = 0;
        int indexCount = 0;
        Mat4 world{};
    };
    // The parts of the visible caster instances, or the receiving parts of the visible receiver instances: opaque, not a reflection
    // material and at most four stages, as 0x12ecd4 appends the shadow stages only then.
    std::vector<ShadowPart> shadowParts(bool casters) const;

private:
    std::unique_ptr<SceneImpl> impl_;
};

}
