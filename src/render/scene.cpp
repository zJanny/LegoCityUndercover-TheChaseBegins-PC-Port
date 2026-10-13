#include "render/scene.hpp"
#include <SDL.h>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <stdexcept>

namespace chase::render {
using namespace chase::assets;

namespace {
GLuint compile(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(shader, sizeof log, nullptr, log);
        throw std::runtime_error(std::string("shader: ") + log);
    }
    return shader;
}

const char* vertexSource = R"(#version 330 core
layout(location = 0) in vec3 position;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec4 colour;
layout(location = 4) in vec2 uv1;
uniform mat4 mvp;
out vec2 vUv; out vec2 vUv1; out vec3 vNormal; out vec4 vColour;
void main() { vUv = uv; vUv1 = uv1; vNormal = normal; vColour = colour; gl_Position = mvp * vec4(position, 1.0); }
)";
const char* fragmentSource = R"(#version 330 core
in vec2 vUv; in vec2 vUv1; in vec3 vNormal; in vec4 vColour;
uniform sampler2D unit0; uniform sampler2D unit1; uniform sampler2D unit2;
uniform int stageCount; uniform ivec4 stageWords[6]; uniform vec4 stageConstant[6];
uniform int bufferBits; uniform int unitMode[3]; uniform vec4 unitFlat[3];
uniform bool hasNormal; uniform bool hasSecondUv; uniform bool dynamicLight; uniform float alphaReference; uniform float brightness;
uniform vec3 lightingStandIn;
out vec4 fragColour;

vec4 unitColour(int unit) {
    if (unitMode[unit] == 2) return unitFlat[unit];
    if (unitMode[unit] == 0) return vec4(1.0);
    vec2 uv = (unit == 0 || !hasSecondUv) ? vUv : vUv1;
    if (unit == 0) return texture(unit0, uv);
    if (unit == 1) return texture(unit1, uv);
    return texture(unit2, uv);
}
vec3 rgbOperand(vec4 c, int op) {
    switch (op) {
    case 0: return c.rgb;
    case 1: return 1.0 - c.rgb;
    case 2: return vec3(c.a);
    case 3: return vec3(1.0 - c.a);
    case 4: return vec3(c.r);
    case 5: return vec3(1.0 - c.r);
    case 8: return vec3(c.g);
    case 9: return vec3(1.0 - c.g);
    case 12: return vec3(c.b);
    default: return vec3(1.0 - c.b);
    }
}
float alphaOperand(vec4 c, int op) {
    switch (op) {
    case 0: return c.a;
    case 1: return 1.0 - c.a;
    case 2: return c.r;
    case 3: return 1.0 - c.r;
    case 4: return c.g;
    case 5: return 1.0 - c.g;
    case 6: return c.b;
    default: return 1.0 - c.b;
    }
}
vec4 source(int s, int stage, vec4 previous, vec4 buffer, vec4 fragPrimary) {
    if (s == 0) return vColour;
    if (s == 1) return fragPrimary;
    if (s == 2) return vec4(0.0);
    if (s == 3) return unitColour(0);
    if (s == 4) return unitColour(1);
    if (s == 5) return unitColour(2);
    if (s == 0xd) return buffer;
    if (s == 0xe) return stageConstant[stage];
    if (s == 0xf) return previous;
    return vec4(0.0);
}
vec3 combine3(int mode, vec3 a0, vec3 a1, vec3 a2) {
    switch (mode) {
    case 0: return a0;
    case 1: return a0 * a1;
    case 2: return a0 + a1;
    case 3: return a0 + a1 - 0.5;
    case 4: return a0 * a2 + a1 * (1.0 - a2);
    case 5: return a0 - a1;
    case 6: case 7: return vec3(4.0 * dot(a0 - 0.5, a1 - 0.5));
    case 8: return a0 * a1 + a2;
    default: return (a0 + a1) * a2;
    }
}
float combine1(int mode, float a0, float a1, float a2) {
    switch (mode) {
    case 0: return a0;
    case 1: return a0 * a1;
    case 2: return a0 + a1;
    case 3: return a0 + a1 - 0.5;
    case 4: return a0 * a2 + a1 * (1.0 - a2);
    case 5: return a0 - a1;
    case 8: return a0 * a1 + a2;
    default: return (a0 + a1) * a2;
    }
}
void main() {
    vec4 c;
    if (stageCount == 0) {
        c = vColour;
        c *= unitColour(0);
        if (c.a < 0.25) discard;
        float shade = 1.0;
        if (hasNormal && dot(vNormal, vNormal) > 0.0)
            shade = 0.45 + 0.55 * max(dot(normalize(vNormal), normalize(vec3(0.4, 0.8, 0.45))), 0.0);
        fragColour = vec4(min(c.rgb * shade * brightness, vec3(1.0)), 1.0);
        return;
    }
    vec4 fragPrimary = dynamicLight ? vec4(lightingStandIn, 1.0) : vec4(0.0);
    vec4 previous = vec4(0.0), buffer = vec4(0.0);
    for (int i = 0; i < stageCount; ++i) {
        if (((bufferBits >> i) & 1) != 0) buffer = previous;
        int src = stageWords[i].x, op = stageWords[i].y, comb = stageWords[i].z, scale = stageWords[i].w;
        vec4 s0 = source(src & 15, i, previous, buffer, fragPrimary);
        vec4 s1 = source((src >> 4) & 15, i, previous, buffer, fragPrimary);
        vec4 s2 = source((src >> 8) & 15, i, previous, buffer, fragPrimary);
        vec3 rgb = clamp(combine3(comb & 15, rgbOperand(s0, op & 15), rgbOperand(s1, (op >> 4) & 15), rgbOperand(s2, (op >> 8) & 15))
                         * float(1 << (scale & 3)), 0.0, 1.0);
        vec4 t0 = source((src >> 16) & 15, i, previous, buffer, fragPrimary);
        vec4 t1 = source((src >> 20) & 15, i, previous, buffer, fragPrimary);
        vec4 t2 = source((src >> 24) & 15, i, previous, buffer, fragPrimary);
        float a = clamp(combine1((comb >> 16) & 15, alphaOperand(t0, (op >> 12) & 7), alphaOperand(t1, (op >> 16) & 7),
                                 alphaOperand(t2, (op >> 20) & 7)) * float(1 << ((scale >> 16) & 3)), 0.0, 1.0);
        previous = vec4(rgb, a);
    }
    if (alphaReference >= 0.0 && !(previous.a > alphaReference)) discard;
    fragColour = vec4(min(previous.rgb * brightness, vec3(1.0)), previous.a);
}
)";

