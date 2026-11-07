#include "assets/path_name_hash.hpp"

#include "assets/assets.hpp"

namespace chase::assets {

std::string pathLookupName(const std::string& name) {
    const std::size_t dot = name.rfind('.');
    if (dot == std::string::npos || dot == 0) return name;
    return name.substr(dot + 1);
}

std::uint32_t pathNameHash(const std::string& name) { return nameHash(pathLookupName(name)); }

}
