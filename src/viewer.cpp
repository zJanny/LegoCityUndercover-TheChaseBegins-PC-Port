// chase-viewer: development model/level viewer (plan step S4b). Not gameplay.
// Loads .bfnmdl models straight from the FUSE archive and draws them with
// their textures. Controls come from config/controls.cfg (3DS button names):
// D-pad moves, L/R lower/raise, X/Y turn, left-drag looks, Escape quits.
#include "assets/assets.hpp"
#include "input.hpp"
#include <SDL.h>
#include <epoxy/gl.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>
#include <sstream>
#include <string>
#include <vector>

namespace {
using namespace chase::assets;
// Column-major; the same memory layout as the game's row-major row-vector matrices.
using Mat4 = std::array<float, 16>;

Mat4 multiply(const Mat4& a, const Mat4& b) {
    Mat4 r{};
    for (int c = 0; c < 4; ++c)
        for (int row = 0; row < 4; ++row)
            for (int k = 0; k < 4; ++k) r[c * 4 + row] += a[k * 4 + row] * b[c * 4 + k];
    return r;
}
Mat4 perspective(float fovY, float aspect, float nearZ, float farZ) {
    float f = 1.0f / std::tan(fovY / 2);
    Mat4 m{};
    m[0] = f / aspect;
    m[5] = f;
    m[10] = (farZ + nearZ) / (nearZ - farZ);
    m[11] = -1;
    m[14] = 2 * farZ * nearZ / (nearZ - farZ);
    return m;
}
// View matrix for a camera at `eye` looking along yaw/pitch (radians), +Y up.
Mat4 view(const float eye[3], float yaw, float pitch) {
    float fx = std::cos(pitch) * std::sin(yaw), fy = std::sin(pitch), fz = -std::cos(pitch) * std::cos(yaw);
    float sx = -fz, sz = fx;  // right = forward x up
    float sl = std::sqrt(sx * sx + sz * sz);
    sx /= sl; sz /= sl;
    float ux = -sz * fy, uy = sz * fx - sx * fz, uz = sx * fy;  // up = right x forward
    Mat4 m{sx, ux, -fx, 0, 0, uy, -fy, 0, sz, uz, -fz, 0, 0, 0, 0, 1};
    m[12] = -(sx * eye[0] + sz * eye[2]);
    m[13] = -(ux * eye[0] + uy * eye[1] + uz * eye[2]);
    m[14] = fx * eye[0] + fy * eye[1] + fz * eye[2];
    return m;
}

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
// Approximation, not the game's PICA combiner setup (not recovered): texture x
// vertex colour, or texture x lightmap (second texture on uv1) for lightmapped parts,
// whose vertex colours are black in the shipped data.
const char* fragmentSource = R"(#version 330 core
in vec2 vUv; in vec2 vUv1; in vec3 vNormal; in vec4 vColour;
uniform sampler2D tex; uniform sampler2D lightmap;
uniform bool hasTexture; uniform bool hasLightmap; uniform bool hasNormal; uniform float brightness;
out vec4 fragColour;
void main() {
    vec4 c = hasLightmap ? vec4(texture(lightmap, vUv1).rgb, 1.0) : vColour;
    if (hasTexture) c *= texture(tex, vec2(vUv.x, vUv.y));
    if (c.a < 0.25) discard;
    float shade = 1.0;
    if (hasNormal && dot(vNormal, vNormal) > 0.0)
        shade = 0.45 + 0.55 * max(dot(normalize(vNormal), normalize(vec3(0.4, 0.8, 0.45))), 0.0);
    fragColour = vec4(min(c.rgb * shade * brightness, vec3(1.0)), 1.0);
}
)";

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
struct DrawPart {
    GLuint vao = 0, vbo = 0, ibo = 0, texture = 0, lightmap = 0;
    GLsizei count = 0;
    bool hasNormal = false;
};

std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return s;
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

