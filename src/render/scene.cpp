#include "render/scene.hpp"
#include "render/draw_order.hpp"
#include "render/frame_stats.hpp"
#include "render/program.hpp"
#include "render/reflection.hpp"
#include "runtime/reflection.hpp"
#include <SDL.h>
#include <epoxy/gl.h>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <optional>
#include <stdexcept>

namespace chase::render {
using namespace chase::assets;

namespace {
bool fogActive = false;
FogParameters activeFog{};

void uploadLut(GLuint program, const char* name, const runtime::LightLut& lut) {
    glUniform4uiv(glGetUniformLocation(program, name), GLsizei(lut.size() / 4), lut.data());
}

bool slotUsed(const MaterialState& state, const ModelPart& part, int slot) {
    for (const int used : state.unitSlot)
        if (used == slot) return true;
    const int unreflected = std::min(part.material.size() > 10 ? (part.material[10] & 0x3f) >> 3 : 0, 3);
    return slot < unreflected;
}

struct GpuVertex {
    float position[3], uv[2], normal[3];
    std::uint8_t colour[4];
    float uv1[2];
};
GpuVertex toGpu(const Vertex& v, unsigned flags) {
    GpuVertex g{};
    std::copy_n(v.position, 3, g.position);
    std::copy_n(v.uv0, 2, g.uv);
    std::copy_n(v.uv1, 2, g.uv1);
    std::copy_n(v.normal, 3, g.normal);
    std::copy_n(v.color, 4, g.colour);
    if (!(flags & AttrColor)) std::fill_n(g.colour, 4, std::uint8_t(255));
    return g;
}

std::string stem(const std::string& path) {
    auto slash = path.find_last_of("/\\");
    std::string name = slash == std::string::npos ? path : path.substr(slash + 1);
    auto dot = name.rfind('.');
    return lower(dot == std::string::npos ? name : name.substr(0, dot));
}
std::string directory(const std::string& path) {
    auto slash = path.find_last_of("/\\");
    return slash == std::string::npos ? std::string() : path.substr(0, slash + 1);
}
bool endsWith(const std::string& text, const std::string& suffix) {
    return text.size() >= suffix.size() && text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0;
}
}

Mat4 identity() { return {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1}; }

Mat4 multiply(const Mat4& a, const Mat4& b) {
    Mat4 r{};
    for (int c = 0; c < 4; ++c)
        for (int row = 0; row < 4; ++row)
            for (int k = 0; k < 4; ++k) r[c * 4 + row] += a[k * 4 + row] * b[c * 4 + k];
    return r;
}

Mat4 rigidInverse(const Mat4& m) {
    Mat4 r{};
    for (int row = 0; row < 3; ++row)
        for (int col = 0; col < 3; ++col) r[col * 4 + row] = m[row * 4 + col];
    for (int row = 0; row < 3; ++row) r[12 + row] = -(r[row] * m[12] + r[4 + row] * m[13] + r[8 + row] * m[14]);
    r[15] = 1;
    return r;
}

Mat4 cutsceneView(const assets::CutsceneCamera& camera) {
    Mat4 world = assets::cameraMatrix(camera);
    for (int i : {0, 1, 2, 8, 9, 10}) world[i] = -world[i];
    return rigidInverse(world);
}

Mat4 perspective(float fovY, float aspect, float nearZ, float farZ) {
    float f = 1.0f / std::tan(fovY / 2);
    Mat4 m{};
    m[0] = -f / aspect;
    m[5] = f;
    m[10] = (farZ + nearZ) / (nearZ - farZ);
    m[11] = -1;
    m[14] = 2 * farZ * nearZ / (nearZ - farZ);
    return m;
}

void cameraAxes(float yaw, float pitch, float forward[3], float right[3]) {
    forward[0] = std::cos(pitch) * std::sin(yaw);
    forward[1] = std::sin(pitch);
    forward[2] = std::cos(pitch) * std::cos(yaw);
    right[0] = std::cos(yaw);
    right[1] = 0;
    right[2] = -std::sin(yaw);
}

Mat4 view(const float eye[3], float yaw, float pitch) {
    float forward[3], right[3];
    cameraAxes(yaw, pitch, forward, right);
    const float fx = forward[0], fy = forward[1], fz = forward[2];
    const float rx = -right[0], rz = -right[2];
    const float ux = -rz * fy, uy = rz * fx - rx * fz, uz = rx * fy;
    Mat4 m{rx, ux, -fx, 0, 0, uy, -fy, 0, rz, uz, -fz, 0, 0, 0, 0, 1};
    m[12] = -(rx * eye[0] + rz * eye[2]);
    m[13] = -(ux * eye[0] + uy * eye[1] + uz * eye[2]);
    m[14] = fx * eye[0] + fy * eye[1] + fz * eye[2];
    return m;
}

void setFrontFace(bool counterClockwise) { glFrontFace(counterClockwise ? GL_CW : GL_CCW); }

void beginFrame(float brightness) {
    const GLuint program = sceneProgram();
    glUseProgram(program);
    glUniform1i(glGetUniformLocation(program, "unit0"), 0);
    glUniform1i(glGetUniformLocation(program, "unit1"), 1);
    glUniform1i(glGetUniformLocation(program, "unit2"), 2);
    glUniform1f(glGetUniformLocation(program, "brightness"), brightness);
    glUniform1f(glGetUniformLocation(program, "instanceFade"), 1.0f);
    const float noTint[3] = {1, 1, 1};
    glUniform3fv(glGetUniformLocation(program, "instanceTint"), 1, noTint);
    static const runtime::LightLut distribution = runtime::distributionLut(), spot = runtime::spotLut(), distance = runtime::distanceLut();
    uploadLut(program, "lutDistribution", distribution);
    uploadLut(program, "lutSpot", spot);
    uploadLut(program, "lutDistance", distance);
    glUniform1i(glGetUniformLocation(program, "clampHighlights"), (runtime::kLightingConfig0 >> 27) & 1);
    glUniform1i(glGetUniformLocation(program, "lightingOn"), 0);
    glUniform1i(glGetUniformLocation(program, "stageCount"), 0);
    glUniform1i(glGetUniformLocation(program, "cubeOn"), 0);
    glUniform1f(glGetUniformLocation(program, "alphaReference"), -1.0f);
    glUniform1i(glGetUniformLocation(program, "fogOn"), 0);
    fogActive = false;
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
}

void setFog(const FogParameters& fog) {
    const GLuint program = sceneProgram();
    glUseProgram(program);
    glUniform1uiv(glGetUniformLocation(program, "fogLut"), 128, fog.table.data());
    glUniform3f(glGetUniformLocation(program, "fogColour"), float(fog.colour[0]) / 255.0f, float(fog.colour[1]) / 255.0f,
                float(fog.colour[2]) / 255.0f);
    glUniform2f(glGetUniformLocation(program, "fogProjection"), fog.projectionNear, fog.projectionFar);
    activeFog = fog;
    fogActive = true;
}

const FogParameters* currentFog() { return fogActive ? &activeFog : nullptr; }

std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return s;
}