constexpr float kLightingStandIn[3] = {138.0f / 255.0f, 161.0f / 255.0f, 182.0f / 255.0f};

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

GLuint createProgram() {
    GLuint program = glCreateProgram();
    glAttachShader(program, compile(GL_VERTEX_SHADER, vertexSource));
    glAttachShader(program, compile(GL_FRAGMENT_SHADER, fragmentSource));
    glLinkProgram(program);
    GLint linked = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked) throw std::runtime_error("shader link failed");
    return program;
}

void beginFrame(GLuint program, float brightness) {
    glUseProgram(program);
    glUniform1i(glGetUniformLocation(program, "unit0"), 0);
    glUniform1i(glGetUniformLocation(program, "unit1"), 1);
    glUniform1i(glGetUniformLocation(program, "unit2"), 2);
    glUniform1f(glGetUniformLocation(program, "brightness"), brightness);
    glUniform3fv(glGetUniformLocation(program, "lightingStandIn"), 1, kLightingStandIn);
    glUniform1i(glGetUniformLocation(program, "stageCount"), 0);
    glUniform1f(glGetUniformLocation(program, "alphaReference"), -1.0f);
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
}

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

Scene::Scene(FuseArchive& archive, std::vector<std::string> exclude) : archive_(archive), exclude_(std::move(exclude)) {
    for (const auto& name : archive_.names()) {
        auto dot = name.rfind('.');
        std::string ext = dot == std::string::npos ? "" : name.substr(dot);
        if (ext == ".btga" || ext == ".bpng" || ext == ".bdds") texturePaths_[stem(name)].push_back(name);
    }
}

