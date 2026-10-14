#pragma once
// Development renderer shared by chase-viewer and chase-game: SDL2 + OpenGL 3.3 core through libepoxy.
// Materials run the texture-environment stages the game builds (assets::materialState); fragment lighting,
// reflections and fog are stand-ins (docs/plans/features/rendering.md).
#include "assets/assets.hpp"
#include <epoxy/gl.h>
#include <array>
#include <map>
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
// Sets which stored winding counts as front: counterClockwise is the winding in the game's own space.
void setFrontFace(bool counterClockwise);

// Shader program with the uniforms Scene::draw sets.
GLuint createProgram();
void beginFrame(GLuint program, float brightness);

std::string lower(std::string s);

// Archive convention, not recovered game logic: levels/levels/<name>/x.blvl draws
// models/<name>/*.bfnmdl without the _l1/_lod1 detail levels, or only <room>.bfnmdl.
std::vector<std::string> levelModels(const assets::FuseArchive& archive, const std::string& levelPath,
                                     const std::string& room);

// Saves the current GL framebuffer as a BMP.
void saveScreenshot(const std::string& path, int width, int height);

class Scene {
public:
    Scene(assets::FuseArchive& archive, std::vector<std::string> exclude);
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
    // Re-skins every skinned part of the model loaded under `key` on the CPU.
    void pose(const std::string& key, const std::vector<assets::Matrix>& skin);
    std::size_t skinnedParts(const std::string& path) const;
    void bounds(float centre[3], float& radius) const;
    void draw(GLuint program, const Mat4& viewProjection) const;
    std::size_t triangles() const;
    void setTransform(std::size_t instance, const Mat4& transform) { instances_.at(instance).transform = transform; }
    std::size_t missingTextures() const { return missing_; }
    std::size_t instances() const { return instances_.size(); }

private:
    struct DrawPart {
        GLuint vao = 0, vbo = 0, ibo = 0;
        GLsizei count = 0;
        bool hasNormal = false;
        bool hasMaterial = false;
        bool secondUv = false;
        assets::MaterialState material;
        GLuint unitTexture[3] = {0, 0, 0};
        float unitFlat[3][4] = {};
        bool unitIsFlat[3] = {false, false, false};
    };
    struct TextureInfo {
        GLuint id = 0;
        bool cube = false;
        float average[4] = {1, 1, 1, 1};
    };
    struct SkinnedPart {
        std::size_t part;
        std::vector<assets::Vertex> vertices;
        unsigned flags;
    };
    struct LoadedModel {
        std::vector<DrawPart> parts;
        std::vector<SkinnedPart> skinned;
        float lo[3] = {1e30f, 1e30f, 1e30f}, hi[3] = {-1e30f, -1e30f, -1e30f};
    };
    struct Instance {
        const LoadedModel* model;
        Mat4 transform;
    };
    const TextureInfo& texture(const std::string& name, const std::string& modelDir);
    DrawPart upload(const assets::ModelPart& part, LoadedModel& model);
    void drawPart(GLuint program, const DrawPart& part, bool culling) const;

    assets::FuseArchive& archive_;
    std::vector<std::string> exclude_;
    std::map<std::string, std::vector<std::string>> texturePaths_;
    std::map<std::string, TextureInfo> textures_;
    std::map<std::string, LoadedModel> models_;
    std::vector<Instance> instances_;
    std::size_t missing_ = 0;
};

}