std::vector<std::string> levelModels(const FuseArchive& archive, const std::string& levelPath, const std::string& room) {
    std::string dir = directory(lower(levelPath));
    dir.pop_back();
    std::string folder = "models/" + dir.substr(dir.find_last_of('/') + 1) + "/";
    std::vector<std::string> out;
    for (const auto& name : archive.names()) {
        if (name.rfind(folder, 0) != 0 || !endsWith(name, ".bfnmdl")) continue;
        if (!room.empty() ? name != folder + room + ".bfnmdl" : endsWith(name, "_l1.bfnmdl") || endsWith(name, "_lod1.bfnmdl"))
            continue;
        out.push_back(name);
    }
    return out;
}

void saveScreenshot(const std::string& path, int width, int height) {
    std::vector<std::uint8_t> pixels(std::size_t(width) * height * 4), flipped(pixels.size());
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    std::size_t row = std::size_t(width) * 4;
    for (int y = 0; y < height; ++y)
        std::copy_n(pixels.begin() + std::ptrdiff_t(row * std::size_t(height - 1 - y)), row,
                    flipped.begin() + std::ptrdiff_t(row * std::size_t(y)));
    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormatFrom(flipped.data(), width, height, 32, int(row),
                                                              SDL_PIXELFORMAT_ABGR8888);
    if (!surface || SDL_SaveBMP(surface, path.c_str()) != 0) throw std::runtime_error(SDL_GetError());
    SDL_FreeSurface(surface);
}

class SceneImpl {
public:
    SceneImpl(FuseArchive& archive, std::vector<std::string> exclude);
    void addGeometry(const std::string& key, const ModelPart& part);
    bool addModel(const std::string& path, const Mat4& transform = identity(),
                  const std::map<std::string, std::string>& textureOverrides = {}, const std::string& key = "", int onlyMesh = -1);
    bool setInstanceMesh(std::size_t instance, const std::string& path, int mesh);
    std::vector<std::ptrdiff_t> addModelNodes(const std::string& path, const Mat4& placement);
    void setNodeMatrix(std::size_t instance, int node, const Matrix& matrix) { instances_.at(instance).nodeMatrix[node] = matrix; }
    void pose(const std::string& key, const std::vector<Matrix>& skin);
    std::size_t skinnedParts(const std::string& path) const;
    void bounds(float centre[3], float& radius) const;
    void draw(const Mat4& viewProjection) const;
    void drawSky(const Mat4& viewProjection) const;
    void setLighting(const runtime::LightSet& lights, const float eye[3]) {
        lights_ = lights;
        std::copy_n(eye, 3, eye_);
    }
    void setInstanceSky(std::size_t instance) { instances_.at(instance).sky = true; }
    std::size_t triangles() const;
    void setTransform(std::size_t instance, const Mat4& transform) { instances_.at(instance).transform = transform; }
    void setInstanceVisible(std::size_t instance, bool visible) { instances_.at(instance).visible = visible; }
    bool instanceVisible(std::size_t instance) const { return instances_.at(instance).visible; }
    Mat4 instanceTransform(std::size_t instance) const { return instances_.at(instance).transform; }
    void setInstanceFade(std::size_t instance, float fade) { instances_.at(instance).fade = fade; }
    void setInstanceNodeShown(std::size_t instance, int node, bool shown, std::uint8_t alpha) { instances_.at(instance).nodeShow[node] = {!shown, alpha}; }
    void setInstanceTint(std::size_t instance, const std::array<float, 3>& tint) { instances_.at(instance).tint = tint; }
    float instanceFade(std::size_t instance) const { return instances_.at(instance).fade; }
    const std::string& instanceModel(std::size_t instance) const { return instances_.at(instance).path; }
    void setDepthRange(float nearZ, float farZ) {
        depthNear_ = nearZ;
        depthFar_ = farZ;
    }
    void setInstanceWhiteMaterials(std::size_t instance, bool white) { instances_.at(instance).whiteMaterials = white; }
    void setMaterialOverride(const std::optional<MaterialOverride>& override) { override_ = override; }
    std::size_t missingTextures() const { return missing_; }
    std::size_t instances() const { return instances_.size(); }
    void setInstanceShadow(std::size_t instance, bool caster, bool receiver) {
        instances_.at(instance).shadowCaster = caster;
        instances_.at(instance).shadowReceiver = receiver;
    }
    std::vector<Scene::ShadowPart> shadowParts(bool casters) const;

private:
    struct DrawPart {
        GLuint vao = 0, vbo = 0, ibo = 0;
        GLsizei count = 0;
        bool hasNormal = false;
        bool hasMaterial = false;
        bool secondUv = false;
        assets::MaterialState material;
        assets::MaterialState whiteMaterial;
        assets::ModelPart source;
        mutable assets::MaterialState overridden;
        mutable std::uint64_t overriddenKey = ~std::uint64_t(0);
        std::vector<GLuint> slotTexture;
        std::vector<bool> slotCube;
        std::uint8_t colour[4] = {255, 255, 255, 255}, specular[4] = {255, 255, 255, 255};
        bool doubled = false;
        int node = -1;
        Mat4 nodeMatrix = identity();
        bool placed = false;
        float sortCentre[3] = {0, 0, 0};
        bool sortsFirst = false;
        std::uint64_t materialKey = 0;
        std::uint8_t sourceIndex = 1, destinationIndex = 0;
    };
    struct TextureInfo {
        GLuint id = 0;
        bool cube = false;
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
        bool visible = true;
        bool sky = false;
        float fade = 1.0f;
        std::array<float, 3> tint{1, 1, 1};
        bool whiteMaterials = false;
        std::map<int, Mat4> nodeMatrix = {};
        struct NodeShow {
            bool hidden = false;
            std::uint8_t alpha = 255;
        };
        std::map<int, NodeShow> nodeShow = {};
        std::string path = {};
        bool shadowCaster = false, shadowReceiver = false;
    };
    struct Queued {
        const Instance* instance;
        const DrawPart* part;
    };
    void drawInstances(const Mat4& viewProjection, bool skyPass) const;
    void drawQueued(GLuint program, const Mat4& viewProjection, const Queued& item, bool culling, bool skyPass) const;
    static Mat4 partWorld(const Instance& instance, const DrawPart& part);
    static bool partHidden(const Instance& instance, const DrawPart& part);
    static float partFade(const Instance& instance, const DrawPart& part);
    std::uint64_t partSortKey(const ModelPart& part);
    const TextureInfo& texture(const std::string& name, const std::string& modelDir);
    DrawPart upload(const ModelPart& part, LoadedModel& model, const Mat4& nodeMatrix = identity());
    void drawPart(GLuint program, const DrawPart& part, const MaterialState& m, bool culling, bool skyPass, bool white) const;
    void setPartLighting(GLuint program, const DrawPart& part, const MaterialState& m, bool white) const;
    const MaterialState& overriddenState(const DrawPart& part) const;