void Scene::addGeometry(const std::string& key, const ModelPart& part) {
    LoadedModel out;
    out.parts.push_back(upload(part, out));
    instances_.push_back({&models_.emplace(key, std::move(out)).first->second, identity()});
}

bool Scene::addModel(const std::string& path, const Mat4& transform,
                     const std::map<std::string, std::string>& textureOverrides, const std::string& key) {
    const std::string& cacheKey = key.empty() ? path : key;
    auto loaded = models_.find(cacheKey);
    if (loaded == models_.end()) {
        const FuseEntry* entry = archive_.find(path);
        if (!entry) return false;
        Model model = parseModel(archive_.read(*entry));
        LoadedModel out;
        const std::string dir = directory(lower(path));
        for (const auto& mesh : model.meshes)
            for (const auto& part : mesh.parts) {
                std::string texName = part.textures.empty() ? "" : part.textures.front();
                bool skip = false;
                for (const auto& word : exclude_) skip |= lower(texName).find(word) != std::string::npos;
                if (skip || part.indices.empty()) continue;
                DrawPart d = upload(part, out);
                for (int unit = 0; unit < 3 && d.hasMaterial; ++unit) {
                    const int slot = d.material.unitSlot[unit];
                    if (slot < 0 || std::size_t(slot) >= part.textures.size()) continue;
                    std::string name = part.textures[std::size_t(slot)];
                    const auto replacement = textureOverrides.find(stem(name));
                    if (replacement != textureOverrides.end()) name = replacement->second;
                    const TextureInfo& info = texture(name, dir);
                    if (info.cube) {
                        d.unitIsFlat[unit] = true;
                        std::copy_n(info.average, 4, d.unitFlat[unit]);
                    } else {
                        d.unitTexture[unit] = info.id;
                    }
                }
                if (part.flags & AttrSkin) out.skinned.push_back({out.parts.size(), part.vertices, part.flags});
                out.parts.push_back(d);
            }
        loaded = models_.emplace(cacheKey, std::move(out)).first;
    }
    instances_.push_back({&loaded->second, transform});
    return true;
}