class Scene {
public:
    Scene(FuseArchive& archive, std::vector<std::string> exclude) : archive_(archive), exclude_(std::move(exclude)) {
        for (const auto& name : archive_.names()) {
            auto dot = name.rfind('.');
            std::string ext = dot == std::string::npos ? "" : name.substr(dot);
            if (ext == ".btga" || ext == ".bpng" || ext == ".bdds") texturePaths_[stem(name)].push_back(name);
        }
    }
    // Adds one instance of a model; the model's GPU data is loaded once and shared.
    bool addModel(const std::string& path, const Mat4& transform = identity()) {
        auto loaded = models_.find(path);
        if (loaded == models_.end()) {
            const FuseEntry* entry = archive_.find(path);
            if (!entry) return false;
            Model model = parseModel(archive_.read(*entry));
            LoadedModel out;
            for (const auto& mesh : model.meshes)
                for (const auto& part : mesh.parts) {
                    std::string texName = part.textures.empty() ? "" : part.textures.front();
                    bool skip = false;
                    for (const auto& word : exclude_) skip |= lower(texName).find(word) != std::string::npos;
                    if (skip || part.indices.empty()) continue;
                    std::string dir = directory(lower(path));
                    DrawPart d = upload(part, texName.empty() ? 0 : texture(texName, dir), out);
                    // Second texture sampled with uv1: a LIGHTMAP_* texture in the shipped data.
                    if ((part.flags & AttrUv1) && part.textures.size() >= 2) d.lightmap = texture(part.textures[1], dir);
                    if (part.flags & AttrSkin) out.skinned.push_back({out.parts.size(), part.vertices, part.flags});
                    out.parts.push_back(d);
                }
            loaded = models_.emplace(path, std::move(out)).first;
        }
        instances_.push_back({&loaded->second, transform});
        return true;
    }
    // Re-skins every skinned part of an already added model on the CPU.
    void pose(const std::string& path, const std::vector<Matrix>& skin) {
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
    std::size_t skinnedParts(const std::string& path) const {
        auto loaded = models_.find(path);
        return loaded == models_.end() ? 0 : loaded->second.skinned.size();
    }
    void bounds(float centre[3], float& radius) const {
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
    void draw(GLuint program, const Mat4& viewProjection) const {
        GLint hasTexture = glGetUniformLocation(program, "hasTexture");
        GLint hasNormal = glGetUniformLocation(program, "hasNormal");
        GLint hasLightmap = glGetUniformLocation(program, "hasLightmap");
        GLint mvp = glGetUniformLocation(program, "mvp");
        for (const auto& instance : instances_) {
            Mat4 m = multiply(viewProjection, instance.transform);
            glUniformMatrix4fv(mvp, 1, GL_FALSE, m.data());
            for (const auto& part : instance.model->parts) {
                glUniform1i(hasTexture, part.texture != 0);
                glUniform1i(hasNormal, part.hasNormal);
                glUniform1i(hasLightmap, part.lightmap != 0);
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, part.lightmap);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, part.texture);
                glBindVertexArray(part.vao);
                glDrawElements(GL_TRIANGLES, part.count, GL_UNSIGNED_SHORT, nullptr);
            }
        }
    }
    std::size_t triangles() const {
        std::size_t n = 0;
        for (const auto& instance : instances_)
            for (const auto& p : instance.model->parts) n += std::size_t(p.count) / 3;
        return n;
    }
    std::size_t missingTextures() const { return missing_; }
    std::size_t instances() const { return instances_.size(); }
    static Mat4 identity() { return {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1}; }

private:
    struct SkinnedPart {
        std::size_t part;
        std::vector<Vertex> vertices;
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
    GLuint texture(const std::string& name, const std::string& modelDir) {
        std::string key = stem(name);
        auto cached = textures_.find(key);
        if (cached != textures_.end()) return cached->second;
        GLuint id = 0;
        auto candidates = texturePaths_.find(key);
        if (candidates != texturePaths_.end()) {
            std::string chosen = candidates->second.front();
            for (const auto& c : candidates->second)
                if (c.rfind(modelDir + "textures/", 0) == 0) chosen = c;
            Image image = decodeTexture(archive_.read(*archive_.find(chosen)));
            glGenTextures(1, &id);
            glBindTexture(GL_TEXTURE_2D, id);
            // Decoded rows are top-down and the game's v=0 is the top row (char_squirrel body
            // and token faces render correctly only this way), so rows upload unflipped.
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, image.width, image.height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                         image.rgba.data());
            glGenerateMipmap(GL_TEXTURE_2D);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        } else {
            ++missing_;
        }
        textures_[key] = id;
        return id;
    }
    DrawPart upload(const ModelPart& part, GLuint tex, LoadedModel& model) {
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
        d.texture = tex;
        d.count = GLsizei(part.indices.size());
        d.hasNormal = part.flags & AttrNormal;
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
    FuseArchive& archive_;
    std::vector<std::string> exclude_;
    std::map<std::string, std::vector<std::string>> texturePaths_;
    std::map<std::string, GLuint> textures_;
    std::map<std::string, LoadedModel> models_;
    std::vector<Instance> instances_;
    std::size_t missing_ = 0;
};

bool endsWith(const std::string& text, const std::string& suffix) {
    return text.size() >= suffix.size() && text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0;
}

// Viewer convenience following the archive's folder naming, not recovered game logic:
// levels/levels/<name>/x.blvl draws models/<name>/*.bfnmdl without the _l1/_lod1 detail
// levels, or only models/<name>/<room>.bfnmdl when a room is chosen.
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

int resolveKey(const std::string& name) {
    std::string spaced = name;
    std::replace(spaced.begin(), spaced.end(), '_', ' ');
    return SDL_GetKeyFromName(spaced.c_str());
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

struct Options {
    std::string fib = "artifacts/romfs/romfs/lego_city3ds.fib";
    std::string names = "artifacts/romfs/romfs/lego_city3ds.csv";
    std::string config = "config/controls.cfg";
    // cull: faces with this winding are front. CCW culls room ceilings seen from above
    // (observed on lvl_00_policestation room3), so it is the default.
    std::string screenshot, room, cull = "ccw";
    // Posing applies to the first model argument. The game's playback rate is set per
    // animation instance by code not yet recovered, so the rate is an option.
    std::string skeleton, animation;
    double frame = -1;
    double fps = 30;
    float brightness = 1.0f;  // inspection aid only; the game's lighting/combiner setup is not recovered
    std::vector<std::string> exclude, models, levels;
    float yaw = 35, pitch = -25;
    std::vector<float> eye;
};

Options parse(int argc, char** argv) {
    Options o;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        auto value = [&]() -> std::string {
            if (i + 1 >= argc) throw std::runtime_error("missing value for " + arg);
            return argv[++i];
        };
        if (arg == "--fib") o.fib = value();
        else if (arg == "--names") o.names = value();
        else if (arg == "--config") o.config = value();
        else if (arg == "--screenshot") o.screenshot = value();
        else if (arg == "--exclude") o.exclude.push_back(lower(value()));
        else if (arg == "--level") o.levels.push_back(value());
        else if (arg == "--room") o.room = lower(value());
        else if (arg == "--cull") o.cull = value();
        else if (arg == "--brightness") o.brightness = std::stof(value());
        else if (arg == "--skeleton") o.skeleton = value();
        else if (arg == "--anim") o.animation = value();
        else if (arg == "--frame") o.frame = std::stod(value());
        else if (arg == "--fps") o.fps = std::stod(value());
        else if (arg == "--yaw") o.yaw = std::stof(value());
        else if (arg == "--pitch") o.pitch = std::stof(value());
        else if (arg == "--eye") {
            std::string text = value();
            std::replace(text.begin(), text.end(), ',', ' ');
            std::istringstream in(text);
            float v;
            while (in >> v) o.eye.push_back(v);
            if (o.eye.size() != 3) throw std::runtime_error("--eye needs X,Y,Z");
        }
        else if (!arg.empty() && arg[0] == '-') throw std::runtime_error("unknown option " + arg);
        else o.models.push_back(arg);
    }
    if (o.models.empty() && o.levels.empty())
        throw std::runtime_error("usage: chase-viewer [--fib F] [--names CSV] [--config CFG] [--exclude WORD]... "
                                 "[--level levels/.../x.blvl]... [--room NAME] [--cull none|cw|ccw] [--brightness F] [--screenshot OUT.bmp] [--yaw DEG] [--pitch DEG] [--eye X,Y,Z] "
                                 "[--skeleton S.bfnskl --anim A.bfnanm [--frame N] [--fps F]] "
                                 "[models/<dir>/<model>.bfnmdl...]");
    if (o.skeleton.empty() != o.animation.empty()) throw std::runtime_error("--skeleton and --anim go together");
    if (!o.skeleton.empty() && o.models.empty()) throw std::runtime_error("--anim needs a model argument to pose");
    return o;
}

int run(const Options& options) {
    std::ifstream config(options.config);
    if (!config) throw std::runtime_error("cannot open controls: " + options.config);
    chase::InputState input(chase::readBindings(config, resolveKey));
    FuseArchive archive(options.fib);
    archive.loadNames(options.names);

    if (SDL_Init(SDL_INIT_VIDEO) != 0) throw std::runtime_error(SDL_GetError());
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    int width = 960, height = 600;
    SDL_Window* window = SDL_CreateWindow("The Chase Begins - development model viewer (not gameplay)",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window) throw std::runtime_error(SDL_GetError());
    SDL_GLContext context = SDL_GL_CreateContext(window);
    if (!context) throw std::runtime_error(SDL_GetError());
    SDL_GL_SetSwapInterval(1);

    GLuint program = glCreateProgram();
    glAttachShader(program, compile(GL_VERTEX_SHADER, vertexSource));
    glAttachShader(program, compile(GL_FRAGMENT_SHADER, fragmentSource));
    glLinkProgram(program);
    GLint linked = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked) throw std::runtime_error("shader link failed");