    FuseArchive& archive_;
    std::vector<std::string> exclude_;
    std::map<std::string, std::vector<std::string>> texturePaths_;
    std::map<std::string, TextureInfo> textures_;
    std::map<std::string, LoadedModel> models_;
    std::map<std::string, std::vector<int>> nodeMeshes_;
    std::vector<Instance> instances_;
    std::size_t missing_ = 0;
    runtime::LightSet lights_;
    float eye_[3] = {0, 0, 0};
    std::optional<MaterialOverride> override_;
    std::map<std::string, std::uint32_t> textureOrder_;
    float depthNear_ = 1.0f, depthFar_ = 500.0f;
};

SceneImpl::SceneImpl(FuseArchive& archive, std::vector<std::string> exclude) : archive_(archive), exclude_(std::move(exclude)) {
    for (const auto& name : archive_.names()) {
        auto dot = name.rfind('.');
        std::string ext = dot == std::string::npos ? "" : name.substr(dot);
        if (ext == ".btga" || ext == ".bpng" || ext == ".bdds") texturePaths_[stem(name)].push_back(name);
    }
}

void SceneImpl::addGeometry(const std::string& key, const ModelPart& part) {
    LoadedModel out;
    out.parts.push_back(upload(part, out));
    Instance instance{&models_.emplace(key, std::move(out)).first->second, identity()};
    instance.path = key;
    instances_.push_back(std::move(instance));
}

bool SceneImpl::addModel(const std::string& path, const Mat4& transform,
                     const std::map<std::string, std::string>& textureOverrides, const std::string& key, int onlyMesh) {
    const std::string& cacheKey = key.empty() ? path : key;
    auto loaded = models_.find(cacheKey);
    if (loaded == models_.end()) {
        const FuseEntry* entry = archive_.find(path);
        if (!entry) return false;
        Model model = parseModel(archive_.read(*entry));
        LoadedModel out;
        const std::string dir = directory(lower(path));
        for (std::size_t node = 0; node < model.nodeMesh.size(); ++node) {
            const int meshIndex = model.nodeMesh[node];
            if (meshIndex < 0 || std::size_t(meshIndex) >= model.meshes.size()) continue;
            if (onlyMesh >= 0 && meshIndex != onlyMesh) continue;
            const Mat4 nodeMatrix = onlyMesh >= 0 ? identity() : nodeDrawMatrix(model, node);
            for (const auto& part : model.meshes[std::size_t(meshIndex)].parts) {
                std::string texName = part.textures.empty() ? "" : part.textures.front();
                bool skip = false;
                for (const auto& word : exclude_) skip |= lower(texName).find(word) != std::string::npos;
                if (skip || part.indices.empty()) continue;
                DrawPart d = upload(part, out, nodeMatrix);
                std::copy_n(part.centre, 3, d.sortCentre);
                d.sortsFirst = model.meshes[std::size_t(meshIndex)].flags & 0x100;
                if (d.hasMaterial) d.materialKey = partSortKey(part);
                d.node = int(node);
                d.nodeMatrix = nodeMatrix;
                d.placed = nodeMatrix != identity();
                const std::size_t slots = d.hasMaterial ? part.textures.size() : 0;
                d.slotTexture.assign(slots, 0);
                d.slotCube.assign(slots, false);
                for (std::size_t slot = 0; slot < slots; ++slot) {
                    if (!slotUsed(d.material, part, int(slot))) continue;
                    std::string name = part.textures[slot];
                    const auto replacement = textureOverrides.find(stem(name));
                    if (replacement != textureOverrides.end()) name = replacement->second;
                    const TextureInfo& info = texture(name, dir);
                    d.slotTexture[slot] = info.id;
                    d.slotCube[slot] = info.cube;
                }
                if (part.flags & AttrSkin) out.skinned.push_back({out.parts.size(), part.vertices, part.flags});
                out.parts.push_back(d);
            }
        }
        loaded = models_.emplace(cacheKey, std::move(out)).first;
    }
    Instance instance{&loaded->second, transform};
    instance.path = path;
    instances_.push_back(std::move(instance));
    return true;
}