void Scene::pose(const std::string& path, const std::vector<Matrix>& skin) {
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

std::size_t Scene::skinnedParts(const std::string& path) const {
    auto loaded = models_.find(path);
    return loaded == models_.end() ? 0 : loaded->second.skinned.size();
}

void Scene::bounds(float centre[3], float& radius) const {
    float lo[3] = {1e30f, 1e30f, 1e30f}, hi[3] = {-1e30f, -1e30f, -1e30f};
    for (const auto& instance : instances_) {
        if (instance.model->parts.empty()) continue;
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

void Scene::drawPart(GLuint program, const DrawPart& part, bool culling) const {
    const MaterialState& m = part.material;
    glUniform1i(glGetUniformLocation(program, "stageCount"), part.hasMaterial ? GLint(m.stages.size()) : 0);
    glUniform1i(glGetUniformLocation(program, "hasNormal"), part.hasNormal);
    glUniform1i(glGetUniformLocation(program, "hasSecondUv"), part.secondUv);
    glUniform1i(glGetUniformLocation(program, "dynamicLight"), m.lighting && m.lightingKey != 1 && m.lightingKey != 4);
    glUniform1f(glGetUniformLocation(program, "alphaReference"), m.alphaTest ? float(m.alphaReference) / 255.0f : -1.0f);
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
        GLint modes[3];
        GLfloat flat[12];
        for (int unit = 0; unit < 3; ++unit) {
            modes[unit] = part.unitIsFlat[unit] ? 2 : part.unitTexture[unit] != 0 ? 1 : 0;
            std::copy_n(part.unitFlat[unit], 4, flat + unit * 4);
        }
        glUniform1iv(glGetUniformLocation(program, "unitMode"), 3, modes);
        glUniform4fv(glGetUniformLocation(program, "unitFlat"), 3, flat);
        for (int unit = 2; unit >= 0; --unit) {
            glActiveTexture(GLenum(GL_TEXTURE0 + unit));
            glBindTexture(GL_TEXTURE_2D, part.unitTexture[unit]);
        }
        if (m.depthTest) {
            static const GLenum functions[8] = {GL_NEVER, GL_ALWAYS, GL_EQUAL, GL_NOTEQUAL, GL_LESS, GL_LEQUAL, GL_GREATER, GL_GEQUAL};
            glDepthFunc(functions[m.depthFunction & 7]);
        } else {
            glDepthFunc(GL_ALWAYS);
        }
        glDepthMask(m.depthWrite ? GL_TRUE : GL_FALSE);
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
        glBindTexture(GL_TEXTURE_2D, part.unitTexture[0]);
        GLint modes[3] = {part.unitTexture[0] != 0 ? 1 : 0, 0, 0};
        glUniform1iv(glGetUniformLocation(program, "unitMode"), 3, modes);
    }
    glBindVertexArray(part.vao);
    glDrawElements(GL_TRIANGLES, part.count, GL_UNSIGNED_SHORT, nullptr);
}

void Scene::draw(GLuint program, const Mat4& viewProjection) const {
    static const GLenum factors[10] = {GL_ZERO, GL_ONE, GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR, GL_DST_COLOR,
                                       GL_ONE_MINUS_DST_COLOR, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_DST_ALPHA,
                                       GL_ONE_MINUS_DST_ALPHA};
    static const GLenum equations[5] = {GL_FUNC_ADD, GL_FUNC_SUBTRACT, GL_FUNC_REVERSE_SUBTRACT, GL_MIN, GL_MAX};
    GLint mvp = glGetUniformLocation(program, "mvp");
    const bool culling = glIsEnabled(GL_CULL_FACE);
    for (int pass = 0; pass < 2; ++pass) {
        for (const auto& instance : instances_) {
            Mat4 m = multiply(viewProjection, instance.transform);
            glUniformMatrix4fv(mvp, 1, GL_FALSE, m.data());
            for (const auto& part : instance.model->parts) {
                const bool blended = part.hasMaterial && (part.material.blendSource != 1 || part.material.blendDestination != 0);
                if (blended != (pass == 1)) continue;
                if (blended) {
                    glEnable(GL_BLEND);
                    glBlendEquation(equations[std::min(part.material.blendEquation, 4u)]);
                    glBlendFunc(factors[std::min(part.material.blendSource, 9u)], factors[std::min(part.material.blendDestination, 9u)]);
                } else {
                    glDisable(GL_BLEND);
                }
                drawPart(program, part, culling);
            }
        }
    }
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
    glUniform1i(glGetUniformLocation(program, "stageCount"), 0);
    glUniform1f(glGetUniformLocation(program, "alphaReference"), -1.0f);
    if (culling) {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
    }
}

std::size_t Scene::triangles() const {
    std::size_t n = 0;
    for (const auto& instance : instances_)
        for (const auto& p : instance.model->parts) n += std::size_t(p.count) / 3;
    return n;
}

const Scene::TextureInfo& Scene::texture(const std::string& name, const std::string& modelDir) {
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
            double sum[4] = {};
            for (std::size_t i = 0; i + 3 < image.rgba.size(); i += 4)
                for (std::size_t c = 0; c < 4; ++c) sum[c] += image.rgba[i + c];
            const double pixels = double(image.rgba.size() / 4);
            for (std::size_t c = 0; c < 4; ++c) info.average[c] = pixels > 0 ? float(sum[c] / pixels / 255.0) : 1.0f;
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

Scene::DrawPart Scene::upload(const ModelPart& part, LoadedModel& model) {
    std::vector<GpuVertex> vertices;
    vertices.reserve(part.vertices.size());
    for (const auto& v : part.vertices) {
        vertices.push_back(toGpu(v, part.flags));
        for (int i = 0; i < 3; ++i) {
            model.lo[i] = std::min(model.lo[i], v.position[i]);
            model.hi[i] = std::max(model.hi[i], v.position[i]);
        }
    }
    DrawPart d;
    d.count = GLsizei(part.indices.size());
    d.hasNormal = part.flags & AttrNormal;
    d.secondUv = part.flags & AttrUv1;
    if (part.material.size() >= 0x60) {
        d.material = materialState(part);
        d.hasMaterial = d.material.supported;
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

}
