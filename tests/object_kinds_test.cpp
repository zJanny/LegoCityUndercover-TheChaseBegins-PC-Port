// Kind byte of a placed object (entity +0x12 = low byte of the effective ObjectType, written from the class file by the object loader
// 0x1a4130 at 0x1a4290 and copied into the entity by each class create, docs/re/object-kinds.md). Synthetic rules first, then the class
// files of the extracted RomFS (exits with 77 without it).
#include "assets/assets.hpp"
#include "assets/object_classes.hpp"
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase::assets;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

LevelParam objectType(std::uint32_t value, std::uint16_t flags) {
    LevelParam param;
    param.hash = nameHash("ObjectType");
    param.flags = flags;
    param.type = 7;
    param.value = value;
    return param;
}

void syntheticRules() {
    LevelObject object;
    Level none;
    require(objectKind(object, none) == 0, "a class without ObjectType has kind 0");

    Level defaults;
    defaults.defaults.push_back(objectType(71, 5));
    require(objectKind(object, defaults) == 71, "class default");

    Level classTable;
    classTable.classParams.push_back(objectType(72, 5));
    require(objectKind(object, classTable) == 72, "class table after the defaults");

    Level both;
    both.defaults.push_back(objectType(10, 5));
    both.classParams.push_back(objectType(20, 5));
    require(objectKind(object, both) == 10, "the defaults are searched first");

    object.params.push_back(objectType(42, 5));
    require(objectKind(object, defaults) == 71, "an instance value is ignored without the slot flag");
    Level slot;
    slot.defaults.push_back(objectType(71, 7));
    require(objectKind(object, slot) == 42, "an instance value overrides a slot parameter");

    Level wide;
    wide.defaults.push_back(objectType(0x1ff, 5));
    LevelObject plain;
    require(objectKind(plain, wide) == 0xff, "only the low byte is stored");
}

struct Pin {
    const char* className;
    int kind;
    const char* provenance;
};

void realClassFiles(const char* fibPath, const char* namesPath) {
    FuseArchive archive(fibPath);
    archive.loadNames(namesPath);
    const ClassLookup classOf = archiveClassLookup(archive);
    // Class files dumped with the C++ loader (objectKind of each class file, 2026-10-18); the descriptor of each kind is registered
    // by `mov r0,#kind; bl 0x246240` at the address in the docs/re entry.
    const Pin pins[] = {
        {"leprops/leprop_door", 42, "descriptor 0x15c238, docs/re/doors.md"},
        {"leprops/leprop_bouncer", 19, "descriptor 0x1748a0, docs/re/props-wobbly-bouncer.md"},
        {"props/prop_buildit", 21, "descriptor 0x15d2d0, docs/re/buildit.md"},
        {"props/ability_detective", 182, "descriptor 0x16f134, docs/re/abilities-detective.md"},
        {"props/ability_detective_reveal_spot", 238, "docs/re/abilities-detective.md"},
        {"props/ability_detective_reveal_dirtmound", 74, "docs/re/abilities-detective.md"},
        {"props/prop_scannerzoneaudio", 201, "descriptor 0x1863c8, docs/re/abilities-scanner.md"},
        {"props/prop_simpleuse", 165, "class id 0xa5, docs/re/use-spot.md"},
        {"vehicles/vehicle_police_trooper", 183, "descriptor 0x155b5c, docs/re/vehicles-system.md"},
        {"pickups/pickup_studgold", 61, "ObjectType 0x3d, docs/re/studs.md"},
        {"props/prop_trafficlight", 204, "docs/re/vehicles-prop-smash.md"},
        {"leprops/leprop_wobblydestructible", 119, "descriptor 0x281c3c, docs/re/props-wobbly-bouncer.md"},
        {"leprops/leprop_pushable", 71, "descriptor 0x180e70 (ToggleUseable group kind 0x47, 0x1cdd18)"},
        {"leprops/leprop_pushhandle", 72, "descriptor 0x1d1ff4 (ToggleUseable child kind 0x48, 0x1cdd38)"},
        {"chars/char_player", 27, "player entity +0x12 = 27 read in the oracle, docs/re/script-natives-flags.md"},
    };
    LevelObject instance;
    for (const Pin& pin : pins) {
        const Level* classFile = classOf(pin.className);
        require(classFile != nullptr, std::string("missing class ") + pin.className);
        require(objectKind(instance, *classFile) == pin.kind, std::string("kind of ") + pin.className + " (" + pin.provenance + ")");
    }

    const Level* pushable = classOf("leprops/leprop_pushable");
    require(pushable->objects.size() == 4, "the pushable class file nests four objects");
    for (const LevelObject& handle : pushable->objects) {
        require(handle.className == "leprops/leprop_pushhandle", "nested class");
        require(objectKind(handle, *classOf(handle.className)) == 72, "nested handle kind");
    }
}
}

int main(int argc, char** argv) {
    try {
        syntheticRules();
        if (argc != 3 || !std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
            std::cout << "object-kinds-test: data not available, real class files skipped\n";
            return 77;
        }
        realClassFiles(argv[1], argv[2]);
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << "\n";
        return 1;
    }
    std::cout << "ok\n";
    return 0;
}