bool SceneImpl::setInstanceMesh(std::size_t instance, const std::string& path, int mesh) {
    const Mat4 transform = instances_.at(instance).transform;
    if (!addModel(path, transform, {}, path + "#" + std::to_string(mesh), mesh)) return false;
    instances_.at(instance).model = instances_.back().model;
    instances_.at(instance).path = path;
    instances_.pop_back();
    return true;
}

std::vector<std::ptrdiff_t> SceneImpl::addModelNodes(const std::string& path, const Mat4& placement) {
    auto known = nodeMeshes_.find(path);
    if (known == nodeMeshes_.end()) {
        const FuseEntry* entry = archive_.find(path);
        if (!entry) return {};
        known = nodeMeshes_.emplace(path, parseModel(archive_.read(*entry)).nodeMesh).first;
    }
    std::vector<std::ptrdiff_t> out;
    for (const int mesh : known->second) {
        if (mesh < 0 || !addModel(path, placement, {}, path + "#" + std::to_string(mesh), mesh)) {
            out.push_back(-1);
            continue;
        }
        out.push_back(std::ptrdiff_t(instances_.size()) - 1);
    }
    return out;
}

void SceneImpl::pose(const std::string& path, const std::vector<Matrix>& skin) {
    auto loaded = models_.find(path);
    if (loaded == models_.end()) return;
    std::vector<GpuVertex> vertices;
    for (const auto& source : loaded->second.skinned) {
        vertices.clear();
        for (const auto& v : source.vertices) {
            GpuVertex g = toGpu(v, source.flags);
            skinVertex(v, skin, g.position, g.normal);
            vertices.push_back(g);
        }
        glBindBuffer(GL_ARRAY_BUFFER, loaded->second.parts[source.part].vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, GLsizeiptr(vertices.size() * sizeof(GpuVertex)), vertices.data());
    }
}

std::size_t SceneImpl::skinnedParts(const std::string& path) const {
    auto loaded = models_.find(path);
    return loaded == models_.end() ? 0 : loaded->second.skinned.size();
}

void SceneImpl::bounds(float centre[3], float& radius) const {
    float lo[3] = {1e30f, 1e30f, 1e30f}, hi[3] = {-1e30f, -1e30f, -1e30f};
    for (const auto& instance : instances_) {
        if (instance.model->parts.empty() || instance.sky) continue;
        for (int corner = 0; corner < 8; ++corner) {
            float p[3];
            for (int i = 0; i < 3; ++i) p[i] = (corner >> i & 1) ? instance.model->hi[i] : instance.model->lo[i];
            for (int i = 0; i < 3; ++i) {
                const Mat4& m = instance.transform;
                float w = m[i] * p[0] + m[4 + i] * p[1] + m[8 + i] * p[2] + m[12 + i];
                lo[i] = std::min(lo[i], w);
                hi[i] = std::max(hi[i], w);
            }
        }
    }
    radius = 1;
    for (int i = 0; i < 3; ++i) {
        centre[i] = lo[i] > hi[i] ? 0 : (lo[i] + hi[i]) / 2;
        radius = std::max(radius, (hi[i] - lo[i]) / 2);
    }
}

std::uint64_t SceneImpl::partSortKey(const ModelPart& part) {
    const std::size_t slots = std::size_t((part.material[10] & 0x3f) >> 3);
    std::vector<std::uint32_t> words(slots, 0);
    for (std::size_t slot = 0; slot < slots && slot < part.textures.size(); ++slot)
        words[slot] = textureOrder_.emplace(lower(part.textures[slot]), std::uint32_t(textureOrder_.size() + 1)).first->second;
    return materialSortKey(part.material.data(), words);
}

const MaterialState& SceneImpl::overriddenState(const DrawPart& part) const {
    const MaterialOverride& o = *override_;
    const std::uint64_t key = std::uint64_t(o.flags & ~0x40u) | std::uint64_t(o.mode) << 8 | std::uint64_t(o.lighting & 0xf) << 16 |
                              std::uint64_t(o.texture & 0xf) << 20 | std::uint64_t(o.alpha & 7) << 24 | std::uint64_t(o.extra & 3) << 28 |
                              std::uint64_t(o.constant & 0xffffff) << 32;
    if (part.overriddenKey != key) {
        part.overridden = materialState(part.source, o);
        part.overriddenKey = key;
    }
    return part.overridden;
}

