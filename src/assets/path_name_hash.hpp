#pragma once
#include <cstdint>
#include <string>

namespace chase::assets {

std::string pathLookupName(const std::string& name);
std::uint32_t pathNameHash(const std::string& name);

}
