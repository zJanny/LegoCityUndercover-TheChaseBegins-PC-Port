#pragma once
// Models of the objects a class file nests: the object loader 0x1a4130 creates them after their parent like placed objects
// (docs/re/object-kinds.md), so a placed `props/prop_crane_ladderlarge` also draws its upper segment `ladder_part_02`. Rules and data:
// docs/re/objects.md. No SDL dependency.
#include "assets/assets.hpp"

#include <vector>

namespace chase::runtime {

// STAND-IN LEVOBJ-S93: the drawn nested objects are the plain models drawableObjects keeps of the class file, placed at their class-file
// placement relative to the parent and standing still; the nested objects of a class with a "startup" script (whose EnableObject calls on
// them the port does not apply, LEVOBJ-8) and the vehicle wheels (drawn by the vehicle path) are left out.
std::vector<assets::Placement> nestedProps(assets::FuseArchive& archive, const assets::Level& classFile, const assets::ClassLookup& classOf);

}