void SceneImpl::drawPart(GLuint program, const DrawPart& part, const MaterialState& m, bool culling, bool skyPass, bool white) const {
    glUniform1i(glGetUniformLocation(program, "stageCount"), part.hasMaterial ? GLint(m.stages.size()) : 0);
    glUniform1i(glGetUniformLocation(program, "hasNormal"), part.hasNormal);
    setPartLighting(program, part, m, white);
    glUniform1f(glGetUniformLocation(program, "alphaReference"), m.alphaTest ? float(m.alphaReference) / 255.0f : -1.0f);
    glUniform1i(glGetUniformLocation(program, "fogOn"), fogActive && part.hasMaterial && m.fogged);
    if (part.hasMaterial) {
        GLint words[6 * 4] = {};
        GLfloat constants[6 * 4] = {};
        for (std::size_t i = 0; i < m.stages.size(); ++i) {
            const TevStage& t = m.stages[i];
            words[i * 4 + 0] = GLint(t.source);
            words[i * 4 + 1] = GLint(t.operand);
            words[i * 4 + 2] = GLint(t.combiner);
            words[i * 4 + 3] = GLint(t.scale);
            for (int c = 0; c < 4; ++c) constants[i * 4 + std::size_t(c)] = float((t.constant >> (8 * c)) & 0xff) / 255.0f;
        }
        glUniform4iv(glGetUniformLocation(program, "stageWords"), 6, words);
        glUniform4fv(glGetUniformLocation(program, "stageConstant"), 6, constants);
        glUniform1i(glGetUniformLocation(program, "bufferBits"), GLint(m.bufferUpdate));
        GLint modes[3] = {0, 0, 0}, uvSets[3] = {0, 0, 0};
        GLuint unitTexture[3] = {0, 0, 0}, cube = 0;
        for (int unit = 0; unit < 3; ++unit) {
            uvSets[unit] = runtime::unitUvSet(unit, m.reflection, part.secondUv);
            const int slot = m.unitSlot[unit];
            if (slot < 0 || std::size_t(slot) >= part.slotTexture.size()) continue;
            const GLuint texture = part.slotTexture[std::size_t(slot)];
            const runtime::UnitSampling sampling = runtime::unitSampling(unit, m.reflection, texture != 0, part.slotCube[std::size_t(slot)]);
            modes[unit] = int(sampling);
            if (sampling == runtime::UnitSampling::Texture) unitTexture[unit] = texture;
            if (sampling == runtime::UnitSampling::Cube) cube = texture;
        }
        glUniform1iv(glGetUniformLocation(program, "unitMode"), 3, modes);
        glUniform1iv(glGetUniformLocation(program, "unitUv"), 3, uvSets);
        glUniform1i(glGetUniformLocation(program, "cubeOn"), m.reflection);
        glUniform3fv(glGetUniformLocation(program, "eyePosition"), 1, eye_);
        bindCubeTexture(program, cube);
        for (int unit = 2; unit >= 0; --unit) {
            glActiveTexture(GLenum(GL_TEXTURE0 + unit));
            glBindTexture(GL_TEXTURE_2D, unitTexture[unit]);
        }
        if (m.depthTest) {
            static const GLenum functions[8] = {GL_NEVER, GL_ALWAYS, GL_EQUAL, GL_NOTEQUAL, GL_LESS, GL_LEQUAL, GL_GREATER, GL_GEQUAL};
            glDepthFunc(functions[m.depthFunction & 7]);
        } else {
            glDepthFunc(GL_ALWAYS);
        }
        if (skyPass) glDepthFunc(GL_ALWAYS);
        glDepthMask(m.depthWrite && !skyPass ? GL_TRUE : GL_FALSE);
        if (culling) {
            if (m.cullMode == 0) {
                glDisable(GL_CULL_FACE);
            } else {
                glEnable(GL_CULL_FACE);
                glCullFace(m.cullMode == 1 ? GL_BACK : GL_FRONT);
            }
        }
    } else {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
        GLint modes[3] = {0, 0, 0};
        glUniform1iv(glGetUniformLocation(program, "unitMode"), 3, modes);
        glUniform1i(glGetUniformLocation(program, "cubeOn"), 0);
    }
    glBindVertexArray(part.vao);
    countDrawCall();
    glDrawElements(GL_TRIANGLES, part.count, GL_UNSIGNED_SHORT, nullptr);
}

void SceneImpl::setPartLighting(GLuint program, const DrawPart& part, const MaterialState& m, bool white) const {
    const std::uint8_t whiteColour[4] = {255, 255, 255, part.colour[3]};
    const runtime::FragmentLighting lighting =
        part.hasMaterial ? runtime::fragmentLighting(lights_, m.lightingKey, white ? whiteColour : part.colour, part.specular, part.doubled)
                         : runtime::FragmentLighting{};
    glUniform1i(glGetUniformLocation(program, "lightingOn"), lighting.enabled);
    if (!lighting.enabled) return;
    const auto unit = [](const std::array<std::uint8_t, 3>& c, GLfloat* out) {
        for (std::size_t i = 0; i < 3; ++i) out[i] = float(c[i]) / 255.0f;
    };
    GLfloat ambient[3], diffuse[8 * 3] = {}, lit[8 * 3] = {}, shine[8 * 3] = {}, vectors[8 * 3] = {}, scales[8] = {};
    GLint flags[8] = {};
    unit(lighting.globalAmbient, ambient);
    const std::size_t count = std::min(lighting.slots.size(), runtime::kMaxLights);
    for (std::size_t i = 0; i < count; ++i) {
        const runtime::LightSlot& slot = lighting.slots[i];
        unit(slot.diffuse, diffuse + i * 3);
        unit(slot.ambient, lit + i * 3);
        unit(slot.specular, shine + i * 3);
        std::copy(slot.vector.begin(), slot.vector.end(), vectors + i * 3);
        flags[i] = (slot.directional ? 1 : 0) | (slot.twoSided ? 2 : 0) | (slot.spotAttenuation ? 4 : 0) | (slot.distanceAttenuation ? 8 : 0);
        scales[i] = slot.distanceScale;
    }
    glUniform3fv(glGetUniformLocation(program, "globalAmbient"), 1, ambient);
    glUniform1i(glGetUniformLocation(program, "lightCount"), GLint(count));
    glUniform3fv(glGetUniformLocation(program, "lightDiffuse"), 8, diffuse);
    glUniform3fv(glGetUniformLocation(program, "lightAmbient"), 8, lit);
    glUniform3fv(glGetUniformLocation(program, "lightSpecular"), 8, shine);
    glUniform3fv(glGetUniformLocation(program, "lightVector"), 8, vectors);
    glUniform1iv(glGetUniformLocation(program, "lightFlags"), 8, flags);
    glUniform1fv(glGetUniformLocation(program, "lightDistanceScale"), 8, scales);
    glUniform3fv(glGetUniformLocation(program, "eyePosition"), 1, eye_);
}