    Scene scene(archive, options.exclude);
    std::vector<std::string> models = options.models;
    if (models.empty())
        for (const auto& levelPath : options.levels) {
            auto found = levelModels(archive, levelPath, options.room);
            models.insert(models.end(), found.begin(), found.end());
        }
    for (const auto& model : models)
        if (!scene.addModel(model)) throw std::runtime_error("not in archive: " + model);
    std::size_t placed = 0, unresolved = 0;
    for (const auto& levelPath : options.levels) {
        const FuseEntry* entry = archive.find(levelPath);
        if (!entry) throw std::runtime_error("not in archive: " + levelPath);
        for (const auto& placement : placements(parseLevel(archive.read(*entry)))) {
            if (!options.room.empty() && lower(placement.room) != options.room) continue;
            if (scene.addModel(meshModelPath(placement.mesh), placementMatrix(placement))) ++placed;
            else ++unresolved;
        }
    }
    Skeleton skeleton;
    Animation animation;
    bool animated = !options.skeleton.empty();
    if (animated) {
        const FuseEntry* skl = archive.find(options.skeleton);
        const FuseEntry* anm = archive.find(options.animation);
        if (!skl || !anm) throw std::runtime_error("skeleton or animation not in archive");
        skeleton = parseSkeleton(archive.read(*skl));
        animation = parseAnimation(archive.read(*anm));
        std::cout << "posing " << scene.skinnedParts(options.models.front()) << " skinned part(s) with "
                  << skeleton.joints.size() << " joints, " << animation.frames << " frames\n";
    }
    double posedFrame = -1;
    float centre[3], radius;
    scene.bounds(centre, radius);
    std::cout << "loaded " << scene.instances() << " instance(s) (" << placed << " placed, " << unresolved
              << " unresolved meshnames), " << scene.triangles() << " triangles, " << scene.missingTextures()
              << " missing textures\n";

