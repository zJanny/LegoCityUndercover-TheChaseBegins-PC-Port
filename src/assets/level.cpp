#include "assets/object_classes.hpp"
#include "block_reader.hpp"
#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstring>
#include <map>
#include <memory>
#include <stdexcept>

namespace chase::assets {
namespace {
using detail::cString;
using detail::readF32;
using Reader = detail::BlockReader;
constexpr std::uint32_t initialPosition = 0x86166f20;     // nameHash("initialposition")
constexpr std::uint32_t initialOrientation = 0xa3b9aa7f;  // nameHash("initialorientation")
constexpr std::uint32_t meshName = 0xacd1ebb8;            // nameHash("meshname")
constexpr const char* minifigMesh = "char_minifig";
constexpr const char* defaultAnimationClass = "CLASS_Generic";
constexpr std::uint32_t firstCustomCharacter = 1, lastCustomCharacter = 8;
constexpr std::uint32_t studObjectType = 0x3d, lastStudKind = 3;

void need(const Bytes& block, std::size_t size) {
    if (block.size() < size) throw std::runtime_error("level block too small");
}

// 0x208ae0: parameter records of 12 bytes; out-of-line data depends on the type.
std::vector<LevelParam> readParams(Reader& r, const Bytes& block, std::size_t count) {
    need(block, count * 12);
    std::vector<LevelParam> params(count);
    for (std::size_t i = 0; i < count; ++i) {
        LevelParam& p = params[i];
        p.hash = readU32(block, i * 12);
        p.flags = readU16(block, i * 12 + 4);
        p.type = readU16(block, i * 12 + 6);
        p.value = readU32(block, i * 12 + 8);
        if (p.type == 1 || p.type == 2 || p.type == 3 || p.type == 6 || (p.type == 4 && p.value))
            p.blocks.push_back(r.next());
        else if (p.type == 8) {
            p.blocks.push_back(r.next());
            p.blocks.push_back(r.next());
        }
    }
    return params;
}
}

std::uint32_t nameHash(const std::string& name) {
    static const auto table = [] {
        std::array<std::uint32_t, 256> t{};
        for (std::uint32_t i = 0; i < 256; ++i) {
            std::uint32_t c = i;
            for (int k = 0; k < 8; ++k) c = c & 1 ? 0xedb88320u ^ (c >> 1) : c >> 1;
            t[i] = c;
        }
        return t;
    }();
    std::uint32_t crc = 0xffffffffu;
    for (unsigned char ch : name) {
        if (ch >= 'A' && ch <= 'Z') ch = static_cast<unsigned char>(ch + 32);
        crc = table[(ch ^ crc) & 0xff] ^ (crc >> 8);
    }
    return ~crc;
}

std::uint32_t objectReferenceHash(const LevelParam& param) {
    if (param.type != 4 || param.blocks.size() != 1 || param.blocks[0].size() < 8) return 0;
    return readU32(param.blocks[0], 4);
}

Level parseLevel(const Bytes& data) {
    Reader r(data);
    r.cursor.push(Pool::M15);
    Bytes head = r.next();
    need(head, 0x10);
    std::size_t n30 = head[1], n10 = readU16(head, 2), n14 = readU16(head, 4), n18 = readU16(head, 6),
                n1c = readU16(head, 8), n20 = readU16(head, 10), n24 = readU16(head, 12), n2c = readU16(head, 14);
    Level level;
    level.defaults = readParams(r, r.next(), n10);
    level.classParams = readParams(r, r.next(), n14);
    Bytes s18 = r.next();
    need(s18, n18 * 0x1c);
    level.scripts.resize(n18);
    for (std::size_t i = 0; i < n18; ++i) {
        LevelScript& script = level.scripts[i];
        script.nameHash = readU32(s18, i * 0x1c);
        const Bytes calls = r.next();
        r.skip();
        const std::uint16_t callCount = readU16(s18, i * 0x1c + 6);
        need(calls, std::size_t(callCount) * 8);
        std::vector<std::string> arguments;
        for (std::uint32_t k = 0; k < readU32(s18, i * 0x1c + 0x14); ++k) arguments.push_back(cString(r.next()));
        r.cursor.push(Pool::M16);
        std::size_t used = 0;
        for (std::uint16_t k = 0; k < callCount; ++k) {
            ScriptCall call;
            call.function = cString(r.next());
            const std::size_t count = readU16(calls, std::size_t(k) * 8 + 2);
            if (used + count > arguments.size()) throw std::runtime_error("script call arguments out of range");
            call.arguments.assign(arguments.begin() + std::ptrdiff_t(used), arguments.begin() + std::ptrdiff_t(used + count));
            used += count;
            script.calls.push_back(std::move(call));
        }
        r.cursor.pop();
    }
    const Bytes s1c = r.next();
    need(s1c, n1c * 0x18);
    level.triggers.resize(n1c);
    for (std::size_t i = 0; i < n1c; ++i) {
        LevelTrigger& trigger = level.triggers[i];
        trigger.nameHash = readU32(s1c, i * 0x18);
        trigger.objectName = cString(r.next());
        trigger.scriptName = cString(r.next());
        r.cursor.push(Pool::M16);
        trigger.event = cString(r.next());
        trigger.firstArgument = cString(r.next());
        trigger.secondArgument = cString(r.next());
        r.cursor.pop();
    }
    Bytes s20 = r.next();
    need(s20, n20 * 0x28);
    level.paths.resize(n20);
    for (std::size_t i = 0; i < n20; ++i) {
        LevelPath& path = level.paths[i];
        path.nameHash = readU32(s20, i * 0x28);
        path.type = s20[i * 0x28 + 8];
        path.closed = s20[i * 0x28 + 9] != 0;
        const std::size_t count = readU16(s20, i * 0x28 + 0xa);
        path.unknownFloat = readF32(s20, i * 0x28 + 0x20);
        if (readU32(s20, i * 0x28 + 4)) path.room = cString(r.next());
        const Bytes points = r.next();
        need(points, count * 12);
        path.points.resize(count);
        for (std::size_t k = 0; k < count; ++k)
            for (std::size_t a = 0; a < 3; ++a) path.points[k][a] = readF32(points, k * 12 + a * 4);
        if (readU32(s20, i * 0x28 + 0x10)) {
            r.skip();
            r.skip();
        }
    }
    Bytes s24 = r.next();
    need(s24, n24 * 0x28);
    for (std::size_t i = 0; i < n24; ++i) {
        LevelBound bound;
        bound.name = cString(r.next());
        for (int a = 0; a < 3; ++a) {
            bound.centre[a] = readF32(s24, i * 0x28 + 0x10 + 4 * std::size_t(a));
            bound.half[a] = readF32(s24, i * 0x28 + 0x1c + 4 * std::size_t(a));
        }
        bound.node = s24[i * 0x28 + 8];
        bound.shape = s24[i * 0x28 + 9];
        bound.enabled = s24[i * 0x28 + 0xa];
        bound.active = s24[i * 0x28 + 0xb];
        bound.extent = readF32(s24, i * 0x28 + 0xc);
        if (readU32(s24, i * 0x28 + 4)) bound.room = cString(r.next());
        level.bounds.push_back(std::move(bound));
    }
    const Bytes s28 = r.next();
    for (std::size_t offset = 0; offset + 0x40 <= s28.size(); offset += 0x40) {
        std::array<float, 16> matrix{};
        for (std::size_t k = 0; k < 16; ++k) matrix[k] = readF32(s28, offset + 4 * k);
        level.boundMatrices.push_back(matrix);
    }
    Bytes s2c = r.next();
    need(s2c, n2c * 0x18);
    level.objects.resize(n2c);
    for (std::size_t i = 0; i < n2c; ++i) {
        LevelObject& obj = level.objects[i];
        obj.instance = cString(r.next());
        obj.room = cString(r.next());
        obj.params = readParams(r, r.next(), s2c[i * 0x18 + 0xd]);
        auto second = readParams(r, r.next(), s2c[i * 0x18 + 0xe]);
        obj.params.insert(obj.params.end(), second.begin(), second.end());
    }
    Bytes s34 = r.next();
    if (!s34.empty()) {
        need(s34, 8);
        const std::uint32_t linkCount = readU32(s34, 0);
        const Bytes records = r.next();
        need(records, std::size_t(linkCount) * 0x10);
        level.roomLinks.resize(linkCount);
        for (std::uint32_t k = 0; k < linkCount; ++k) {
            RoomLinks& links = level.roomLinks[k];
            links.roomHash = readU32(records, std::size_t(k) * 0x10);
            links.worldLevelHash = readU32(records, std::size_t(k) * 0x10 + 4);
            const std::uint32_t entries = readU32(records, std::size_t(k) * 0x10 + 8);
            const Bytes list = r.next();
            need(list, std::size_t(entries) * 12);
            for (std::uint32_t e = 0; e < entries; ++e)
                links.linked.push_back({readU32(list, std::size_t(e) * 12), readU32(list, std::size_t(e) * 12 + 4),
                                        readU32(list, std::size_t(e) * 12 + 8)});
        }
    }
    r.cursor.push(Pool::M16);
    for (auto& obj : level.objects) obj.className = cString(r.next());
    r.cursor.pop();
    if (n30) {
        Bytes s30 = r.next();
        need(s30, n30 * 0x28);
        for (std::size_t i = 0; i < n30; ++i) {
            r.skip();
            std::vector<std::string> names;
            for (std::uint16_t k = 0; k < readU16(s30, i * 0x28); ++k) names.push_back(cString(r.next()));
            Bytes descriptors = r.next();
            need(descriptors, names.size() * 0x18);
            for (std::size_t k = 0; k < names.size(); ++k) {
                NavRoom room;
                room.room = names[k];
                room.heightUnit = readF32(s30, i * 0x28 + 0xc);
                room.cellSize = readF32(s30, i * 0x28 + 0x10);
                for (int a = 0; a < 3; ++a) room.origin[a] = readF32(s30, i * 0x28 + 0x14 + 4 * std::size_t(a));
                room.grids = readU16(descriptors, k * 0x18);
                room.paths = readU16(descriptors, k * 0x18 + 2);
                level.navRooms.push_back(std::move(room));
            }
        }
    }
    r.cursor.pop();
    if (!r.cursor.exhausted()) throw std::runtime_error("unconsumed level blocks");
    return level;
}

namespace {
Placement placementOf(const LevelObject& obj) {
    Placement p;
    p.instance = obj.instance;
    p.room = obj.room;
    for (const auto& param : obj.params) {
        if (param.blocks.empty() || param.blocks[0].empty()) continue;
        const Bytes& b = param.blocks[0];
        if (param.hash == meshName && param.type == 1) p.mesh = cString(b);
        float* target = param.hash == initialPosition ? p.position
                      : param.hash == initialOrientation ? p.orientation : nullptr;
        if (target && param.type == 2 && b.size() >= 12) std::memcpy(target, b.data(), 12);
    }
    return p;
}

std::string stringParam(const std::vector<LevelParam>& params, const std::string& name) {
    std::uint32_t hash = nameHash(name);
    for (const auto& param : params)
        if (param.hash == hash && param.type == 1 && !param.blocks.empty()) return cString(param.blocks[0]);
    return "";
}

const LevelParam* findParam(const std::vector<LevelParam>& params, std::uint32_t hash) {
    for (const auto& param : params)
        if (param.hash == hash) return &param;
    return nullptr;
}

// Lookup 0x217080: only parameters of the class table (+0x10, then +0x14) exist for an object. A slot
// parameter (flags bit 1) holds the instance value when the level object sets it, else the class value.
const LevelParam* effectiveParam(const LevelObject& obj, const Level& cls, std::uint32_t hash) {
    const LevelParam* classParam = findParam(cls.defaults, hash);
    if (!classParam) classParam = findParam(cls.classParams, hash);
    if (!classParam) return nullptr;
    if (classParam->flags & 2)
        if (const LevelParam* instance = findParam(obj.params, hash)) return instance;
    return classParam;
}

std::string effectiveString(const LevelObject& obj, const Level& cls, const std::string& name) {
    const LevelParam* param = effectiveParam(obj, cls, nameHash(name));
    return param && param->type == 1 && !param->blocks.empty() ? cString(param->blocks[0]) : "";
}

std::uint32_t effectiveEnum(const LevelObject& obj, const Level& cls, const std::string& name) {
    const LevelParam* param = effectiveParam(obj, cls, nameHash(name));
    return param && param->type == 7 ? param->value : 0;
}

std::string lowered(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return text;
}

float effectiveFloat(const LevelObject& obj, const Level& cls, const std::string& name) {
    const LevelParam* param = effectiveParam(obj, cls, nameHash(name));
    if (!param || param->type != 0) return 0;
    float value;
    std::memcpy(&value, &param->value, 4);
    return value;
}

bool isStudClass(const LevelObject& obj, const Level& cls) {
    return effectiveEnum(obj, cls, "ObjectType") == studObjectType && effectiveEnum(obj, cls, "pickuptype") <= lastStudKind;
}

bool float3Of(const LevelParam* param, float* out) {
    if (!param || param->type != 2 || param->blocks.empty() || param->blocks[0].size() < 12) return false;
    std::memcpy(out, param->blocks[0].data(), 12);
    return true;
}
}

bool isStudObject(const LevelObject& object, const Level& classFile) { return isStudClass(object, classFile); }

std::vector<Placement> placements(const Level& level) {
    std::vector<Placement> out;
    for (const auto& obj : level.objects) {
        Placement p = placementOf(obj);
        if (!p.mesh.empty()) out.push_back(p);
    }
    return out;
}

std::vector<Placement> objectPlacements(const Level& level, const ClassLookup& classOf) {
    std::vector<Placement> out;
    for (std::size_t index = 0; index < level.objects.size(); ++index) {
        const LevelObject& obj = level.objects[index];
        const Level* cls = classOf(obj.className);
        if (!cls) continue;
        const LevelParam* meshParam = effectiveParam(obj, *cls, meshName);
        if (!meshParam || meshParam->type != 1 || meshParam->blocks.empty()) continue;
        Placement p;
        p.instance = obj.instance;
        p.room = obj.room;
        p.className = obj.className;
        p.objectIndex = index;
        p.mesh = cString(meshParam->blocks[0]);
        if (p.mesh.empty()) continue;
        float3Of(effectiveParam(obj, *cls, initialPosition), p.position);
        float3Of(effectiveParam(obj, *cls, initialOrientation), p.orientation);
        out.push_back(std::move(p));
    }
    return out;
}

ClassLookup archiveClassLookup(FuseArchive& archive) {
    auto classes = std::make_shared<std::map<std::string, Level>>();
    return [&archive, classes](const std::string& name) -> const Level* {
        auto found = classes->find(name);
        if (found == classes->end()) {
            const FuseEntry* entry = archive.find("levels/" + name + ".blvl");
            found = classes->emplace(name, entry ? parseLevel(archive.read(*entry)) : Level{}).first;
        }
        return &found->second;
    };
}

DrawableObjects drawableObjects(FuseArchive& archive, const Level& level) {
    const ClassLookup classOf = archiveClassLookup(archive);
    DrawableObjects out;
    for (auto& placement : objectPlacements(level, classOf)) {
        const std::string mesh = lowered(placement.mesh);
        const LevelObject* object = &level.objects[placement.objectIndex];
        const Level& cls = *classOf(placement.className);
        if (placement.className.rfind("props/map_", 0) == 0) {
            ++out.markers;
        } else if (isStudClass(*object, cls)) {
            StudObject stud;
            stud.kind = effectiveEnum(*object, cls, "pickuptype");
            std::copy_n(placement.position, 3, stud.position);
            stud.group = std::uint32_t(effectiveFloat(*object, cls, "DisabledOnStartUp")) & 0xff;
            stud.groupOrder = std::uint32_t(effectiveFloat(*object, cls, "GroupOrder")) & 0xff;
            stud.instance = placement.instance;
            stud.room = placement.room;
            out.studs.push_back(std::move(stud));
        } else if (mesh == minifigMesh) {
            const std::uint32_t characterEnum = effectiveEnum(*object, cls, "CharacterEnum");
            if (characterEnum >= firstCustomCharacter && characterEnum <= lastCustomCharacter) {
                ++out.customCharacters;
                continue;
            }
            CharacterObject character;
            character.appearance.body = placement.mesh;
            character.appearance.head = effectiveString(*object, cls, "HeadModel");
            character.appearance.face = effectiveString(*object, cls, "FaceTexture");
            character.appearance.lower = effectiveString(*object, cls, "LowerTexture");
            character.appearance.upper = effectiveString(*object, cls, "UpperTexture");
            character.appearance.characterEnum = characterEnum;
            character.hat = effectiveString(*object, cls, "HatModel");
            const std::string animationClass = effectiveString(*object, cls, "AnimationClass");
            character.animationClass = lowered(animationClass.empty() ? defaultAnimationClass : animationClass);
            character.placement = std::move(placement);
            out.characters.push_back(std::move(character));
        } else if (mesh.rfind("char_", 0) == 0) {
            ++out.skeletal;
        } else if (!archive.find(meshModelPath(placement.mesh))) {
            ++out.missingModel;
        } else {
            out.objects.push_back(std::move(placement));
        }
    }
    return out;
}

std::optional<Placement> playerStart(const Level& level) {
    for (const auto& obj : level.objects)
        if (obj.instance == "Player1Start") return placementOf(obj);
    return std::nullopt;
}

std::optional<Placement> objectStart(const Level& level, const std::string& instance) {
    const auto fold = [](std::string text) {
        std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
        return text;
    };
    const std::string wanted = fold(instance);
    for (const auto& obj : level.objects)
        if (fold(obj.instance) == wanted) return placementOf(obj);
    return std::nullopt;
}

CharacterAppearance characterAppearance(const Level& characterClass) {
    CharacterAppearance a;
    a.body = stringParam(characterClass.defaults, "meshname");
    a.head = stringParam(characterClass.defaults, "HeadModel");
    a.face = stringParam(characterClass.defaults, "FaceTexture");
    a.lower = stringParam(characterClass.defaults, "LowerTexture");
    a.upper = stringParam(characterClass.defaults, "UpperTexture");
    const std::uint32_t hash = nameHash("CharacterEnum");
    for (const auto& param : characterClass.defaults)
        if (param.hash == hash && param.type == 7) a.characterEnum = param.value;
    return a;
}

std::array<float, 16> placementMatrix(const Placement& placement) {
    constexpr float degreesToRadians = 0.0174532942f;
    float x = placement.orientation[0] * degreesToRadians, y = placement.orientation[1] * degreesToRadians,
          z = placement.orientation[2] * degreesToRadians;
    float sx = std::sin(x), cx = std::cos(x), sy = std::sin(y), cy = std::cos(y), sz = std::sin(z), cz = std::cos(z);
    return {cy * cz,  sx * sy * cz + cx * sz, sx * sz - cx * sy * cz, 0,
            -cy * sz, cx * cz - sx * sy * sz, sx * cz + cx * sy * sz, 0,
            sy,       -sx * cy,               cx * cy,                0,
            placement.position[0], placement.position[1], placement.position[2], 1};
}

std::string meshModelPath(const std::string& mesh) {
    std::string m = mesh;
    std::replace(m.begin(), m.end(), '\\', '/');
    std::transform(m.begin(), m.end(), m.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    if (m.find('/') == std::string::npos) m += "/" + m;
    return "models/" + m + ".bfnmdl";
}
} // namespace chase::assets