void SceneImpl::draw(const Mat4& viewProjection) const { drawInstances(viewProjection, false); }

void SceneImpl::drawSky(const Mat4& viewProjection) const { drawInstances(viewProjection, true); }

namespace {

std::uint8_t nodeAlphaByte(float fade) { return std::uint8_t(std::clamp(int(fade * 255.0f + 0.5f), 0, 255)); }

float clipDepth(const Mat4& clip, const float point[3]) { return clip[3] * point[0] + clip[7] * point[1] + clip[11] * point[2] + clip[15]; }

}

bool SceneImpl::partHidden(const Instance& instance, const DrawPart& part) {
    const auto shown = instance.nodeShow.find(part.node);
    return shown != instance.nodeShow.end() && shown->second.hidden;
}

float SceneImpl::partFade(const Instance& instance, const DrawPart& part) {
    const auto shown = instance.nodeShow.find(part.node);
    if (shown == instance.nodeShow.end() || shown->second.alpha == 255) return instance.fade;
    return instance.fade * (float(shown->second.alpha) / 255.0f);
}

Mat4 SceneImpl::partWorld(const Instance& instance, const DrawPart& part) {
    const auto replaced = instance.nodeMatrix.find(part.node);
    if (replaced != instance.nodeMatrix.end()) return multiply(instance.transform, replaced->second);
    return part.placed ? multiply(instance.transform, part.nodeMatrix) : instance.transform;
}

void SceneImpl::drawQueued(GLuint program, const Mat4& viewProjection, const Queued& item, bool culling, bool skyPass) const {
    static const GLenum factors[10] = {GL_ZERO, GL_ONE, GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR, GL_SRC_ALPHA,
                                       GL_ONE_MINUS_SRC_ALPHA, GL_DST_COLOR, GL_ONE_MINUS_DST_COLOR, GL_DST_ALPHA,
                                       GL_ONE_MINUS_DST_ALPHA};
    static const GLenum equations[5] = {GL_FUNC_ADD, GL_FUNC_SUBTRACT, GL_FUNC_REVERSE_SUBTRACT, GL_MIN, GL_MAX};
    const Instance& instance = *item.instance;
    const DrawPart& part = *item.part;
    const Mat4 world = partWorld(instance, part);
    const Mat4 clip = multiply(viewProjection, world);
    glUniformMatrix4fv(glGetUniformLocation(program, "mvp"), 1, GL_FALSE, clip.data());
    glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, world.data());
    const float fade = partFade(instance, part);
    glUniform1f(glGetUniformLocation(program, "instanceFade"), fade);
    glUniform3fv(glGetUniformLocation(program, "instanceTint"), 1, instance.tint.data());
    const std::uint8_t nodeAlpha = nodeAlphaByte(fade);
    const std::array<std::uint8_t, 2> indices = drawBlendIndices(part.sourceIndex, part.destinationIndex, drawAlpha(part.colour[3], nodeAlpha));
    if (!part.hasMaterial) {
        glDisable(GL_BLEND);
    } else if (nodeAlpha < 255) {
        // STAND-IN PED-S12: the shader already multiplies colour and alpha by the node alpha (HUD-S70), so the source factor is one.
        glEnable(GL_BLEND);
        glBlendEquation(GL_FUNC_ADD);
        glBlendFunc(GL_ONE, indices[1] == 1 ? GL_ONE : GL_ONE_MINUS_SRC_ALPHA);
    } else if (indices[0] == part.sourceIndex && indices[1] == part.destinationIndex) {
        if (part.material.blendSource == 1 && part.material.blendDestination == 0 && part.material.blendEquation == 0) {
            glDisable(GL_BLEND);
        } else {
            static const GLenum picaFactors[10] = {GL_ZERO, GL_ONE, GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR, GL_DST_COLOR,
                                                   GL_ONE_MINUS_DST_COLOR, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_DST_ALPHA,
                                                   GL_ONE_MINUS_DST_ALPHA};
            glEnable(GL_BLEND);
            glBlendEquation(equations[std::min(part.material.blendEquation, 4u)]);
            glBlendFunc(picaFactors[std::min(part.material.blendSource, 9u)], picaFactors[std::min(part.material.blendDestination, 9u)]);
        }
    } else {
        glEnable(GL_BLEND);
        glBlendEquation(equations[std::min(part.material.blendEquation, 4u)]);
        glBlendFunc(factors[std::min<unsigned>(indices[0], 9u)], factors[std::min<unsigned>(indices[1], 9u)]);
    }
    const bool overridden = override_ && part.hasMaterial;
    const bool white = instance.whiteMaterials && part.hasMaterial && part.whiteMaterial.supported;
    const MaterialState* state = white ? &part.whiteMaterial : &part.material;
    if (overridden && !white && overriddenState(part).supported) state = &overriddenState(part);
    drawPart(program, part, *state, culling, skyPass, white);
}