    float yaw = options.yaw * float(M_PI) / 180, pitch = options.pitch * float(M_PI) / 180;
    float eye[3];
    auto placeCamera = [&]() {
        float d = radius * 2.2f;
        eye[0] = centre[0] - d * std::cos(pitch) * std::sin(yaw);
        eye[1] = centre[1] - d * std::sin(pitch);
        eye[2] = centre[2] + d * std::cos(pitch) * std::cos(yaw);
    };
    placeCamera();
    if (!options.eye.empty()) std::copy_n(options.eye.begin(), 3, eye);
    glEnable(GL_DEPTH_TEST);
    if (options.cull != "none") {
        glEnable(GL_CULL_FACE);
        glFrontFace(options.cull == "cw" ? GL_CW : GL_CCW);
    }
    bool running = true, dragging = false;
    Uint32 last = SDL_GetTicks();
    int frames = 0;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT: running = false; break;
            case SDL_KEYDOWN: case SDL_KEYUP:
                if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) running = false;
                input.event(chase::Source::Key, event.key.keysym.sym, event.type == SDL_KEYDOWN, event.key.repeat != 0);
                break;
            case SDL_MOUSEBUTTONDOWN: case SDL_MOUSEBUTTONUP:
                input.event(chase::Source::Mouse, event.button.button, event.type == SDL_MOUSEBUTTONDOWN);
                if (event.button.button == SDL_BUTTON_LEFT) dragging = event.type == SDL_MOUSEBUTTONDOWN;
                break;
            case SDL_MOUSEMOTION:
                if (dragging) {
                    yaw += event.motion.xrel * 0.005f;
                    pitch = std::clamp(pitch - event.motion.yrel * 0.005f, -1.5f, 1.5f);
                }
                break;
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_FOCUS_LOST) { input.reset(); dragging = false; }
                break;
            default: break;
            }
        }
        Uint32 now = SDL_GetTicks();
        float dt = std::min(0.1f, (now - last) / 1000.0f);
        last = now;
        float speed = radius * 0.5f * dt;
        float turn = 1.5f * dt;
        yaw += turn * float(input.active(chase::Action::Y) - input.active(chase::Action::X));
        float forward = float(input.vertical()), strafe = float(input.horizontal());
        float rise = float(input.active(chase::Action::R) - input.active(chase::Action::L));
        eye[0] += speed * (forward * std::cos(pitch) * std::sin(yaw) + strafe * std::cos(yaw));
        eye[1] += speed * (forward * std::sin(pitch) + rise);
        eye[2] += speed * (-forward * std::cos(pitch) * std::cos(yaw) + strafe * std::sin(yaw));

        if (animated) {
            double frame = options.frame >= 0 ? options.frame : now * options.fps / 1000.0;
            if (frame != posedFrame) {
                scene.pose(options.models.front(), skinMatrices(skeleton, animation, frame));
                posedFrame = frame;
            }
        }
        SDL_GL_GetDrawableSize(window, &width, &height);
        glViewport(0, 0, width, height);
        glClearColor(0.16f, 0.17f, 0.2f, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(program);
        Mat4 mvp = multiply(perspective(1.0f, float(width) / float(std::max(height, 1)), radius * 0.002f, radius * 20),
                            view(eye, yaw, pitch));
        glUniform1i(glGetUniformLocation(program, "tex"), 0);
        glUniform1i(glGetUniformLocation(program, "lightmap"), 1);
        glUniform1f(glGetUniformLocation(program, "brightness"), options.brightness);
        scene.draw(program, mvp);
        if (!options.screenshot.empty() && ++frames >= 2) {
            glFinish();
            saveScreenshot(options.screenshot, width, height);
            std::cout << "saved " << options.screenshot << '\n';
            running = false;
        }
        SDL_GL_SwapWindow(window);
    }
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
}

int main(int argc, char** argv) {
    try {
        return run(parse(argc, argv));
    } catch (const std::exception& error) {
        std::cerr << "chase-viewer: " << error.what() << '\n';
        return 1;
    }
}
