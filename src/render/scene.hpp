#pragma once
// Development renderer shared by chase-viewer and chase-game: SDL2 + OpenGL 3.3 core through libepoxy.
// An approximation of the game's look; the PICA combiner and lighting setup is not recovered.
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
Mat4 perspective(float fovY, float aspect, float nearZ, float farZ);
// View matrix for a camera at `eye` looking along yaw/pitch (radians), +Y up.
Mat4 view(const float eye[3], float yaw, float pitch);

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
    bool addModel(const std::string& path, const Mat4& transform = identity());
    // Re-skins every skinned part of an already added model on the CPU.
    void pose(const std::string& path, const std::vector<assets::Matrix>& skin);
    std::size_t skinnedParts(const std::string& path) const;
    void bounds(float centre[3], float& radius) const;
    void draw(GLuint program, const Mat4& viewProjection) const;
    std::size_t triangles() const;
    void setTransform(std::size_t instance, const Mat4& transform) { instances_.at(instance).transform = transform; }
    std::size_t missingTextures() const { return missing_; }
    std::size_t instances() const { return instances_.size(); }

private:
    struct DrawPart {
        GLuint vao = 0, vbo = 0, ibo = 0, texture = 0, lightmap = 0;
        GLsizei count = 0;
        bool hasNormal = false;
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
    GLuint texture(const std::string& name, const std::string& modelDir);
    DrawPart upload(const assets::ModelPart& part, GLuint tex, LoadedModel& model);

    assets::FuseArchive& archive_;
    std::vector<std::string> exclude_;
    std::map<std::string, std::vector<std::string>> texturePaths_;
    std::map<std::string, GLuint> textures_;
    std::map<std::string, LoadedModel> models_;
    std::vector<Instance> instances_;
    std::size_t missing_ = 0;
};

}
