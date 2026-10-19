#include "runtime/nested_props.hpp"

#include <algorithm>

namespace chase::runtime {

namespace {
constexpr const char* kWheelClass = "vehicles/wheel";

bool hasStartupScript(const assets::Level& classFile) {
    const std::uint32_t hash = assets::nameHash("startup");
    return std::any_of(classFile.scripts.begin(), classFile.scripts.end(), [hash](const assets::LevelScript& script) { return script.nameHash == hash; });
}
}

std::vector<assets::Placement> nestedProps(assets::FuseArchive& archive, const assets::Level& classFile, const assets::ClassLookup& classOf) {
    if (classFile.objects.empty() || hasStartupScript(classFile)) return {};
    std::vector<assets::Placement> out;
    for (assets::Placement& child : assets::drawableObjects(archive, classFile, classOf).objects)
        if (child.className != kWheelClass) out.push_back(std::move(child));
    return out;
}

}