void SceneImpl::drawInstances(const Mat4& viewProjection, bool skyPass) const {
    const GLuint program = sceneProgram();
    const bool culling = glIsEnabled(GL_CULL_FACE);
    std::vector<Queued> queued, skyOpaque, skyBlended;
    DrawList list;
    for (const auto& instance : instances_) {
        if (!instance.visible || instance.sky != skyPass) continue;
        for (const auto& part : instance.model->parts) {
            if (override_ && part.hasMaterial && (override_->flags & 0x40) && part.material.additive) continue;
            if (partHidden(instance, part)) continue;
            const std::uint8_t nodeAlpha = nodeAlphaByte(partFade(instance, part));
            const bool blended = part.hasMaterial && blendedPass(part.colour[3], part.destinationIndex, nodeAlpha);
            if (skyPass) {
                (blended ? skyBlended : skyOpaque).push_back({&instance, &part});
                continue;
            }
            std::uint32_t depth = depthSortKey(clipDepth(multiply(viewProjection, partWorld(instance, part)), part.sortCentre), depthNear_, depthFar_);
            if (blended && part.sortsFirst) depth = 0;
            if (blended) {
                list.addBlended(blendedItemKey(part.materialKey, depth), queued.size());
            } else {
                list.addOpaque(opaqueItemKey(part.materialKey, depth), queued.size());
            }
            queued.push_back({&instance, &part});
        }
    }
    for (const Queued& item : skyOpaque) drawQueued(program, viewProjection, item, culling, skyPass);
    for (const Queued& item : skyBlended) drawQueued(program, viewProjection, item, culling, skyPass);
    for (const DrawItem& item : list.sortedOpaque()) drawQueued(program, viewProjection, queued[item.ref], culling, skyPass);
    for (const DrawItem& item : list.sortedBlended()) drawQueued(program, viewProjection, queued[item.ref], culling, skyPass);
    glDisable(GL_BLEND);
    glUniform1f(glGetUniformLocation(program, "instanceFade"), 1.0f);
    const float noTint[3] = {1, 1, 1};
    glUniform3fv(glGetUniformLocation(program, "instanceTint"), 1, noTint);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
    glUniform1i(glGetUniformLocation(program, "stageCount"), 0);
    glUniform1f(glGetUniformLocation(program, "alphaReference"), -1.0f);
    glUniform1i(glGetUniformLocation(program, "lightingOn"), 0);
    if (culling) {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
    }
}

std::vector<Scene::ShadowPart> SceneImpl::shadowParts(bool casters) const {
    std::vector<Scene::ShadowPart> out;
    for (const auto& instance : instances_) {
        if (!instance.visible || instance.sky || !(casters ? instance.shadowCaster : instance.shadowReceiver)) continue;
        for (const auto& part : instance.model->parts) {
            if (partHidden(instance, part)) continue;
            const std::uint8_t nodeAlpha = nodeAlphaByte(partFade(instance, part));
            const bool blended = part.hasMaterial && blendedPass(part.colour[3], part.destinationIndex, nodeAlpha);
            if (!casters && (!part.hasMaterial || blended || part.material.reflection || part.material.stages.size() > 4)) continue;
            out.push_back({part.vao, int(part.count), partWorld(instance, part)});
        }
    }
    return out;
}

std::size_t SceneImpl::triangles() const {
    std::size_t n = 0;
    for (const auto& instance : instances_)
        for (const auto& p : instance.model->parts) n += std::size_t(p.count) / 3;
    return n;
}

const SceneImpl::TextureInfo& SceneImpl::texture(const std::string& name, const std::string& modelDir) {
    std::string key = stem(name);
    auto cached = textures_.find(key);
    if (cached != textures_.end()) return cached->second;
    TextureInfo info;
    auto candidates = texturePaths_.find(key);
    if (candidates != texturePaths_.end()) {
        std::string chosen = candidates->second.front();
        for (const auto& c : candidates->second)
            if (c.rfind(modelDir + "textures/", 0) == 0) chosen = c;
        Image image = decodeTexture(archive_.read(*archive_.find(chosen)));
        if (image.faces == 6) {
            info.cube = true;
            info.id = uploadCubeTexture(image);
        } else {
            glGenTextures(1, &info.id);
            glBindTexture(GL_TEXTURE_2D, info.id);
            // Decoded rows are top-down and the game's v=0 is the top row (char_squirrel body
            // and token faces render correctly only this way), so rows upload unflipped.
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, image.width, image.height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                         image.rgba.data());
            glGenerateMipmap(GL_TEXTURE_2D);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }
    } else {
        ++missing_;
    }
    return textures_.emplace(key, info).first->second;
}

