#include "block_reader.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <stdexcept>

namespace chase::assets {
namespace {
using Reader = detail::BlockReader;

// Track hashes of the cutscene camera handler 0x1c1fbc (consecutive per axis).
constexpr std::uint32_t positionX = 0x191a8df3, rotationX = 0x66c4d914;

std::string replaceSuffix(const std::string& path, const std::string& from, const std::string& to) {
    if (path.size() >= from.size() && path.compare(path.size() - from.size(), from.size(), from) == 0)
        return path.substr(0, path.size() - from.size()) + to;
    return path;
}
}

std::vector<std::string> parseManifest(const Bytes& data) {
    Reader r(data);
    r.cursor.push(Pool::M14);
    Bytes text = r.next();
    r.cursor.pop();
    if (!r.cursor.exhausted()) throw std::runtime_error("unconsumed manifest blocks");
    std::vector<std::string> lines;
    std::string line;
    for (std::uint8_t c : text) {
        if (c == '\n' || c == 0) {
            if (!line.empty()) lines.push_back(line);
            line.clear();
            if (c == 0) break;
        } else if (c >= 0x20 && c < 0x7f) {
            line += char(c);
        }
    }
    if (!line.empty()) lines.push_back(line);
    return lines;
}

std::vector<CutsceneActor> cutsceneActors(const std::vector<std::string>& manifest, const std::string& folder) {
    std::vector<CutsceneActor> actors;
    auto lower = [](std::string s) {
        for (auto& c : s) c = char(std::tolower(static_cast<unsigned char>(c)));
        return s;
    };
    for (const auto& entry : manifest) {
        std::string path = folder + "/" + lower(entry);
        std::string group = lower(entry.substr(0, entry.find('/')));
        auto find = [&]() -> CutsceneActor& {
            for (auto& actor : actors)
                if (actor.name == group) return actor;
            actors.push_back({group, "", "", {}});
            return actors.back();
        };
        if (path.size() > 6 && path.compare(path.size() - 6, 6, ".fnmdl") == 0) find().model = replaceSuffix(path, ".fnmdl", ".bfnmdl");
        else if (path.size() > 6 && path.compare(path.size() - 6, 6, ".fnskl") == 0) find().skeleton = replaceSuffix(path, ".fnskl", ".bfnskl");
        else if (path.size() > 6 && path.compare(path.size() - 6, 6, ".fnanm") == 0) find().animations.push_back(replaceSuffix(path, ".fnanm", ".bfnanm"));
    }
    return actors;
}

CutsceneCamera cutsceneCamera(const Animation& animation, float time) {
    CutsceneCamera camera;
    for (const auto& tracks : animation.subAnimations)
        for (const auto& track : tracks) {
            float value = evaluateTrack(track, time);
            std::uint32_t h = track.hash;
            if (h >= positionX && h < positionX + 3) camera.position[h - positionX] = value;
            else if (h >= rotationX && h < rotationX + 3) camera.rotation[h - rotationX] = value;
            else if (h == 0xc251f0c2) camera.fieldOfView = value;
            else if (h == 0x45218801) camera.field7 = value;
            else if (h == 0x00b68d33) camera.field8 = value;
            else if (h == 0x89afeba4) camera.field9 = value;
            else if (h == 0x041a48a8) camera.field10 = value;
            else if (h == 0x7d690130) camera.field11 = value;
        }
    return camera;
}

ActorPose actorPose(const Animation& animation, float time) {
    constexpr std::uint32_t position = 0xdcd633e0, yaw = 0x4f44e168, scale = 0x7e190104;
    ActorPose pose;
    for (const auto& tracks : animation.subAnimations)
        for (const auto& track : tracks) {
            float value = evaluateTrack(track, time);
            std::uint32_t h = track.hash;
            if (h >= position && h < position + 3) pose.position[h - position] = h == position ? -value : value;
            else if (h == yaw) pose.yaw = -value;
            else if (h >= scale && h < scale + 3) pose.scale[h - scale] = value;
        }
    return pose;
}

Matrix actorMatrix(const ActorPose& pose) {
    float c = std::cos(pose.yaw), s = std::sin(pose.yaw);
    return {c * pose.scale[0],  0, -s * pose.scale[0], 0,
            0, pose.scale[1], 0, 0,
            s * pose.scale[2],  0, c * pose.scale[2],  0,
            pose.position[0], pose.position[1], pose.position[2], 1};
}

Matrix cameraMatrix(const CutsceneCamera& camera) {
    float sx = std::sin(camera.rotation[0]), cx = std::cos(camera.rotation[0]);
    float sy = std::sin(camera.rotation[1]), cy = std::cos(camera.rotation[1]);
    float sz = std::sin(camera.rotation[2]), cz = std::cos(camera.rotation[2]);
    return {-(cy * cz),                cy * sz,                  -sy,      0,
            cx * sz - cz * sx * sy,    cx * cz + sx * sy * sz,   cy * sx,  0,
            sx * sz + cx * cz * sy,    cz * sx - cx * sy * sz,   -cx * cy, 0,
            -camera.position[0],       camera.position[1],       camera.position[2], 1};
}

CutsceneContent loadCutscene(FuseArchive& archive, const std::string& id) {
    const auto read = [&](const std::string& path) {
        const FuseEntry* entry = archive.find(path);
        if (!entry) throw std::runtime_error("not in archive: " + path);
        return archive.read(*entry);
    };
    std::string folder = "csdata/models/" + id;
    std::transform(folder.begin(), folder.end(), folder.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    CutsceneContent content;
    for (const CutsceneActor& cast : cutsceneActors(parseManifest(read(folder + "/manifest.bstxt")), folder)) {
        if (cast.animations.empty()) continue;
        if (cast.model.empty()) {
            content.camera = parseAnimation(read(cast.animations.front()));
            continue;
        }
        CutsceneCast actor;
        actor.model = cast.model;
        for (const std::string& path : cast.animations)
            if (archive.find(path)) actor.clips.push_back({cutsceneClipStart(cast.name, path), parseAnimation(read(path))});
        const Model model = parseModel(read(cast.model));
        actor.nodeNames = model.nodeNames;
        actor.nodeMesh = model.nodeMesh;
        if (!cast.skeleton.empty() && archive.find(cast.skeleton)) actor.skeleton = parseSkeleton(read(cast.skeleton));
        content.actors.push_back(std::move(actor));
    }
    if (content.camera.subAnimations.empty()) throw std::runtime_error("cutscene has no camera animation: " + id);
    const std::string soundPath = "music/csaudio/" + folder.substr(folder.rfind('/') + 1) + ".bwav";
    if (archive.find(soundPath)) content.soundPath = soundPath;
    return content;
}

double cutsceneNormalised(const Animation& animation, double frame) {
    const double last = animation.frames > 1 ? animation.frames - 1 : 1;
    return std::min(frame, last) / last;
}

unsigned cutsceneClipStart(const std::string& actor, const std::string& animationPath) {
    const std::string file = animationPath.substr(animationPath.rfind('/') + 1), prefix = actor + "_";
    const std::size_t end = prefix.size() + 4;
    const bool matches = file.size() > end && file.compare(0, prefix.size(), prefix) == 0 && (file[end] == '.' || file[end] == '_') &&
                         std::all_of(file.begin() + long(prefix.size()), file.begin() + long(end), [](unsigned char c) { return std::isdigit(c) != 0; });
    if (!matches) throw std::runtime_error("cutscene clip name without start frame: " + animationPath);
    return unsigned(std::stoul(file.substr(prefix.size(), 4)));
}

std::optional<CutsceneClipFrame> cutsceneClipAt(const CutsceneCast& actor, double cameraFrame) {
    const CutsceneClip* driving = nullptr;
    for (const CutsceneClip& clip : actor.clips)
        if (clip.startFrame <= cameraFrame && (!driving || clip.startFrame >= driving->startFrame)) driving = &clip;
    if (!driving) return std::nullopt;
    const double last = driving->animation.frames ? driving->animation.frames - 1 : 0;
    return CutsceneClipFrame{&driving->animation, std::min(cameraFrame - driving->startFrame, last)};
}
std::optional<CutsceneClipFrame> cutsceneRestingClip(const CutsceneCast& actor) {
    const CutsceneClip* first = nullptr;
    for (const CutsceneClip& clip : actor.clips)
        if (!first || clip.startFrame < first->startFrame) first = &clip;
    if (!first) return std::nullopt;
    return CutsceneClipFrame{&first->animation, 0};
}

Matrix cutsceneObjectMatrix(const Animation& clip, std::size_t object, double frame) {
    const unsigned frames = clip.frames ? clip.frames : 1;
    const double held = std::min(std::max(frame, 0.0), double(frames - 1));
    const unsigned first = unsigned(held), second = std::min(first + 1, frames - 1);
    const float fraction = float(held - first);
    const Bytes& curve = clip.objectBlocks.at(object)[0];
    const JointChannels a = decodeChannels(curve, first, frames);
    if (fraction == 0 || second == first || a.rawMatrix) return localMatrix(a);
    const JointChannels b = decodeChannels(curve, second, frames);
    if (b.rawMatrix) return localMatrix(a);
    return poseMatrix(blendPoses({{jointPose(a), 1 - fraction}, {jointPose(b), fraction}}));
}

std::vector<Matrix> cutsceneNodeMatrices(const CutsceneCast& actor, const CutsceneClipFrame& clip, const Matrix& actorMatrix) {
    const auto lower = [](std::string text) {
        for (char& c : text) c = char(std::tolower(static_cast<unsigned char>(c)));
        return text;
    };
    std::vector<Matrix> matrices;
    for (const std::string& node : actor.nodeNames) {
        const auto object = std::find_if(clip.animation->objects.begin(), clip.animation->objects.end(),
                                         [&](const std::string& name) { return lower(name) == lower(node); });
        matrices.push_back(object == clip.animation->objects.end()
                               ? actorMatrix
                               : multiply(cutsceneObjectMatrix(*clip.animation, std::size_t(object - clip.animation->objects.begin()), clip.frame), actorMatrix));
    }
    return matrices;
}
} // namespace chase::assets