SceneImpl::DrawPart SceneImpl::upload(const ModelPart& part, LoadedModel& model, const Mat4& nodeMatrix) {
    std::vector<GpuVertex> vertices;
    vertices.reserve(part.vertices.size());
    for (const auto& v : part.vertices) {
        vertices.push_back(toGpu(v, part.flags));
        for (int i = 0; i < 3; ++i) {
            const float p = v.position[0] * nodeMatrix[i] + v.position[1] * nodeMatrix[4 + i] + v.position[2] * nodeMatrix[8 + i] + nodeMatrix[12 + i];
            model.lo[i] = std::min(model.lo[i], p);
            model.hi[i] = std::max(model.hi[i], p);
        }
    }
    DrawPart d;
    d.count = GLsizei(part.indices.size());
    d.hasNormal = part.flags & AttrNormal;
    d.secondUv = part.flags & AttrUv1;
    if (part.material.size() >= 0x60) {
        d.material = materialState(part);
        d.hasMaterial = d.material.supported;
        d.whiteMaterial = materialState(whiteMaterialPart(part));
        d.source.material = part.material;
        d.source.textureSlots = part.textureSlots;
        std::copy_n(part.material.begin() + 0x10, 4, d.colour);
        std::copy_n(part.material.begin() + 0x14, 4, d.specular);
        d.doubled = (part.material[0xb] >> 5) & 1;
        d.sourceIndex = part.material[8];
        d.destinationIndex = part.material[9];
    }
    glGenVertexArrays(1, &d.vao);
    glBindVertexArray(d.vao);
    glGenBuffers(1, &d.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, d.vbo);
    glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(vertices.size() * sizeof(GpuVertex)), vertices.data(), (part.flags & AttrSkin) ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
    glGenBuffers(1, &d.ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, d.ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, GLsizeiptr(part.indices.size() * 2), part.indices.data(), GL_STATIC_DRAW);
    auto attrib = [](GLuint index, GLint size, GLenum type, GLboolean norm, std::size_t offset) {
        glEnableVertexAttribArray(index);
        glVertexAttribPointer(index, size, type, norm, sizeof(GpuVertex), reinterpret_cast<const void*>(offset));
    };
    attrib(0, 3, GL_FLOAT, GL_FALSE, offsetof(GpuVertex, position));
    attrib(1, 2, GL_FLOAT, GL_FALSE, offsetof(GpuVertex, uv));
    attrib(2, 3, GL_FLOAT, GL_FALSE, offsetof(GpuVertex, normal));
    attrib(3, 4, GL_UNSIGNED_BYTE, GL_TRUE, offsetof(GpuVertex, colour));
    attrib(4, 2, GL_FLOAT, GL_FALSE, offsetof(GpuVertex, uv1));
    return d;
}

Scene::Scene(FuseArchive& archive, std::vector<std::string> exclude)
    : impl_(std::make_unique<SceneImpl>(archive, std::move(exclude))) {}

Scene::~Scene() = default;

void Scene::addGeometry(const std::string& key, const ModelPart& part) { impl_->addGeometry(key, part); }

bool Scene::addModel(const std::string& path, const Mat4& transform, const std::map<std::string, std::string>& textureOverrides,
                     const std::string& key, int onlyMesh) {
    return impl_->addModel(path, transform, textureOverrides, key, onlyMesh);
}

bool Scene::setInstanceMesh(std::size_t instance, const std::string& path, int mesh) {
    return impl_->setInstanceMesh(instance, path, mesh);
}
std::vector<std::ptrdiff_t> Scene::addModelNodes(const std::string& path, const Mat4& placement) {
    return impl_->addModelNodes(path, placement);
}

void Scene::setNodeMatrix(std::size_t instance, int node, const Matrix& matrix) { impl_->setNodeMatrix(instance, node, matrix); }

void Scene::pose(const std::string& key, const std::vector<Matrix>& skin) { impl_->pose(key, skin); }

std::size_t Scene::skinnedParts(const std::string& path) const { return impl_->skinnedParts(path); }

void Scene::bounds(float centre[3], float& radius) const { impl_->bounds(centre, radius); }

void Scene::draw(const Mat4& viewProjection) const { impl_->draw(viewProjection); }

void Scene::drawSky(const Mat4& viewProjection) const { impl_->drawSky(viewProjection); }

void Scene::setLighting(const runtime::LightSet& lights, const float eye[3]) { impl_->setLighting(lights, eye); }

void Scene::setInstanceSky(std::size_t instance) { impl_->setInstanceSky(instance); }

std::size_t Scene::triangles() const { return impl_->triangles(); }

void Scene::setTransform(std::size_t instance, const Mat4& transform) { impl_->setTransform(instance, transform); }

void Scene::setInstanceVisible(std::size_t instance, bool visible) { impl_->setInstanceVisible(instance, visible); }

bool Scene::instanceVisible(std::size_t instance) const { return impl_->instanceVisible(instance); }

Mat4 Scene::instanceTransform(std::size_t instance) const { return impl_->instanceTransform(instance); }

void Scene::setInstanceFade(std::size_t instance, float fade) { impl_->setInstanceFade(instance, fade); }

void Scene::setInstanceNodeShown(std::size_t instance, int node, bool shown, std::uint8_t alpha) { impl_->setInstanceNodeShown(instance, node, shown, alpha); }

void Scene::setInstanceTint(std::size_t instance, const std::array<float, 3>& tint) { impl_->setInstanceTint(instance, tint); }

float Scene::instanceFade(std::size_t instance) const { return impl_->instanceFade(instance); }
const std::string& Scene::instanceModel(std::size_t instance) const { return impl_->instanceModel(instance); }
void Scene::setDepthRange(float nearZ, float farZ) { impl_->setDepthRange(nearZ, farZ); }

void Scene::setInstanceWhiteMaterials(std::size_t instance, bool white) { impl_->setInstanceWhiteMaterials(instance, white); }

void Scene::setMaterialOverride(const std::optional<MaterialOverride>& override) { impl_->setMaterialOverride(override); }

std::size_t Scene::missingTextures() const { return impl_->missingTextures(); }

std::size_t Scene::instances() const { return impl_->instances(); }

void Scene::setInstanceShadow(std::size_t instance, bool caster, bool receiver) { impl_->setInstanceShadow(instance, caster, receiver); }

std::vector<Scene::ShadowPart> Scene::shadowParts(bool casters) const { return impl_->shadowParts(casters); }

}
